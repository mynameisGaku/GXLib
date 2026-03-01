#pragma once
/// @file RollbackNetcode.h
/// @brief ロールバックネットコード（GGPO風入力遅延＋ロールバック＋再シミュレーション）
///
/// 格闘ゲームやアクションゲーム向けの決定論的ネットコードシステム。
/// 各フレームの入力を交換し、予測が外れた場合にゲーム状態をロールバックして
/// 再シミュレーションを行う。
///
/// 参考: GGPO (Good Game Peace Out), Gaffer On Games "Networked Physics"

#include "pch_common.h"
#include <vector>
#include <array>
#include <functional>
#include <cstdint>
#include <cstring>

namespace gx
{

/// @addtogroup grp_network
/// @{

// ─────────────────────────────────────────────────────────
// Structs
// ─────────────────────────────────────────────────────────

/// @brief プレイヤー入力（1フレーム分）
struct PlayerInput
{
    uint32_t buttons = 0;     ///< ボタンビットマスク
    float    axisX   = 0.0f;  ///< 水平軸（-1〜+1）
    float    axisY   = 0.0f;  ///< 垂直軸（-1〜+1）

    bool operator==(const PlayerInput& o) const
    {
        return buttons == o.buttons && axisX == o.axisX && axisY == o.axisY;
    }
    bool operator!=(const PlayerInput& o) const { return !(*this == o); }
};

/// @brief ゲーム状態スナップショット
struct GameStateSnapshot
{
    std::vector<uint8_t> data;       ///< シリアライズされたゲーム状態
    uint32_t             checksum = 0; ///< CRC32チェックサム
    int                  frame    = -1; ///< フレーム番号
};

/// @brief ロールバックコールバック群
struct RollbackCallbacks
{
    /// ゲーム状態を保存する
    std::function<GameStateSnapshot()> saveState;
    /// ゲーム状態を復元する
    std::function<void(const GameStateSnapshot&)> loadState;
    /// 1フレーム進める（全プレイヤーの入力を受け取る）
    std::function<void(const std::vector<PlayerInput>&)> advanceFrame;
    /// チェックサムを計算する
    std::function<uint32_t(const GameStateSnapshot&)> computeChecksum;
};

/// @brief ロールバックネットコード設定
struct RollbackSettings
{
    int maxPlayers  = 2;     ///< 最大プレイヤー数（1-4）
    int inputDelay  = 2;     ///< 入力遅延フレーム数（1-8）
    int maxRollback = 8;     ///< 最大ロールバックフレーム数
    int historySize = 256;   ///< 入力履歴リングバッファサイズ
};

// ─────────────────────────────────────────────────────────
// RollbackNetcode
// ─────────────────────────────────────────────────────────

/// @brief ロールバックネットコードシステム
///
/// GGPO スタイルの決定論的ネットコード。各プレイヤーの入力を収集し、
/// 予測が外れた場合にロールバック＋再シミュレーションを行う。
class RollbackNetcode
{
public:
    /// @brief デフォルトコンストラクタ
    RollbackNetcode() = default;

    /// @brief デストラクタ（Shutdown を呼ぶ）
    ~RollbackNetcode();

    /// @brief システムを初期化する
    /// @param settings ロールバック設定
    /// @param callbacks コールバック群
    /// @return 成功した場合 true
    bool Initialize(const RollbackSettings& settings, const RollbackCallbacks& callbacks);

    /// @brief システムをシャットダウンする
    void Shutdown();

    /// @brief 現在の設定を取得する
    /// @return 設定への const 参照
    const RollbackSettings& GetSettings() const { return m_settings; }

    // ─── 入力管理 ───

    /// @brief ローカルプレイヤーの入力を登録する
    /// @param playerIndex プレイヤーインデックス（0-based）
    /// @param input 入力データ
    void AddLocalInput(int playerIndex, const PlayerInput& input);

    /// @brief リモートプレイヤーの入力を登録する
    /// @param playerIndex プレイヤーインデックス（0-based）
    /// @param frame 入力が対応するフレーム番号
    /// @param input 入力データ
    void AddRemoteInput(int playerIndex, int frame, const PlayerInput& input);

    /// @brief フレームを進める（予測／ロールバック含む）
    void AdvanceFrame();

    // ─── 状態クエリ ───

    /// @brief 現在のフレーム番号を返す
    int GetCurrentFrame() const { return m_currentFrame; }

    /// @brief 全プレイヤーの入力が確定した最新フレームを返す
    int GetConfirmedFrame() const { return m_confirmedFrame; }

    /// @brief 予測入力を返す
    /// @param playerIndex プレイヤーインデックス
    /// @param frame フレーム番号
    /// @return 予測された入力
    PlayerInput GetPredictedInput(int playerIndex, int frame) const;

    /// @brief 確定入力を返す（未確定なら nullptr）
    /// @param playerIndex プレイヤーインデックス
    /// @param frame フレーム番号
    /// @return 確定入力へのポインタ、または nullptr
    const PlayerInput* GetInputForFrame(int playerIndex, int frame) const;

    /// @brief デシンクが検出されたかどうか
    bool IsDesyncDetected() const { return m_desyncDetected; }

    /// @brief デシンクが検出されたフレーム（未検出なら -1）
    int GetDesyncFrame() const { return m_desyncFrame; }

    /// @brief ローカル vs リモートのフレーム差
    int GetFrameAdvantage() const;

    /// @brief 直近フレームのロールバック数
    int GetRollbackCount() const { return m_lastRollbackCount; }

    /// @brief 入力履歴のサイズ
    int GetInputHistorySize() const { return m_settings.historySize; }

    /// @brief 入力遅延を動的に変更する（1〜8 にクランプ）
    /// @param frames 新しい遅延フレーム数
    void SetInputDelay(int frames);

private:
    /// @brief 予測入力を生成する（直近の確定入力をリピート）
    PlayerInput PredictInput(int playerIndex, int frame) const;

    /// @brief 指定フレームまでロールバックして再シミュレーションする
    void PerformRollback(int toFrame);

    /// @brief 現在の状態を履歴に保存する
    void SaveState();

    /// @brief 確定フレームのチェックサムを比較しデシンクを検出する
    void CheckDesync(int frame);

    /// @brief 指定フレームの入力履歴インデックスを計算する
    int HistoryIndex(int frame) const;

    /// @brief 確定フレームを更新する
    void UpdateConfirmedFrame();

    // ─── メンバ変数 ───

    RollbackSettings  m_settings{};
    RollbackCallbacks m_callbacks{};

    int  m_currentFrame      = 0;
    int  m_confirmedFrame    = -1;

    /// @brief プレイヤーごとの入力履歴（リングバッファ）
    std::array<std::vector<PlayerInput>, 4> m_inputHistory;

    /// @brief プレイヤーごとの確定済み入力フラグ（リングバッファ）
    std::array<std::vector<bool>, 4> m_inputConfirmed;

    /// @brief ゲーム状態履歴（リングバッファ）
    std::vector<GameStateSnapshot> m_stateHistory;

    bool m_desyncDetected    = false;
    int  m_desyncFrame       = -1;
    int  m_lastRollbackCount = 0;
    bool m_initialized       = false;
};

/// @}

} // namespace gx
