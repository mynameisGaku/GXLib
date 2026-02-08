/// @file SpriteSheet.cpp
/// @brief スプライトシートの実装
#include "pch.h"
#include "Graphics/Rendering/SpriteSheet.h"
#include "Core/Logger.h"

namespace GX
{

bool SpriteSheet::LoadDivGraph(TextureManager& textureManager,
                                const std::wstring& filePath,
                                int allNum, int xNum, int yNum,
                                int xSize, int ySize,
                                int* handleArray)
{
    // まず1枚のテクスチャとしてロード
    int baseHandle = textureManager.LoadTexture(filePath);
    if (baseHandle < 0)
    {
        GX_LOG_ERROR("SpriteSheet: Failed to load base texture");
        return false;
    }

    // UV矩形ハンドルを作成
    int firstHandle = textureManager.CreateRegionHandles(baseHandle, allNum, xNum, yNum, xSize, ySize);
    if (firstHandle < 0)
    {
        GX_LOG_ERROR("SpriteSheet: Failed to create region handles");
        return false;
    }

    // ハンドル配列に書き込み
    for (int i = 0; i < allNum; ++i)
    {
        handleArray[i] = firstHandle + i;
    }

    GX_LOG_INFO("SpriteSheet loaded: %d divisions (%dx%d)", allNum, xNum, yNum);
    return true;
}

} // namespace GX
