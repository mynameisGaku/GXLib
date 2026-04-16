#pragma once
/// @file pch_common.h
/// @brief 共通プリコンパイルドヘッダー（Windows + STL）
///
/// グラフィックス・オーディオに依存しないモジュール用の最小PCH。
/// GXLib_Foundation, GXLib_Core, GXLib_Input 等で使用。

// ============================================================================
// Windows
// ============================================================================
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// ============================================================================
// COM スマートポインタ
// ============================================================================
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

// ============================================================================
// C++ 標準ライブラリ
// ============================================================================
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cassert>
#include <cstdint>
#include <array>
#include <stdexcept>
#include <format>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cmath>
#include <random>

// ============================================================================
// SIMD intrinsics — SSE/SSE2/SSE4.1
// ============================================================================
#include <immintrin.h>

// ============================================================================
// DirectXMath — エンジン内部で使用（公開math型はDirectXMath非依存）
// ============================================================================
#include <DirectXMath.h>
using namespace DirectX;

// ============================================================================
// GXLib コンテナライブラリ — gx::Vector, gx::String 等を全モジュールで利用可能
// ============================================================================
#include "Container/ContainerFwd.h"
