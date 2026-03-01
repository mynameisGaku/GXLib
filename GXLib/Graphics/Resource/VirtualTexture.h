#pragma once
/// @file VirtualTexture.h
/// @brief 仮想テクスチャリング（Virtual Texturing）
///
/// 巨大テクスチャを小さなページ（タイル）に分割し、必要なページだけ
/// GPU の物理タイルプールに読み込む。メガテクスチャなど VRAM に
/// 乗り切らないサイズのテクスチャをストリーミングで描画するために使う。
/// ページの LRU 管理・最大フレーム毎ロード数の制御も行う。
/// @addtogroup grp_gfx_resource/// @{

#include "pch_graphics.h"
#include <unordered_map>
#include <deque>

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
class VirtualTexture
{
public:
    VirtualTexture() = default;
    ~VirtualTexture() = default;

    /// @brief 初期化
    bool Initialize(ID3D12Device* device, const VirtualTextureConfig& config = {});

    /// @brief フィードバックバッファを解析して必要タイルを特定
    void AnalyzeFeedback(const std::vector<VTPageId>& requestedPages, uint64_t frameIndex);

    /// @brief タイルキャッシュを更新（最大maxTilesPerFrameタイル/フレーム）
    uint32_t UpdateTileCache();

    /// @brief ページテーブルをバインド（シェーダリソースとして）
    void BindPageTable(ID3D12GraphicsCommandList* cmdList);

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

private:
    uint32_t AllocatePhysicalTile();
    void EvictOldestTile();

    VirtualTextureConfig m_config;   ///< 仮想テクスチャリング設定
    bool m_initialized = false;      ///< 初期化済みフラグ

    std::unordered_map<VTPageId, CachedTile, VTPageIdHash> m_tileCache;  ///< タイルキャッシュ
    std::deque<VTPageId> m_pendingRequests;                               ///< 保留リクエストキュー
    std::vector<bool> m_physicalTileUsed;  ///< 物理タイル割当ビットマップ
    uint64_t m_currentFrame = 0;           ///< 現在のフレーム番号
};

} // namespace gx
/// @}
