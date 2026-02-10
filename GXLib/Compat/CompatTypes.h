#pragma once
/// @file CompatTypes.h
/// @brief GXLib簡易API用の型定義・定数
///
/// GXLibの簡易APIで使用する定数・型を定義します。
/// DXLibからの移行を容易にするための互換定数も含みます。

#include <windows.h>

// ============================================================================
// 描画先
// ============================================================================
#define GX_SCREEN_FRONT  0  ///< 表画面（直接表示される画面）
#define GX_SCREEN_BACK   1  ///< 裏画面（ScreenFlipで表に転送される）

// ============================================================================
// ブレンドモード
// ============================================================================
#define GX_BLENDMODE_NOBLEND  0  ///< ブレンド無し（不透明描画）
#define GX_BLENDMODE_ALPHA    1  ///< アルファブレンド
#define GX_BLENDMODE_ADD      2  ///< 加算ブレンド
#define GX_BLENDMODE_SUB      3  ///< 減算ブレンド
#define GX_BLENDMODE_MUL      4  ///< 乗算ブレンド
#define GX_BLENDMODE_SCREEN   5  ///< スクリーンブレンド

// ============================================================================
// サウンド再生タイプ
// ============================================================================
#define GX_PLAYTYPE_NORMAL  0  ///< 通常再生（再生完了まで待機）
#define GX_PLAYTYPE_BACK    1  ///< バックグラウンド再生（SE用）
#define GX_PLAYTYPE_LOOP    2  ///< ループ再生（BGM用）

// ============================================================================
// キー定数 (DXLib互換: DirectInput DIK_* コード体系)
// ============================================================================
#define KEY_INPUT_ESCAPE    0x01
#define KEY_INPUT_1         0x02
#define KEY_INPUT_2         0x03
#define KEY_INPUT_3         0x04
#define KEY_INPUT_4         0x05
#define KEY_INPUT_5         0x06
#define KEY_INPUT_6         0x07
#define KEY_INPUT_7         0x08
#define KEY_INPUT_8         0x09
#define KEY_INPUT_9         0x0A
#define KEY_INPUT_0         0x0B
#define KEY_INPUT_MINUS     0x0C
#define KEY_INPUT_BACK      0x0E
#define KEY_INPUT_TAB       0x0F
#define KEY_INPUT_Q         0x10
#define KEY_INPUT_W         0x11
#define KEY_INPUT_E         0x12
#define KEY_INPUT_R         0x13
#define KEY_INPUT_T         0x14
#define KEY_INPUT_Y         0x15
#define KEY_INPUT_U         0x16
#define KEY_INPUT_I         0x17
#define KEY_INPUT_O         0x18
#define KEY_INPUT_P         0x19
#define KEY_INPUT_LBRACKET  0x1A
#define KEY_INPUT_RBRACKET  0x1B
#define KEY_INPUT_RETURN    0x1C
#define KEY_INPUT_LCONTROL  0x1D
#define KEY_INPUT_A         0x1E
#define KEY_INPUT_S         0x1F
#define KEY_INPUT_D         0x20
#define KEY_INPUT_F         0x21
#define KEY_INPUT_G         0x22
#define KEY_INPUT_H         0x23
#define KEY_INPUT_J         0x24
#define KEY_INPUT_K         0x25
#define KEY_INPUT_L         0x26
#define KEY_INPUT_SEMICOLON 0x27
#define KEY_INPUT_LSHIFT    0x2A
#define KEY_INPUT_BACKSLASH 0x2B
#define KEY_INPUT_Z         0x2C
#define KEY_INPUT_X         0x2D
#define KEY_INPUT_C         0x2E
#define KEY_INPUT_V         0x2F
#define KEY_INPUT_B         0x30
#define KEY_INPUT_N         0x31
#define KEY_INPUT_M         0x32
#define KEY_INPUT_COMMA     0x33
#define KEY_INPUT_PERIOD    0x34
#define KEY_INPUT_SLASH     0x35
#define KEY_INPUT_RSHIFT    0x36
#define KEY_INPUT_MULTIPLY  0x37
#define KEY_INPUT_LALT      0x38
#define KEY_INPUT_SPACE     0x39
#define KEY_INPUT_CAPSLOCK  0x3A
#define KEY_INPUT_F1        0x3B
#define KEY_INPUT_F2        0x3C
#define KEY_INPUT_F3        0x3D
#define KEY_INPUT_F4        0x3E
#define KEY_INPUT_F5        0x3F
#define KEY_INPUT_F6        0x40
#define KEY_INPUT_F7        0x41
#define KEY_INPUT_F8        0x42
#define KEY_INPUT_F9        0x43
#define KEY_INPUT_F10       0x44
#define KEY_INPUT_NUMLOCK   0x45
#define KEY_INPUT_SCROLL    0x46
#define KEY_INPUT_NUMPAD7   0x47
#define KEY_INPUT_NUMPAD8   0x48
#define KEY_INPUT_NUMPAD9   0x49
#define KEY_INPUT_SUBTRACT  0x4A
#define KEY_INPUT_NUMPAD4   0x4B
#define KEY_INPUT_NUMPAD5   0x4C
#define KEY_INPUT_NUMPAD6   0x4D
#define KEY_INPUT_ADD       0x4E
#define KEY_INPUT_NUMPAD1   0x4F
#define KEY_INPUT_NUMPAD2   0x50
#define KEY_INPUT_NUMPAD3   0x51
#define KEY_INPUT_NUMPAD0   0x52
#define KEY_INPUT_DECIMAL   0x53
#define KEY_INPUT_F11       0x57
#define KEY_INPUT_F12       0x58
#define KEY_INPUT_RCONTROL  0x9D
#define KEY_INPUT_DIVIDE    0xB5
#define KEY_INPUT_RALT      0xB8
#define KEY_INPUT_HOME      0xC7
#define KEY_INPUT_UP        0xC8
#define KEY_INPUT_PGUP      0xC9
#define KEY_INPUT_LEFT      0xCB
#define KEY_INPUT_RIGHT     0xCD
#define KEY_INPUT_END       0xCF
#define KEY_INPUT_DOWN      0xD0
#define KEY_INPUT_PGDN      0xD1
#define KEY_INPUT_INSERT    0xD2
#define KEY_INPUT_DELETE    0xD3

// ============================================================================
// マウスボタン
// ============================================================================
#define MOUSE_INPUT_LEFT    1  ///< マウス左ボタン
#define MOUSE_INPUT_RIGHT   2  ///< マウス右ボタン
#define MOUSE_INPUT_MIDDLE  4  ///< マウス中ボタン（ホイールクリック）

// ============================================================================
// ゲームパッド
// ============================================================================
#define GX_INPUT_PAD1   0  ///< パッド1（1P）
#define GX_INPUT_PAD2   1  ///< パッド2（2P）
#define GX_INPUT_PAD3   2  ///< パッド3（3P）
#define GX_INPUT_PAD4   3  ///< パッド4（4P）

#define PAD_INPUT_DOWN   0x00000001  ///< 十字キー下
#define PAD_INPUT_LEFT   0x00000002  ///< 十字キー左
#define PAD_INPUT_RIGHT  0x00000004  ///< 十字キー右
#define PAD_INPUT_UP     0x00000008  ///< 十字キー上
#define PAD_INPUT_A      0x00000010  ///< Aボタン
#define PAD_INPUT_B      0x00000020  ///< Bボタン
#define PAD_INPUT_C      0x00000040  ///< Cボタン
#define PAD_INPUT_X      0x00000080  ///< Xボタン
#define PAD_INPUT_Y      0x00000100  ///< Yボタン
#define PAD_INPUT_Z      0x00000200  ///< Zボタン
#define PAD_INPUT_L      0x00000400  ///< Lボタン（左トリガー）
#define PAD_INPUT_R      0x00000800  ///< Rボタン（右トリガー）
#define PAD_INPUT_START  0x00001000  ///< STARTボタン
#define PAD_INPUT_M      0x00002000  ///< Mボタン

// ============================================================================
// TRUE/FALSE（互換用の真偽定数）
// ============================================================================
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// ============================================================================
// フォントタイプ
// ============================================================================
#define GX_FONTTYPE_NORMAL       0  ///< 通常フォント
#define GX_FONTTYPE_EDGE         1  ///< エッジ付きフォント
#define GX_FONTTYPE_ANTIALIASING 2  ///< アンチエイリアスフォント

// ============================================================================
// 互換型
// ============================================================================

/// @brief 3Dベクトル（簡易API用）
struct VECTOR
{
    float x;  ///< X成分
    float y;  ///< Y成分
    float z;  ///< Z成分
};

/// @brief 4x4行列（簡易API用）
struct MATRIX
{
    float m[4][4];  ///< 行列要素（m[行][列]）
};

/// @brief カラー型（0xFFRRGGBB形式の符号なし整数）
typedef unsigned int COLOR_U8;

// ============================================================================
// 3Dマテリアル用ヘルパー定数
// ============================================================================
#define GX_MATERIAL_TEX_ALBEDO       0
#define GX_MATERIAL_TEX_NORMAL       1
#define GX_MATERIAL_TEX_METALROUGH   2
#define GX_MATERIAL_TEX_AO           3
#define GX_MATERIAL_TEX_EMISSIVE     4

typedef struct GX_MATERIAL_PARAM
{
    float albedoR;
    float albedoG;
    float albedoB;
    float albedoA;
    float metallic;
    float roughness;
    float aoStrength;
    float emissiveStrength;
    float emissiveR;
    float emissiveG;
    float emissiveB;
} GX_MATERIAL_PARAM;
