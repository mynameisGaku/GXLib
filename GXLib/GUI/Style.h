#pragma once
/// @file Style.h
/// @brief GUI スタイル構造体・プロパティ定義
///
/// CSS-like なスタイルプロパティを定義する構造体群。
/// Flexboxレイアウト、ボックスモデル、テキスト、背景、影などを表現する。

#include "pch.h"

namespace GX { namespace GUI {

// ============================================================================
// サイズ単位
// ============================================================================

/// @brief サイズの単位
enum class SizeUnit { Px, Percent, Auto };

/// @brief 単位付きの長さ値
struct StyleLength
{
    float value = 0.0f;
    SizeUnit unit = SizeUnit::Auto;

    /// ピクセル値を作成
    static StyleLength Px(float v) { return { v, SizeUnit::Px }; }
    /// パーセント値を作成
    static StyleLength Pct(float v) { return { v, SizeUnit::Percent }; }
    /// Auto値を作成
    static StyleLength Auto() { return { 0.0f, SizeUnit::Auto }; }

    bool IsAuto() const { return unit == SizeUnit::Auto; }

    /// 親サイズを基準にピクセル値を解決する
    float Resolve(float parentSize) const
    {
        switch (unit)
        {
        case SizeUnit::Px:      return value;
        case SizeUnit::Percent: return parentSize * value * 0.01f;
        case SizeUnit::Auto:    return 0.0f;
        }
        return 0.0f;
    }
};

// ============================================================================
// 色
// ============================================================================

/// @brief RGBA色
struct StyleColor
{
    float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;

    StyleColor() = default;
    StyleColor(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    /// "#RRGGBB" or "#RRGGBBAA" 形式からパース
    static StyleColor FromHex(const std::string& hex)
    {
        StyleColor c;
        if (hex.empty() || hex[0] != '#') return c;

        auto parseHex = [](const std::string& s, size_t offset, size_t len) -> uint8_t
        {
            unsigned int val = 0;
            for (size_t i = 0; i < len; ++i)
            {
                char ch = s[offset + i];
                val <<= 4;
                if (ch >= '0' && ch <= '9') val += (ch - '0');
                else if (ch >= 'a' && ch <= 'f') val += (ch - 'a' + 10);
                else if (ch >= 'A' && ch <= 'F') val += (ch - 'A' + 10);
            }
            return static_cast<uint8_t>(val);
        };

        if (hex.size() >= 7) // #RRGGBB
        {
            c.r = parseHex(hex, 1, 2) / 255.0f;
            c.g = parseHex(hex, 3, 2) / 255.0f;
            c.b = parseHex(hex, 5, 2) / 255.0f;
            c.a = 1.0f;
        }
        if (hex.size() >= 9) // #RRGGBBAA
        {
            c.a = parseHex(hex, 7, 2) / 255.0f;
        }
        return c;
    }

    bool IsTransparent() const { return a <= 0.0f; }
};

// ============================================================================
// エッジ（margin/padding）
// ============================================================================

/// @brief 四辺の値（margin, padding）
struct StyleEdges
{
    float top = 0.0f, right = 0.0f, bottom = 0.0f, left = 0.0f;

    StyleEdges() = default;
    StyleEdges(float all) : top(all), right(all), bottom(all), left(all) {}
    StyleEdges(float v, float h) : top(v), right(h), bottom(v), left(h) {}
    StyleEdges(float t, float r, float b, float l) : top(t), right(r), bottom(b), left(l) {}

    float HorizontalTotal() const { return left + right; }
    float VerticalTotal() const { return top + bottom; }
};

// ============================================================================
// テキスト揃え
// ============================================================================

enum class TextAlign { Left, Center, Right };
enum class VAlign { Top, Center, Bottom };

// ============================================================================
// Flexbox
// ============================================================================

enum class FlexDirection { Row, Column };
enum class JustifyContent { Start, Center, End, SpaceBetween, SpaceAround };
enum class AlignItems { Start, Center, End, Stretch };
enum class PositionType { Relative, Absolute };
enum class OverflowMode { Visible, Hidden, Scroll };

// ============================================================================
// スタイル構造体
// ============================================================================

/// @brief ウィジェットの計算済みスタイル
struct Style
{
    // --- サイズ ---
    StyleLength width  = StyleLength::Auto();
    StyleLength height = StyleLength::Auto();
    StyleLength minWidth  = StyleLength::Px(0);
    StyleLength minHeight = StyleLength::Px(0);
    StyleLength maxWidth  = StyleLength::Px(100000.0f);
    StyleLength maxHeight = StyleLength::Px(100000.0f);

    // --- ボックスモデル ---
    StyleEdges margin;
    StyleEdges padding;
    float borderWidth = 0.0f;
    StyleColor borderColor;

    // --- 背景 ---
    StyleColor backgroundColor;
    float cornerRadius = 0.0f;

    // --- テキスト ---
    StyleColor color = { 1.0f, 1.0f, 1.0f, 1.0f };
    float fontSize = 16.0f;
    std::string fontFamily;
    TextAlign textAlign = TextAlign::Left;
    VAlign verticalAlign = VAlign::Top;

    // --- Flexbox レイアウト ---
    FlexDirection flexDirection = FlexDirection::Column;
    JustifyContent justifyContent = JustifyContent::Start;
    AlignItems alignItems = AlignItems::Stretch;
    float flexGrow = 0.0f;
    float flexShrink = 1.0f;
    float gap = 0.0f;

    // --- 位置 ---
    PositionType position = PositionType::Relative;
    StyleLength posLeft   = StyleLength::Auto();
    StyleLength posTop    = StyleLength::Auto();

    // --- オーバーフロー ---
    OverflowMode overflow = OverflowMode::Visible;

    // --- 影 ---
    float shadowOffsetX = 0.0f, shadowOffsetY = 0.0f;
    float shadowBlur = 0.0f;
    StyleColor shadowColor;

    // --- アニメーション ---
    float transitionDuration = 0.0f;
};

}} // namespace GX::GUI
