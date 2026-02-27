#pragma once
/// @file pch_graphics.h
/// @brief グラフィックス用プリコンパイルドヘッダー（Windows + STL + D3D12 + DirectXMath）
///
/// GXLib_Graphics, GXLib_GUI 等で使用。

#include "pch_common.h"

// ============================================================================
// DirectX 12
// ============================================================================
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <d3d12sdklayers.h>
#include <crtdbg.h>

// ============================================================================
// DirectWrite / Direct2D — テキストレンダリング用
// ============================================================================
#include <dwrite.h>
#include <d2d1.h>
