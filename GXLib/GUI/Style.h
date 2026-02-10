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

/// @brief RGBA色（0.0〜1.0のfloat）
/// 初学者向け: 0が暗く、1が最大の明るさ（アルファは透明度）です。
struct StyleColor
{
    float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;

    StyleColor() = default;
    StyleColor(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    /// "#RRGGBB" または "#RRGGBBAA" 形式を解析する
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
// エッジ（マージン/パディング）
// ============================================================================

/// @brief 四辺の値（マージン/パディング）
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
enum class UIEffectType { None, Ripple };

// ============================================================================
// フレックスボックス
// 初学者向け: 子要素を横/縦に並べるためのレイアウト設定です。
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

    // --- Opacity / Transform ---
    float opacity = 1.0f;
    float translateX = 0.0f;
    float translateY = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float rotate = 0.0f; // degrees
    float pivotX = 0.5f;
    float pivotY = 0.5f;

    // --- Effects ---
    UIEffectType effectType = UIEffectType::None;
    float effectStrength = 0.0f;
    float effectWidth = 0.0f;
    float effectDuration = 0.0f;

    // --- Image UV ---
    float imageUVScaleX = 1.0f;
    float imageUVScaleY = 1.0f;
    float imageUVSpeedX = 0.0f;
    float imageUVSpeedY = 0.0f;

    // --- アニメーション ---
    float transitionDuration = 0.0f;
};

// ============================================================================
// スタイル補助関数（アニメーション用）
// ============================================================================

inline bool NearlyEqual(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}

inline StyleColor LerpColor(const StyleColor& a, const StyleColor& b, float t)
{
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    };
}

inline bool VisualEquals(const Style& a, const Style& b)
{
    return NearlyEqual(a.backgroundColor.r, b.backgroundColor.r) &&
           NearlyEqual(a.backgroundColor.g, b.backgroundColor.g) &&
           NearlyEqual(a.backgroundColor.b, b.backgroundColor.b) &&
           NearlyEqual(a.backgroundColor.a, b.backgroundColor.a) &&
           NearlyEqual(a.borderColor.r, b.borderColor.r) &&
           NearlyEqual(a.borderColor.g, b.borderColor.g) &&
           NearlyEqual(a.borderColor.b, b.borderColor.b) &&
           NearlyEqual(a.borderColor.a, b.borderColor.a) &&
           NearlyEqual(a.color.r, b.color.r) &&
           NearlyEqual(a.color.g, b.color.g) &&
           NearlyEqual(a.color.b, b.color.b) &&
           NearlyEqual(a.color.a, b.color.a) &&
           NearlyEqual(a.shadowColor.r, b.shadowColor.r) &&
           NearlyEqual(a.shadowColor.g, b.shadowColor.g) &&
           NearlyEqual(a.shadowColor.b, b.shadowColor.b) &&
           NearlyEqual(a.shadowColor.a, b.shadowColor.a) &&
           NearlyEqual(a.cornerRadius, b.cornerRadius) &&
           NearlyEqual(a.borderWidth, b.borderWidth) &&
           NearlyEqual(a.shadowOffsetX, b.shadowOffsetX) &&
           NearlyEqual(a.shadowOffsetY, b.shadowOffsetY) &&
           NearlyEqual(a.shadowBlur, b.shadowBlur) &&
           NearlyEqual(a.opacity, b.opacity) &&
           NearlyEqual(a.translateX, b.translateX) &&
           NearlyEqual(a.translateY, b.translateY) &&
           NearlyEqual(a.scaleX, b.scaleX) &&
           NearlyEqual(a.scaleY, b.scaleY) &&
           NearlyEqual(a.rotate, b.rotate) &&
           NearlyEqual(a.pivotX, b.pivotX) &&
           NearlyEqual(a.pivotY, b.pivotY) &&
           a.effectType == b.effectType &&
           NearlyEqual(a.effectStrength, b.effectStrength) &&
           NearlyEqual(a.effectWidth, b.effectWidth) &&
           NearlyEqual(a.effectDuration, b.effectDuration) &&
           NearlyEqual(a.imageUVScaleX, b.imageUVScaleX) &&
           NearlyEqual(a.imageUVScaleY, b.imageUVScaleY) &&
           NearlyEqual(a.imageUVSpeedX, b.imageUVSpeedX) &&
           NearlyEqual(a.imageUVSpeedY, b.imageUVSpeedY);
}

inline Style LerpVisual(const Style& from, const Style& to, float t)
{
    Style out = to; // レイアウト系は target を採用
    out.backgroundColor = LerpColor(from.backgroundColor, to.backgroundColor, t);
    out.borderColor = LerpColor(from.borderColor, to.borderColor, t);
    out.color = LerpColor(from.color, to.color, t);
    out.shadowColor = LerpColor(from.shadowColor, to.shadowColor, t);
    out.cornerRadius = from.cornerRadius + (to.cornerRadius - from.cornerRadius) * t;
    out.borderWidth = from.borderWidth + (to.borderWidth - from.borderWidth) * t;
    out.shadowOffsetX = from.shadowOffsetX + (to.shadowOffsetX - from.shadowOffsetX) * t;
    out.shadowOffsetY = from.shadowOffsetY + (to.shadowOffsetY - from.shadowOffsetY) * t;
    out.shadowBlur = from.shadowBlur + (to.shadowBlur - from.shadowBlur) * t;
    out.opacity = from.opacity + (to.opacity - from.opacity) * t;
    out.translateX = from.translateX + (to.translateX - from.translateX) * t;
    out.translateY = from.translateY + (to.translateY - from.translateY) * t;
    out.scaleX = from.scaleX + (to.scaleX - from.scaleX) * t;
    out.scaleY = from.scaleY + (to.scaleY - from.scaleY) * t;
    out.rotate = from.rotate + (to.rotate - from.rotate) * t;
    out.pivotX = from.pivotX + (to.pivotX - from.pivotX) * t;
    out.pivotY = from.pivotY + (to.pivotY - from.pivotY) * t;
    out.effectType = to.effectType;
    out.effectStrength = from.effectStrength + (to.effectStrength - from.effectStrength) * t;
    out.effectWidth = from.effectWidth + (to.effectWidth - from.effectWidth) * t;
    out.effectDuration = from.effectDuration + (to.effectDuration - from.effectDuration) * t;
    out.imageUVScaleX = from.imageUVScaleX + (to.imageUVScaleX - from.imageUVScaleX) * t;
    out.imageUVScaleY = from.imageUVScaleY + (to.imageUVScaleY - from.imageUVScaleY) * t;
    out.imageUVSpeedX = from.imageUVSpeedX + (to.imageUVSpeedX - from.imageUVSpeedX) * t;
    out.imageUVSpeedY = from.imageUVSpeedY + (to.imageUVSpeedY - from.imageUVSpeedY) * t;
    return out;
}

}} // namespace GX::GUI
