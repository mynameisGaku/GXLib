#pragma once
/// @file Decal.h
/// @brief デカールシステム（Deferred Box Projection）

#include "pch.h"
#include "Graphics/3D/Transform3D.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Math/Color.h"

namespace GX
{

class Camera3D;
class TextureManager;

/// @brief デカールデータ
struct DecalData
{
    Transform3D transform;           ///< ワールド位置・向き・サイズ
    int textureHandle = -1;          ///< TextureManagerハンドル
    Color color = {1.0f, 1.0f, 1.0f, 1.0f};
    float fadeDistance = 0.5f;        ///< エッジフェード距離
    float normalThreshold = 0.7f;    ///< 法線方向しきい値（dotでフェード）
    float lifetime = -1.0f;          ///< 負=永続、正=秒後にフェード削除
    float age = 0.0f;                ///< 経過時間
};

/// @brief デカール描画システム
///
/// Deferred Box Projection方式でシーンにデカールを投影する。
/// 深度バッファからワールド座標を復元し、デカールのローカル空間でUV計算を行う。
class DecalSystem
{
public:
    static constexpr uint32_t k_MaxDecals = 256;

    DecalSystem() = default;
    ~DecalSystem() = default;

    /// @brief 初期化
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief デカールを追加する
    /// @return デカールハンドル
    int AddDecal(const DecalData& decal);

    /// @brief デカールを削除する
    void RemoveDecal(int handle);

    /// @brief 寿命管理の更新
    void Update(float deltaTime);

    /// @brief デカールを描画する
    void Render(ID3D12GraphicsCommandList* cmdList,
                const Camera3D& camera,
                ID3D12Resource* depthSRV,
                TextureManager& texManager,
                uint32_t frameIndex);

    /// @brief デカール数を取得する
    int GetDecalCount() const;

    /// @brief デカールデータを取得する
    DecalData* GetDecal(int handle);

    /// @brief リソース解放
    void Shutdown();

private:
    struct DecalEntry
    {
        DecalData data;
        bool valid = false;
    };

    /// @brief デカール定数バッファ（GPU送信用、256バイトアラインメント）
    struct DecalCB
    {
        XMFLOAT4X4 invViewProj;     ///< 逆ビュープロジェクション行列
        XMFLOAT4X4 decalWorld;       ///< デカールワールド行列
        XMFLOAT4X4 decalInvWorld;    ///< デカール逆ワールド行列
        XMFLOAT4   decalColor;       ///< デカールカラー
        float      fadeDistance;      ///< エッジフェード距離
        float      normalThreshold;   ///< 法線方向しきい値
        XMFLOAT2   screenSize;       ///< 画面サイズ
        float      padding[8];       ///< 256バイトアラインメント用パディング
    };

    std::vector<DecalEntry> m_decals;
    std::vector<int> m_freeList;

    // Unit cube mesh for decal volume
    Buffer m_cubeVB;
    Buffer m_cubeIB;

    // PSO
    ComPtr<ID3D12PipelineState> m_pso;
    ComPtr<ID3D12RootSignature> m_rs;
    DynamicBuffer m_cb;
    DescriptorHeap m_srvHeap;

    uint32_t m_width = 0;
    uint32_t m_height = 0;

    bool m_initialized = false;

    bool CreateCubeMesh(ID3D12Device* device);
    bool CreatePSO(ID3D12Device* device);
};

} // namespace GX
