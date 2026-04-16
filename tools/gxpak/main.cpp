/// @file main.cpp
/// @brief gxpak CLIツール — GXPAKアセットバンドルの作成・展開・一覧表示
///
/// 使い方:
///   gxpak pack   -o output.gxpak -d input_dir/ [--compress]  ... パック
///   gxpak unpack -i input.gxpak  -d output_dir/              ... 展開
///   gxpak list   -i input.gxpak                              ... 一覧表示

#include "gxpak.h"
#include "lz4.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

/// ヘルプメッセージを表示する
static void PrintUsage()
{
    printf("Usage:\n");
    printf("  gxpak pack   -o output.gxpak -d input_dir/ [--compress]\n");
    printf("  gxpak unpack -i input.gxpak  -d output_dir/\n");
    printf("  gxpak list   -i input.gxpak\n");
    printf("  gxpak add    -i input.gxpak  -f file -p \"path/in/pak\"\n");
    printf("  gxpak remove -i input.gxpak  -p \"path/in/pak\"\n");
}

// ============================================================
// TOCシリアライズヘルパー
// ============================================================

/// @brief パックツール内部で使うエントリ情報
struct PakEntry
{
    std::string path;                    ///< バンドル内の相対パス
    gxfmt::GxpakAssetType assetType;     ///< アセット種別
    bool compressed;                      ///< LZ4圧縮済みか
    uint64_t dataOffset;                  ///< データのファイル内オフセット
    uint32_t compressedSize;              ///< 圧縮後サイズ
    uint32_t originalSize;                ///< 元サイズ
};

/// TOCエントリ1つをファイルに書き出す (可変長パス + 固定フィールド)
static void WriteTocEntry(FILE* f, const PakEntry& entry)
{
    uint32_t pathLen = static_cast<uint32_t>(entry.path.size());
    fwrite(&pathLen, 4, 1, f);
    fwrite(entry.path.data(), 1, pathLen, f);
    fwrite(&entry.assetType, 1, 1, f);
    uint8_t comp = entry.compressed ? 1 : 0;
    fwrite(&comp, 1, 1, f);
    uint8_t pad[2] = {};
    fwrite(pad, 1, 2, f);
    fwrite(&entry.dataOffset, 8, 1, f);
    fwrite(&entry.compressedSize, 4, 1, f);
    fwrite(&entry.originalSize, 4, 1, f);
}

/// ファイルからTOCエントリ1つを読み込む
static PakEntry ReadTocEntry(FILE* f)
{
    PakEntry entry;
    uint32_t pathLen = 0;
    fread(&pathLen, 4, 1, f);
    std::vector<char> pathBuf(pathLen + 1, '\0');
    fread(pathBuf.data(), 1, pathLen, f);
    entry.path = pathBuf.data();

    fread(&entry.assetType, 1, 1, f);
    uint8_t comp;
    fread(&comp, 1, 1, f);
    entry.compressed = (comp != 0);
    uint8_t pad[2];
    fread(pad, 1, 2, f);
    fread(&entry.dataOffset, 8, 1, f);
    fread(&entry.compressedSize, 4, 1, f);
    fread(&entry.originalSize, 4, 1, f);
    return entry;
}

// ============================================================
// packコマンド: ディレクトリ内のファイルをGXPAKにまとめる
// ============================================================

static int CmdPack(const std::string& outputPath, const std::string& inputDir, bool compress)
{
    std::vector<std::pair<std::string, fs::path>> files; // (バンドル内相対パス, ディスクパス)

    for (auto& entry : fs::recursive_directory_iterator(inputDir))
    {
        if (!entry.is_regular_file()) continue;
        fs::path rel = fs::relative(entry.path(), inputDir);
        std::string relStr = rel.generic_string();
        files.push_back({relStr, entry.path()});
    }

    if (files.empty())
    {
        fprintf(stderr, "Error: No files found in %s\n", inputDir.c_str());
        return 1;
    }

    std::sort(files.begin(), files.end());

    FILE* f = fopen(outputPath.c_str(), "wb");
    if (!f) { fprintf(stderr, "Error: Cannot open %s\n", outputPath.c_str()); return 1; }

    // 仮ヘッダ書き出し (tocOffset/tocSizeは後で上書き)
    gxfmt::GxpakHeader header{};
    header.magic = gxfmt::k_GxpakMagic;
    header.version = gxfmt::k_GxpakVersion;
    header.entryCount = static_cast<uint32_t>(files.size());
    header.flags = compress ? 1 : 0;
    fwrite(&header, sizeof(header), 1, f);

    // データエントリを書き出し (LZ4圧縮で小さくなる場合のみ圧縮)
    std::vector<PakEntry> entries;
    uint64_t dataStart = sizeof(gxfmt::GxpakHeader);

    for (auto& [relPath, fullPath] : files)
    {
        // Read source file
        FILE* src = fopen(fullPath.string().c_str(), "rb");
        if (!src) continue;

        fseek(src, 0, SEEK_END);
        long srcSize = ftell(src);
        fseek(src, 0, SEEK_SET);

        std::vector<uint8_t> srcData(srcSize);
        fread(srcData.data(), 1, srcSize, src);
        fclose(src);

        PakEntry entry;
        entry.path = relPath;
        entry.assetType = gxfmt::DetectAssetType(relPath.c_str());
        entry.originalSize = static_cast<uint32_t>(srcSize);
        entry.dataOffset = static_cast<uint64_t>(ftell(f));

        if (compress && srcSize > 64)
        {
            int maxDst = LZ4_compressBound(static_cast<int>(srcSize));
            std::vector<uint8_t> compData(maxDst);
            int compSize = LZ4_compress_default(
                reinterpret_cast<const char*>(srcData.data()),
                reinterpret_cast<char*>(compData.data()),
                static_cast<int>(srcSize), maxDst);

            if (compSize > 0 && compSize < static_cast<int>(srcSize))
            {
                entry.compressed = true;
                entry.compressedSize = static_cast<uint32_t>(compSize);
                fwrite(compData.data(), 1, compSize, f);
            }
            else
            {
                entry.compressed = false;
                entry.compressedSize = entry.originalSize;
                fwrite(srcData.data(), 1, srcSize, f);
            }
        }
        else
        {
            entry.compressed = false;
            entry.compressedSize = entry.originalSize;
            fwrite(srcData.data(), 1, srcSize, f);
        }

        entries.push_back(entry);
    }

    // TOCをファイル末尾に書き出す
    uint64_t tocOffset = static_cast<uint64_t>(ftell(f));
    for (auto& e : entries)
        WriteTocEntry(f, e);
    uint64_t tocSize = static_cast<uint64_t>(ftell(f)) - tocOffset;

    // ヘッダを更新 (TOCオフセット/サイズを確定値で上書き)
    header.tocOffset = tocOffset;
    header.tocSize = tocSize;
    fseek(f, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, f);

    fclose(f);

    printf("Packed %zu files into %s\n", entries.size(), outputPath.c_str());
    if (compress) printf("  (LZ4 compression enabled)\n");

    return 0;
}

// ============================================================
// listコマンド: GXPAKの内容一覧を表示する
// ============================================================

static int CmdList(const std::string& inputPath)
{
    FILE* f = fopen(inputPath.c_str(), "rb");
    if (!f) { fprintf(stderr, "Error: Cannot open %s\n", inputPath.c_str()); return 1; }

    gxfmt::GxpakHeader header{};
    fread(&header, sizeof(header), 1, f);

    if (header.magic != gxfmt::k_GxpakMagic)
    {
        fprintf(stderr, "Error: Not a GXPAK file\n");
        fclose(f);
        return 1;
    }

    printf("GXPAK: %s\n", inputPath.c_str());
    printf("  Version: %u, Entries: %u\n\n", header.version, header.entryCount);

    _fseeki64(f, static_cast<long long>(header.tocOffset), SEEK_SET);

    auto typeStr = [](gxfmt::GxpakAssetType t) -> const char* {
        switch (t) {
        case gxfmt::GxpakAssetType::Model: return "Model";
        case gxfmt::GxpakAssetType::Animation: return "Anim";
        case gxfmt::GxpakAssetType::Texture: return "Tex";
        default: return "Other";
        }
    };

    for (uint32_t i = 0; i < header.entryCount; ++i)
    {
        PakEntry e = ReadTocEntry(f);
        printf("  [%u] %-8s %s", i, typeStr(e.assetType), e.path.c_str());
        if (e.compressed)
            printf("  (%u -> %u bytes, %.1f%%)", e.originalSize, e.compressedSize,
                   100.0f * e.compressedSize / (e.originalSize > 0 ? e.originalSize : 1));
        else
            printf("  (%u bytes)", e.originalSize);
        printf("\n");
    }

    fclose(f);
    return 0;
}

// ============================================================
// unpackコマンド: GXPAKの全エントリをディレクトリに展開する
// ============================================================

static int CmdUnpack(const std::string& inputPath, const std::string& outputDir)
{
    FILE* f = fopen(inputPath.c_str(), "rb");
    if (!f) { fprintf(stderr, "Error: Cannot open %s\n", inputPath.c_str()); return 1; }

    gxfmt::GxpakHeader header{};
    fread(&header, sizeof(header), 1, f);

    if (header.magic != gxfmt::k_GxpakMagic)
    {
        fprintf(stderr, "Error: Not a GXPAK file\n");
        fclose(f);
        return 1;
    }

    // Read TOC
    _fseeki64(f, static_cast<long long>(header.tocOffset), SEEK_SET);
    std::vector<PakEntry> entries;
    for (uint32_t i = 0; i < header.entryCount; ++i)
        entries.push_back(ReadTocEntry(f));

    // Extract each entry
    for (auto& entry : entries)
    {
        fs::path outPath = fs::path(outputDir) / entry.path;
        fs::create_directories(outPath.parent_path());

        _fseeki64(f, static_cast<long long>(entry.dataOffset), SEEK_SET);
        std::vector<uint8_t> rawData(entry.compressedSize);
        fread(rawData.data(), 1, entry.compressedSize, f);

        std::vector<uint8_t> fileData;
        if (entry.compressed)
        {
            fileData.resize(entry.originalSize);
            int result = LZ4_decompress_safe(
                reinterpret_cast<const char*>(rawData.data()),
                reinterpret_cast<char*>(fileData.data()),
                static_cast<int>(entry.compressedSize),
                static_cast<int>(entry.originalSize));
            if (result < 0)
            {
                fprintf(stderr, "Error: Failed to decompress %s\n", entry.path.c_str());
                continue;
            }
        }
        else
        {
            fileData = std::move(rawData);
        }

        FILE* out = fopen(outPath.string().c_str(), "wb");
        if (out)
        {
            fwrite(fileData.data(), 1, fileData.size(), out);
            fclose(out);
            printf("  Extracted: %s\n", entry.path.c_str());
        }
    }

    fclose(f);
    printf("Unpacked %zu files to %s\n", entries.size(), outputDir.c_str());
    return 0;
}

// ============================================================
// エントリポイント: サブコマンドを判定して実行
// ============================================================

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        PrintUsage();
        return 1;
    }

    std::string cmd = argv[1];

    // Parse arguments
    std::string inputPath, outputPath, dirPath, filePath, pakPath;
    bool compress = false;

    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc)
            inputPath = argv[++i];
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
            outputPath = argv[++i];
        else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc)
            dirPath = argv[++i];
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
            filePath = argv[++i];
        else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
            pakPath = argv[++i];
        else if (strcmp(argv[i], "--compress") == 0)
            compress = true;
    }

    if (cmd == "pack")
    {
        if (outputPath.empty() || dirPath.empty())
        {
            fprintf(stderr, "Error: pack requires -o and -d\n");
            return 1;
        }
        return CmdPack(outputPath, dirPath, compress);
    }
    else if (cmd == "list")
    {
        if (inputPath.empty()) { fprintf(stderr, "Error: list requires -i\n"); return 1; }
        return CmdList(inputPath);
    }
    else if (cmd == "unpack")
    {
        if (inputPath.empty() || dirPath.empty())
        {
            fprintf(stderr, "Error: unpack requires -i and -d\n");
            return 1;
        }
        return CmdUnpack(inputPath, dirPath);
    }
    else if (cmd == "add")
    {
        fprintf(stderr, "Error: 'add' command not yet implemented\n");
        return 1;
    }
    else if (cmd == "remove")
    {
        fprintf(stderr, "Error: 'remove' command not yet implemented\n");
        return 1;
    }
    else
    {
        fprintf(stderr, "Error: Unknown command: %s\n", cmd.c_str());
        PrintUsage();
        return 1;
    }
}
