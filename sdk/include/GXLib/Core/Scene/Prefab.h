#pragma once
/// @file Prefab.h
/// @brief エンティティテンプレート（プレハブ）
///
/// キャラや障害物など繰り返し配置するオブジェクトを
/// JSON 文字列としてひな形保存し、シーンに何度でも複製できる。
/// ファイル単位での保存・読み込みにも対応。
/// @addtogroup grp_scene/// @{

#include "pch_common.h"

namespace gx
{

class Entity;
class Scene;

/// @brief エンティティテンプレート
class Prefab
{
public:
    Prefab() = default;
    ~Prefab() = default;

    /// @brief エンティティの状態をキャプチャする
    /// @param entity キャプチャ元のエンティティ
    /// @return 成功した場合true
    bool CaptureFromEntity(const Entity& entity);

    /// @brief ファイルからプレハブデータを読み込む
    /// @param filePath JSONファイルパス
    /// @return 成功した場合true
    bool LoadFromFile(const gx::String& filePath);

    /// @brief プレハブデータをファイルに保存する
    /// @param filePath JSONファイルパス
    /// @return 成功した場合true
    bool SaveToFile(const gx::String& filePath) const;

    /// @brief シーンにインスタンスを生成する
    /// @param scene 生成先のシーン
    /// @param name エンティティ名（空の場合はキャプチャ時の名前を使用）
    /// @return 生成されたエンティティ（失敗時はnullptr）
    Entity* Instantiate(Scene& scene, const gx::String& name = "") const;

    /// @brief 有効なプレハブデータを保持しているか判定する
    /// @return 有効なデータを保持している場合 true
    bool IsValid() const { return !m_json.empty(); }

    /// @brief 内部のJSON文字列を取得する
    /// @return プレハブデータのJSON文字列
    const gx::String& GetJson() const { return m_json; }

private:
    gx::String m_json; ///< エンティティデータを保持するJSON文字列
};

} // namespace gx
/// @}
