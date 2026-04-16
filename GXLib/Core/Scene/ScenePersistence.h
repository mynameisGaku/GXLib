#pragma once
/// @file ScenePersistence.h
/// @brief シーンのファイル保存・読み込み（テキスト / バイナリ両対応）
///
/// ステージデータをファイルに書き出し、次回起動時に復元する用途で使う。
/// テキスト形式はデバッグしやすく、バイナリ形式はサイズが小さい。
/// @addtogroup grp_scene/// @{

#include "pch_common.h"

namespace gx
{

class Scene;

/// @brief シーンファイルフォーマット
enum class SceneFileFormat
{
    Text,   ///< テキスト形式（人間が読みやすい）
    Binary, ///< バイナリ形式（サイズが小さい）
};

/// @brief シーン永続化ユーティリティ
class ScenePersistence
{
public:
    /// @brief シーンをファイルに保存する
    /// @param scene 保存するシーン
    /// @param path 出力ファイルパス
    /// @param format ファイル形式（Text または Binary）
    /// @return 保存が成功した場合 true
    static bool SaveToFile(const Scene& scene, const gx::String& path, SceneFileFormat format = SceneFileFormat::Text);

    /// @brief ファイルからシーンを読み込む
    /// @param path 入力ファイルパス
    /// @param format ファイル形式（Text または Binary）
    /// @return 読み込まれたシーン。失敗時は nullptr
    static std::unique_ptr<Scene> LoadFromFile(const gx::String& path, SceneFileFormat format = SceneFileFormat::Text);

    /// @brief シーンをテキスト文字列にシリアライズする
    /// @param scene シリアライズするシーン
    /// @return テキスト形式の文字列
    static gx::String SerializeToString(const Scene& scene);

    /// @brief テキスト文字列からシーンをデシリアライズする
    /// @param data テキスト形式の文字列
    /// @return デシリアライズされたシーン。失敗時は nullptr
    static std::unique_ptr<Scene> DeserializeFromString(const gx::String& data);

    /// @brief シーンをバイナリ形式にシリアライズする
    /// @param scene シリアライズするシーン
    /// @return バイナリデータ
    static gx::Vector<uint8_t> SerializeToBinary(const Scene& scene);

    /// @brief バイナリ形式からシーンをデシリアライズする
    /// @param data バイナリデータ
    /// @return デシリアライズされたシーン。失敗時は nullptr
    static std::unique_ptr<Scene> DeserializeFromBinary(const gx::Vector<uint8_t>& data);
};

} // namespace gx
/// @}
