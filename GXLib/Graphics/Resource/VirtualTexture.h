#pragma once
/// @file VirtualTexture.h
/// @brief 仮想テクスチャリング（Virtual Texturing）
///
/// 巨大テクスチャを小さなページ（タイル）に分割し、必要なページだけ
/// GPU の物理タイルプールに読み込む。メガテクスチャなど VRAM に
/// 乗り切らないサイズのテクスチャをストリーミングで描画するために使う。
/// ページの LRU 管理・最大フレーム毎ロード数の制御も行う。
///
/// GPU フィードバックモード:
///   - InitializeGPU() で画面サイズに応じたフィードバックバッファ (R32G32_UINT UAV) を作成
///   - シェーダがフィードバックバッファに (textureId, packed mip|tileX|tileY) を書き込む
///   - ReadbackFeedback() でリードバック→CPU解析→AnalyzeFeedback() に自動投入
///   - BindPageTable() でページテーブルテクスチャ (SRV) をルートシグネチャにバインド
/// @addtogroup grp_gfx_resource/// @{

#include "pch_graphics.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace gx
{

/// @brief 仮想テクスチャのページID
struct VTPageId
{
    uint32_t textureId = 0;  ///< テクスチャID
    uint32_t mipLevel  = 0;  ///< ミップレベル
    uint32_t tileX     = 0;  ///< タイルX座標
    uint32_t tileY     = 0;  ///< タイルY座標

    bool operator==(const VTPageId& o) const
    {
        return textureId == o.textureId && mipLevel == o.mipLevel
            && tileX == o.tileX && tileY == o.tileY;
    }
};

/// @brief VTPageId用ハッシュ
struct VTPageIdHash
{
    size_t operator()(const VTPageId& p) const
    {
        size_t h = std::hash<uint32_t>()(p.textureId);
        h ^= std::hash<uint32_t>()(p.mipLevel) << 8;
        h ^= std::hash<uint32_t>()(p.tileX) << 16;
        h ^= std::hash<uint32_t>()(p.tileY) << 24;
        return h;
    }
};

/// @brief 仮想テクスチャリング設定
struct VirtualTextureConfig
{
    uint32_t pageSize       = 128;   ///< タイルサイズ（ピクセル）
    uint32_t poolWidth      = 2048;  ///< 物理タイルプール幅
    uint32_t poolHeight     = 2048;  ///< 物理タイルプール高
    uint32_t maxCachedTiles = 256;   ///< 最大キャッシュタイル数
    uint32_t maxTilesPerFrame = 4;   ///< フレーム毎の最大ロードタイル数
};

/// @brief キャッシュされたタイル情報
struct CachedTile
{
    VTPageId pageId;
    uint32_t physicalX  = 0;   ///< 物理プール内X
    uint32_t physicalY  = 0;   ///< 物理プール内Y
    uint64_t lastAccessFrame = 0;  ///< 最終アクセスフレーム番号
};

/// @brief 仮想テクスチャリングシステム
///
/// CPU ページ管理に加え、GPU フィードバックバッファ / ページテーブルテクスチャを
/// 管理し、シェーダが必要とするページを自動でストリーミングする。
class VirtualTexture
{
public:
    VirtualTexture() = default;
    ~VirtualTexture() = default;

    /// @brief 初期化（CPU-only モード。GPU フィードバックは InitializeGPU() で追加初期化）
    bool Initialize(ID3D12Device* device, const VirtualTextureConfig& config = {});

    /// @brief GPU フィードバックバッファとページテーブルテクスチャを作成する
    /// @param device D3D12 デバイス
    /// @param screenWidth 画面幅（ピクセル）
    /// @param screenHeight 画面高（ピクセル）
    /// @return 成功なら true
    bool InitializeGPU(ID3D12Device* device, uint32_t screenWidth, uint32_t screenHeight);

    /// @brief フィードバックバッファを解析して必要タイルを特定
    void AnalyzeFeedback(const gx::Vector<VTPageId>& requestedPages, uint64_t frameIndex);

    /// @brief タイルキャッシュを更新（最大maxTilesPerFrameタイル/フレーム）
    uint32_t UpdateTileCache();

    /// @brief GPU フィードバックバッファをリードバックし AnalyzeFeedback に投入する
    /// @param cmdList コマンドリスト
    void ReadbackFeedback(ID3D12GraphicsCommandList* cmdList);

    /// @brief ページテーブルテクスチャをシェーダリソースとしてバインドする
    /// @param cmdList コマンドリスト
    /// @param rootParamIndex ルートパラメータインデックス
    void BindPageTable(ID3D12GraphicsCommandList* cmdList, uint32_t rootParamIndex);

    /// @brief キャッシュ内容に基づきページテーブルテクスチャを更新する
    void UpdatePageTableTexture();

    /// @brief 指定タイルがキャッシュにあるか
    bool IsTileCached(const VTPageId& pageId) const;

    /// @brief キャッシュ済みタイル数
    uint32_t GetCachedTileCount() const { return static_cast<uint32_t>(m_tileCache.size()); }

    /// @brief ペンディングリクエスト数
    uint32_t GetPendingRequestCount() const { return static_cast<uint32_t>(m_pendingRequests.size()); }

    /// @brief 設定を取得
    const VirtualTextureConfig& GetConfig() const { return m_config; }

    /// @brief 物理プールサイズ（タイル数）
    uint32_t GetPoolTileCapacity() const;

    /// @brief タイルキャッシュをクリア
    void ClearCache();

    /// @brief GPU リソースが初期化済みか
    bool IsGPUInitialized() const { return m_gpuInitialized; }

    /// @brief フィードバックバッファの幅を取得
    uint32_t GetFeedbackWidth() const { return m_feedbackWidth; }

    /// @brief フィードバックバッファの高さを取得
    uint32_t GetFeedbackHeight() const { return m_feedbackHeight; }

private:
    uint32_t AllocatePhysicalTile();
    void EvictOldestTile();

    VirtualTextureConfig m_config;   ///< 仮想テクスチャリング設定
    bool m_initialized = false;      ///< 初期化済みフラグ
    bool m_gpuInitialized = false;   ///< GPU リソース初期化済みフラグ

    gx::HashMap<VTPageId, CachedTile, VTPageIdHash> m_tileCache;  ///< タイルキャッシュ
    gx::Deque<VTPageId> m_pendingRequests;                        ///< 保留リクエストキュー
    gx::Vector<bool> m_physicalTileUsed;  ///< 物理タイル割当ビットマップ
    uint64_t m_currentFrame = 0;           ///< 現在のフレーム番号

    // --- GPU feedback resources ---
    ComPtr<ID3D12Resource> m_feedbackBuffer;     ///< GPU フィードバック UAV (R32G32_UINT)
    ComPtr<ID3D12Resource> m_feedbackReadback;   ///< READBACK バッファ (CPU 読み取り用)
    ComPtr<ID3D12Resource> m_pageTableTexture;   ///< ページテーブル SRV (R32G32_UINT)
    ComPtr<ID3D12Resource> m_pageTableUpload;    ///< ページテーブル UPLOAD バッファ
    DescriptorHeap m_feedbackHeap;               ///< UAV + SRV ディスクリプタヒープ
    uint32_t m_feedbackWidth  = 0;               ///< フィードバックバッファ幅 (screen / 16)
    uint32_t m_feedbackHeight = 0;               ///< フィードバックバッファ高 (screen / 16)
    uint32_t m_pageTableWidth  = 0;              ///< ページテーブルテクスチャ幅
    uint32_t m_pageTableHeight = 0;              ///< ページテーブルテクスチャ高
    ID3D12Device* m_device = nullptr;            ///< デバイスポインタ（弱参照）
};

} // namespace gx
/// @}
