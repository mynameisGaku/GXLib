#pragma once
/// @file TextRenderer.h
/// @brief テキストレンダラー — SpriteBatchを利用したテキスト描画
///
/// 【初学者向け解説】
/// TextRendererは、FontManagerで作成したフォントアトラスと
/// SpriteBatchの矩形描画機能を組み合わせてテキストを描画します。
///
/// 文字列の各文字について：
/// 1. FontManagerからグリフ情報（アトラス上の位置）を取得
/// 2. SpriteBatch::DrawRectGraphで該当部分を描画
/// 3. 次の文字の位置に移動（advance幅分）
///
/// DXLibのDrawString/DrawFormatStringに相当する機能です。

#include "pch.h"
#include "Graphics/Rendering/FontManager.h"

namespace GX
{

class SpriteBatch;

/// @brief SpriteBatchベースのテキスト描画クラス
class TextRenderer
{
public:
    TextRenderer() = default;
    ~TextRenderer() = default;

    /// 初期化
    void Initialize(SpriteBatch* spriteBatch, FontManager* fontManager);

    /// 文字列を描画（SpriteBatchのBegin/End間で呼ぶこと）
    void DrawString(int fontHandle, float x, float y, const std::wstring& text, uint32_t color = 0xFFFFFFFF);

    /// printf形式で文字列を描画
    void DrawFormatString(int fontHandle, float x, float y, uint32_t color, const wchar_t* format, ...);

    /// 文字列の描画幅を計算
    int GetStringWidth(int fontHandle, const std::wstring& text);

private:
    SpriteBatch* m_spriteBatch = nullptr;
    FontManager* m_fontManager = nullptr;
};

} // namespace GX
