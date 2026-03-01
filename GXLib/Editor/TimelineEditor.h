#pragma once
/// @file TimelineEditor.h
/// @brief タイムラインエディタ — キーフレームを時間軸上に並べてアニメーションを作る
///
/// Transform・プロパティ・イベント・音声などのトラックにキーフレームを配置し、
/// Linear/Bezier 等の補間モードで滑らかに動く演出を作成できる。
/// カットシーンやUIアニメーションの制作に使う。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <optional>

namespace gx
{

/// @brief エディタトラック種別
enum class EditorTrackType
{
    Transform, ///< 位置・回転・スケールのトラック
    Property,  ///< 任意プロパティ値のトラック
    Event,     ///< イベント発火トラック
    Audio,     ///< オーディオトラック
    Cutscene   ///< カットシーン制御トラック
};

/// @brief 補間モード
enum class InterpolationMode
{
    Linear,    ///< 線形補間
    EaseIn,    ///< イーズイン（加速）
    EaseOut,   ///< イーズアウト（減速）
    EaseInOut, ///< イーズインアウト（加速→減速）
    Bezier,    ///< ベジェ曲線（スムーズステップ）
    Step       ///< ステップ（即時切替）
};

/// @brief カーブ評価ユーティリティ
class CurveEvaluator
{
public:
    /// @brief 補間モードに基づいてtを再マッピングする
    /// @param mode 補間モード
    /// @param t 0-1の正規化時間
    /// @return 再マッピングされたt値
    static float EvaluateInterpolation(InterpolationMode mode, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        switch (mode)
        {
        case InterpolationMode::Linear:
            return t;
        case InterpolationMode::EaseIn:
            return t * t;
        case InterpolationMode::EaseOut:
            return t * (2.0f - t);
        case InterpolationMode::EaseInOut:
            return 3.0f * t * t - 2.0f * t * t * t;
        case InterpolationMode::Bezier:
            return 3.0f * t * t - 2.0f * t * t * t; // smoothstep default
        case InterpolationMode::Step:
            return t >= 1.0f ? 1.0f : 0.0f;
        default:
            return t;
        }
    }
};

/// @brief タイムラインキーフレーム
struct TimelineKeyframe
{
    float time = 0.0f;                                       ///< キーフレーム時刻（秒）
    float value[3] = { 0.0f, 0.0f, 0.0f };                   ///< キーフレーム値（float3）
    InterpolationMode interpolation = InterpolationMode::Linear; ///< 補間モード
};

/// @brief エディタトラック
struct EditorTrack
{
    std::string name;                                        ///< トラック名
    EditorTrackType type = EditorTrackType::Property;        ///< トラック種別
    bool muted = false;                                      ///< ミュート状態
    bool locked = false;                                     ///< ロック状態（編集不可）
    std::vector<TimelineKeyframe> keyframes;                 ///< キーフレームリスト
};

/// @brief タイムラインエディタ
class TimelineEditor
{
public:
    TimelineEditor() = default;
    ~TimelineEditor() = default;

    // --- トラック管理 ---
    uint32_t AddTrack(const std::string& name, EditorTrackType type);
    void RemoveTrack(uint32_t trackIndex);
    EditorTrack* GetTrack(uint32_t trackIndex);
    const EditorTrack* GetTrack(uint32_t trackIndex) const;
    uint32_t GetTrackCount() const { return static_cast<uint32_t>(m_tracks.size()); }

    // --- キーフレーム操作 ---
    uint32_t AddKeyframe(uint32_t trackIndex, float time, float v0, float v1 = 0.0f, float v2 = 0.0f);
    void RemoveKeyframe(uint32_t trackIndex, uint32_t keyframeIndex);
    void MoveKeyframe(uint32_t trackIndex, uint32_t keyframeIndex, float newTime);
    void SetKeyframeValue(uint32_t trackIndex, uint32_t keyframeIndex, float v0, float v1 = 0.0f, float v2 = 0.0f);
    void SetKeyframeInterpolation(uint32_t trackIndex, uint32_t keyframeIndex, InterpolationMode mode);

    // --- 再生制御 ---
    void Play();
    void Pause();
    void Stop();
    void SetCurrentTime(float time);
    float GetCurrentTime() const { return m_currentTime; }
    bool IsPlaying() const { return m_playing; }
    bool IsPaused() const { return m_paused; }

    // --- デュレーション ---
    void SetDuration(float duration);
    float GetDuration() const { return m_duration; }

    // --- 再生速度 ---
    void SetPlaybackSpeed(float speed);
    float GetPlaybackSpeed() const { return m_playbackSpeed; }

    // --- ループ ---
    void SetLooping(bool looping) { m_looping = looping; }
    bool IsLooping() const { return m_looping; }

    // --- トラックミュート/ロック ---
    void MuteTrack(uint32_t trackIndex, bool muted);
    void LockTrack(uint32_t trackIndex, bool locked);
    bool IsTrackMuted(uint32_t trackIndex) const;
    bool IsTrackLocked(uint32_t trackIndex) const;

    // --- トラック評価 ---
    /// @brief 指定トラックを指定時刻で評価する
    /// @param trackIndex トラックインデックス
    /// @param time 評価する時刻
    /// @param outValue 出力値（float3）
    /// @return 有効な値が得られた場合true
    bool EvaluateTrack(uint32_t trackIndex, float time, float outValue[3]) const;

    // --- コピー/ペースト ---
    void CopyKeyframe(uint32_t trackIndex, uint32_t keyframeIndex);
    uint32_t PasteKeyframe(uint32_t trackIndex, float time);
    bool HasClipboard() const { return m_clipboard.has_value(); }
    const std::optional<TimelineKeyframe>& GetClipboard() const { return m_clipboard; }

    // --- Undo/Redo ---
    void Undo();
    void Redo();
    bool CanUndo() const { return !m_undoStack.empty(); }
    bool CanRedo() const { return !m_redoStack.empty(); }

private:
    void PushUndoState();

    std::vector<EditorTrack> m_tracks;   ///< トラックリスト
    float m_currentTime = 0.0f;          ///< 現在の再生位置（秒）
    float m_duration = 10.0f;            ///< タイムラインの総尺（秒）
    float m_playbackSpeed = 1.0f;        ///< 再生速度倍率
    bool m_playing = false;              ///< 再生中フラグ
    bool m_paused = false;               ///< 一時停止フラグ
    bool m_looping = false;              ///< ループ再生フラグ

    std::optional<TimelineKeyframe> m_clipboard; ///< コピーしたキーフレーム

    /// @brief Undo/Redo用の全トラック状態スナップショット
    struct UndoState
    {
        std::vector<EditorTrack> tracks; ///< トラック状態のコピー
    };
    std::vector<UndoState> m_undoStack;  ///< Undoスタック
    std::vector<UndoState> m_redoStack;  ///< Redoスタック
    static constexpr size_t k_MaxUndoSteps = 50;
};

} // namespace gx
/// @}
