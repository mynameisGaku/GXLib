/// @file main.cpp
/// @brief gxconv CLIツールのエントリポイント
///
/// OBJ/FBX/glTFファイルを.gxmd/.gxan形式に変換するコマンドラインツール。
/// 使い方: gxconv <input> [output] [options]

#include "converter.h"
#include <cstdio>
#include <cstring>

/// ヘルプメッセージを表示する
static void PrintUsage()
{
    printf("Usage: gxconv <input> [output] [options]\n\n");
    printf("Converts 3D model files to GXMD binary format.\n\n");
    printf("Supported inputs: .obj, .fbx, .gltf, .glb\n");
    printf("Output: .gxmd (default) or .gxan (with --anim-only)\n\n");
    printf("Options:\n");
    printf("  --info              Show file info without converting\n");
    printf("  --shader-model <N>  Force shader model (standard/unlit/toon/phong/subsurface/clearcoat)\n");
    printf("  --toon-outline <W>  Toon outline width\n");
    printf("  --index16           Use 16-bit indices\n");
    printf("  --no-anim           Exclude animation data\n");
    printf("  --anim-only         Export animations as .gxan (Phase 5)\n");
    printf("  --help              Show this help\n");
}

/// コマンドライン引数を解析してConverterを実行する
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        PrintUsage();
        return 1;
    }

    gxconv::ConvertOptions options;
    int positionalIndex = 0; // 位置引数のカウンタ (0=input, 1=output)

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            PrintUsage();
            return 0;
        }
        else if (strcmp(argv[i], "--info") == 0)
        {
            options.infoOnly = true;
        }
        else if (strcmp(argv[i], "--index16") == 0)
        {
            options.useIndex16 = true;
        }
        else if (strcmp(argv[i], "--no-anim") == 0)
        {
            options.excludeAnimations = true;
        }
        else if (strcmp(argv[i], "--anim-only") == 0)
        {
            options.animOnly = true;
        }
        else if (strcmp(argv[i], "--shader-model") == 0)
        {
            if (i + 1 >= argc) { fprintf(stderr, "Error: --shader-model requires an argument\n"); return 1; }
            ++i;
            options.hasShaderModelOverride = true;
            options.shaderModelOverride = gxfmt::ShaderModelFromString(argv[i]);
        }
        else if (strcmp(argv[i], "--toon-outline") == 0)
        {
            if (i + 1 >= argc) { fprintf(stderr, "Error: --toon-outline requires an argument\n"); return 1; }
            ++i;
            options.toonOutlineWidth = static_cast<float>(atof(argv[i]));
        }
        else if (argv[i][0] == '-')
        {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            return 1;
        }
        else
        {
            if (positionalIndex == 0)
                options.inputPath = argv[i];
            else if (positionalIndex == 1)
                options.outputPath = argv[i];
            ++positionalIndex;
        }
    }

    if (options.inputPath.empty())
    {
        fprintf(stderr, "Error: No input file specified\n");
        return 1;
    }

    gxconv::Converter converter;
    return converter.Run(options);
}
