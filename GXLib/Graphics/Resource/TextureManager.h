#pragma once
/// @file TextureManager.h
/// @brief テクスチャマネージャー — ハンドルベースのテクスチャ管理
///
/// 【初学者向け解説】
/// DXLibでは LoadGraph() で画像を読み込むとint型の「ハンドル」が返ります。
/// TextureManagerは同じ仕組みを提供します：
/// - LoadTexture(filePath) → ハンドル(int)を返す
/// - GetTexture(handle) → Textureオブジェクトを取得
/// - 同じパスを2回読み込んでもキャッシュから返す（メモリ節約）

#include "pch.h"
#include "Graphics/Resource/Texture.h"
#include "Graphics/Device/DescriptorHeap.h"
#include <unordered_map>

namespace GX
{

/// @brief テクスチャのUV矩形情報（スプライトシート用）
struct TextureRegion
{
    float u0 = 0.0f, v0 = 0.0f;  ///< 左上UV
    float u1 = 1.0f, v1 = 1.0f;  ///< 右下UV
    int textureHandle = -1;       ///< 元テクスチャのハンドル
};

/// @brief ハンドルベースのテクスチャ管理クラス
class TextureManager
{
public:
    static constexpr uint32_t k_MaxTextures = 256;

    TextureManager() = default;
    ~TextureManager() = default;

    /// マネージャーを初期化
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);

    /// テクスチャを読み込み、ハンドルを返す
    /// @return テクスチャハンドル（失敗時は-1）
    int LoadTexture(const std::wstring& filePath);

    /// CPUメモリからテクスチャを作成
    int CreateTextureFromMemory(const void* pixels, uint32_t width, uint32_t height);

    /// 既存テクスチャのピクセルデータを更新（ハンドル・SRV維持）
    bool UpdateTextureFromMemory(int handle, const void* pixels, uint32_t width, uint32_t height);

    /// ハンドルからテクスチャを取得
    Texture* GetTexture(int handle);

    /// ハンドルからテクスチャリージョン（UV矩形）を取得
    const TextureRegion& GetRegion(int handle) const;

    /// ハンドルからテクスチャのファイルパスを取得
    const std::wstring& GetFilePath(int handle) const;

    /// テクスチャを解放
    void ReleaseTexture(int handle);

    /// 分割テクスチャ用：1枚のテクスチャに対して複数のUV矩形ハンドルを作成
    /// @return 作成した最初のハンドル（連番で allNum 個作成）
    int CreateRegionHandles(int baseHandle, int allNum, int xNum, int yNum, int xSize, int ySize);

    /// Shader-visibleなSRVヒープを取得
    DescriptorHeap& GetSRVHeap() { return m_srvHeap; }

private:
    int AllocateHandle();

    ID3D12Device*       m_device   = nullptr;
    ID3D12CommandQueue* m_cmdQueue = nullptr;

    DescriptorHeap m_srvHeap;

    struct TextureEntry
    {
        std::unique_ptr<Texture> texture;
        TextureRegion region;
        std::wstring filePath;
        bool isRegionOnly = false;  ///< UV矩形のみ（テクスチャは別ハンドルが所有）
    };

    std::vector<TextureEntry>                  m_entries;
    std::unordered_map<std::wstring, int>      m_pathCache;
    std::vector<int>                           m_freeHandles;
    int                                        m_nextHandle = 0;
};

} // namespace GX
