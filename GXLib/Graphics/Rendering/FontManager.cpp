/// @file FontManager.cpp
/// @brief フォントマネージャーの実装 — DirectWriteラスタライズとテクスチャアトラス
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

    // DirectWrite ファクトリ作成
    HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));

    if (FAILED(hr))
    {
        GX_LOG_ERROR("DWriteCreateFactory failed: 0x%08X", static_cast<unsigned>(hr));
        return false;
    }

    // Direct2D ファクトリ作成（ラスタライズ用）
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());
    if (FAILED(hr))
    {
        GX_LOG_ERROR("D2D1CreateFactory failed: 0x%08X", static_cast<unsigned>(hr));
        return false;
    }

    m_entries.reserve(k_MaxFonts);
    GX_LOG_INFO("FontManager initialized");
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
    entry.lineHeight = fontSize + 4;  // フォントサイズ＋少し余白
    entry.cursorX = 1;
    entry.cursorY = 1;
    entry.rowHeight = 0;

    // テクスチャアトラス用ピクセルバッファを初期化（透明黒）
    entry.atlasPixels.resize(k_AtlasSize * k_AtlasSize, 0x00000000);

    // DirectWrite TextFormat作成
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

    // ASCII基本文字を一括ラスタライズ
    RasterizeBasicChars(entry);

    // アトラスをGPUにアップロード
    UploadAtlas(entry);

    entry.valid = true;
    GX_LOG_INFO("Font created: size=%d (handle: %d)", fontSize, handle);
    return handle;
}

void FontManager::RasterizeBasicChars(FontEntry& entry)
{
    // ASCII印字可能文字(32-126)を一括ラスタライズ
    for (wchar_t ch = 32; ch <= 126; ++ch)
    {
        RasterizeGlyph(entry, ch);
    }
}

bool FontManager::RasterizeGlyph(FontEntry& entry, wchar_t ch)
{
    // 既にラスタライズ済みならスキップ
    if (entry.glyphs.contains(ch))
        return true;

    // スペースの特別処理
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

    // テキストメトリクスを取得
    DWRITE_TEXT_METRICS metrics = {};
    textLayout->GetMetrics(&metrics);

    int glyphW = static_cast<int>(ceilf(metrics.widthIncludingTrailingWhitespace)) + 2;
    int glyphH = static_cast<int>(ceilf(metrics.height)) + 2;

    if (glyphW <= 0) glyphW = 1;
    if (glyphH <= 0) glyphH = 1;

    // D2Dでラスタライズ（WIC BitmapRenderTargetを使用）
    // WICビットマップを作成
    ComPtr<IWICImagingFactory> wicFactory;
    hr = CoCreateInstance(
        CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wicFactory));
    if (FAILED(hr))
        return false;

    ComPtr<IWICBitmap> wicBitmap;
    hr = wicFactory->CreateBitmap(
        static_cast<UINT>(glyphW), static_cast<UINT>(glyphH),
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapCacheOnLoad,
        &wicBitmap);
    if (FAILED(hr))
        return false;

    // D2D RenderTarget作成（WICBitmap上に描画）
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

    // ピクセルデータを読み出し
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

    // アトラスに配置（改行チェック）
    if (entry.cursorX + glyphW + 1 > static_cast<int>(k_AtlasSize))
    {
        entry.cursorX = 1;
        entry.cursorY += entry.rowHeight + 1;
        entry.rowHeight = 0;
    }

    if (entry.cursorY + glyphH + 1 > static_cast<int>(k_AtlasSize))
    {
        GX_LOG_WARN("Font atlas full, cannot add glyph for char %d", static_cast<int>(ch));
        return false;
    }

    // アトラスにコピー（BGRA → RGBA変換）
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

    // カーソルを進める
    entry.cursorX += glyphW + 1;
    if (glyphH > entry.rowHeight)
        entry.rowHeight = glyphH;

    return true;
}

void FontManager::UploadAtlas(FontEntry& entry)
{
    if (!m_textureManager)
        return;

    // 既存のテクスチャを解放
    if (entry.atlasTextureHandle >= 0)
    {
        m_textureManager->ReleaseTexture(entry.atlasTextureHandle);
    }

    // 新しいテクスチャを作成
    entry.atlasTextureHandle = m_textureManager->CreateTextureFromMemory(
        entry.atlasPixels.data(), k_AtlasSize, k_AtlasSize);

    if (entry.atlasTextureHandle < 0)
    {
        GX_LOG_ERROR("Failed to upload font atlas texture");
    }
}

const GlyphInfo* FontManager::GetGlyphInfo(int fontHandle, wchar_t ch)
{
    if (fontHandle < 0 || fontHandle >= static_cast<int>(m_entries.size()))
        return nullptr;

    auto& entry = m_entries[fontHandle];
    if (!entry.valid)
        return nullptr;

    // グリフが未登録なら動的にラスタライズ
    auto it = entry.glyphs.find(ch);
    if (it == entry.glyphs.end())
    {
        if (!RasterizeGlyph(entry, ch))
            return nullptr;

        // アトラスを再アップロード
        UploadAtlas(entry);

        it = entry.glyphs.find(ch);
        if (it == entry.glyphs.end())
            return nullptr;
    }

    return &it->second;
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

void FontManager::Shutdown()
{
    m_entries.clear();
    m_freeHandles.clear();
    m_dwriteFactory.Reset();
    m_d2dFactory.Reset();
    GX_LOG_INFO("FontManager shutdown");
}

} // namespace GX
