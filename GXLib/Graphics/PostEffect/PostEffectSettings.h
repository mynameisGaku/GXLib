#pragma once
/// @file PostEffectSettings.h
/// @brief ポストエフェクト設定のJSON読み書き
///
/// PostEffectPipelineの全パラメータをJSONファイルに保存/復元する。
/// F12キーで保存、起動時に自動ロードする想定。

#include <string>

namespace GX
{

class PostEffectPipeline;

/// @brief PostEffectPipelineの設定をJSONファイルとして入出力するユーティリティ
///
/// 全エフェクトの有効/無効・パラメータをnlohmann/jsonで一括管理する。
class PostEffectSettings
{
public:
    /// @brief JSONファイルからパイプライン設定を復元する
    /// @param filePath JSONファイルのパス
    /// @param pipeline 設定を適用する対象
    /// @return 成功でtrue
    static bool Load(const std::string& filePath, PostEffectPipeline& pipeline);

    /// @brief パイプライン設定をJSONファイルに保存する
    /// @param filePath 出力先JSONファイルのパス
    /// @param pipeline 保存する設定の取得元
    /// @return 成功でtrue
    static bool Save(const std::string& filePath, const PostEffectPipeline& pipeline);
};

} // namespace GX
