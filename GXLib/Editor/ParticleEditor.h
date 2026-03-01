#pragma once
/// @file ParticleEditor.h
/// @brief パーティクルエディタ — エミッター設定とカーブを編集してエフェクトを作る
///
/// パーティクルの生成数・速度・寿命・色などをカーブ（時間→値）で制御できる。
/// 設定を JSON にエクスポートしてゲーム実行中に読み込むことも想定している。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

namespace gx
{

/// @brief パーティクルカーブ（時間→値マッピング）
struct ParticleCurve
{
    struct ControlPoint
    {
        float time  = 0.0f;  ///< 0-1正規化時間
        float value = 0.0f;
    };

    std::vector<ControlPoint> points;

    /// @brief 時間tでの値を評価（二分探索+線形補間）
    float Evaluate(float t) const
    {
        if (points.empty()) return 0.0f;
        if (points.size() == 1) return points[0].value;
        if (t <= points.front().time) return points.front().value;
        if (t >= points.back().time) return points.back().value;

        // Binary search for segment
        size_t lo = 0, hi = points.size() - 1;
        while (lo + 1 < hi)
        {
            size_t mid = (lo + hi) / 2;
            if (points[mid].time <= t) lo = mid; else hi = mid;
        }

        float segT = (t - points[lo].time) / (points[hi].time - points[lo].time);
        return points[lo].value + (points[hi].value - points[lo].value) * segT;
    }

    void AddPoint(float time, float value)
    {
        ControlPoint cp = { time, value };
        auto it = std::lower_bound(points.begin(), points.end(), cp,
            [](const ControlPoint& a, const ControlPoint& b) { return a.time < b.time; });
        points.insert(it, cp);
    }

    void Clear() { points.clear(); }
    size_t GetPointCount() const { return points.size(); }
};

/// @brief カラーカーブ（RGBA）
struct ColorCurve
{
    ParticleCurve r, g, b, a;

    void Evaluate(float t, float& outR, float& outG, float& outB, float& outA) const
    {
        outR = r.Evaluate(t);
        outG = g.Evaluate(t);
        outB = b.Evaluate(t);
        outA = a.Evaluate(t);
    }
};

/// @brief エディタ用エミッタ形状
enum class EditorEmitterShape
{
    Sphere, ///< 球体
    Box,    ///< 直方体
    Cone,   ///< 円錐
    Ring    ///< リング
};

/// @brief エミッタ形状可視化パラメータ
struct EmitterShapeViz
{
    EditorEmitterShape shape = EditorEmitterShape::Sphere; ///< エミッタ形状
    float radius = 1.0f;        ///< 球体/円錐の半径
    float width  = 1.0f;        ///< 直方体の幅
    float height = 1.0f;        ///< 直方体の高さ
    float depth  = 1.0f;        ///< 直方体の奥行き
    float angle  = 30.0f;       ///< 円錐の広がり角度（度）
    float ringRadius = 2.0f;    ///< リングの半径
    bool showWireframe = true;  ///< ワイヤーフレーム表示フラグ
};

/// @brief パーティクルエミッタ設定（エディタ用）
struct ParticleEmitterSettings
{
    std::string name = "Default";    ///< エミッタ名
    EmitterShapeViz shapeViz;        ///< エミッタ形状の可視化設定

    float emissionRate = 10.0f;      ///< 放出レート（パーティクル/秒）
    uint32_t maxParticles = 1000;    ///< 最大パーティクル数
    float lifetime = 2.0f;           ///< パーティクルの寿命（秒）
    float startSpeed = 5.0f;         ///< 初速（ワールド単位/秒）
    float startSize = 0.1f;          ///< 初期サイズ（ワールド単位）
    float gravity = -9.81f;          ///< 重力加速度（m/s²）

    ParticleCurve sizeCurve;         ///< サイズ変化カーブ
    ParticleCurve speedCurve;        ///< 速度変化カーブ
    ParticleCurve alphaCurve;        ///< 透明度変化カーブ
    ColorCurve colorCurve;           ///< 色変化カーブ（RGBA）

    bool looping = true;             ///< ループ再生するか
    bool worldSpace = true;          ///< ワールド空間で放出するか（false=ローカル空間）
};

/// @brief パーティクルエディタ
class ParticleEditor
{
public:
    ParticleEditor() = default;
    ~ParticleEditor() = default;

    /// @brief エミッタ設定を設定
    void SetEmitterSettings(const ParticleEmitterSettings& settings);

    /// @brief エミッタ設定を取得
    const ParticleEmitterSettings& GetEmitterSettings() const { return m_settings; }

    // --- 基本パラメータ ---
    void SetEmissionRate(float rate);
    float GetEmissionRate() const { return m_settings.emissionRate; }

    void SetMaxParticles(uint32_t count);
    uint32_t GetMaxParticles() const { return m_settings.maxParticles; }

    void SetLifetime(float seconds);
    float GetLifetime() const { return m_settings.lifetime; }

    void SetStartSpeed(float speed);
    float GetStartSpeed() const { return m_settings.startSpeed; }

    void SetStartSize(float size);
    float GetStartSize() const { return m_settings.startSize; }

    void SetGravity(float gravity);
    float GetGravity() const { return m_settings.gravity; }

    // --- エミッタ形状 ---
    void SetEmitterShape(EditorEmitterShape shape);
    EditorEmitterShape GetEmitterShape() const { return m_settings.shapeViz.shape; }

    void SetShapeRadius(float radius);
    float GetShapeRadius() const { return m_settings.shapeViz.radius; }

    // --- カーブ ---
    ParticleCurve& GetSizeCurve() { return m_settings.sizeCurve; }
    ParticleCurve& GetSpeedCurve() { return m_settings.speedCurve; }
    ParticleCurve& GetAlphaCurve() { return m_settings.alphaCurve; }
    ColorCurve& GetColorCurve() { return m_settings.colorCurve; }

    // --- ループ/空間 ---
    void SetLooping(bool looping) { m_settings.looping = looping; m_dirty = true; }
    bool IsLooping() const { return m_settings.looping; }

    void SetWorldSpace(bool worldSpace) { m_settings.worldSpace = worldSpace; m_dirty = true; }
    bool IsWorldSpace() const { return m_settings.worldSpace; }

    // --- プリセット ---
    enum class Preset
    {
        Fire,   ///< 炎エフェクト
        Smoke,  ///< 煙エフェクト
        Sparks, ///< 火花エフェクト
        Snow    ///< 雪エフェクト
    };
    void LoadPreset(Preset preset);
    static std::string GetPresetName(Preset preset);

    // --- 再生制御 ---
    void Play();
    void Pause();
    void Stop();
    void Restart();
    bool IsPlaying() const { return m_playing; }
    bool IsPaused() const { return m_paused; }

    float GetPlaybackTime() const { return m_playbackTime; }
    void SetPlaybackTime(float time) { m_playbackTime = std::max(0.0f, time); }

    /// @brief ダーティフラグ
    bool IsDirty() const { return m_dirty; }
    void ClearDirty() { m_dirty = false; }

private:
    ParticleEmitterSettings m_settings; ///< エミッタ設定
    bool m_playing = false;             ///< 再生中フラグ
    bool m_paused = false;              ///< 一時停止中フラグ
    float m_playbackTime = 0.0f;        ///< 現在の再生時間（秒）
    bool m_dirty = false;               ///< 未保存の変更があるか
};

} // namespace gx
/// @}
