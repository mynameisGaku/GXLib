#pragma once
/// @file InstanceBuffer.h
/// @brief インスタンシング描画用データバッファ
///
/// 同一メッシュを異なるワールド変換で複数描画するためのバッファ管理。
/// DynamicBuffer上にインスタンスデータを書き込み、SRVとしてシェーダーに渡す。

#include "pch.h"
#include "Graphics/3D/Transform3D.h"
#include "Graphics/Resource/DynamicBuffer.h"

namespace GX
{

/// @brief 1インスタンスのGPUデータ (128B, 16Bアライン)
struct InstanceData
{
    XMFLOAT4X4 world;               ///< ワールド変換行列（転置済み）
    XMFLOAT4X4 worldInvTranspose;   ///< 法線変換用逆転置行列（転置済み）
};
static_assert(sizeof(InstanceData) == 128, "InstanceData must be 128 bytes");

/// @brief インスタンスバッファ管理クラス
///
/// CPU側でインスタンスデータを蓄積し、DynamicBufferを通じてGPUにアップロードする。
/// SV_InstanceID でシェーダーから StructuredBuffer<InstanceData> としてアクセスする。
class InstanceBuffer
{
public:
    /// @brief 初期化
    /// @param device D3D12デバイス
    /// @param maxInstances 最大インスタンス数
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t maxInstances = k_DefaultMaxInstances);

    /// @brief フレーム開始時にバッファをリセット
    void Reset();

    /// @brief インスタンスを追加する
    /// @param transform ワールド変換
    void AddInstance(const Transform3D& transform);

    /// @brief ワールド行列を直接指定してインスタンスを追加する
    /// @param worldMatrix ワールド行列
    void AddInstance(const XMMATRIX& worldMatrix);

    /// @brief 蓄積したインスタンスデータをGPUにアップロードする
    /// @param frameIndex フレームインデックス
    void Upload(uint32_t frameIndex);

    /// @brief インスタンス数を取得する
    /// @return 現在のインスタンス数
    uint32_t GetInstanceCount() const { return static_cast<uint32_t>(m_instances.size()); }

    /// @brief GPU仮想アドレスを取得する（SRVバインド用）
    /// @param frameIndex フレームインデックス
    /// @return GPU仮想アドレス
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t frameIndex) const;

    /// @brief 使用中のバイト数を取得する
    /// @return バイト数
    uint32_t GetUsedSize() const { return static_cast<uint32_t>(m_instances.size() * sizeof(InstanceData)); }

    /// @brief 内部リソースを取得する（SRV作成用）
    /// @param frameIndex フレームインデックス
    /// @return ID3D12Resourceポインタ
    ID3D12Resource* GetResource(uint32_t frameIndex) const { return m_buffer.GetResource(frameIndex); }

    static constexpr uint32_t k_DefaultMaxInstances = 1024;

private:
    DynamicBuffer m_buffer;
    std::vector<InstanceData> m_instances;
    uint32_t m_maxInstances = k_DefaultMaxInstances;
};

} // namespace GX
