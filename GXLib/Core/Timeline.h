#pragma once
/// @file Timeline.h
/// @brief タイムライン/シーケンサー（キーフレームアニメーション + イベント）
///
/// カットシーンやUI演出等で使う、時間ベースのアニメーション管理。
/// 複数のトラック（Float3Track, EventTrack等）を持ち、
/// Play/Pause/Stop/Seek で制御する。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx
{

/// @brief タイムライントラック基底
///
/// Float3Track, FloatTrack, EventTrack の共通インターフェース。
/// Evaluate() で指定時刻の値を評価し、GetDuration() で長さを返す。
class TimelineTrack
{
public:
    virtual ~TimelineTrack() = default;

    /// @brief 指定時刻のトラック値を評価する
    /// @param time 評価する時刻（秒）
    virtual void Evaluate(float time) = 0;

    /// @brief トラックの再生時間を取得する
    /// @return トラック長（秒）
    virtual float GetDuration() const = 0;
};

/// @brief Float3トラック（位置/回転/スケール等のアニメーション）
class Float3Track : public TimelineTrack
{
public:
    /// @brief キーフレームデータ
    struct Keyframe
    {
        float time;       ///< キーフレーム時刻（秒）
        XMFLOAT3 value;   ///< キーフレーム値（3Dベクトル）
    };

    /// @brief キーフレームを追加する
    /// @param time キーフレーム時刻（秒）
    /// @param value キーフレーム値
    void AddKeyframe(float time, const XMFLOAT3& value);

    /// @brief 値の出力先を設定する
    /// @param target 評価結果を受け取るコールバック
    void SetTarget(std::function<void(const XMFLOAT3&)> target) { m_target = std::move(target); }

    /// @brief 指定時刻のトラック値を評価してターゲットに出力する
    /// @param time 評価する時刻（秒）
    void Evaluate(float time) override;

    /// @brief トラックの再生時間を取得する
    /// @return 最後のキーフレーム時刻（秒）
    float GetDuration() const override;

private:
    std::vector<Keyframe> m_keyframes;                ///< キーフレーム配列（時刻順）
    std::function<void(const XMFLOAT3&)> m_target;   ///< 値の出力先コールバック
};

/// @brief Floatトラック（スカラーアニメーション）
class FloatTrack : public TimelineTrack
{
public:
    /// @brief キーフレームデータ
    struct Keyframe
    {
        float time;   ///< キーフレーム時刻（秒）
        float value;  ///< キーフレーム値（スカラー）
    };

    /// @brief キーフレームを追加する
    /// @param time キーフレーム時刻（秒）
    /// @param value キーフレーム値
    void AddKeyframe(float time, float value);

    /// @brief 値の出力先を設定する
    /// @param target 評価結果を受け取るコールバック
    void SetTarget(std::function<void(float)> target) { m_target = std::move(target); }

    /// @brief 指定時刻のトラック値を評価してターゲットに出力する
    /// @param time 評価する時刻（秒）
    void Evaluate(float time) override;

    /// @brief トラックの再生時間を取得する
    /// @return 最後のキーフレーム時刻（秒）
    float GetDuration() const override;

private:
    std::vector<Keyframe> m_keyframes;         ///< キーフレーム配列（時刻順）
    std::function<void(float)> m_target;       ///< 値の出力先コールバック
};

/// @brief イベントトラック（特定時刻にコールバック発火）
class EventTrack : public TimelineTrack
{
public:
    /// @brief イベントデータ
    struct Event
    {
        float time;                       ///< イベント発火時刻（秒）
        std::function<void()> callback;   ///< 発火時に呼ばれるコールバック
        bool fired = false;               ///< 発火済みフラグ
    };

    /// @brief イベントを追加する
    /// @param time イベント発火時刻（秒）
    /// @param callback 発火時に呼ばれるコールバック
    void AddEvent(float time, std::function<void()> callback);

    /// @brief 指定時刻のイベントを評価・発火する
    /// @param time 評価する時刻（秒）
    void Evaluate(float time) override;

    /// @brief トラックの再生時間を取得する
    /// @return 最後のイベント時刻（秒）
    float GetDuration() const override;

    /// @brief 発火状態をリセットする
    void ResetFired();

private:
    std::vector<Event> m_events;   ///< イベント配列
    float m_lastTime = -1.0f;      ///< 前回評価時刻（イベント判定用）
};

/// @brief タイムライン（複数トラックの統合管理）
class Timeline
{
public:
    /// @brief トラックを追加する
    /// @param track 追加するトラック（所有権を移転）
    void AddTrack(std::unique_ptr<TimelineTrack> track);

    /// @brief 再生開始
    void Play();

    /// @brief 一時停止
    void Pause();

    /// @brief 停止（先頭に戻る）
    void Stop();

    /// @brief 指定時刻にシークする
    /// @param time シーク先の時刻（秒）
    void Seek(float time);

    /// @brief 毎フレーム更新
    /// @param deltaTime 前フレームからの経過秒数
    void Update(float deltaTime);

    /// @brief ループ再生を設定する
    /// @param loop true ならループ再生
    void SetLoop(bool loop) { m_loop = loop; }

    /// @brief 再生速度を設定する
    /// @param speed 再生倍率（1.0 = 通常速度、2.0 = 2倍速）
    void SetSpeed(float speed) { m_speed = speed; }

    /// @brief 再生中か判定する
    /// @return 再生中なら true
    bool IsPlaying() const { return m_playing; }

    /// @brief 現在の再生位置を取得する
    /// @return 現在の再生時刻（秒）
    float GetCurrentTime() const { return m_currentTime; }

    /// @brief タイムライン全体の長さを取得する
    /// @return 全トラック中の最大長（秒）
    float GetDuration() const;

    /// @brief トラック数を取得する
    /// @return 登録済みトラック数
    size_t GetTrackCount() const { return m_tracks.size(); }

private:
    std::vector<std::unique_ptr<TimelineTrack>> m_tracks;  ///< トラック配列
    float m_currentTime = 0.0f;  ///< 現在の再生時刻（秒）
    float m_speed       = 1.0f;  ///< 再生倍率
    bool  m_playing     = false; ///< 再生中フラグ
    bool  m_loop        = false; ///< ループ再生フラグ
};

} // namespace gx
/// @}
