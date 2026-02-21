/// @file Samples/TextLayoutShowcase/main.cpp
/// @brief TextRenderer layout demo - Left/Center/Right alignment + DrawStringInRect.
///
/// Pure 2D demo (no Renderer3D). Shows 3 columns of aligned text and
/// a clipped rectangle with GetStringHeight readout.
///
/// Controls:
///   ESC - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"

class TextLayoutShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Text Layout";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 15; config.bgG = 18; config.bgB = 30;
        return config;
    }

    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        m_font = ctx.fontManager.CreateFont(L"Meiryo", 18);
        if (m_font < 0) m_font = ctx.fontManager.CreateFont(L"MS Gothic", 18);
        if (m_font < 0) m_font = ctx.defaultFontHandle;

        m_fontLarge = ctx.fontManager.CreateFont(L"Meiryo", 28);
        if (m_fontLarge < 0) m_fontLarge = m_font;
    }

    void Update(float dt) override
    {
        m_elapsed += dt;
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();

        float columnWidth = 350.0f;
        float startY = 80.0f;
        float rectY = 380;
        float rectW = 500, rectH = 120;

        // Phase 1: rectangles (PrimitiveBatch) first
        DrawBox(0, 0, 1280, 720, GetColor(15, 18, 30), TRUE);

        float cols[] = { 40, 460, 880 };
        for (float c : cols)
            DrawBox((int)c, (int)(startY + 18), (int)(c + columnWidth), (int)(startY + 19),
                    GetColor(60, 60, 80), TRUE);

        DrawBox(40, (int)(rectY + 18), (int)(40 + rectW), (int)(rectY + 18 + rectH),
                GetColor(30, 35, 50), TRUE);
        DrawBox(40, (int)(rectY + 18), (int)(40 + rectW), (int)(rectY + 19),
                GetColor(80, 80, 120), TRUE);
        DrawBox(40, (int)(rectY + 18 + rectH - 1), (int)(40 + rectW), (int)(rectY + 18 + rectH),
                GetColor(80, 80, 120), TRUE);

        // Phase 2: text (SpriteBatch)
        unsigned int headerCol = GetColor(200, 200, 220);
        unsigned int textCol = 0xFFDDDDDD;

        DrawString(40, 20, TEXT("TextRenderer Layout Demo"), GetColor(68, 204, 255));

        const std::wstring sampleText =
            L"GXLib is a DirectX 12 game engine.\n"
            L"\u30c6\u30ad\u30b9\u30c8\u30ec\u30a4\u30a2\u30a6\u30c8\u6a5f\u80fd\u306e\u30c7\u30e2\u3067\u3059\u3002\n"
            L"Word wrap and line breaking are handled automatically. "
            L"Long sentences will be split at word boundaries. "
            L"\u65e5\u672c\u8a9e\u306e\u6587\u306f\u6587\u5b57\u5358\u4f4d\u3067\u6298\u308a\u8fd4\u3055\u308c\u307e\u3059\u3002";

        GX::TextRenderer::TextLayoutOptions opts;
        opts.maxWidth = columnWidth;
        opts.lineSpacing = 1.3f;
        opts.wordWrap = true;

        ctx.EnsureSpriteBatch();

        // Left
        DrawString((int)cols[0], (int)(startY - 5), TEXT("[ Left Align ]"), headerCol);
        opts.align = GX::TextRenderer::TextAlign::Left;
        ctx.textRenderer.DrawStringLayout(m_font, cols[0], startY + 25, sampleText, textCol, opts);

        // Center
        DrawString((int)cols[1], (int)(startY - 5), TEXT("[ Center Align ]"), headerCol);
        opts.align = GX::TextRenderer::TextAlign::Center;
        ctx.textRenderer.DrawStringLayout(m_font, cols[1], startY + 25, sampleText, textCol, opts);

        // Right
        DrawString((int)cols[2], (int)(startY - 5), TEXT("[ Right Align ]"), headerCol);
        opts.align = GX::TextRenderer::TextAlign::Right;
        ctx.textRenderer.DrawStringLayout(m_font, cols[2], startY + 25, sampleText, textCol, opts);

        // DrawStringInRect demo
        DrawString(40, (int)(rectY - 5), TEXT("[ DrawStringInRect - Clipped to box ]"), headerCol);

        const std::wstring longText =
            L"This text is rendered inside a fixed-size rectangle. "
            L"If the text overflows the rectangle height, it gets clipped. "
            L"\u3053\u306e\u9577\u3044\u30c6\u30ad\u30b9\u30c8\u306f\u6307\u5b9a\u3055\u308c\u305f\u77e9\u5f62\u9818\u57df\u5185\u306b\u53ce\u307e\u308b\u3088\u3046\u306b\u30af\u30ea\u30c3\u30d4\u30f3\u30b0\u3055\u308c\u307e\u3059\u3002"
            L"\u77e9\u5f62\u306e\u5916\u306b\u306f\u307f\u51fa\u305f\u90e8\u5206\u306f\u8868\u793a\u3055\u308c\u307e\u305b\u3093\u3002"
            L"DrawStringInRect is useful for UI panels and dialog boxes.";

        opts.align = GX::TextRenderer::TextAlign::Left;
        opts.maxWidth = rectW - 16;
        ctx.textRenderer.DrawStringInRect(m_font, 48, rectY + 26, rectW - 16, rectH - 16,
                                           longText, textCol, opts);

        int fullHeight = ctx.textRenderer.GetStringHeight(m_font, longText, opts);
        DrawString(560, (int)(rectY + 60),
            FormatT(TEXT("Full text height: {}px"), fullHeight).c_str(),
            GetColor(180, 180, 200));
        DrawString(560, (int)(rectY + 85),
            FormatT(TEXT("Visible rect: {}px"), (int)rectH - 16).c_str(),
            GetColor(180, 180, 200));

        DrawString(40, 680, TEXT("2D only - no 3D rendering  ESC: Quit"), GetColor(100, 100, 120));
    }

private:
    int m_font = -1;
    int m_fontLarge = -1;
    float m_elapsed = 0;
};

GX_EASY_APP(TextLayoutShowcaseApp)
