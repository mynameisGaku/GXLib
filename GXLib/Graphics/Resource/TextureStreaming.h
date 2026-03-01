#pragma once
/// @file TextureStreaming.h
/// @brief テクスチャストリーミングマネージャー（距離ベースmip選択 + 予算管理）
///
/// VRAM 予算の範囲内で、カメラからの距離に応じてテクスチャの
/// ミップレベルを動的に切り替える。遠くのオブジェクトは低解像度 mip、
/// 近くのオブジェクトは高解像度 mip を使うことで VRAM を節約する。
/// 予算超過時は最終アクセスが古いテクスチャ（LRU）から先に低画質化する。
/// @addtogroup grp_gfx_resource/// @{

#include "pch_graphics.h"
#include "Math/Vector3.h"
#include <string>
#include <vector>

struct ID3D12Device;
struct ID3D12GraphicsCommandList;

namespace gx {

/// @brief ミップリクエスト優先度
enum class MipPriority
{
    Low,       ///< 低優先度
    Normal,    ///< 通常
    High,      ///< 高優先度
    Immediate  ///< 即時ロード
};

/// @brief ストリーミング統計情報
struct StreamingStats {
    uint64_t totalBudgetBytes = 0;    ///< VRAM予算（バイト）
    uint64_t usedBytes = 0;           ///< 使用中バイト数
    uint32_t registeredTextures = 0;  ///< 登録テクスチャ数
    uint32_t loadedMips = 0;          ///< ロード済みミップ数
    uint32_t pendingRequests = 0;     ///< 保留リクエスト数
    uint32_t evictedMips = 0;         ///< エビクト済みミップ数
};

/// @brief テクスチャストリーミングエントリ
struct TextureStreamingEntry {
    int handle = -1;                   ///< テクスチャハンドル
    std::wstring filePath;             ///< テクスチャファイルパス
    uint32_t width = 0;                ///< テクスチャ幅（ピクセル）
    uint32_t height = 0;               ///< テクスチャ高さ（ピクセル）
    uint32_t mipCount = 0;             ///< 総ミップレベル数
    uint32_t currentMipLevel = 0;      ///< 現在のミップレベル（0=最高解像度）
    uint32_t requestedMipLevel = 0;    ///< リクエスト中のミップレベル
    MipPriority priority = MipPriority::Normal;  ///< リクエスト優先度
    uint64_t estimatedBytes = 0;       ///< 推定VRAM使用量（バイト）
    uint64_t lastAccessFrame = 0;      ///< 最終アクセスフレーム番号
};

class TextureStreamingManager {
public:
    TextureStreamingManager();
    ~TextureStreamingManager();

    /// @brief 初期化
    /// @param device D3D12デバイス
    /// @param budgetBytes VRAM予算（バイト、デフォルト512MB）
    /// @return 成功した場合true
    bool Initialize(ID3D12Device* device, uint64_t budgetBytes = 512ULL * 1024 * 1024);
    /// @brief シャットダウン
    void Shutdown();

    /// @brief テクスチャを登録する
    /// @param handle テクスチャハンドル
    /// @param path テクスチャファイルパス
    /// @param width テクスチャ幅
    /// @param height テクスチャ高さ
    /// @param mipCount ミップレベル数
    void RegisterTexture(int handle, const std::wstring& path, uint32_t width, uint32_t height, uint32_t mipCount);
    /// @brief テクスチャの登録を解除する
    /// @param handle テクスチャハンドル
    void UnregisterTexture(int handle);
    /// @brief ミップレベルをリクエストする
    /// @param handle テクスチャハンドル
    /// @param mipLevel 要求ミップレベル（0=最高解像度）
    /// @param priority 優先度
    void RequestMipLevel(int handle, uint32_t mipLevel, MipPriority priority = MipPriority::Normal);

    /// @brief カメラ位置に基づいてミップレベルを自動選択する
    /// @param cameraPos カメラのワールド座標
    /// @param fovDegrees 視野角（度）
    void UpdateFromCamera(const Vector3& cameraPos, float fovDegrees);
    /// @brief ストリーミング処理を実行する
    /// @param cmdList コマンドリスト
    void Update(ID3D12GraphicsCommandList* cmdList);

    /// @brief 統計情報を取得する
    /// @return StreamingStats構造体
    StreamingStats GetStats() const;
    /// @brief 登録テクスチャ数を取得する
    /// @return テクスチャ数
    uint32_t GetTextureCount() const { return static_cast<uint32_t>(m_textures.size()); }

private:
    uint64_t EstimateMipSize(uint32_t width, uint32_t height, uint32_t mipLevel) const;
    void EvictLRU();

    std::vector<TextureStreamingEntry> m_textures;  ///< 登録テクスチャリスト
    uint64_t m_budgetBytes = 512ULL * 1024 * 1024;   ///< VRAM予算（バイト）
    uint64_t m_usedBytes = 0;                         ///< 使用中バイト数
    uint64_t m_currentFrame = 0;                      ///< 現在のフレーム番号
    uint32_t m_pendingRequests = 0;                   ///< 保留リクエスト数
    uint32_t m_evictedCount = 0;                      ///< エビクト済みミップ数
    ID3D12Device* m_device = nullptr;                 ///< D3D12デバイス
    bool m_initialized = false;                       ///< 初期化済みフラグ

    ComPtr<ID3D12Resource> m_stagingBuffer;  ///< mipストリーミング用ステージングバッファ (UPLOAD)
};

} // namespace gx
/// @}
