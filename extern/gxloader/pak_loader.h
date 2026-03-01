#pragma once
/// @file pak_loader.h
/// @brief GXPAKバンドルランタイムローダー
///
/// .gxpakアーカイブのTOCを読み込み、パス指定でエントリを取り出す。
/// LZ4圧縮されたエントリは自動的に展開される。

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "gxpak.h"

namespace gxloader
{

/// @brief GXPAKアーカイブの読み取りクラス
/// @details Open()でTOCをメモリに読み込み、Read()で個別エントリのデータを取得する。
///          PakFileProvider (VFS) から利用される。
class PakLoader
{
public:
    /// @brief GXPAKファイルを開いてTOCを読み込む
    /// @param filePath .gxpakファイルパス
    /// @return 成功時true
    bool Open(const std::string& filePath);

    /// @brief アーカイブを閉じてTOCを解放する
    void Close();

    /// @brief 指定パスのエントリが存在するか確認する
    /// @param path バンドル内のパス
    /// @return 存在すればtrue
    bool Contains(const std::string& path) const;

    /// @brief 指定パスのエントリデータを読み込む (LZ4自動展開)
    /// @param path バンドル内のパス
    /// @return エントリのバイトデータ。見つからない場合は空
    std::vector<uint8_t> Read(const std::string& path) const;

    /// @brief 全エントリの一覧を返す
    /// @return エントリ配列のコピー
    std::vector<gxfmt::GxpakEntry> GetEntries() const;

    /// @brief 指定種別のエントリのみ抽出して返す
    /// @param type フィルタするアセット種別
    /// @return 該当エントリの配列
    std::vector<gxfmt::GxpakEntry> GetEntriesByType(gxfmt::GxpakAssetType type) const;

private:
    std::string m_filePath;                   ///< 開いているアーカイブのパス
    std::vector<gxfmt::GxpakEntry> m_entries; ///< メモリ上のTOC
};

} // namespace gxloader
