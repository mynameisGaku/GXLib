#pragma once
/// @file AssetMeta.h
/// @brief .meta サイドカーファイルの読み書き
///
/// アセットファイルに対応する .meta ファイルを管理し、
/// GUID とインポート設定を永続化する。
/// @addtogroup grp_core
/// @{

#include "pch_common.h"
#include "Core/GUID.h"

namespace gx
{

/// @brief .meta ファイルの内容を表す構造体
struct AssetMetaData
{
    GUID guid;                                     ///< アセットのGUID
    gx::HashMap<gx::String, gx::String> importSettings; ///< インポート設定 (key=value)
};

/// @brief .meta サイドカーファイルの読み書きユーティリティ
class AssetMeta
{
public:
    /// @brief .meta ファイルを読み込む
    /// @param metaPath .meta ファイルのパス
    /// @param outData 読み込んだメタデータの出力先
    /// @return 読み込み成功で true
    static bool Load(const gx::String& metaPath, AssetMetaData& outData);

    /// @brief .meta ファイルを書き出す
    /// @param metaPath .meta ファイルのパス
    /// @param data 書き込むメタデータ
    /// @return 書き込み成功で true
    static bool Save(const gx::String& metaPath, const AssetMetaData& data);

    /// @brief アセットファイルパスから .meta ファイルパスを生成する
    /// @param assetPath アセットファイルのパス (例: "model.gxmdl")
    /// @return .meta ファイルパス (例: "model.gxmdl.meta")
    static gx::String GetMetaPath(const gx::String& assetPath);

    /// @brief .meta ファイルが存在するか判定する
    /// @param assetPath アセットファイルのパス
    /// @return 対応する .meta ファイルが存在すれば true
    static bool HasMeta(const gx::String& assetPath);
};

} // namespace gx
/// @}
