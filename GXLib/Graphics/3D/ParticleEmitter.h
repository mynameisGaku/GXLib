#pragma once
/// @file ParticleEmitter.h
/// @brief パーティクルエミッター（粒子発生器）
///
/// CPU駆動のパーティクルシステム。
/// エミッターは粒子の発生パラメータ（レート、寿命、サイズ、色など）を持ち、
/// Update()で粒子の生成・物理更新・寿命管理を行う。
/// 描画はParticleSystem3Dが担当し、ビルボードクワッドとしてレンダリングする。

#include "pch.h"

namespace GX
{

/// @brief パーティクル発生形状
enum class ParticleShape
{
    Point,    ///< 1点から発生
    Sphere,   ///< 球面上からランダムに発生
    Cone,     ///< 円錐内にランダムに発生
    Box,      ///< 直方体内にランダムに発生
};

/// @brief パーティクルブレンドタイプ
enum class ParticleBlend
{
    Alpha,    ///< アルファブレンド（半透明）
    Additive, ///< 加算ブレンド（発光）
};

/// @brief パーティクルエミッター設定
struct ParticleEmitterConfig
{
    float    emissionRate   = 10.0f;     ///< 粒子/秒
    uint32_t maxParticles   = 1000;      ///< 最大粒子数

    float lifeMin  = 1.0f;              ///< 最小寿命（秒）
    float lifeMax  = 2.0f;              ///< 最大寿命（秒）
    float sizeMin  = 0.1f;              ///< 初期最小サイズ（ワールド単位）
    float sizeMax  = 0.3f;              ///< 初期最大サイズ
    float speedMin = 1.0f;              ///< 初期最小速度
    float speedMax = 3.0f;              ///< 初期最大速度

    XMFLOAT4 colorStart = { 1, 1, 1, 1 };  ///< 生成時の色（RGBA）
    XMFLOAT4 colorEnd   = { 1, 1, 1, 0 };  ///< 寿命終了時の色（フェードアウト）

    float sizeOverLife = 0.0f;          ///< 寿命に応じたサイズ変化率（正で拡大）

    ParticleShape shape       = ParticleShape::Point;
    float         shapeRadius = 1.0f;   ///< Sphere/Coneの半径
    float         coneAngle   = 30.0f;  ///< Coneの角度（度）
    XMFLOAT3      boxHalfExtents = { 1, 1, 1 }; ///< Boxの半サイズ

    XMFLOAT3 gravity = { 0, -9.8f, 0 }; ///< 重力加速度
    float    drag    = 0.0f;             ///< 空気抵抗係数（0=なし）

    int textureHandle = -1;             ///< テクスチャハンドル（-1でデフォルト白）
    ParticleBlend blend = ParticleBlend::Alpha;
};

/// @brief 1つの粒子データ
struct Particle
{
    XMFLOAT3 position;   ///< ワールド座標
    XMFLOAT3 velocity;   ///< 速度ベクトル
    XMFLOAT4 color;      ///< 現在の色（RGBA）
    float    size;        ///< 現在のサイズ
    float    rotation;    ///< Z軸回転（ラジアン）
    float    life;        ///< 残り寿命（秒）
    float    maxLife;     ///< 初期寿命（補間の基準値）
};

/// @brief パーティクルエミッター
///
/// 粒子の生成・更新・寿命管理を行うCPU側のクラス。
/// 毎フレームUpdate()を呼ぶことで粒子が物理シミュレーションされる。
class ParticleEmitter
{
public:
    ParticleEmitter() = default;

    /// @brief エミッターを初期化する
    /// @param config 発生パラメータ
    void Initialize(const ParticleEmitterConfig& config);

    /// @brief エミッターの位置を設定する
    /// @param pos ワールド座標
    void SetPosition(const XMFLOAT3& pos) { m_position = pos; }

    /// @brief エミッターの発射方向を設定する
    /// @param dir 方向ベクトル（正規化済み）
    void SetDirection(const XMFLOAT3& dir) { m_direction = dir; }

    /// @brief 粒子の更新処理（生成 + 物理 + 寿命管理）
    /// @param deltaTime フレーム経過時間（秒）
    void Update(float deltaTime);

    /// @brief エミッターの有効/無効を設定する
    /// @param active trueで粒子を生成する
    void SetActive(bool active) { m_active = active; }

    /// @brief エミッターが有効かどうかを取得する
    /// @return 有効ならtrue
    bool IsActive() const { return m_active; }

    /// @brief 一度に指定数の粒子を発生させる（爆発エフェクト等）
    /// @param count 発生させる粒子数
    void Burst(uint32_t count);

    /// @brief 現在の粒子配列を取得する
    /// @return 粒子配列へのconst参照
    const std::vector<Particle>& GetParticles() const { return m_particles; }

    /// @brief 生存中の粒子数を取得する
    /// @return 粒子数
    uint32_t GetParticleCount() const { return static_cast<uint32_t>(m_particles.size()); }

    /// @brief エミッター設定を取得する
    /// @return 設定への参照
    const ParticleEmitterConfig& GetConfig() const { return m_config; }

    /// @brief エミッター設定を変更可能な参照で取得する
    /// @return 設定への参照
    ParticleEmitterConfig& GetConfigMutable() { return m_config; }

private:
    /// @brief 1つの粒子を生成する
    void SpawnParticle();

    ParticleEmitterConfig m_config;
    std::vector<Particle> m_particles;
    XMFLOAT3 m_position  = { 0, 0, 0 };
    XMFLOAT3 m_direction = { 0, 1, 0 };
    float    m_emissionAccum = 0.0f;   ///< 発生タイミングの蓄積値
    bool     m_active = true;

    // 乱数生成器
    std::mt19937 m_rng{ std::random_device{}() };
};

} // namespace GX
