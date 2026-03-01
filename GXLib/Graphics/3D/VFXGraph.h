#pragma once
/// @file VFXGraph.h
/// @brief ビジュアルエフェクトグラフ（データ駆動パーティクルエフェクト定義）
///
/// GPUParticleSystemのパラメータをデータ駆動で定義・組み合わせる。
/// エミッター + モジュール構成でエフェクトを宣言的に記述し、
/// JSON/バイナリでシリアライズ可能。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"

namespace gx
{

class GPUParticleSystem;

/// @brief VFXモジュールの種類
enum class VFXModuleType
{
    InitialVelocity,    ///< 初期速度
    InitialSize,        ///< 初期サイズ
    InitialColor,       ///< 初期色
    InitialRotation,    ///< 初期回転
    InitialLifetime,    ///< 初期寿命
    VelocityOverLife,   ///< 寿命に応じた速度変化
    SizeOverLife,       ///< 寿命に応じたサイズ変化
    ColorOverLife,      ///< 寿命に応じた色変化
    RotationOverLife,   ///< 寿命に応じた回転変化
    Gravity,            ///< 重力
    Noise,              ///< ノイズ擾乱
    Drag,               ///< 空気抵抗
    Orbit,              ///< 軌道運動
    Attraction,         ///< 引力ポイント
    TextureSheet,       ///< テクスチャシートアニメーション
    SubEmitter,         ///< サブエミッター
};

/// @brief 値の範囲（ランダム化用）
struct VFXRange
{
    float min = 0.0f;
    float max = 1.0f;

    float Evaluate(float t) const { return min + t * (max - min); }
    float GetRandom() const;
};

/// @brief 色の範囲
struct VFXColorRange
{
    float r0 = 1, g0 = 1, b0 = 1, a0 = 1;
    float r1 = 1, g1 = 1, b1 = 1, a1 = 1;
};

/// @brief カーブキーフレーム
struct VFXCurveKey
{
    float time = 0.0f;   ///< 正規化時間(0-1)
    float value = 0.0f;
};

/// @brief アニメーションカーブ
struct VFXCurve
{
    std::vector<VFXCurveKey> keys;

    /// @brief 指定時刻の値を線形補間で取得
    float Evaluate(float t) const;

    /// @brief 定数カーブを作成
    static VFXCurve Constant(float value);

    /// @brief 線形カーブを作成(start->end)
    static VFXCurve Linear(float start, float end);
};

/// @brief VFXモジュール基底
struct VFXModule
{
    VFXModuleType type;
    bool enabled = true;

    // モジュール別パラメータ（unionの代わりにstruct集約）
    // InitialVelocity
    VFXRange velocityX, velocityY, velocityZ;
    bool velocityRadial = false;
    VFXRange radialSpeed;

    // InitialSize
    VFXRange sizeMin = {0.1f, 0.1f}, sizeMax = {1.0f, 1.0f};

    // InitialColor
    VFXColorRange colorRange;

    // InitialRotation
    VFXRange rotationRange = {0.0f, 6.28f};

    // InitialLifetime
    VFXRange lifetimeRange = {1.0f, 3.0f};

    // OverLife curves
    VFXCurve sizeCurve;
    VFXCurve alphaCurve;
    VFXCurve velocityCurve;
    VFXCurve rotationSpeedCurve;
    VFXColorRange colorStart, colorEnd;

    // Gravity
    float gravityStrength = -9.81f;

    // Noise
    float noiseFrequency = 1.0f;
    float noiseAmplitude = 0.5f;

    // Drag
    float dragCoefficient = 0.1f;

    // Orbit
    float orbitSpeed = 1.0f;
    float orbitRadius = 1.0f;

    // Attraction
    XMFLOAT3 attractPoint = {0,0,0};
    float attractStrength = 1.0f;

    // TextureSheet
    int sheetRows = 1, sheetCols = 1;
    float sheetFPS = 10.0f;

    // SubEmitter index
    int subEmitterIndex = -1;
};

/// @brief エミッター形状
enum class VFXEmitterShape
{
    Point,      ///< 点
    Sphere,     ///< 球体
    Hemisphere, ///< 半球
    Cone,       ///< 円錐
    Box,        ///< ボックス
    Circle,     ///< 円（2D）
    Edge,       ///< 辺（線分）
};

/// @brief VFXエミッター定義
struct VFXEmitter
{
    std::string name;   ///< エミッター名

    // 放出設定
    float emitRate = 10.0f;          ///< 毎秒放出数
    int   maxParticles = 1000;       ///< 最大パーティクル数
    float duration = 5.0f;           ///< エミッター持続時間（秒）
    bool  looping = true;            ///< ループ再生
    float startDelay = 0.0f;         ///< 開始遅延

    // 形状
    VFXEmitterShape shape = VFXEmitterShape::Point;
    float shapeRadius = 1.0f;
    XMFLOAT3 shapeSize = {1, 1, 1};
    float shapeConeAngle = 0.5f;     ///< コーン半角(ラジアン)
    bool emitFromEdge = false;       ///< エッジからのみ放出

    // ワールド/ローカル座標
    bool worldSpace = true;

    // モジュール
    std::vector<VFXModule> modules;

    // テクスチャ
    int textureHandle = -1;

    // ブレンドモード
    int blendMode = 0;  // 0=Alpha, 1=Additive
};

/// @brief VFXエフェクト定義（複数エミッターの集合）
class VFXEffect
{
public:
    VFXEffect() = default;
    ~VFXEffect() = default;

    /// @brief エミッターを追加
    uint32_t AddEmitter(const VFXEmitter& emitter);

    /// @brief エミッターを取得
    VFXEmitter& GetEmitter(uint32_t index);
    const VFXEmitter& GetEmitter(uint32_t index) const;
    uint32_t GetEmitterCount() const { return static_cast<uint32_t>(m_emitters.size()); }

    /// @brief モジュールをエミッターに追加
    void AddModule(uint32_t emitterIndex, const VFXModule& module);

    /// @brief 名前
    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }

    /// @brief JSONからロード
    bool LoadFromJSON(const std::string& jsonPath);

    /// @brief JSONに保存
    bool SaveToJSON(const std::string& jsonPath) const;

    /// @brief バイナリからロード
    bool LoadFromBinary(const std::string& path);

    /// @brief バイナリに保存
    bool SaveToBinary(const std::string& path) const;

    /// @brief エフェクトをGPUParticleSystemに適用
    void ApplyToParticleSystem(GPUParticleSystem& ps, uint32_t emitterIndex = 0) const;

private:
    std::string m_name;                    ///< エフェクト名
    std::vector<VFXEmitter> m_emitters;    ///< エミッター配列
};

/// @brief VFXエフェクトインスタンス（再生状態管理）
class VFXInstance
{
public:
    VFXInstance() = default;

    void SetEffect(const VFXEffect* effect) { m_effect = effect; }
    const VFXEffect* GetEffect() const { return m_effect; }

    void Play();
    void Stop();
    void Pause();
    void Resume();

    void Update(float deltaTime);

    bool IsPlaying() const { return m_playing; }
    bool IsFinished() const { return m_finished; }
    float GetElapsedTime() const { return m_elapsed; }

    void SetPosition(const XMFLOAT3& pos) { m_position = pos; }
    XMFLOAT3 GetPosition() const { return m_position; }
    void SetScale(float s) { m_scale = s; }
    float GetScale() const { return m_scale; }

private:
    const VFXEffect* m_effect = nullptr;   ///< 再生中のエフェクト定義
    bool m_playing = false;                ///< 再生中フラグ
    bool m_paused = false;                 ///< 一時停止フラグ
    bool m_finished = false;               ///< 再生完了フラグ
    float m_elapsed = 0.0f;                ///< 経過時間（秒）
    XMFLOAT3 m_position = {0,0,0};        ///< インスタンスのワールド位置
    float m_scale = 1.0f;                  ///< スケール倍率
    float m_emitAccumulator = 0.0f;        ///< 放出蓄積カウンター
};

} // namespace gx
/// @}
