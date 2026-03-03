/// @file LoadingScreen.cpp
/// @brief ローディング画面描画の実装

#include "pch_graphics.h"
#include "Graphics/Rendering/LoadingScreen.h"

namespace gx {

bool LoadingScreen::Initialize(ID3D12Device* device, ID3D12CommandQueue* queue,
                               uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    if (!m_spriteBatch.Initialize(device, queue, width, height))
        return false;
    if (!m_primitiveBatch.Initialize(device, width, height))
        return false;
    if (!m_fontManager.Initialize(device, &m_spriteBatch.GetTextureManager()))
        return false;
    m_textRenderer.Initialize(&m_spriteBatch, &m_fontManager);

    // フォント作成
    m_font = m_fontManager.CreateFont(L"Meiryo", 28);
    if (m_font < 0)
        m_font = m_fontManager.CreateFont(L"MS Gothic", 28);

    m_timer.Reset();
    return true;
}

void LoadingScreen::Shutdown()
{
    m_fontManager.Shutdown();
}

void LoadingScreen::Render(ID3D12GraphicsCommandList* cmdList,
                           D3D12_CPU_DESCRIPTOR_HANDLE rtv,
                           ID3D12Resource* backBuffer,
                           uint32_t frameIndex,
                           float progress,
                           const wchar_t* status)
{
    m_timer.Tick();

    // バックバッファを RENDER_TARGET 状態にする
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = backBuffer;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    cmdList->ResourceBarrier(1, &barrier);

    // 背景クリア (暗い紺色)
    const float clearColor[4] = { 0.102f, 0.102f, 0.180f, 1.0f }; // #1a1a2e
    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    // ビューポート・シザー設定
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f,
        static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0,
        static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    // --- プログレスバー描画 (PrimitiveBatch) ---
    if (progress >= 0.0f)
    {
        m_primitiveBatch.Begin(cmdList, frameIndex);

        float barWidth  = m_width * 0.5f;
        float barHeight = 16.0f;
        float barX = (m_width - barWidth) * 0.5f;
        float barY = m_height * 0.55f;

        // バー背景 (暗灰色)
        m_primitiveBatch.DrawBox(barX, barY, barX + barWidth, barY + barHeight,
                                 0xFF404050, true);

        // バー前景 (明るい青)
        float fillWidth = barWidth * (progress > 1.0f ? 1.0f : progress);
        if (fillWidth > 0.0f)
        {
            m_primitiveBatch.DrawBox(barX, barY, barX + fillWidth, barY + barHeight,
                                     0xFF6090E0, true);
        }

        // バー枠 (白)
        m_primitiveBatch.DrawBox(barX, barY, barX + barWidth, barY + barHeight,
                                 0xFF808090, false);

        m_primitiveBatch.End();
    }

    // --- テキスト描画 (SpriteBatch + TextRenderer) ---
    if (m_font >= 0)
    {
        m_spriteBatch.Begin(cmdList, frameIndex);

        // アニメーションドット: 0.3秒間隔で . → .. → ... → (なし) 循環
        double totalTime = m_timer.GetTotalTime();
        int dotCount = static_cast<int>(totalTime / 0.3) % 4;

        gx::WString text(status);
        for (int i = 0; i < dotCount; ++i)
            text += L'.';

        // テキスト幅を計算して中央揃え
        int textWidth = m_textRenderer.GetStringWidth(m_font, text);
        float textX = (m_width - textWidth) * 0.5f;
        float textY = m_height * 0.45f;

        m_textRenderer.DrawString(m_font, textX, textY, text, 0xFFE0E0E0);

        // 進捗パーセンテージ表示
        if (progress >= 0.0f)
        {
            int pct = static_cast<int>(progress * 100.0f);
            if (pct > 100) pct = 100;
            gx::WString pctText = std::to_wstring(pct) + L"%";
            int pctWidth = m_textRenderer.GetStringWidth(m_font, pctText);
            float pctX = (m_width - pctWidth) * 0.5f;
            float pctY = m_height * 0.55f + 24.0f;
            m_textRenderer.DrawString(m_font, pctX, pctY, pctText, 0xFFA0A0B0);
        }

        m_spriteBatch.End();
        m_fontManager.FlushAtlasUpdates();
    }

    // バックバッファを PRESENT 状態に戻す
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    cmdList->ResourceBarrier(1, &barrier);
}

} // namespace gx
