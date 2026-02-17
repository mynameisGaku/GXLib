/// @file TextureManager.cpp
/// @brief テクスチャマネージャーの実装
#include "pch.h"
#include "Graphics/Resource/TextureManager.h"
#include "Core/Logger.h"

namespace GX
{

bool TextureManager::Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    m_device   = device;
    m_cmdQueue = cmdQueue;

    // Shader-visible な CBV_SRV_UAV ディスクリプタヒープを作成
    if (!m_srvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, k_MaxTextures, true))
    {
        GX_LOG_ERROR("Failed to create SRV heap for TextureManager");
        return false;
    }

    m_entries.reserve(k_MaxTextures);
    GX_LOG_INFO("TextureManager initialized (max: %u textures)", k_MaxTextures);
    return true;
}

int TextureManager::AllocateHandle()
{
    if (!m_freeHandles.empty())
    {
        int handle = m_freeHandles.back();
        m_freeHandles.pop_back();
        return handle;
    }

    if (m_nextHandle >= static_cast<int>(k_MaxTextures))
    {
        GX_LOG_ERROR("TextureManager: handle limit reached (max: %u)", k_MaxTextures);
        return -1;
    }

    int handle = m_nextHandle++;
    if (handle >= static_cast<int>(m_entries.size()))
    {
        m_entries.resize(handle + 1);
    }
    return handle;
}

int TextureManager::LoadTexture(const std::wstring& filePath)
{
    // キャッシュチェック
    auto it = m_pathCache.find(filePath);
    if (it != m_pathCache.end())
    {
        return it->second;
    }

    int handle = AllocateHandle();
    if (handle < 0) return -1;

    uint32_t srvIndex = m_srvHeap.AllocateIndex();
    if (srvIndex == DescriptorHeap::k_InvalidIndex)
    {
        m_freeHandles.push_back(handle);
        return -1;
    }

    auto& entry = m_entries[handle];
    entry.texture = std::make_unique<Texture>();
    entry.filePath = filePath;
    entry.isRegionOnly = false;
    entry.region = TextureRegion{ 0.0f, 0.0f, 1.0f, 1.0f, handle };

    if (!entry.texture->LoadFromFile(m_device, m_cmdQueue, filePath, &m_srvHeap, srvIndex))
    {
        entry.texture.reset();
        m_freeHandles.push_back(handle);
        m_srvHeap.Free(srvIndex);
        return -1;
    }

    m_pathCache[filePath] = handle;
    return handle;
}

int TextureManager::CreateTextureFromMemory(const void* pixels, uint32_t width, uint32_t height)
{
    int handle = AllocateHandle();
    if (handle < 0) return -1;

    uint32_t srvIndex = m_srvHeap.AllocateIndex();
    if (srvIndex == DescriptorHeap::k_InvalidIndex)
    {
        m_freeHandles.push_back(handle);
        return -1;
    }

    auto& entry = m_entries[handle];
    entry.texture = std::make_unique<Texture>();
    entry.isRegionOnly = false;
    entry.region = TextureRegion{ 0.0f, 0.0f, 1.0f, 1.0f, handle };

    if (!entry.texture->CreateFromMemory(m_device, m_cmdQueue, pixels, width, height, &m_srvHeap, srvIndex))
    {
        entry.texture.reset();
        m_freeHandles.push_back(handle);
        m_srvHeap.Free(srvIndex);
        return -1;
    }

    return handle;
}

bool TextureManager::UpdateTextureFromMemory(int handle, const void* pixels, uint32_t width, uint32_t height)
{
    if (handle < 0 || handle >= static_cast<int>(m_entries.size()))
        return false;

    auto& entry = m_entries[handle];
    if (!entry.texture || entry.isRegionOnly)
        return false;

    return entry.texture->UpdatePixels(m_device, m_cmdQueue, pixels, width, height);
}

Texture* TextureManager::GetTexture(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_entries.size()))
        return nullptr;

    auto& entry = m_entries[handle];
    if (entry.isRegionOnly)
    {
        // リージョンハンドルの場合、元テクスチャを返す
        return GetTexture(entry.region.textureHandle);
    }
    return entry.texture.get();
}

const TextureRegion& TextureManager::GetRegion(int handle) const
{
    static const TextureRegion defaultRegion;
    if (handle < 0 || handle >= static_cast<int>(m_entries.size()))
        return defaultRegion;
    return m_entries[handle].region;
}

void TextureManager::ReleaseTexture(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_entries.size()))
        return;

    auto& entry = m_entries[handle];

    // パスキャッシュから削除
    if (!entry.filePath.empty())
    {
        m_pathCache.erase(entry.filePath);
        entry.filePath.clear();
    }

    entry.texture.reset();
    entry.isRegionOnly = false;
    m_freeHandles.push_back(handle);
}

int TextureManager::CreateRegionHandles(int baseHandle, int allNum, int xNum, int yNum, int xSize, int ySize)
{
    Texture* baseTex = GetTexture(baseHandle);
    if (!baseTex)
        return -1;

    float texWidth  = static_cast<float>(baseTex->GetWidth());
    float texHeight = static_cast<float>(baseTex->GetHeight());

    int firstHandle = -1;

    for (int i = 0; i < allNum; ++i)
    {
        int col = i % xNum;
        int row = i / xNum;

        int handle = AllocateHandle();
        if (handle < 0)
        {
            GX_LOG_ERROR("TextureManager: Failed to allocate region handle %d/%d", i, allNum);
            return firstHandle; // return what we have so far (-1 if none)
        }
        if (firstHandle == -1) firstHandle = handle;

        auto& entry = m_entries[handle];
        entry.texture.reset();
        entry.isRegionOnly = true;
        entry.region.textureHandle = baseHandle;
        entry.region.u0 = (col * xSize) / texWidth;
        entry.region.v0 = (row * ySize) / texHeight;
        entry.region.u1 = ((col + 1) * xSize) / texWidth;
        entry.region.v1 = ((row + 1) * ySize) / texHeight;
    }

    return firstHandle;
}

} // namespace GX
