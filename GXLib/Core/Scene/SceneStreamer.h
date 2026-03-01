#pragma once
/// @file SceneStreamer.h
/// @brief シーンストリーミング（アディティブ読み込み+非同期遷移）
///
/// プレイヤー位置に基づいてシーンボリュームの読み込み/解放を管理する。
/// loadRadius / unloadRadius のヒステリシスで頻繁な切り替えを防ぐ。

#include "pch_common.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <cmath>

namespace gx
{
/// @addtogroup grp_scene
/// @{

class Scene;

/// @brief ストリーミング状態
enum class StreamingState
{
    Unloaded,   ///< 未読み込み
    Loading,    ///< 読み込み中
    Loaded,     ///< 読み込み完了
    Unloading   ///< 解放中
};

/// @brief ストリーミングボリューム定義
struct StreamingVolume
{
    std::string scenePath;                      ///< シーンファイルパス
    float loadRadius   = 100.0f;                ///< 読み込み開始距離
    float unloadRadius = 150.0f;                ///< 解放開始距離（> loadRadius, ヒステリシス）
    struct { float x = 0.0f, y = 0.0f, z = 0.0f; } position;  ///< ボリューム中心座標
    StreamingState state = StreamingState::Unloaded; ///< 現在の状態
};

/// @brief シーンストリーミング管理
///
/// 複数のストリーミングボリュームを管理し、プレイヤー位置に基づいて
/// シーンの読み込み/解放を自動制御する。
class SceneStreamer
{
public:
    SceneStreamer() = default;
    ~SceneStreamer() = default;

    /// @brief ストリーミングボリュームを追加
    /// @param volume ボリューム定義
    /// @return ボリュームハンドル
    int AddVolume(const StreamingVolume& volume);

    /// @brief ストリーミングボリュームを削除
    /// @param handle ボリュームハンドル
    void RemoveVolume(int handle);

    /// @brief プレイヤー位置に基づいてボリュームの状態を更新
    /// @param playerX プレイヤーX座標
    /// @param playerY プレイヤーY座標
    /// @param playerZ プレイヤーZ座標
    void Update(float playerX, float playerY, float playerZ);

    /// @brief シーン読み込み完了コールバックを設定
    void SetOnSceneLoaded(std::function<void(int volumeHandle, Scene&)> callback);

    /// @brief シーン解放コールバックを設定
    void SetOnSceneUnloaded(std::function<void(int volumeHandle)> callback);

    /// @brief ボリュームの現在の状態を取得
    StreamingState GetVolumeState(int handle) const;

    /// @brief 登録ボリューム数を取得
    uint32_t GetVolumeCount() const;

    /// @brief ボリューム情報を取得
    const StreamingVolume* GetVolume(int handle) const;

    /// @brief 即座にシーンを読み込む
    void ForceLoad(int handle);

    /// @brief 即座にシーンを解放する
    void ForceUnload(int handle);

private:
    /// @brief プレイヤーからボリューム中心までの距離を計算
    static float DistanceTo(float px, float py, float pz,
                            float vx, float vy, float vz);

    struct VolumeEntry
    {
        StreamingVolume volume;
        bool valid = true;
    };

    std::vector<VolumeEntry> m_volumes;
    std::unordered_map<int, std::unique_ptr<Scene>> m_loadedScenes;
    std::function<void(int, Scene&)> m_onLoaded;
    std::function<void(int)> m_onUnloaded;
};

/// @}
} // namespace gx
