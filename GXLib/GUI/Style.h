#pragma once
/// @file Style.h
/// @brief GUI スタイル構造体・プロパティ定義
///
/// CSS-like なスタイルプロパティを定義する構造体群。
/// Flexboxレイアウト、ボックスモデル、テキスト、背景、影などを表現する。

#include "pch_graphics.h"

namespace gx { namespace GUI {
/// @addtogroup grp_gui
/// @{

// ============================================================================
// サイズ単位
// ============================================================================

/// @brief サイズの単位
enum class SizeUnit
{
    Px,      ///< ピクセル（絶対値）
    Percent, ///< 親要素に対する百分率
    Auto     ///< 自動計算（内容やレイアウトに応じる）
};

/// @brief 単位付きの長さ値
struct StyleLength
{
    float value = 0.0f;          ///< 数値
    SizeUnit unit = SizeUnit::Auto; ///< 単位

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

/// @brief RGBA色（各成分は 0.0~1.0 の float）
/// r/g/b は赤/緑/青の明るさ（0=なし, 1=最大）、a は不透明度（0=透明, 1=不透明）
struct StyleColor
{
    float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;

    StyleColor() = default;
    StyleColor(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    /// @brief "#RRGGBB" または "#RRGGBBAA" 形式の16進数カラー文字列を解析する
    /// @param hex "#FF8800" のようなカラー文字列
    /// @return パース結果のStyleColor
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

    /// @brief 完全に透明かどうかを判定する
    /// @return アルファが0以下なら true
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

    /// @brief 左右の合計値を返す
    float HorizontalTotal() const { return left + right; }
    /// @brief 上下の合計値を返す
    float VerticalTotal() const { return top + bottom; }
};

// ============================================================================
// テキスト揃え
// ============================================================================

/// @brief テキストの水平方向の揃え
enum class TextAlign { Left, Center, Right };
/// @brief テキストの垂直方向の揃え
enum class VAlign { Top, Center, Bottom };
/// @brief UIエフェクトの種類（ボタン押下時のリップル等）
enum class UIEffectType { None, Ripple };

// ============================================================================
// フレックスボックス（子要素を横/縦に並べるレイアウト方式。CSSのFlexboxと同等）
// ============================================================================

/// @brief 子要素の並び方向。Row=横並び、Column=縦並び
enum class FlexDirection { Row, Column };
/// @brief 主軸方向の配置。Start=先頭詰め、Center=中央、End=末尾詰め、SpaceBetween=均等配置
enum class JustifyContent { Start, Center, End, SpaceBetween, SpaceAround };
/// @brief 交差軸方向の揃え。Stretch=親の幅いっぱいに引き伸ばす
enum class AlignItems { Start, Center, End, Stretch };
/// @brief 配置方式。Relative=通常フロー、Absolute=親のコンテンツ領域基準で絶対配置
enum class PositionType { Relative, Absolute };
/// @brief はみ出し時の挙動。Visible=表示、Hidden=クリップ、Scroll=スクロール可能にクリップ
enum class OverflowMode { Visible, Hidden, Scroll };

// ============================================================================
// スタイル構造体
// ============================================================================

/// @brief ウィジェットの計算済みスタイル
/// CSSのプロパティに対応するフィールドを持つ。
/// StyleSheetから自動計算されるか、コードから直接設定する。
struct Style
{
    // --- サイズ ---
    StyleLength width  = StyleLength::Auto();              ///< 幅
    StyleLength height = StyleLength::Auto();              ///< 高さ
    StyleLength minWidth  = StyleLength::Px(0);            ///< 最小幅
    StyleLength minHeight = StyleLength::Px(0);            ///< 最小高さ
    StyleLength maxWidth  = StyleLength::Px(100000.0f);    ///< 最大幅
    StyleLength maxHeight = StyleLength::Px(100000.0f);    ///< 最大高さ

    // --- ボックスモデル ---
    StyleEdges margin;                                     ///< 外側余白
    StyleEdges padding;                                    ///< 内側余白
    float borderWidth = 0.0f;                              ///< ボーダー幅（ピクセル）
    StyleColor borderColor;                                ///< ボーダー色

    // --- 背景 ---
    StyleColor backgroundColor;                            ///< 背景色
    float cornerRadius = 0.0f;                             ///< 角丸半径（ピクセル）

    // --- テキスト ---
    StyleColor color = { 1.0f, 1.0f, 1.0f, 1.0f };       ///< テキスト色
    float fontSize = 16.0f;                                ///< フォントサイズ（ピクセル）
    std::string fontFamily;                                ///< フォントファミリー名
    TextAlign textAlign = TextAlign::Left;                 ///< テキスト水平揃え
    VAlign verticalAlign = VAlign::Top;                    ///< テキスト垂直揃え

    // --- Flexbox レイアウト ---
    FlexDirection flexDirection = FlexDirection::Column;   ///< 子要素の並び方向
    JustifyContent justifyContent = JustifyContent::Start; ///< 主軸方向の配置
    AlignItems alignItems = AlignItems::Stretch;           ///< 交差軸方向の揃え
    float flexGrow = 0.0f;                                 ///< 余白配分比率（0=伸びない）
    float flexShrink = 1.0f;                               ///< 縮小比率（0=縮まない）
    float gap = 0.0f;                                      ///< 子要素間の間隔（ピクセル）

    // --- 位置 ---
    PositionType position = PositionType::Relative;        ///< 配置方式
    StyleLength posLeft   = StyleLength::Auto();           ///< 左位置（Absolute時に使用）
    StyleLength posTop    = StyleLength::Auto();           ///< 上位置（Absolute時に使用）

    // --- オーバーフロー ---
    OverflowMode overflow = OverflowMode::Visible;         ///< はみ出し時の挙動

    // --- 影 ---
    float shadowOffsetX = 0.0f, shadowOffsetY = 0.0f;     ///< 影のオフセット（ピクセル）
    float shadowBlur = 0.0f;                               ///< 影のぼかし半径（ピクセル）
    StyleColor shadowColor;                                ///< 影の色

    // --- Opacity / Transform ---
    float opacity = 1.0f;                                  ///< 不透明度（0.0~1.0）
    float translateX = 0.0f;                               ///< X方向移動（ピクセル）
    float translateY = 0.0f;                               ///< Y方向移動（ピクセル）
    float scaleX = 1.0f;                                   ///< X方向スケール
    float scaleY = 1.0f;                                   ///< Y方向スケール
    float rotate = 0.0f;                                   ///< 回転角度（度）
    float pivotX = 0.5f;                                   ///< 回転/スケールの基点X（0~1）
    float pivotY = 0.5f;                                   ///< 回転/スケールの基点Y（0~1）

    // --- Effects ---
    UIEffectType effectType = UIEffectType::None;          ///< エフェクトの種類
    float effectStrength = 0.0f;                           ///< エフェクト強度（0~1）
    float effectWidth = 0.0f;                              ///< エフェクト幅（0~1）
    float effectDuration = 0.0f;                           ///< エフェクト持続時間（秒）

    // --- Image UV ---
    float imageUVScaleX = 1.0f;                            ///< 画像UVスケールX
    float imageUVScaleY = 1.0f;                            ///< 画像UVスケールY
    float imageUVSpeedX = 0.0f;                            ///< 画像UVスクロール速度X
    float imageUVSpeedY = 0.0f;                            ///< 画像UVスクロール速度Y

    // --- アニメーション ---
    float transitionDuration = 0.0f;                       ///< スタイル遷移アニメーション時間（秒）
};

// ============================================================================
// スタイル補助関数（アニメーション用）
// ============================================================================

/// @brief 2つのfloat値がほぼ等しいか判定する（スタイル遷移の変化検出用）
inline bool NearlyEqual(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}

/// @brief 2つの色を線形補間する
/// @param a 開始色
/// @param b 終了色
/// @param t 補間係数（0.0=a, 1.0=b）
/// @return 補間結果の色
inline StyleColor LerpColor(const StyleColor& a, const StyleColor& b, float t)
{
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    };
}

/// @brief 2つのスタイルの視覚的プロパティが等しいか判定する（遷移の変化検出用）
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

/// @brief 2つのスタイルの視覚的プロパティを線形補間する（transitionDurationによるアニメーション用）
/// @param from 開始スタイル
/// @param to 終了スタイル
/// @param t 補間係数（0.0=from, 1.0=to）
/// @return 補間結果のスタイル（レイアウト系プロパティは to の値を採用）
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

/// @}
}} // namespace gx::GUI
