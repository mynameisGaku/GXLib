#pragma once
/// @file GX/Config.h
/// @brief エンジン設定

#include <string>
#include "Math/Color.h"

namespace gx {

/// @brief エンジン初期化設定
struct Config
{
    std::string  title   = "GXLib Application";
    int          width   = 1280;
    int          height  = 720;
    bool         windowed = true;
    bool         vsync   = false;
    Color        background = { 0.0f, 0.0f, 0.0f, 1.0f };

    // フレーム制御
    float        targetFps    = 60.0f;
    float        maxDeltaTime = 0.1f;

    // 自動制御
    bool         autoClear  = true;
    bool         autoPresent = true;
    bool         allowEscapeExit = true;
};

} // namespace gx
