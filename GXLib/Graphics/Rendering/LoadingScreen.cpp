/// @file LoadingScreen.cpp
/// @brief ローディング画面描画の実装

#include "pch_graphics.h"
#include "Graphics/Rendering/LoadingScreen.h"
#include <cmath>
#include <algorithm>

namespace gx {

// ---------------------------------------------------------------------------
// 定数
// ---------------------------------------------------------------------------
static constexpr float k_Pi       = 3.14159265358979323846f;
static constexpr float k_TwoPi    = k_Pi * 2.0f;
static constexpr int   k_DotCount = 6;                // 周回パーティクル数
static constexpr float k_HexRadius = 54.0f;           // ヘキサゴン半径
static constexpr float k_OrbitRadius = 82.0f;          // パーティクル軌道半径
static constexpr float k_ArcRadius = 100.0f;           // プログレス弧半径
static constexpr int   k_ArcSegments = 128;            // 弧のセグメント数（高精細）
static constexpr float k_DotSize = 2.5f;               // パーティクルの半辺長
static constexpr float k_LerpSpeed = 4.0f;             // プログレス補間速度 (1/s)

// ---------------------------------------------------------------------------
// カラーパレット（ミニマルモノクローム + 暖色アクセント）
// ---------------------------------------------------------------------------
// 背景:       #08080f (漆黒に近い深紺)
// 主線:       #d0d8e8 (冷たいシルバー)
// 主線(淡):   #607090 (グレーブルー)
// アクセント:  #e8a040 (アンバーゴールド)
// 弧背景:     #1a1e28 (ほぼ見えない暗灰)
// 弧前景:     #607090 → #f0f4ff (グレーブルー→白)
// テキスト:   #b0b8c8 (落ち着いたシルバー)
// パーセント: #606878 (控えめグレー)

// ---------------------------------------------------------------------------
// ユーティリティ
// ---------------------------------------------------------------------------

uint32_t LoadingScreen::ApplyAlpha(uint32_t baseColor, float alpha)
{
    uint32_t a = (baseColor >> 24) & 0xFF;
    a = static_cast<uint32_t>(a * alpha);
    if (a > 255) a = 255;
    return (a << 24) | (baseColor & 0x00FFFFFF);
}

// ---------------------------------------------------------------------------
// Initialize / Shutdown
// ---------------------------------------------------------------------------

bool LoadingScreen::Initialize(ID3D12Device* device, ID3D12CommandQueue* queue,
                               uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;
    m_displayProgress = 0.0f;

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

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void LoadingScreen::Render(ID3D12GraphicsCommandList* cmdList,
                           D3D12_CPU_DESCRIPTOR_HANDLE rtv,
                           ID3D12Resource* backBuffer,
                           uint32_t frameIndex,
                           float progress,
                           const wchar_t* status,
                           float alpha)
{
    m_timer.Tick();
    float dt = static_cast<float>(m_timer.GetDeltaTime());

    // --- プログレス値のスムーズ補間 ---
    // 目標値に向かって指数的に追従（急なジャンプを滑らかに吸収）
    if (progress >= 0.0f)
    {
        float target = std::clamp(progress, 0.0f, 1.0f);
        // exponential lerp: 大きな差は速く、小さな差はゆっくり追従
        float speed = k_LerpSpeed + 2.0f * std::abs(target - m_displayProgress);
        m_displayProgress += (target - m_displayProgress) * std::min(speed * dt, 1.0f);
        // 目標にほぼ到達したらスナップ（浮動小数の残差除去）
        if (std::abs(target - m_displayProgress) < 0.001f)
            m_displayProgress = target;
    }

    // バックバッファを RENDER_TARGET 状態にする
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = backBuffer;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    cmdList->ResourceBarrier(1, &barrier);

    // 背景クリア — 漆黒に近い深紺 #08080f
    const float clearColor[4] = {
        0.031f * alpha, 0.031f * alpha, 0.059f * alpha, 1.0f
    };
    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    // ビューポート・シザー設定
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f,
        static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0,
        static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    // --- 幾何学アニメーション (PrimitiveBatch) ---
    float cx = m_width * 0.5f;
    float cy = m_height * 0.43f;
    float totalTime = static_cast<float>(m_timer.GetTotalTime());

    m_primitiveBatch.Begin(cmdList, frameIndex);

    // 1) 回転ヘキサゴン
    DrawHexagon(cx, cy, k_HexRadius, totalTime, alpha);

    // 2) 周回パーティクル
    DrawOrbitingDots(cx, cy, k_OrbitRadius, totalTime, alpha);

    // 3) プログレス弧（補間済みの値を使用）
    if (progress >= 0.0f)
        DrawProgressArc(cx, cy, k_ArcRadius, m_displayProgress, totalTime, alpha);

    m_primitiveBatch.End();

    // --- テキスト描画 (SpriteBatch + TextRenderer) ---
    if (m_font >= 0)
    {
        m_spriteBatch.Begin(cmdList, frameIndex);

        // アニメーションドット: 0.4秒間隔で . → .. → ... → (なし)
        int dotCount = static_cast<int>(totalTime / 0.4) % 4;

        gx::WString text(status);
        for (int i = 0; i < dotCount; ++i)
            text += L'.';

        // テキスト幅を計算して中央揃え
        int textWidth = m_textRenderer.GetStringWidth(m_font, text);
        float textX = (m_width - textWidth) * 0.5f;
        float textY = cy + k_ArcRadius + 44.0f;

        // 落ち着いたシルバー #b0b8c8
        uint32_t textColor = ApplyAlpha(0xFFb0b8c8, alpha);
        m_textRenderer.DrawString(m_font, textX, textY, text, textColor);

        // 進捗パーセンテージ表示（補間値を使用）
        if (progress >= 0.0f)
        {
            int pct = static_cast<int>(m_displayProgress * 100.0f);
            if (pct > 100) pct = 100;
            gx::WString pctText = std::to_wstring(pct) + L"%";
            int pctWidth = m_textRenderer.GetStringWidth(m_font, pctText);
            float pctX = (m_width - pctWidth) * 0.5f;
            float pctY = textY + 36.0f;
            // 控えめグレー #606878
            uint32_t pctColor = ApplyAlpha(0xFF606878, alpha);
            m_textRenderer.DrawString(m_font, pctX, pctY, pctText, pctColor);
        }

        m_spriteBatch.End();
        m_fontManager.FlushAtlasUpdates();
    }

    // バックバッファを PRESENT 状態に戻す
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    cmdList->ResourceBarrier(1, &barrier);
}

// ---------------------------------------------------------------------------
// DrawHexagon — 回転するヘキサゴン（シルバー + ブリーズアニメーション）
// ---------------------------------------------------------------------------

void LoadingScreen::DrawHexagon(float cx, float cy, float radius, float angle, float alpha)
{
    // 外側ヘキサゴン — ゆっくり回転 (0.35 rad/s), 呼吸するように半径が脈動
    float rot = angle * 0.35f;
    float breathe = 1.0f + 0.03f * std::sin(angle * 1.5f);
    float outerR = radius * breathe;

    // シルバー #d0d8e8、不透明度は呼吸で 70%〜100%
    float outerAlpha = 0.70f + 0.30f * (0.5f + 0.5f * std::sin(angle * 1.5f));
    uint32_t outerColor = ApplyAlpha(0xFFd0d8e8, alpha * outerAlpha);

    for (int i = 0; i < 6; ++i)
    {
        float a0 = rot + (k_TwoPi / 6.0f) * i;
        float a1 = rot + (k_TwoPi / 6.0f) * (i + 1);
        m_primitiveBatch.DrawLine(
            cx + outerR * std::cos(a0), cy + outerR * std::sin(a0),
            cx + outerR * std::cos(a1), cy + outerR * std::sin(a1),
            outerColor);
    }

    // 内側ヘキサゴン — 逆回転, グレーブルー #607090, 控えめ
    float innerR = radius * 0.6f;
    float innerRot = -angle * 0.2f;
    uint32_t innerColor = ApplyAlpha(0x50607090, alpha);

    for (int i = 0; i < 6; ++i)
    {
        float a0 = innerRot + (k_TwoPi / 6.0f) * i;
        float a1 = innerRot + (k_TwoPi / 6.0f) * (i + 1);
        m_primitiveBatch.DrawLine(
            cx + innerR * std::cos(a0), cy + innerR * std::sin(a0),
            cx + innerR * std::cos(a1), cy + innerR * std::sin(a1),
            innerColor);
    }
}

// ---------------------------------------------------------------------------
// DrawOrbitingDots — 軌道を回るパーティクル（アンバーアクセント）
// ---------------------------------------------------------------------------

void LoadingScreen::DrawOrbitingDots(float cx, float cy, float orbitRadius, float time, float alpha)
{
    float baseSpeed = 0.8f;

    for (int i = 0; i < k_DotCount; ++i)
    {
        float phase = (k_TwoPi / k_DotCount) * i;
        float angle = time * baseSpeed + phase;

        float dx = cx + orbitRadius * std::cos(angle);
        float dy = cy + orbitRadius * std::sin(angle);

        // 不透明度を正弦波で変化 — 星のように明滅
        float opacity = 0.25f + 0.75f * (0.5f + 0.5f * std::sin(time * 1.8f + phase * 2.0f));

        // アンバーゴールド #e8a040 ↔ シルバー #d0d0e0
        float warm = 0.5f + 0.5f * std::sin(time * 0.7f + phase);
        uint32_t r = static_cast<uint32_t>(0xd0 + warm * (0xe8 - 0xd0));
        uint32_t g = static_cast<uint32_t>(0xb0 + warm * (0xa0 - 0xb0));
        uint32_t b = static_cast<uint32_t>(0xd0 + warm * (0x40 - 0xd0));
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;

        uint32_t dotColor = ApplyAlpha((0xFF000000 | (r << 16) | (g << 8) | b), alpha * opacity);

        float sz = k_DotSize * (0.7f + 0.3f * opacity);
        m_primitiveBatch.DrawBox(dx - sz, dy - sz, dx + sz, dy + sz, dotColor, true);
    }
}

// ---------------------------------------------------------------------------
// DrawProgressArc — プログレス弧（float精度で描画、端数セグメント対応）
// ---------------------------------------------------------------------------

void LoadingScreen::DrawProgressArc(float cx, float cy, float radius, float progress, float time, float alpha)
{
    if (progress < 0.0f) return;
    progress = std::clamp(progress, 0.0f, 1.0f);

    float startAngle = -k_Pi * 0.5f; // 12時方向から
    float segStep = k_TwoPi / static_cast<float>(k_ArcSegments);

    // 背景弧 — ほぼ見えない暗灰 #1a1e28
    uint32_t bgColor = ApplyAlpha(0x301a1e28, alpha);
    for (int i = 0; i < k_ArcSegments; ++i)
    {
        float a0 = startAngle + segStep * i;
        float a1 = startAngle + segStep * (i + 1);
        m_primitiveBatch.DrawLine(
            cx + radius * std::cos(a0), cy + radius * std::sin(a0),
            cx + radius * std::cos(a1), cy + radius * std::sin(a1),
            bgColor);
    }

    // 前景弧 — float 精度で fillSegments を計算し、端数セグメントも描画
    float exactSegments = k_ArcSegments * progress;
    int fullSegments = static_cast<int>(exactSegments);
    float fractional = exactSegments - fullSegments; // 最後のセグメントの端数 (0〜1)

    auto calcArcColor = [&](float t, float segAlpha) -> uint32_t {
        // グレーブルー #607090 → 白 #f0f4ff（先端に向かって指数的に明るく）
        float t3 = t * t * t; // 先端側に強い重み
        uint32_t r = static_cast<uint32_t>(0x60 + t3 * (0xf0 - 0x60));
        uint32_t g = static_cast<uint32_t>(0x70 + t3 * (0xf4 - 0x70));
        uint32_t b = static_cast<uint32_t>(0x90 + t3 * (0xff - 0x90));
        r = std::min(r, 255u);
        g = std::min(g, 255u);
        b = std::min(b, 255u);
        return ApplyAlpha((0xFF000000 | (r << 16) | (g << 8) | b), alpha * segAlpha);
    };

    int totalFill = fullSegments + (fractional > 0.001f ? 1 : 0);
    for (int i = 0; i < totalFill && i < k_ArcSegments; ++i)
    {
        // このセグメントの「弧内での位置」(0→1)
        float t = (exactSegments > 1.0f)
            ? static_cast<float>(i) / (exactSegments - 1.0f)
            : 1.0f;
        t = std::clamp(t, 0.0f, 1.0f);

        float a0 = startAngle + segStep * i;
        float a1;

        // 最後の端数セグメント: 角度を端数分だけ描画
        bool isFractionalSeg = (i == fullSegments && fractional > 0.001f);
        if (isFractionalSeg)
            a1 = a0 + segStep * fractional;
        else
            a1 = startAngle + segStep * (i + 1);

        // 先端付近（最後の5セグメント）をグロー
        float distFromTip = exactSegments - static_cast<float>(i);
        float segAlpha = (distFromTip < 5.0f)
            ? 0.7f + 0.3f * (1.0f - distFromTip / 5.0f)
            : 0.45f + 0.55f * t;

        uint32_t arcColor = calcArcColor(t, segAlpha);

        m_primitiveBatch.DrawLine(
            cx + radius * std::cos(a0), cy + radius * std::sin(a0),
            cx + radius * std::cos(a1), cy + radius * std::sin(a1),
            arcColor);
    }
}

} // namespace gx
