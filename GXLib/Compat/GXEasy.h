#pragma once
// GXEasy - 初学者向けアプリラッパー（ACS風）。
// 必要最小限の関数だけで動かせるようにする。

#include "Compat/GXLib.h"
#include <string>
#include <format>

// --- TCHAR 型定義 + FormatT（全サンプル共通） ---
#ifdef UNICODE
using TChar = wchar_t;
#else
using TChar = char;
#endif
using TString = std::basic_string<TChar>;

/// @brief UNICODE / ANSI 両対応の std::format ラッパー
/// @note Args は by-value 必須（P2905R2/MSVC14.44 で make_format_args が lvalue 要求）
template <class... Args>
TString FormatT(const TChar* fmt, Args... args)
{
#ifdef UNICODE
    return std::vformat(fmt, std::make_wformat_args(args...));
#else
    return std::vformat(fmt, std::make_format_args(args...));
#endif
}

namespace GXEasy
{

struct AppConfig
{
    std::wstring title = L"GXLib Easy App";
    int width = 1280;
    int height = 720;
    int colorBitNum = 32;
    bool windowed = true;
    bool autoClear = true;
    bool autoPresent = true;
    bool allowEscapeExit = true;
    bool vsync = false;          // 垂直同期を有効にするか
    int targetFps = 0;           // 0=無制限、>0でFPS上限
    int bgR = 0;
    int bgG = 0;
    int bgB = 0;
    float maxDeltaTime = 0.1f;
};

class App
{
public:
    virtual ~App() = default;

    // 既定のウィンドウ設定を変えたいときにオーバーライドする。
    virtual AppConfig GetConfig() const { return {}; }

    // GX_Init() の前に呼ばれる。
    virtual void OnBoot() {}

    // GX_Init() の直後に1回だけ呼ばれる。
    virtual void Start() {}

    // 毎フレーム呼ばれる（更新処理を書く場所）。
    virtual void Update(float dt) { (void)dt; }

    // Update()の後に毎フレーム呼ばれる（描画向け）。
    virtual void Draw() {}

    // GX_End() の前に1回だけ呼ばれる（後片付け用）。
    virtual void Release() {}
};

int Run(App& app, const AppConfig& config);

} // namespace GXEasy

// エントリーポイント用ヘルパー（ACSと同じ書き味）
#define GX_EASY_APP(AppClass) \
int WINAPI WinMain( \
    _In_ HINSTANCE hInstance, \
    _In_ HINSTANCE hPrevInstance, \
    _In_ LPSTR lpCmdLine, \
    _In_ int nCmdShow) \
{ \
    AppClass app; \
    return GXEasy::Run(app, app.GetConfig()); \
}
