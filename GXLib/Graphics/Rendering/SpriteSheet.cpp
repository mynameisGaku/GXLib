/// @file SpriteSheet.cpp
/// @brief SpriteSheet の実装
///
/// テクスチャを1枚だけロードし、TextureManager のリージョンハンドル機能で
/// UV矩形を分割する。テクスチャ実体は1枚だけなのでVRAM効率が良い。
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
    // 元画像を1枚のテクスチャとしてGPUにロード
    int baseHandle = textureManager.LoadTexture(filePath);
    if (baseHandle < 0)
    {
        GX_LOG_ERROR("SpriteSheet: Failed to load base texture");
        return false;
    }

    // グリッド分割に応じたUV矩形ハンドルを一括作成
    // 各ハンドルは baseHandle と同じテクスチャを参照しつつ、異なるUV範囲を持つ
    int firstHandle = textureManager.CreateRegionHandles(baseHandle, allNum, xNum, yNum, xSize, ySize);
    if (firstHandle < 0)
    {
        GX_LOG_ERROR("SpriteSheet: Failed to create region handles");
        return false;
    }

    // 連番ハンドルをユーザの配列にコピー
    for (int i = 0; i < allNum; ++i)
    {
        handleArray[i] = firstHandle + i;
    }

    GX_LOG_INFO("SpriteSheet loaded: %d divisions (%dx%d)", allNum, xNum, yNum);
    return true;
}

} // namespace GX
