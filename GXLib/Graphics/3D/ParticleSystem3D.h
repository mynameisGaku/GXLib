#pragma once
/// @file ParticleSystem3D.h
/// @brief 3Dパーティクルシステム（ビルボード描画）
///
/// 複数のParticleEmitterを管理し、全粒子をカメラ向きのビルボードクワッドで描画する。
/// SV_VertexIDベースのクワッド生成を使い、頂点バッファにはパーティクルデータのみを格納する。
/// DynamicBufferで毎フレームGPUにアップロードし、専用PSO（Alpha/Additive）で描画する。

#include "pch.h"
#include "Graphics/3D/ParticleEmitter.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

class TextureManager;

/// @brief GPU送信用パーティクル頂点（32B）
struct ParticleVertex
{
    XMFLOAT3 position;  ///< ワールド座標
    float    size;       ///< ワールド単位のサイズ
    XMFLOAT4 color;     ///< RGBA色
    float    rotation;   ///< Z軸回転（ラジアン）
    float    _pad[3];    ///< 48Bアライン
};
static_assert(sizeof(ParticleVertex) == 48, "ParticleVertex must be 48 bytes");

/// @brief パーティクル定数バッファ（b0）
struct ParticleCB
{
    XMFLOAT4X4 viewProj;      ///< ビュー射影行列（転置済み）
    XMFLOAT3   cameraRight;   ///< カメラ右方向（ビルボード展開用）
    float      _pad0;
    XMFLOAT3   cameraUp;      ///< カメラ上方向
    float      _pad1;
};
static_assert(sizeof(ParticleCB) == 96, "ParticleCB must be 96 bytes");

/// @brief 3Dパーティクルシステム
///
/// 複数エミッターを管理し、カメラに向かうビルボードとしてパーティクルを描画する。
/// パーティクルシェーダーはSV_VertexIDでクワッドを生成するため、頂点バッファ不要。
/// StructuredBuffer<ParticleVertex>としてパーティクルデータをSRVで参照する。
class ParticleSystem3D
{
public:
    ParticleSystem3D() = default;
    ~ParticleSystem3D() = default;

    /// @brief パーティクルシステムを初期化する
    /// @param device D3D12デバイス
    /// @param textureManager テクスチャマネージャー（SRVヒープ共有）
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, TextureManager& textureManager);

    /// @brief エミッターを追加する
    /// @param config エミッター設定
    /// @return エミッターインデックス（GetEmitter用）
    int AddEmitter(const ParticleEmitterConfig& config);

    /// @brief エミッターを取得する
    /// @param index AddEmitterの戻り値
    /// @return エミッターへの参照
    ParticleEmitter& GetEmitter(int index) { return m_emitters[index]; }

    /// @brief 全エミッターの粒子を更新する
    /// @param deltaTime フレーム経過時間（秒）
    void Update(float deltaTime);

    /// @brief 全パーティクルを描画する
    /// @param cmdList コマンドリスト
    /// @param camera 描画カメラ（ビルボード計算用）
    /// @param frameIndex フレームインデックス
    void Draw(ID3D12GraphicsCommandList* cmdList, const Camera3D& camera, uint32_t frameIndex);

    /// @brief エミッター数を取得する
    /// @return エミッター数
    int GetEmitterCount() const { return static_cast<int>(m_emitters.size()); }

    /// @brief 全パーティクルの合計数を取得する
    /// @return 粒子数
    uint32_t GetTotalParticleCount() const;

    /// @brief リソースを解放する
    void Shutdown();

private:
    bool CreatePipelineStates(ID3D12Device* device);
    bool CreateRootSignature(ID3D12Device* device);

    ID3D12Device* m_device = nullptr;
    TextureManager* m_textureManager = nullptr;

    std::vector<ParticleEmitter> m_emitters;

    // GPUリソース
    DynamicBuffer m_particleBuffer;   ///< パーティクルデータのアップロードバッファ
    DynamicBuffer m_constantBuffer;   ///< 定数バッファ（ParticleCB）
    Shader        m_shaderCompiler;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_psoAlpha;     ///< アルファブレンドPSO
    ComPtr<ID3D12PipelineState> m_psoAdditive;  ///< 加算ブレンドPSO

    // SRVスロット（テクスチャマネージャーヒープ内）
    uint32_t m_particleSRVSlot[2] = { 0, 0 };
    bool     m_srvInitialized = false;

    static constexpr uint32_t k_MaxTotalParticles = 10000;
};

} // namespace GX
