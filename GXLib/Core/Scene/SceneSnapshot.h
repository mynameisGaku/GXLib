#pragma once
/// @file SceneSnapshot.h
/// @brief シーン状態の一時保存と復元（Play-in-Editor 用）
///
/// エディタの「再生」ボタンを押した瞬間のシーン状態をキャプチャし、
/// 「停止」時に元の状態へ戻す。ゲームのチェックポイント機能にも流用できる。
/// @addtogroup grp_scene/// @{

#include "pch_common.h"

namespace gx
{

class Scene;

/// @brief シーンの状態を保存・復元するスナップショットクラス
///
/// Play-in-Editor 機能で使用する。Play開始時にシーン状態をキャプチャし、
/// Stop時に元の状態に復元する。
class SceneSnapshot
{
public:
    /// @brief シーンの現在の状態をキャプチャする
    /// @param scene キャプチャ対象のシーン
    void Capture(const Scene& scene);

    /// @brief キャプチャした状態をシーンに復元する
    /// @param scene 復元先のシーン
    void Restore(Scene& scene) const;

    /// @brief スナップショットが有効かどうか
    /// @return Capture済みなら true
    bool IsValid() const { return m_valid; }

    /// @brief キャプチャされたエンティティ数を取得する
    /// @return エンティティ数
    uint32_t GetEntityCount() const { return static_cast<uint32_t>(m_entityData.size()); }

private:
    /// @brief コンポーネントの保存データ
    struct ComponentData
    {
        uint32_t type;               ///< コンポーネント種類ID
        // シリアライズされたコンポーネント状態
        int soundHandle = -1;        ///< サウンドハンドル（AudioSource用）
        bool playOnStart = false;    ///< シーン開始時に自動再生するか
        bool loop = false;           ///< ループ再生するか
        bool enabled = true;         ///< 有効フラグ
        std::string name;            ///< 名前（NameComponent用）
        std::string tag;             ///< タグ（TagComponent用）
        /// @brief 軽量3次元ベクトル
        struct Vec3 { float x = 0, y = 0, z = 0; };
        Vec3 position, rotation, scale; ///< トランスフォーム値（TransformComponent用）
    };

    /// @brief エンティティの保存データ
    struct EntityData
    {
        uint32_t id;                            ///< エンティティID
        std::string name;                       ///< エンティティ名
        bool active;                            ///< アクティブフラグ
        float posX, posY, posZ;                 ///< ワールド位置
        float rotX, rotY, rotZ;                 ///< 回転（オイラー角）
        float scaleX, scaleY, scaleZ;           ///< スケール
        std::string parentName;                 ///< 親エンティティ名（空の場合ルート）
        std::vector<ComponentData> components;  ///< コンポーネントデータ配列
    };

    bool m_valid = false;                       ///< スナップショットが有効かどうか
    std::string m_sceneName;                    ///< キャプチャ時のシーン名
    std::vector<EntityData> m_entityData;       ///< 保存されたエンティティデータ一覧
};

} // namespace gx
/// @}
