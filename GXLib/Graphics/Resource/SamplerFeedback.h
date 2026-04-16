#pragma once
/// @file SamplerFeedback.h
/// @brief サンプラーフィードバックによるテクスチャストリーミング制御
///
/// サンプラーフィードバック (Sampler Feedback) はGPUがシェーダで実際にアクセスした
/// テクスチャのミップレベルを記録する機能。この情報を使って、テクスチャストリーミングの
/// 最適化を行う:
///   - 画面上で大きく表示されるテクスチャは高解像度ミップを読み込む
///   - 遠くにあるテクスチャは低解像度ミップで十分
///
/// Tier 0.9: MinMip feedback のみ（最小使用ミップレベルの記録）
/// Tier 1.0: MinMip + MipRegionUsed（使用されたミップ領域の詳細情報）
/// @addtogroup grp_gfx_resource/// @{

#include "pch_graphics.h"

namespace gx
{

/// @brief サンプラーフィードバック対応レベル
enum class SamplerFeedbackTier : uint32_t
{
    None = 0,     ///< サンプラーフィードバック非対応
    Tier0_9,      ///< MinMip feedback のみ
    Tier1_0,      ///< MinMip + MipRegionUsed
};

/// @brief テクスチャのミップレベルストリーミングリクエスト
struct MipRequest
{
    uint32_t textureIndex;       ///< 対象テクスチャのインデックス
    uint32_t requestedMipLevel;  ///< 要求されるミップレベル（0=最高解像度）
    bool     needsStreaming;     ///< 現在のミップレベルより高解像度が必要か
};

/// @brief サンプラーフィードバックを使ったテクスチャストリーミング制御
///
/// GPUが実際にアクセスしたミップレベルを分析し、
/// テクスチャストリーミングの優先度を決定する。
class SamplerFeedback
{
public:
    /// @brief 初期化とGPU対応レベル検出
    /// @param device D3D12デバイス
    /// @param maxTrackedTextures 追跡するテクスチャの最大数
    /// @return 初期化成功でtrue（非対応GPUでもtrue）
    bool Initialize(ID3D12Device* device, uint32_t maxTrackedTextures);

    /// @brief サポートされている対応レベルを取得
    SamplerFeedbackTier GetSupportedTier() const { return m_tier; }

    /// @brief サンプラーフィードバックが利用可能か
    /// @return 対応GPUならtrue
    bool IsAvailable() const { return m_tier != SamplerFeedbackTier::None; }

    /// @brief 指定テクスチャに対するフィードバックマップを作成する
    /// @param device D3D12デバイス
    /// @param texIndex テクスチャインデックス（0〜maxTrackedTextures-1）
    /// @param pairedTexture フィードバック対象のテクスチャリソース
    /// @return 成功でtrue
    bool CreateFeedbackFor(ID3D12Device* device, uint32_t texIndex, ID3D12Resource* pairedTexture);

    /// @brief フィードバックマップをリゾルブ（読み取り可能な形式に変換）する
    /// @param cmdList コマンドリスト
    void ResolveFeedback(ID3D12GraphicsCommandList* cmdList);

    /// @brief リゾルブ結果を分析してミップリクエストリストを生成する
    /// @param frameIndex 現在のフレームインデックス
    void AnalyzeFeedback(uint32_t frameIndex);

    /// @brief 生成されたミップリクエストリストを取得する
    /// @return MipRequestの配列への参照
    const gx::Vector<MipRequest>& GetMipRequests() const { return m_mipRequests; }

    /// @brief 現在追跡中のテクスチャ数を取得する
    uint32_t GetTrackedTextureCount() const { return m_trackedCount; }

private:
    SamplerFeedbackTier m_tier = SamplerFeedbackTier::None;  ///< GPU対応レベル
    uint32_t m_maxTracked  = 0;    ///< 最大追跡テクスチャ数
    uint32_t m_trackedCount = 0;   ///< 現在の追跡テクスチャ数

    gx::Vector<ComPtr<ID3D12Resource>> m_feedbackMaps;   ///< フィードバックマップリソース
    gx::Vector<ComPtr<ID3D12Resource>> m_resolveBuffers; ///< リゾルブ先バッファ
    gx::Vector<MipRequest>             m_mipRequests;    ///< 分析結果のリクエストリスト
};

} // namespace gx
/// @}
