#pragma once
/// @file PlayInEditor.h
/// @brief Play-in-Editor (PIE) — エディタを離れずにシーンをプレイ・一時停止する
///
/// 実行前のシーン状態をスナップショットとして保存し、停止時に復元する。
/// Play/Pause/Resume/Stop の状態遷移を管理し、ゲームロジックのテストを素早く行える。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"
#include <functional>
#include <vector>
#include <cstdint>
#include <string>

namespace gx
{

/// @brief PIE実行状態
enum class PIEState : uint32_t
{
    Stopped = 0, ///< 停止中（エディタモード）
    Playing,     ///< プレイ中（ゲームロジック実行中）
    Paused       ///< 一時停止中
};

/// @brief PIE設定
struct PIEConfig
{
    bool startPaused       = false;          ///< true なら開始時に一時停止状態で始まる
    bool captureInput      = true;           ///< true ならPIE中にキーボード/マウス入力を捕捉する
    bool showDebugOverlay  = true;           ///< true ならデバッグオーバーレイを表示する
    bool simulatePhysics   = true;           ///< true なら物理シミュレーションを実行する
    bool simulateAI        = true;           ///< true ならAI処理を実行する
    bool simulateAudio     = false;          ///< true ならオーディオ処理を実行する
    float fixedTimeStep    = 1.0f / 60.0f;   ///< 固定タイムステップ（秒）
};

/// @brief シーンスナップショット
struct PIESnapshot
{
    uint32_t entityCount    = 0;     ///< スナップショット時のエンティティ数
    uint32_t componentCount = 0;     ///< スナップショット時のコンポーネント総数
    bool     hasData        = false; ///< スナップショットが存在するか
};

/// @brief エンティティごとの保存データ（PIE内部用）
struct PIEEntityState
{
    uint32_t    id       = 0;     ///< エンティティID
    std::string name;             ///< エンティティ名
    bool        active   = true;  ///< アクティブ状態
    float       posX = 0, posY = 0, posZ = 0; ///< 位置 (X, Y, Z)
    float       rotX = 0, rotY = 0, rotZ = 0; ///< 回転オイラー角 (X, Y, Z)
    float       sclX = 1, sclY = 1, sclZ = 1; ///< スケール (X, Y, Z)
};

/// @brief Play-in-Editor (PIE) コントローラ
class PlayInEditor
{
public:
    PlayInEditor() = default;
    ~PlayInEditor() = default;

    // --- 設定 ---
    void SetConfig(const PIEConfig& cfg) { m_config = cfg; }
    const PIEConfig& GetConfig() const { return m_config; }

    // --- 状態 ---
    PIEState GetState() const { return m_state; }
    bool IsPlaying() const { return m_state == PIEState::Playing; }
    bool IsPaused()  const { return m_state == PIEState::Paused; }
    bool IsStopped() const { return m_state == PIEState::Stopped; }

    // --- シーン ---
    void SetScene(Scene* scene) { m_scene = scene; }
    Scene* GetScene() const { return m_scene; }

    // --- プレイモード制御 ---

    void EnterPlayMode()
    {
        if (m_state != PIEState::Stopped)
            return;

        if (m_scene)
            CaptureSnapshot();

        m_frameCount  = 0;
        m_elapsedTime = 0.0f;

        if (m_config.startPaused)
            m_state = PIEState::Paused;
        else
            m_state = PIEState::Playing;

        if (OnEnterPlay) OnEnterPlay();
    }

    void ExitPlayMode()
    {
        if (m_state == PIEState::Stopped)
            return;

        if (m_scene && !m_savedEntities.empty())
            RestoreFromSaved();

        m_state       = PIEState::Stopped;
        m_frameCount  = 0;
        m_elapsedTime = 0.0f;

        if (OnExitPlay) OnExitPlay();
    }

    void Pause()
    {
        if (m_state == PIEState::Playing)
            m_state = PIEState::Paused;
    }

    void Resume()
    {
        if (m_state == PIEState::Paused)
            m_state = PIEState::Playing;
    }

    void StepFrame()
    {
        if (m_state != PIEState::Paused)
            return;

        m_elapsedTime += m_config.fixedTimeStep;
        m_frameCount++;

        if (m_scene)
            m_scene->Update(m_config.fixedTimeStep);
    }

    // --- フレーム / 時間 ---
    uint64_t GetFrameCount() const { return m_frameCount; }
    float GetElapsedTime() const { return m_elapsedTime; }

    void Update(float deltaTime)
    {
        if (m_state != PIEState::Playing)
            return;

        m_elapsedTime += deltaTime;
        m_frameCount++;

        if (m_scene)
            m_scene->Update(deltaTime);
    }

    // --- スナップショット ---

    /// @brief 対象シーンのスナップショットを取得する
    PIESnapshot TakeSnapshot(const Scene& scene) const
    {
        PIESnapshot snap;
        snap.entityCount = scene.GetEntityCount();
        snap.hasData = true;
        snap.componentCount = 0;
        return snap;
    }

    /// @brief スナップショットからシーンを復元する
    void RestoreSnapshot(Scene& scene, const PIESnapshot& /*snapshot*/) const
    {
        if (m_savedEntities.empty())
            return;

        for (const auto& s : m_savedEntities)
        {
            Entity* target = scene.FindEntityByID(s.id);
            if (!target)
                target = scene.FindEntity(s.name);
            if (target)
            {
                target->SetName(s.name);
                target->SetActive(s.active);
                target->GetTransform().SetPosition(s.posX, s.posY, s.posZ);
                target->GetTransform().SetRotation(s.rotX, s.rotY, s.rotZ);
                target->GetTransform().SetScale(s.sclX, s.sclY, s.sclZ);
            }
        }
        scene.SetName(m_savedSceneName);
    }

    /// @brief スナップショットが存在するか
    bool HasSnapshot() const { return !m_savedEntities.empty(); }

    /// @brief 現在のスナップショットを取得する
    const PIESnapshot& GetSnapshot() const { return m_snapshotInfo; }

    // --- コールバック ---
    std::function<void()> OnEnterPlay;
    std::function<void()> OnExitPlay;

private:
    void CaptureSnapshot()
    {
        m_savedEntities.clear();
        const auto& entities = m_scene->GetEntities();
        m_savedEntities.reserve(entities.size());

        for (const auto& e : entities)
        {
            PIEEntityState s;
            s.id     = e->GetID();
            s.name   = e->GetName();
            s.active = e->IsActive();

            const auto& pos = e->GetTransform().GetPosition();
            s.posX = pos.x; s.posY = pos.y; s.posZ = pos.z;

            const auto& rot = e->GetTransform().GetRotation();
            s.rotX = rot.x; s.rotY = rot.y; s.rotZ = rot.z;

            const auto& scl = e->GetTransform().GetScale();
            s.sclX = scl.x; s.sclY = scl.y; s.sclZ = scl.z;

            m_savedEntities.push_back(std::move(s));
        }

        m_savedSceneName = m_scene->GetName();
        m_snapshotInfo.entityCount = m_scene->GetEntityCount();
        m_snapshotInfo.hasData = true;
    }

    void RestoreFromSaved()
    {
        for (const auto& s : m_savedEntities)
        {
            Entity* target = m_scene->FindEntityByID(s.id);
            if (!target)
                target = m_scene->FindEntity(s.name);
            if (target)
            {
                target->SetName(s.name);
                target->SetActive(s.active);
                target->GetTransform().SetPosition(s.posX, s.posY, s.posZ);
                target->GetTransform().SetRotation(s.rotX, s.rotY, s.rotZ);
                target->GetTransform().SetScale(s.sclX, s.sclY, s.sclZ);
            }
        }
        m_scene->SetName(m_savedSceneName);
    }

    PIEState     m_state       = PIEState::Stopped; ///< 現在のPIE実行状態
    PIEConfig    m_config;                          ///< PIE設定
    PIESnapshot m_snapshotInfo;                     ///< 最新のスナップショット情報
    std::vector<PIEEntityState> m_savedEntities;    ///< 保存されたエンティティ状態リスト
    std::string  m_savedSceneName;                  ///< 保存されたシーン名
    uint64_t     m_frameCount  = 0;                 ///< プレイ開始からの累積フレーム数
    float        m_elapsedTime = 0.0f;              ///< プレイ開始からの累積時間（秒）
    Scene*       m_scene       = nullptr;           ///< 対象シーンへのポインタ
};

} // namespace gx
/// @}
