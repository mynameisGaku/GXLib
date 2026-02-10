/// @file FontManager.cpp
/// @brief フォントマネージャーの実装（DirectWriteのラスタライズ + テクスチャアトラス）
#include "pch.h"
#include "Graphics/Rendering/FontManager.h"
#include "Core/Logger.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>

namespace GX
{

bool FontManager::Initialize(ID3D12Device* device, TextureManager* textureManager)
{
    m_device = device;
    m_textureManager = textureManager;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (hr == S_OK)
    {
        m_comInitialized = true;
    }
    else if (hr == S_FALSE || hr == RPC_E_CHANGED_MODE)
    {
        m_comInitialized = false;
        if (hr == RPC_E_CHANGED_MODE)
        {
            GX_LOG_WARN("CoInitializeEx returned RPC_E_CHANGED_MODE (COM already initialized with different model).");
        }
    }
    else
    {
        GX_LOG_ERROR("CoInitializeEx failed: 0x%08X", static_cast<unsigned>(hr));
        return false;
    }

    // DirectWriteファクトリ作成
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));

    if (FAILED(hr))
    {
        GX_LOG_ERROR("DWriteCreateFactory failed: 0x%08X", static_cast<unsigned>(hr));
        return false;
    }

    // Direct2Dファクトリ作成（ラスタライズに使用）
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());
    if (FAILED(hr))
    {
        GX_LOG_ERROR("D2D1CreateFactory failed: 0x%08X", static_cast<unsigned>(hr));
        return false;
    }

    // WICファクトリ作成（ビットマップ変換に使用）
    hr = CoCreateInstance(
        CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        __uuidof(IWICImagingFactory),
        reinterpret_cast<void**>(m_wicFactory.GetAddressOf()));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("WICImagingFactory creation failed: 0x%08X", static_cast<unsigned>(hr));
        return false;
    }

    m_entries.reserve(k_MaxFonts);
    GX_LOG_INFO("FontManager initialized (atlas: %ux%u)", k_AtlasSize, k_AtlasSize);
    return true;
}

int FontManager::AllocateHandle()
{
    if (!m_freeHandles.empty())
    {
        int handle = m_freeHandles.back();
        m_freeHandles.pop_back();
        return handle;
    }

    int handle = m_nextHandle++;
    if (handle >= static_cast<int>(m_entries.size()))
    {
        m_entries.resize(handle + 1);
    }
    return handle;
}

int FontManager::CreateFont(const std::wstring& fontName, int fontSize, bool bold, bool italic)
{
    int handle = AllocateHandle();
    auto& entry = m_entries[handle];

    entry.fontSize = fontSize;
    entry.lineHeight = fontSize;
    entry.baseline = static_cast<float>(fontSize);
    entry.cursorX = 1;
    entry.cursorY = 1;
    entry.rowHeight = 0;
    entry.atlasDirty = false;

    // アトラス用ピクセルバッファ初期化（ARGB）
    entry.atlasPixels.resize(k_AtlasSize * k_AtlasSize, 0x00000000);

    // DirectWrite TextFormat菴懈・
    HRESULT hr = m_dwriteFactory->CreateTextFormat(
        fontName.c_str(),
        nullptr,
        bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
        italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        static_cast<float>(fontSize),
        L"ja-jp",
        &entry.textFormat);

    if (FAILED(hr))
    {
        GX_LOG_ERROR("CreateTextFormat failed: 0x%08X", static_cast<unsigned>(hr));
        return -1;
    }

    // フォントのメトリクスから行間とベースラインを計算（余白を最小化）
    {
        ComPtr<IDWriteFontCollection> fontCollection;
        if (SUCCEEDED(m_dwriteFactory->GetSystemFontCollection(&fontCollection)))
        {
            UINT32 index = 0;
            BOOL exists = FALSE;
            if (SUCCEEDED(fontCollection->FindFamilyName(fontName.c_str(), &index, &exists)) && exists)
            {
                ComPtr<IDWriteFontFamily> family;
                if (SUCCEEDED(fontCollection->GetFontFamily(index, &family)))
                {
                    ComPtr<IDWriteFont> font;
                    if (SUCCEEDED(family->GetFirstMatchingFont(
                        bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                        italic ? DWRITE_FONT_STRETCH_NORMAL : DWRITE_FONT_STRETCH_NORMAL,
                        italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
                        &font)))
                    {
                        ComPtr<IDWriteFontFace> face;
                        if (SUCCEEDED(font->CreateFontFace(&face)))
                        {
                            DWRITE_FONT_METRICS fm = {};
                            face->GetMetrics(&fm);
                            float scale = static_cast<float>(fontSize) / static_cast<float>(fm.designUnitsPerEm);
                            float ascent = fm.ascent * scale;
                            float descent = fm.descent * scale;
                            // lineGapは使わず、見た目の余白を抑える
                            float lineHeight = ascent + descent;
                            if (lineHeight < 1.0f) lineHeight = static_cast<float>(fontSize);

                            entry.lineHeight = static_cast<int>(ceilf(lineHeight));
                            entry.baseline = ascent;
                            entry.textFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM,
                                                             lineHeight, entry.baseline);

                            // キャップハイト差分で上側の空白を抑える
                            float capHeight = 0.0f;
                            ComPtr<IDWriteFontFace1> face1;
                            if (SUCCEEDED(face.As(&face1)))
                            {
                                DWRITE_FONT_METRICS1 fm1 = {};
                                face1->GetMetrics(&fm1);
                                capHeight = fm1.capHeight * scale;
                            }
                            if (capHeight <= 0.0f)
                                capHeight = ascent * 0.8f;
                            float capOffset = ascent - capHeight;
                            if (capOffset < 0.0f) capOffset = 0.0f;
                            float maxOffset = lineHeight * 0.25f;
                            if (capOffset > maxOffset) capOffset = maxOffset;
                            entry.capOffset = capOffset;
                        }
                    }
                }
            }
        }
    }

    // 基本文字セットを事前にラスタライズ（ASCII + ひらがな + カタカナ + CJK記号 + 全角記号）
    RasterizeBasicChars(entry);

    // アトラスをGPUへアップロード
    UploadAtlas(entry);

    entry.valid = true;
    GX_LOG_INFO("Font created: size=%d (handle: %d, glyphs: %zu)",
                fontSize, handle, entry.glyphs.size());
    return handle;
}

void FontManager::RasterizeBasicChars(FontEntry& entry)
{
    // ASCII基本文字（32-126）
    for (wchar_t ch = 32; ch <= 126; ++ch)
        RasterizeGlyph(entry, ch);

    // ひらがな（U+3040-U+309F）
    for (wchar_t ch = 0x3040; ch <= 0x309F; ++ch)
        RasterizeGlyph(entry, ch);

    // カタカナ（U+30A0-U+30FF）
    for (wchar_t ch = 0x30A0; ch <= 0x30FF; ++ch)
        RasterizeGlyph(entry, ch);

    // CJK記号（U+3000-U+303F）
    for (wchar_t ch = 0x3000; ch <= 0x303F; ++ch)
        RasterizeGlyph(entry, ch);

    // 全角記号（U+FF01-U+FF60）
    for (wchar_t ch = 0xFF01; ch <= 0xFF60; ++ch)
        RasterizeGlyph(entry, ch);
}

bool FontManager::RasterizeGlyph(FontEntry& entry, wchar_t ch)
{
    // 既にラスタライズ済みならスキップ
    if (entry.glyphs.contains(ch))
        return true;

    // スペースは幅だけ確保
    if (ch == L' ')
    {
        GlyphInfo info = {};
        info.width = entry.fontSize / 3;
        info.height = entry.fontSize;
        info.advance = static_cast<float>(info.width);
        info.u0 = info.v0 = info.u1 = info.v1 = 0.0f;
        entry.glyphs[ch] = info;
        return true;
    }

    // 全角スペース（U+3000）
    if (ch == L'\u3000')
    {
        GlyphInfo info = {};
        info.width = entry.fontSize;
        info.height = entry.fontSize;
        info.advance = static_cast<float>(entry.fontSize);
        info.u0 = info.v0 = info.u1 = info.v1 = 0.0f;
        entry.glyphs[ch] = info;
        return true;
    }

    // 1文字分のテキストレイアウトを作成
    ComPtr<IDWriteTextLayout> textLayout;
    wchar_t str[2] = { ch, L'\0' };
    HRESULT hr = m_dwriteFactory->CreateTextLayout(
        str, 1,
        entry.textFormat.Get(),
        static_cast<float>(k_AtlasSize),
        static_cast<float>(k_AtlasSize),
        &textLayout);

    if (FAILED(hr))
        return false;

    // テキストメトリクス取得
    DWRITE_TEXT_METRICS metrics = {};
    textLayout->GetMetrics(&metrics);

    int glyphW = static_cast<int>(ceilf(metrics.widthIncludingTrailingWhitespace)) + 2;
    int glyphH = static_cast<int>(ceilf(metrics.height)) + 2;

    if (glyphW <= 0) glyphW = 1;
    if (glyphH <= 0) glyphH = 1;

    // WICビットマップ作成（ピクセル読み出し用）
    ComPtr<IWICBitmap> wicBitmap;
    hr = m_wicFactory->CreateBitmap(
        static_cast<UINT>(glyphW), static_cast<UINT>(glyphH),
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapCacheOnLoad,
        &wicBitmap);
    if (FAILED(hr))
        return false;

    // D2D RenderTarget作成（WICBitmapに描画するため）
    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_SOFTWARE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    ComPtr<ID2D1RenderTarget> d2dRT;
    hr = m_d2dFactory->CreateWicBitmapRenderTarget(wicBitmap.Get(), rtProps, &d2dRT);
    if (FAILED(hr))
        return false;

    // 白色ブラシ作成
    ComPtr<ID2D1SolidColorBrush> brush;
    hr = d2dRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &brush);
    if (FAILED(hr))
        return false;

    // 描画
    d2dRT->BeginDraw();
    d2dRT->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    d2dRT->DrawTextLayout(D2D1::Point2F(0.0f, 0.0f), textLayout.Get(), brush.Get());
    d2dRT->EndDraw();

    // ピクセルデータを読み出す
    ComPtr<IWICBitmapLock> lock;
    WICRect lockRect = { 0, 0, glyphW, glyphH };
    hr = wicBitmap->Lock(&lockRect, WICBitmapLockRead, &lock);
    if (FAILED(hr))
        return false;

    UINT bufferSize = 0;
    BYTE* pixelData = nullptr;
    lock->GetDataPointer(&bufferSize, &pixelData);

    UINT stride = 0;
    lock->GetStride(&stride);

    // アトラスに書き込む位置を決定（必要なら改行）
    if (entry.cursorX + glyphW + 1 > static_cast<int>(k_AtlasSize))
    {
        entry.cursorX = 1;
        entry.cursorY += entry.rowHeight + 1;
        entry.rowHeight = 0;
    }

    if (entry.cursorY + glyphH + 1 > static_cast<int>(k_AtlasSize))
    {
        GX_LOG_WARN("Font atlas full, cannot add glyph for char %d (0x%04X)",
                    static_cast<int>(ch), static_cast<int>(ch));
        return false;
    }

    // アトラスへコピー（BGRA → RGBA の順に並べ替え）
    for (int y = 0; y < glyphH; ++y)
    {
        for (int x = 0; x < glyphW; ++x)
        {
            int srcIdx = y * static_cast<int>(stride) + x * 4;
            BYTE b = pixelData[srcIdx + 0];
            BYTE g = pixelData[srcIdx + 1];
            BYTE r = pixelData[srcIdx + 2];
            BYTE a = pixelData[srcIdx + 3];

            int atlasX = entry.cursorX + x;
            int atlasY = entry.cursorY + y;
            uint32_t pixel = (static_cast<uint32_t>(a) << 24) |
                             (static_cast<uint32_t>(b) << 16) |
                             (static_cast<uint32_t>(g) << 8)  |
                             static_cast<uint32_t>(r);
            entry.atlasPixels[atlasY * k_AtlasSize + atlasX] = pixel;
        }
    }

    // グリフ情報を登録
    GlyphInfo info = {};
    info.u0 = static_cast<float>(entry.cursorX) / static_cast<float>(k_AtlasSize);
    info.v0 = static_cast<float>(entry.cursorY) / static_cast<float>(k_AtlasSize);
    info.u1 = static_cast<float>(entry.cursorX + glyphW) / static_cast<float>(k_AtlasSize);
    info.v1 = static_cast<float>(entry.cursorY + glyphH) / static_cast<float>(k_AtlasSize);
    info.width = glyphW;
    info.height = glyphH;
    info.offsetX = 0;
    info.offsetY = 0;
    info.advance = metrics.widthIncludingTrailingWhitespace;

    entry.glyphs[ch] = info;

    // カーソルを更新
    entry.cursorX += glyphW + 1;
    if (glyphH > entry.rowHeight)
        entry.rowHeight = glyphH;

    return true;
}

void FontManager::UploadAtlas(FontEntry& entry)
{
    if (!m_textureManager)
        return;

    if (entry.atlasTextureHandle < 0)
    {
        // 初回: テクスチャ作成
        entry.atlasTextureHandle = m_textureManager->CreateTextureFromMemory(
            entry.atlasPixels.data(), k_AtlasSize, k_AtlasSize);

        if (entry.atlasTextureHandle < 0)
        {
            GX_LOG_ERROR("Failed to create font atlas texture");
        }
    }
    else
    {
        // 更新: 既存テクスチャへ書き戻し（ハンドル/SRVは維持）
        if (!m_textureManager->UpdateTextureFromMemory(
                entry.atlasTextureHandle, entry.atlasPixels.data(), k_AtlasSize, k_AtlasSize))
        {
            GX_LOG_ERROR("Failed to update font atlas texture");
        }
    }

    entry.atlasDirty = false;
}

const GlyphInfo* FontManager::GetGlyphInfo(int fontHandle, wchar_t ch)
{
    if (fontHandle < 0 || fontHandle >= static_cast<int>(m_entries.size()))
        return nullptr;

    auto& entry = m_entries[fontHandle];
    if (!entry.valid)
        return nullptr;

    // グリフ未登録ならラスタライズし、GPU転送が必要になる
    auto it = entry.glyphs.find(ch);
    if (it == entry.glyphs.end())
    {
        if (!RasterizeGlyph(entry, ch))
            return nullptr;

        // dirtyフラグを立てて後でまとめて転送（FlushAtlasUpdates）
        entry.atlasDirty = true;

        it = entry.glyphs.find(ch);
        if (it == entry.glyphs.end())
            return nullptr;
    }

    return &it->second;
}

void FontManager::FlushAtlasUpdates()
{
    for (auto& entry : m_entries)
    {
        if (entry.valid && entry.atlasDirty)
        {
            UploadAtlas(entry);
        }
    }
}

int FontManager::GetAtlasTextureHandle(int fontHandle) const
{
    if (fontHandle < 0 || fontHandle >= static_cast<int>(m_entries.size()))
        return -1;

    return m_entries[fontHandle].atlasTextureHandle;
}

int FontManager::GetLineHeight(int fontHandle) const
{
    if (fontHandle < 0 || fontHandle >= static_cast<int>(m_entries.size()))
        return 0;

    return m_entries[fontHandle].lineHeight;
}

float FontManager::GetCapOffset(int fontHandle) const
{
    if (fontHandle < 0 || fontHandle >= static_cast<int>(m_entries.size()))
        return 0.0f;

    return m_entries[fontHandle].capOffset;
}

void FontManager::Shutdown()
{
    m_entries.clear();
    m_freeHandles.clear();
    m_wicFactory.Reset();
    m_dwriteFactory.Reset();
    m_d2dFactory.Reset();

    if (m_comInitialized)
    {
        CoUninitialize();
        m_comInitialized = false;
    }
    GX_LOG_INFO("FontManager shutdown");
}

} // namespace GX

