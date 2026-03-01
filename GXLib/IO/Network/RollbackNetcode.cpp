/// @file RollbackNetcode.cpp
/// @brief ロールバックネットコード実装

#include "pch_common.h"
#include "IO/Network/RollbackNetcode.h"
#include <algorithm>
#include <numeric>

namespace gx
{

// ─────────────────────────────────────────────────────────
// Destructor
// ─────────────────────────────────────────────────────────

RollbackNetcode::~RollbackNetcode()
{
    Shutdown();
}

// ─────────────────────────────────────────────────────────
// Initialize / Shutdown
// ─────────────────────────────────────────────────────────

bool RollbackNetcode::Initialize(const RollbackSettings& settings,
                                 const RollbackCallbacks& callbacks)
{
    if (m_initialized)
        return false;

    // コールバック検証
    if (!callbacks.saveState || !callbacks.loadState || !callbacks.advanceFrame)
        return false;

    // 設定のクランプ
    m_settings = settings;
    m_settings.maxPlayers  = std::clamp(m_settings.maxPlayers, 1, 4);
    m_settings.inputDelay  = std::clamp(m_settings.inputDelay, 1, 8);
    m_settings.maxRollback = std::max(m_settings.maxRollback, 1);
    m_settings.historySize = std::max(m_settings.historySize, 16);

    m_callbacks = callbacks;

    // 入力履歴の初期化
    for (int p = 0; p < 4; ++p)
    {
        m_inputHistory[p].resize(m_settings.historySize);
        m_inputConfirmed[p].resize(m_settings.historySize, false);
    }

    // 状態履歴の初期化（maxRollback + inputDelay + マージン分）
    int stateHistSize = m_settings.maxRollback + m_settings.inputDelay + 4;
    m_stateHistory.resize(stateHistSize);

    m_currentFrame      = 0;
    m_confirmedFrame    = -1;
    m_desyncDetected    = false;
    m_desyncFrame       = -1;
    m_lastRollbackCount = 0;
    m_initialized       = true;

    return true;
}

void RollbackNetcode::Shutdown()
{
    if (!m_initialized)
        return;

    for (int p = 0; p < 4; ++p)
    {
        m_inputHistory[p].clear();
        m_inputConfirmed[p].clear();
    }
    m_stateHistory.clear();

    m_callbacks      = {};
    m_currentFrame   = 0;
    m_confirmedFrame = -1;
    m_desyncDetected = false;
    m_desyncFrame    = -1;
    m_lastRollbackCount = 0;
    m_initialized    = false;
}

// ─────────────────────────────────────────────────────────
// Input Management
// ─────────────────────────────────────────────────────────

void RollbackNetcode::AddLocalInput(int playerIndex, const PlayerInput& input)
{
    if (!m_initialized)
        return;
    if (playerIndex < 0 || playerIndex >= m_settings.maxPlayers)
        return;

    // ローカル入力は inputDelay フレーム先に登録する
    int targetFrame = m_currentFrame + m_settings.inputDelay;
    int idx = HistoryIndex(targetFrame);

    m_inputHistory[playerIndex][idx]  = input;
    m_inputConfirmed[playerIndex][idx] = true;
}

void RollbackNetcode::AddRemoteInput(int playerIndex, int frame, const PlayerInput& input)
{
    if (!m_initialized)
        return;
    if (playerIndex < 0 || playerIndex >= m_settings.maxPlayers)
        return;
    if (frame < 0)
        return;

    int idx = HistoryIndex(frame);

    // 既に確定済みの入力が異なる場合はロールバックが必要
    bool needsRollback = false;
    if (frame < m_currentFrame)
    {
        // 過去のフレームに対する入力 — 予測と異なればロールバック
        if (!m_inputConfirmed[playerIndex][idx] ||
            m_inputHistory[playerIndex][idx] != input)
        {
            needsRollback = true;
        }
    }

    m_inputHistory[playerIndex][idx]  = input;
    m_inputConfirmed[playerIndex][idx] = true;

    // 確定フレームを更新
    UpdateConfirmedFrame();

    // ロールバックが必要な場合
    if (needsRollback && frame < m_currentFrame)
    {
        int rollbackFrames = m_currentFrame - frame;
        if (rollbackFrames <= m_settings.maxRollback)
        {
            PerformRollback(frame);
        }
    }
}

// ─────────────────────────────────────────────────────────
// Frame Advance
// ─────────────────────────────────────────────────────────

void RollbackNetcode::AdvanceFrame()
{
    if (!m_initialized)
        return;

    m_lastRollbackCount = 0;

    // 1. 全プレイヤーの入力を収集（確定または予測）
    std::vector<PlayerInput> frameInputs(m_settings.maxPlayers);
    for (int p = 0; p < m_settings.maxPlayers; ++p)
    {
        const PlayerInput* confirmed = GetInputForFrame(p, m_currentFrame);
        if (confirmed)
        {
            frameInputs[p] = *confirmed;
        }
        else
        {
            frameInputs[p] = PredictInput(p, m_currentFrame);
            // 予測入力を履歴に仮登録（後でロールバック時に使用）
            int idx = HistoryIndex(m_currentFrame);
            m_inputHistory[p][idx] = frameInputs[p];
            // confirmed フラグは立てない（予測なので）
        }
    }

    // 2. 現在の状態を履歴に保存
    SaveState();

    // 3. コールバックでフレームを進める
    m_callbacks.advanceFrame(frameInputs);

    // 4. フレーム番号をインクリメント
    m_currentFrame++;

    // 5. 確定フレームを更新
    UpdateConfirmedFrame();

    // 6. デシンクチェック（確定フレームがあれば）
    if (m_confirmedFrame >= 0 && m_callbacks.computeChecksum)
    {
        CheckDesync(m_confirmedFrame);
    }
}

// ─────────────────────────────────────────────────────────
// State Queries
// ─────────────────────────────────────────────────────────

PlayerInput RollbackNetcode::GetPredictedInput(int playerIndex, int frame) const
{
    if (!m_initialized)
        return {};
    if (playerIndex < 0 || playerIndex >= m_settings.maxPlayers)
        return {};

    return PredictInput(playerIndex, frame);
}

const PlayerInput* RollbackNetcode::GetInputForFrame(int playerIndex, int frame) const
{
    if (!m_initialized)
        return nullptr;
    if (playerIndex < 0 || playerIndex >= m_settings.maxPlayers)
        return nullptr;
    if (frame < 0)
        return nullptr;

    int idx = HistoryIndex(frame);
    if (m_inputConfirmed[playerIndex][idx])
    {
        return &m_inputHistory[playerIndex][idx];
    }

    return nullptr;
}

int RollbackNetcode::GetFrameAdvantage() const
{
    return m_currentFrame - m_confirmedFrame;
}

void RollbackNetcode::SetInputDelay(int frames)
{
    if (!m_initialized)
        return;
    m_settings.inputDelay = std::clamp(frames, 1, 8);
}

// ─────────────────────────────────────────────────────────
// Private — Prediction
// ─────────────────────────────────────────────────────────

PlayerInput RollbackNetcode::PredictInput(int playerIndex, int frame) const
{
    // 直近の確定入力を探す（frame-1 から遡る）
    for (int f = frame - 1; f >= 0 && f >= frame - m_settings.historySize; --f)
    {
        int idx = HistoryIndex(f);
        if (m_inputConfirmed[playerIndex][idx])
        {
            return m_inputHistory[playerIndex][idx];
        }
    }

    // 確定入力が無い場合はデフォルト（ニュートラル）
    return {};
}

// ─────────────────────────────────────────────────────────
// Private — Rollback
// ─────────────────────────────────────────────────────────

void RollbackNetcode::PerformRollback(int toFrame)
{
    if (!m_initialized)
        return;
    if (toFrame < 0 || toFrame >= m_currentFrame)
        return;

    // 1. toFrame の状態スナップショットを探す
    int stateIdx = toFrame % static_cast<int>(m_stateHistory.size());
    const auto& snapshot = m_stateHistory[stateIdx];

    if (snapshot.frame != toFrame)
    {
        // スナップショットが見つからない（履歴が足りない）
        return;
    }

    // 2. 状態を復元
    m_callbacks.loadState(snapshot);

    // 3. toFrame から currentFrame まで再シミュレーション
    int savedCurrentFrame = m_currentFrame;
    m_currentFrame = toFrame;

    for (int f = toFrame; f < savedCurrentFrame; ++f)
    {
        // 全プレイヤーの入力を収集
        std::vector<PlayerInput> frameInputs(m_settings.maxPlayers);
        for (int p = 0; p < m_settings.maxPlayers; ++p)
        {
            const PlayerInput* confirmed = GetInputForFrame(p, f);
            if (confirmed)
            {
                frameInputs[p] = *confirmed;
            }
            else
            {
                frameInputs[p] = PredictInput(p, f);
                int idx = HistoryIndex(f);
                m_inputHistory[p][idx] = frameInputs[p];
            }
        }

        // 状態を保存（再シミュレーション中も状態履歴を更新）
        SaveState();

        // フレームを進める
        m_callbacks.advanceFrame(frameInputs);
        m_currentFrame++;
    }

    m_lastRollbackCount = savedCurrentFrame - toFrame;
}

// ─────────────────────────────────────────────────────────
// Private — State Management
// ─────────────────────────────────────────────────────────

void RollbackNetcode::SaveState()
{
    if (!m_callbacks.saveState)
        return;

    GameStateSnapshot snapshot = m_callbacks.saveState();
    snapshot.frame = m_currentFrame;

    // チェックサムを計算
    if (m_callbacks.computeChecksum)
    {
        snapshot.checksum = m_callbacks.computeChecksum(snapshot);
    }

    int stateIdx = m_currentFrame % static_cast<int>(m_stateHistory.size());
    m_stateHistory[stateIdx] = std::move(snapshot);
}

// ─────────────────────────────────────────────────────────
// Private — Desync Detection
// ─────────────────────────────────────────────────────────

void RollbackNetcode::CheckDesync(int frame)
{
    if (m_desyncDetected)
        return; // 既に検出済み

    if (!m_callbacks.computeChecksum)
        return;

    // 対象フレームの状態スナップショットを取得
    int stateIdx = frame % static_cast<int>(m_stateHistory.size());
    const auto& snapshot = m_stateHistory[stateIdx];

    if (snapshot.frame != frame)
        return; // スナップショットが無い

    // チェックサムを再計算して比較
    uint32_t currentChecksum = m_callbacks.computeChecksum(snapshot);
    if (currentChecksum != snapshot.checksum)
    {
        m_desyncDetected = true;
        m_desyncFrame    = frame;
    }
}

// ─────────────────────────────────────────────────────────
// Private — Helpers
// ─────────────────────────────────────────────────────────

int RollbackNetcode::HistoryIndex(int frame) const
{
    // frame を正の値として扱い、historySize でモジュロ
    int size = m_settings.historySize;
    return ((frame % size) + size) % size;
}

void RollbackNetcode::UpdateConfirmedFrame()
{
    // confirmedFrame+1 から順にチェック
    for (int f = m_confirmedFrame + 1; f < m_currentFrame; ++f)
    {
        bool allConfirmed = true;
        int idx = HistoryIndex(f);
        for (int p = 0; p < m_settings.maxPlayers; ++p)
        {
            if (!m_inputConfirmed[p][idx])
            {
                allConfirmed = false;
                break;
            }
        }

        if (allConfirmed)
        {
            m_confirmedFrame = f;
        }
        else
        {
            break; // 連続していない場合は止める
        }
    }
}

} // namespace gx
