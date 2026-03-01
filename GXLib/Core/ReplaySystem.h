#pragma once
/// @file ReplaySystem.h
/// @brief リプレイシステム — ゲーム状態の録画・再生
///
/// ReplayRecorder でフレームごとのエンティティ状態を記録し、
/// ReplayPlayer で任意の速度・シーク位置で再生する。
/// タイムスタンプベースの補間とループ再生に対応。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <unordered_map>

namespace gx
{

/// @brief リプレイフレーム内のエンティティ状態
struct ReplayEntityState
{
    uint32_t entityId;     ///< エンティティID
    float position[3];     ///< 位置 (x, y, z)
    float rotation[4];     ///< 回転 (クォータニオン x, y, z, w)
    float scale[3];        ///< スケール (x, y, z)
};

/// @brief 1フレーム分のリプレイデータ
struct ReplayFrame
{
    float    timestamp;   ///< タイムスタンプ（秒）
    uint32_t frameNumber; ///< フレーム番号
    std::vector<ReplayEntityState> entities;                ///< エンティティ状態リスト
    std::unordered_map<std::string, float> variables;       ///< カスタム変数
};

/// @brief リプレイヘッダー情報
struct ReplayHeader
{
    std::string name;           ///< リプレイ名
    float       duration   = 0.0f; ///< 合計時間（秒）
    uint32_t    frameCount = 0;    ///< フレーム数
};

/// @brief リプレイ録画クラス
class ReplayRecorder
{
public:
    /// @brief 録画を開始する
    /// @param name リプレイ名（省略可）
    void StartRecording(const std::string& name = "");

    /// @brief 録画を停止する
    void StopRecording();

    /// @brief 録画中かどうか
    /// @return 録画中なら true
    bool IsRecording() const;

    /// @brief 1フレーム分のエンティティ状態を記録する
    /// @param timestamp フレームのタイムスタンプ（秒）
    /// @param entities エンティティ状態の配列
    void RecordFrame(float timestamp, const std::vector<ReplayEntityState>& entities);

    /// @brief カスタム変数を現在のフレームに設定する
    /// @param name 変数名
    /// @param value 変数値
    void SetVariable(const std::string& name, float value);

    /// @brief ヘッダー情報を返す
    /// @return リプレイヘッダーへの参照
    const ReplayHeader& GetHeader() const;

    /// @brief 全フレームデータを返す
    /// @return フレームデータ配列への参照
    const std::vector<ReplayFrame>& GetFrames() const;

    /// @brief フレーム数を返す
    /// @return 録画済みフレーム数
    size_t GetFrameCount() const;

    /// @brief 合計録画時間を返す
    /// @return 録画時間（秒）
    float GetDuration() const;

    /// @brief 録画データをクリアする
    void Clear();

    /// @brief 最大フレーム数を設定する
    /// @param max 最大フレーム数
    void SetMaxFrames(size_t max);

    /// @brief 録画間隔を設定する（秒）
    /// @param interval 録画間隔（秒）
    void SetRecordInterval(float interval);

private:
    ReplayHeader              m_header;                      ///< リプレイヘッダー
    std::vector<ReplayFrame>  m_frames;                      ///< 録画フレームデータ
    bool   m_recording      = false;                         ///< 録画中フラグ
    float  m_lastRecordTime  = -1.0f;                        ///< 最後に記録したタイムスタンプ
    float  m_recordInterval  = 1.0f / 30.0f;                 ///< 録画間隔（デフォルト 30fps）
    size_t m_maxFrames       = 18000;                        ///< 最大フレーム数（デフォルト 10分@30fps）
    std::unordered_map<std::string, float> m_currentVars;    ///< 現在のカスタム変数
};

/// @brief リプレイ再生クラス
class ReplayPlayer
{
public:
    /// @brief リプレイデータを設定する
    /// @param frames フレームデータ配列
    /// @param header リプレイヘッダー
    void SetReplay(const std::vector<ReplayFrame>& frames, const ReplayHeader& header);

    /// @brief リプレイデータが設定済みかどうか
    /// @return データが設定済みなら true
    bool HasReplay() const;

    /// @brief 再生を開始する
    void Play();

    /// @brief 再生を一時停止する
    void Pause();

    /// @brief 再生を停止する（先頭に戻る）
    void Stop();

    /// @brief 指定タイムスタンプにシークする
    /// @param timestamp シーク先のタイムスタンプ（秒）
    void Seek(float timestamp);

    /// @brief 再生を更新する
    /// @param deltaTime デルタタイム（秒）
    void Update(float deltaTime);

    /// @brief 再生中かどうか
    /// @return 再生中なら true
    bool IsPlaying() const;

    /// @brief 一時停止中かどうか
    /// @return 一時停止中なら true
    bool IsPaused() const;

    /// @brief 再生が終端に達したかどうか
    /// @return 終端に達していれば true
    bool IsFinished() const;

    /// @brief 現在の再生時刻を返す
    /// @return 再生時刻（秒）
    float GetCurrentTime() const;

    /// @brief 合計再生時間を返す
    /// @return 合計時間（秒）
    float GetDuration() const;

    /// @brief 現在のフレームインデックスを返す
    /// @return フレームインデックス
    uint32_t GetCurrentFrameIndex() const;

    /// @brief 再生速度を返す
    /// @return 再生速度倍率（1.0 が等速）
    float GetPlaybackSpeed() const;

    /// @brief 再生速度を設定する
    /// @param speed 再生速度倍率（1.0 が等速）
    void SetPlaybackSpeed(float speed);

    /// @brief ループ再生の有効/無効を設定する
    /// @param loop true でループ有効
    void SetLooping(bool loop);

    /// @brief ループ再生が有効かどうか
    /// @return ループ有効なら true
    bool IsLooping() const;

    /// @brief 現在のフレームデータを返す（存在しない場合は nullptr）
    /// @return フレームデータへのポインタ、またはnullptr
    const ReplayFrame* GetCurrentFrame() const;

private:
    std::vector<ReplayFrame> m_frames;    ///< リプレイフレームデータ
    ReplayHeader  m_header;               ///< リプレイヘッダー
    float    m_currentTime     = 0.0f;    ///< 現在の再生時刻（秒）
    float    m_playbackSpeed   = 1.0f;    ///< 再生速度倍率
    bool     m_playing         = false;   ///< 再生中フラグ
    bool     m_paused          = false;   ///< 一時停止フラグ
    bool     m_looping         = false;   ///< ループ再生フラグ
    uint32_t m_currentFrameIdx = 0;       ///< 現在のフレームインデックス

    /// @brief 現在時刻に対応するフレームインデックスを検索する
    void UpdateFrameIndex();
};

} // namespace gx
/// @}
