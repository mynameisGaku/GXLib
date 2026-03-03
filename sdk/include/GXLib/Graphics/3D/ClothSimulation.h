#pragma once
/// @file ClothSimulation.h
/// @brief GPU布シミュレーション（Verlet積分 + PBD制約ソルバー）
///
/// Position-Based Dynamics (PBD) による布地シミュレーション。
/// GPU (Compute Shader) とCPU-onlyモード（テスト用device=nullptr）をサポート。

#include "pch_graphics.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"
#include "Math/Vector3.h"

#include <vector>
#include <cmath>

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

/// @brief 布パーティクル
struct ClothParticle
{
    Vector3 position    = { 0.0f, 0.0f, 0.0f };  ///< 現在位置
    Vector3 prevPosition = { 0.0f, 0.0f, 0.0f };  ///< 前フレーム位置
    Vector3 acceleration = { 0.0f, 0.0f, 0.0f };  ///< 加速度
    float    inverseMass  = 1.0f;                    ///< 逆質量（0=ピン留め）
};

/// @brief 布制約タイプ
enum class ClothConstraintType : uint32_t
{
    Stretch = 0,  ///< 伸縮制約（隣接水平/垂直）
    Shear   = 1,  ///< せん断制約（対角線）
    Bend    = 2,  ///< 屈曲制約（1つ飛ばし）
};

/// @brief 布制約
struct ClothConstraint
{
    ClothConstraintType type = ClothConstraintType::Stretch;
    uint32_t particleA  = 0;        ///< パーティクルAインデックス
    uint32_t particleB  = 0;        ///< パーティクルBインデックス
    float    restLength = 0.0f;     ///< 静止長
    float    stiffness  = 1.0f;     ///< 剛性係数
};

/// @brief 布コライダー形状
enum class ClothColliderShape : uint32_t
{
    Sphere  = 0,  ///< 球コライダー
    Capsule = 1,  ///< カプセルコライダー
};

/// @brief 布コライダー
struct ClothCollider
{
    ClothColliderShape shape = ClothColliderShape::Sphere;
    Vector3 position       = { 0.0f, 0.0f, 0.0f };  ///< 位置（球の中心/カプセル始点）
    float    radius         = 1.0f;                     ///< 半径
    Vector3 capsuleEnd     = { 0.0f, 0.0f, 0.0f };   ///< カプセル終点
    float    capsuleRadius  = 0.0f;                     ///< カプセル半径
};

/// @brief 布シミュレーション設定
struct ClothSettings
{
    Vector3 gravity          = { 0.0f, -9.8f, 0.0f };  ///< 重力加速度
    float    damping          = 0.99f;                    ///< 減衰係数
    uint32_t solverIterations = 8;                        ///< PBDソルバー反復回数
    float    timeStep         = 1.0f / 60.0f;             ///< シミュレーション時間刻み
    Vector3 windForce        = { 0.0f, 0.0f, 0.0f };   ///< 風力
};

/// @brief GPU布シミュレーション（Verlet積分 + PBDソルバー）
///
/// device=nullptr でCPU-onlyモード動作。テスト時や非GPU環境で使用。
class ClothSimulation
{
public:
    ClothSimulation() = default;
    ~ClothSimulation() { Shutdown(); }

    /// @brief 初期化
    /// @param device グラフィックスデバイス（nullptrでCPU-onlyモード）
    /// @param width  グリッド幅
    /// @param height グリッド高さ
    /// @return 成功時 true
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief 終了処理
    void Shutdown();

    /// @brief 設定を変更
    void SetSettings(const ClothSettings& settings) { m_settings = settings; }

    /// @brief 現在の設定を取得
    const ClothSettings& GetSettings() const { return m_settings; }

    /// @brief グリッド状の布メッシュを生成する
    /// @param width   グリッド幅（パーティクル数）
    /// @param height  グリッド高さ（パーティクル数）
    /// @param spacing パーティクル間隔
    void CreateGrid(uint32_t width, uint32_t height, float spacing);

    /// @brief パーティクル数を取得
    uint32_t GetParticleCount() const { return static_cast<uint32_t>(m_particles.size()); }

    /// @brief 制約数を取得
    uint32_t GetConstraintCount() const { return static_cast<uint32_t>(m_constraints.size()); }

    /// @brief パーティクルを取得（書き込み可）
    ClothParticle& GetParticle(uint32_t index) { return m_particles[index]; }

    /// @brief パーティクルを取得（読み取り専用）
    const ClothParticle& GetParticle(uint32_t index) const { return m_particles[index]; }

    /// @brief パーティクルをピン留め（inverseMass=0）
    void PinParticle(uint32_t index);

    /// @brief パーティクルのピン留めを解除
    /// @param index パーティクルインデックス
    /// @param mass  復元する質量（デフォルト1.0）
    void UnpinParticle(uint32_t index, float mass = 1.0f);

    /// @brief パーティクルがピン留めされているか
    bool IsParticlePinned(uint32_t index) const;

    /// @brief コライダーを追加
    /// @return コライダーID
    int AddCollider(const ClothCollider& collider);

    /// @brief コライダーを除去
    void RemoveCollider(int id);

    /// @brief コライダー数を取得
    uint32_t GetColliderCount() const { return static_cast<uint32_t>(m_colliders.size()); }

    /// @brief シミュレーションを1ステップ更新
    /// @param deltaTime 経過時間（秒）
    void Update(float deltaTime);

    /// @brief グリッド幅を取得
    uint32_t GetWidth() const { return m_width; }

    /// @brief グリッド高さを取得
    uint32_t GetHeight() const { return m_height; }

private:
    // CPU-only シミュレーションメソッド
    void VerletIntegrate(float dt);
    void SolveConstraints();
    void HandleCollisions();
    void ApplyWind();

    // パーティクル・制約・コライダー
    gx::Vector<ClothParticle>  m_particles;
    gx::Vector<ClothConstraint> m_constraints;
    gx::Vector<ClothCollider>  m_colliders;

    // 設定
    ClothSettings m_settings;
    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // GPU リソース
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_particleBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_constraintBuffer;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>  m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>  m_verletPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>  m_constraintPSO;

    // 状態
    bool m_cpuMode     = true;
    bool m_initialized = false;
    int  m_nextColliderId = 0;
};

/// @}
} // namespace gx
