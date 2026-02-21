#include "pch.h"
/// @file ProfilerOverlay.cpp
/// @brief ゲーム内プロファイラオーバーレイ実装

#include "Core/ProfilerOverlay.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include "Graphics/Rendering/PrimitiveBatch.h"
#include "Graphics/Rendering/TextRenderer.h"
#include "Graphics/Device/GPUProfiler.h"

namespace GX
{

void ProfilerOverlay::Initialize(SpriteBatch* spriteBatch, PrimitiveBatch* primitiveBatch,
                                  TextRenderer* textRenderer, int fontHandle)
{
    m_spriteBatch = spriteBatch;
    m_primitiveBatch = primitiveBatch;
    m_textRenderer = textRenderer;
    m_fontHandle = fontHandle;
}

void ProfilerOverlay::CycleMode()
{
    int m = static_cast<int>(m_mode);
    m = (m + 1) % 3;
    m_mode = static_cast<Mode>(m);
}

void ProfilerOverlay::Draw(const GPUProfiler& gpuProfiler, float deltaTime)
{
    if (!m_visible || !m_textRenderer || m_fontHandle < 0) return;

    // FPS履歴更新
    float fps = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;
    m_fpsHistory[m_historyIndex] = fps;
    m_frameTimeHistory[m_historyIndex] = deltaTime * 1000.0f; // ms
    m_historyIndex = (m_historyIndex + 1) % k_HistorySize;

    switch (m_mode)
    {
    case Mode::Minimal:
        DrawMinimal(deltaTime);
        break;
    case Mode::Detailed:
        DrawDetailed(gpuProfiler, deltaTime);
        break;
    case Mode::Graph:
        DrawDetailed(gpuProfiler, deltaTime);
        DrawGraph(deltaTime);
        break;
    }
}

void ProfilerOverlay::DrawMinimal(float deltaTime)
{
    float fps = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;
    float frameMs = deltaTime * 1000.0f;

    wchar_t buf[128];
    swprintf_s(buf, L"FPS: %.1f  Frame: %.2fms", fps, frameMs);

    // 背景半透明ボックス
    if (m_primitiveBatch)
    {
        m_primitiveBatch->DrawBox(5.0f, 5.0f, 280.0f, 28.0f, 0x80000000, true);
    }

    m_textRenderer->DrawString(m_fontHandle, 10.0f, 8.0f, buf, 0xFF00FF00);
}

void ProfilerOverlay::DrawDetailed(const GPUProfiler& gpuProfiler, float deltaTime)
{
    float fps = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;
    float frameMs = deltaTime * 1000.0f;
    float gpuMs = gpuProfiler.GetFrameGPUTimeMs();

    float y = 8.0f;

    // 背景
    float bgHeight = 30.0f + gpuProfiler.GetResults().size() * 18.0f + 10.0f;
    if (m_primitiveBatch)
    {
        m_primitiveBatch->DrawBox(5.0f, 5.0f, 320.0f, 5.0f + bgHeight, 0xC0000000, true);
    }

    // FPS / Frame time
    {
        wchar_t buf[128];
        swprintf_s(buf, L"FPS: %.1f  CPU: %.2fms  GPU: %.2fms", fps, frameMs, gpuMs);
        m_textRenderer->DrawString(m_fontHandle, 10.0f, y, buf, 0xFF00FF00);
        y += 20.0f;
    }

    // GPU パス別詳細
    const auto& results = gpuProfiler.GetResults();
    for (const auto& r : results)
    {
        wchar_t buf[128];
        // const char* → wchar_t 変換
        wchar_t nameBuf[64];
        size_t converted = 0;
        mbstowcs_s(&converted, nameBuf, r.name, _TRUNCATE);
        swprintf_s(buf, L"  %s: %.3fms", nameBuf, r.durationMs);

        uint32_t color = 0xFFCCCCCC;
        if (r.durationMs > 5.0f) color = 0xFFFF4444; // 赤: 重い
        else if (r.durationMs > 2.0f) color = 0xFFFFAA00; // オレンジ: 注意

        m_textRenderer->DrawString(m_fontHandle, 10.0f, y, buf, color);
        y += 18.0f;
    }
}

void ProfilerOverlay::DrawGraph(float deltaTime)
{
    if (!m_primitiveBatch) return;

    // グラフ領域
    float graphX = 330.0f;
    float graphY = 10.0f;
    float graphW = 250.0f;
    float graphH = 80.0f;

    // 背景
    m_primitiveBatch->DrawBox(graphX - 2, graphY - 2, graphX + graphW + 2, graphY + graphH + 2,
                               0xC0000000, true);

    // 60FPSライン
    float fps60Y = graphY + graphH - (60.0f / 120.0f) * graphH;
    m_primitiveBatch->DrawLine(graphX, fps60Y, graphX + graphW, fps60Y, 0x4000FF00);

    // 30FPSライン
    float fps30Y = graphY + graphH - (30.0f / 120.0f) * graphH;
    m_primitiveBatch->DrawLine(graphX, fps30Y, graphX + graphW, fps30Y, 0x40FFFF00);

    // FPS折れ線グラフ
    float barWidth = graphW / static_cast<float>(k_HistorySize);
    for (int i = 0; i < k_HistorySize - 1; ++i)
    {
        int idx0 = (m_historyIndex + i) % k_HistorySize;
        int idx1 = (m_historyIndex + i + 1) % k_HistorySize;

        float fps0 = m_fpsHistory[idx0];
        float fps1 = m_fpsHistory[idx1];

        // 0-120 FPS範囲にクランプ
        float t0 = (std::min)(fps0 / 120.0f, 1.0f);
        float t1 = (std::min)(fps1 / 120.0f, 1.0f);

        float x0 = graphX + i * barWidth;
        float y0 = graphY + graphH - t0 * graphH;
        float x1 = graphX + (i + 1) * barWidth;
        float y1 = graphY + graphH - t1 * graphH;

        uint32_t color = 0xFF00FF00;
        if (fps0 < 30.0f) color = 0xFFFF0000;
        else if (fps0 < 60.0f) color = 0xFFFFFF00;

        m_primitiveBatch->DrawLine(x0, y0, x1, y1, color);
    }

    // ラベル
    if (m_textRenderer)
    {
        m_textRenderer->DrawString(m_fontHandle, graphX, graphY + graphH + 4.0f, L"FPS Graph", 0xFF888888);
    }
}

} // namespace GX
