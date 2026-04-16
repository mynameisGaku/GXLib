#pragma once
/// @file pch_audio.h
/// @brief オーディオ用プリコンパイルドヘッダー（Windows + STL + XAudio2）
///
/// GXLib_Audio で使用。

#include "pch_common.h"

// ============================================================================
// XAudio2 — オーディオAPI
// ============================================================================
#include <xaudio2.h>
#include <x3daudio.h>
#pragma comment(lib, "xaudio2.lib")
