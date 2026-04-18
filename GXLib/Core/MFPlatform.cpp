/// @file Core/MFPlatform.cpp
/// @brief MFPlatform の実装 — MF 初期化の参照カウントラッパー (ADR-0020 E4)

#include "pch_common.h"
#include "Core/MFPlatform.h"
#include "Core/Logger.h"

#include <windows.h>
#include <mfapi.h>
#include <mutex>

#pragma comment(lib, "mfplat.lib")

namespace gx
{

namespace
{
// プロセス全体で 1 つの MF refcount。
// 0 → 1 の遷移で MFStartup、1 → 0 の遷移で MFShutdown を呼ぶ。
std::mutex g_mfMutex;
int        g_mfRefCount = 0;
} // anonymous namespace

bool MFPlatform::Acquire()
{
    std::lock_guard<std::mutex> lock(g_mfMutex);

    if (g_mfRefCount == 0)
    {
        HRESULT hr = MFStartup(MF_VERSION);
        if (FAILED(hr))
        {
            GX_LOG_ERROR("MFPlatform::Acquire - MFStartup failed: 0x%08lX",
                         static_cast<unsigned long>(hr));
            return false; // refcount 不変
        }
    }

    ++g_mfRefCount;
    return true;
}

void MFPlatform::Release()
{
    std::lock_guard<std::mutex> lock(g_mfMutex);

    if (g_mfRefCount <= 0)
    {
        GX_LOG_ERROR("MFPlatform::Release - unbalanced Release (refcount=%d)",
                     g_mfRefCount);
        return;
    }

    --g_mfRefCount;

    if (g_mfRefCount == 0)
    {
        HRESULT hr = MFShutdown();
        if (FAILED(hr))
        {
            GX_LOG_ERROR("MFPlatform::Release - MFShutdown failed: 0x%08lX",
                         static_cast<unsigned long>(hr));
            // refcount は既に 0 なので復帰不能 — 後続 Acquire は再度 MFStartup を試みる
        }
    }
}

bool MFPlatform::IsInitialized()
{
    std::lock_guard<std::mutex> lock(g_mfMutex);
    return g_mfRefCount > 0;
}

int MFPlatform::GetRefCount()
{
    std::lock_guard<std::mutex> lock(g_mfMutex);
    return g_mfRefCount;
}

} // namespace gx
