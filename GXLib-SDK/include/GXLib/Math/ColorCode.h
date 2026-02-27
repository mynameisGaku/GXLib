#pragma once
/// @file ColorCode.h
/// @brief Draw系関数で使える 0xFFRRGGBB 形式のカラー定数

namespace gx {
namespace ColorCode {

// --- 基本色 ---
constexpr unsigned int Black       = 0xFF000000;
constexpr unsigned int White       = 0xFFFFFFFF;
constexpr unsigned int Red         = 0xFFFF0000;
constexpr unsigned int Green       = 0xFF00FF00;
constexpr unsigned int Blue        = 0xFF0000FF;
constexpr unsigned int Yellow      = 0xFFFFFF00;
constexpr unsigned int Cyan        = 0xFF00FFFF;
constexpr unsigned int Magenta     = 0xFFFF00FF;

// --- グレースケール ---
constexpr unsigned int Gray        = 0xFF808080;
constexpr unsigned int DarkGray    = 0xFF404040;
constexpr unsigned int LightGray   = 0xFFC0C0C0;
constexpr unsigned int Silver      = 0xFFC0C0C0;
constexpr unsigned int DimGray     = 0xFF696969;

// --- 赤系 ---
constexpr unsigned int DarkRed     = 0xFF8B0000;
constexpr unsigned int Crimson     = 0xFFDC143C;
constexpr unsigned int Firebrick   = 0xFFB22222;
constexpr unsigned int IndianRed   = 0xFFCD5C5C;
constexpr unsigned int Salmon      = 0xFFFA8072;
constexpr unsigned int Coral       = 0xFFFF7F50;
constexpr unsigned int Tomato      = 0xFFFF6347;
constexpr unsigned int OrangeRed   = 0xFFFF4500;

// --- オレンジ系 ---
constexpr unsigned int Orange      = 0xFFFFA500;
constexpr unsigned int DarkOrange  = 0xFFFF8C00;
constexpr unsigned int Gold        = 0xFFFFD700;

// --- 黄系 ---
constexpr unsigned int LightYellow = 0xFFFFFFE0;
constexpr unsigned int Khaki       = 0xFFF0E68C;
constexpr unsigned int DarkKhaki   = 0xFFBDB76B;

// --- 緑系 ---
constexpr unsigned int Lime        = 0xFF00FF00;
constexpr unsigned int LimeGreen   = 0xFF32CD32;
constexpr unsigned int LawnGreen   = 0xFF7CFC00;
constexpr unsigned int Chartreuse  = 0xFF7FFF00;
constexpr unsigned int SpringGreen = 0xFF00FF7F;
constexpr unsigned int MediumSpringGreen = 0xFF00FA9A;
constexpr unsigned int ForestGreen = 0xFF228B22;
constexpr unsigned int DarkGreen   = 0xFF006400;
constexpr unsigned int SeaGreen    = 0xFF2E8B57;
constexpr unsigned int Olive       = 0xFF808000;
constexpr unsigned int OliveDrab   = 0xFF6B8E23;

// --- 水色・シアン系 ---
constexpr unsigned int Aqua        = 0xFF00FFFF;
constexpr unsigned int Turquoise   = 0xFF40E0D0;
constexpr unsigned int Teal        = 0xFF008080;
constexpr unsigned int DarkCyan    = 0xFF008B8B;
constexpr unsigned int LightCyan   = 0xFFE0FFFF;
constexpr unsigned int CadetBlue   = 0xFF5F9EA0;
constexpr unsigned int Aquamarine  = 0xFF7FFFD4;

// --- 青系 ---
constexpr unsigned int Navy        = 0xFF000080;
constexpr unsigned int DarkBlue    = 0xFF00008B;
constexpr unsigned int MediumBlue  = 0xFF0000CD;
constexpr unsigned int RoyalBlue   = 0xFF4169E1;
constexpr unsigned int DodgerBlue  = 0xFF1E90FF;
constexpr unsigned int DeepSkyBlue = 0xFF00BFFF;
constexpr unsigned int SkyBlue     = 0xFF87CEEB;
constexpr unsigned int LightBlue   = 0xFFADD8E6;
constexpr unsigned int SteelBlue   = 0xFF4682B4;
constexpr unsigned int CornflowerBlue = 0xFF6495ED;
constexpr unsigned int MidnightBlue   = 0xFF191970;

// --- 紫系 ---
constexpr unsigned int Purple      = 0xFF800080;
constexpr unsigned int DarkViolet  = 0xFF9400D3;
constexpr unsigned int BlueViolet  = 0xFF8A2BE2;
constexpr unsigned int Indigo      = 0xFF4B0082;
constexpr unsigned int MediumPurple= 0xFF9370DB;
constexpr unsigned int Orchid      = 0xFFDA70D6;
constexpr unsigned int Plum        = 0xFFDDA0DD;
constexpr unsigned int Violet      = 0xFFEE82EE;
constexpr unsigned int Lavender    = 0xFFE6E6FA;

// --- ピンク系 ---
constexpr unsigned int Pink        = 0xFFFFC0CB;
constexpr unsigned int HotPink     = 0xFFFF69B4;
constexpr unsigned int DeepPink    = 0xFFFF1493;
constexpr unsigned int MediumVioletRed = 0xFFC71585;

// --- 茶系 ---
constexpr unsigned int Brown       = 0xFFA52A2A;
constexpr unsigned int SaddleBrown = 0xFF8B4513;
constexpr unsigned int Sienna      = 0xFFA0522D;
constexpr unsigned int Chocolate   = 0xFFD2691E;
constexpr unsigned int Peru        = 0xFFCD853F;
constexpr unsigned int SandyBrown  = 0xFFF4A460;
constexpr unsigned int Tan         = 0xFFD2B48C;
constexpr unsigned int Maroon      = 0xFF800000;

// --- 白系 ---
constexpr unsigned int Snow        = 0xFFFFFAFA;
constexpr unsigned int Ivory       = 0xFFFFFFF0;
constexpr unsigned int Linen       = 0xFFFAF0E6;
constexpr unsigned int AntiqueWhite= 0xFFFAEBD7;
constexpr unsigned int Beige       = 0xFFF5F5DC;
constexpr unsigned int Wheat       = 0xFFF5DEB3;
constexpr unsigned int Cornsilk    = 0xFFFFF8DC;

} // namespace ColorCode
} // namespace gx
