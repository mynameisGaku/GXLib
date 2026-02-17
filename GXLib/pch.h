#pragma once
/// @file pch.h
/// @brief プリコンパイルドヘッダー
///
/// 【初学者向け解説】
/// プリコンパイルドヘッダー（PCH）とは、よく使うヘッダーファイルを
/// 事前にコンパイルしておくことで、ビルド時間を短縮する仕組みです。
/// Windows.hやDirectX 12のヘッダーは巨大なので、PCHに入れると効果的です。
/// すべての.cppファイルはこのpch.hを最初にインクルードします。

// ============================================================================
// Windows
// ============================================================================
#define WIN32_LEAN_AND_MEAN  // 使用頻度の低いAPIを除外してコンパイル高速化
#define NOMINMAX             // min/maxマクロを無効化（STLと衝突防止）
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// ============================================================================
// DirectX 12
// ============================================================================
#include <d3d12.h>           // Direct3D 12 コアAPI
#include <dxgi1_6.h>         // DXGI 1.6（アダプタ列挙、スワップチェーン等）
#include <dxcapi.h>          // DXC シェーダーコンパイラAPI
#include <d3d12sdklayers.h>  // デバッグレイヤー
#include <crtdbg.h>          // CRTメモリリーク検出

// ============================================================================
// DirectXMath — 高速な数学ライブラリ
// ============================================================================
#include <DirectXMath.h>
using namespace DirectX;

// ============================================================================
// COM スマートポインタ
// ============================================================================
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

// ============================================================================
// XAudio2 — オーディオAPI
// ============================================================================
#include <xaudio2.h>

// ============================================================================
// DirectWrite / Direct2D — テキストレンダリング用
// ============================================================================
#include <dwrite.h>
#include <d2d1.h>

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
