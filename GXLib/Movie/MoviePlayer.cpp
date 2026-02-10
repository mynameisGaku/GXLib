#include "pch.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include "Movie/MoviePlayer.h"
#include "Core/Logger.h"

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfuuid.lib")

namespace GX {

MoviePlayer::MoviePlayer()
{
    QueryPerformanceCounter(&m_lastFrameTime);
}

MoviePlayer::~MoviePlayer()
{
    Close();
}

bool MoviePlayer::Open(const std::string& filePath, GraphicsDevice& device, TextureManager& texManager)
{
    Close();
    m_texManager = &texManager;

    // Media Foundation を初期化。Windowsの動画再生APIを使うための準備。
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("MoviePlayer: MFStartup failed: 0x%08X", hr);
        return false;
    }
    m_mfInitialized = true;

    // パスを wide 文字列へ変換（WinAPI用）。
    int wLen = MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, nullptr, 0);
    std::wstring wPath(wLen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, wPath.data(), wLen);

    // ソースリーダーを作成（動画ストリームを読む入口）。
    IMFAttributes* pAttributes = nullptr;
    hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("MoviePlayer: MFCreateAttributes failed");
        return false;
    }

    hr = MFCreateSourceReaderFromURL(wPath.c_str(), pAttributes, &m_reader);
    pAttributes->Release();

    if (FAILED(hr))
    {
        GX_LOG_ERROR("MoviePlayer: Failed to create source reader for: %s (0x%08X)",
            filePath.c_str(), hr);
        return false;
    }

    // 出力をRGB32に設定（1ピクセル4バイトの扱いに統一）。
    IMFMediaType* pType = nullptr;
    hr = MFCreateMediaType(&pType);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("MoviePlayer: MFCreateMediaType failed");
        return false;
    }

    pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    hr = m_reader->SetCurrentMediaType(
        static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, pType);
    pType->Release();

    if (FAILED(hr))
    {
        GX_LOG_ERROR("MoviePlayer: SetCurrentMediaType failed: 0x%08X", hr);
        return false;
    }

    // 実際の出力形式から幅・高さを取得
    IMFMediaType* pOutputType = nullptr;
    hr = m_reader->GetCurrentMediaType(
        static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &pOutputType);
    if (SUCCEEDED(hr))
    {
        UINT32 w = 0, h = 0;
        MFGetAttributeSize(pOutputType, MF_MT_FRAME_SIZE, &w, &h);
        m_width = w;
        m_height = h;

        // フレームレートを取得
        UINT32 num = 0, den = 1;
        MFGetAttributeRatio(pOutputType, MF_MT_FRAME_RATE, &num, &den);
        if (num > 0)
            m_frameInterval = static_cast<double>(den) / static_cast<double>(num);
        else
            m_frameInterval = 1.0 / 30.0; // default 30fps

        pOutputType->Release();
    }

    // 再生時間を取得
    PROPVARIANT var;
    PropVariantInit(&var);
    hr = m_reader->GetPresentationAttribute(
        static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE),
        MF_PD_DURATION, &var);
    if (SUCCEEDED(hr))
    {
        // 時間は100ナノ秒単位なので秒に変換
        m_duration = static_cast<double>(var.uhVal.QuadPart) / 10000000.0;
        PropVariantClear(&var);
    }

    GX_LOG_INFO("MoviePlayer: Opened %s (%ux%u, %.1f sec, %.1f fps)",
        filePath.c_str(), m_width, m_height, m_duration,
        m_frameInterval > 0 ? 1.0 / m_frameInterval : 0.0);

    m_state = MovieState::Stopped;
    m_finished = false;
    m_position = 0.0;

    return true;
}

void MoviePlayer::Close()
{
    m_state = MovieState::Stopped;

    if (m_textureHandle >= 0 && m_texManager)
    {
        m_texManager->ReleaseTexture(m_textureHandle);
        m_textureHandle = -1;
    }

    if (m_reader)
    {
        m_reader->Release();
        m_reader = nullptr;
    }

    if (m_mfInitialized)
    {
        MFShutdown();
        m_mfInitialized = false;
    }

    m_width = 0;
    m_height = 0;
    m_duration = 0.0;
    m_position = 0.0;
    m_finished = false;
    m_texManager = nullptr;
}

void MoviePlayer::Play()
{
    if (m_reader)
    {
        m_state = MovieState::Playing;
        m_finished = false;
        QueryPerformanceCounter(&m_lastFrameTime);
    }
}

void MoviePlayer::Pause()
{
    if (m_state == MovieState::Playing)
        m_state = MovieState::Paused;
}

void MoviePlayer::Stop()
{
    m_state = MovieState::Stopped;
    m_position = 0.0;
    m_finished = false;
    Seek(0.0);
}

void MoviePlayer::Seek(double timeSeconds)
{
    if (!m_reader) return;

    PROPVARIANT var;
    PropVariantInit(&var);
    var.vt = VT_I8;
    var.hVal.QuadPart = static_cast<LONGLONG>(timeSeconds * 10000000.0);

    m_reader->SetCurrentPosition(GUID_NULL, var);
    PropVariantClear(&var);
    m_position = timeSeconds;
    m_finished = false;
}

bool MoviePlayer::Update(GraphicsDevice& device)
{
    if (m_state != MovieState::Playing || !m_reader)
        return false;

    // フレーム間隔のチェック（時間が来たら次フレームへ）
    LARGE_INTEGER now, freq;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&freq);
    double elapsed = static_cast<double>(now.QuadPart - m_lastFrameTime.QuadPart)
                   / static_cast<double>(freq.QuadPart);

    if (elapsed < m_frameInterval)
        return false;

    m_lastFrameTime = now;
    return DecodeNextFrame(device);
}

bool MoviePlayer::DecodeNextFrame(GraphicsDevice& device)
{
    if (!m_reader) return false;

    DWORD streamIndex, flags;
    LONGLONG timestamp;
    IMFSample* pSample = nullptr;

    HRESULT hr = m_reader->ReadSample(
        static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
        0, &streamIndex, &flags, &timestamp, &pSample);

    if (FAILED(hr))
        return false;

    if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
    {
        m_finished = true;
        m_state = MovieState::Stopped;
        if (pSample) pSample->Release();
        return false;
    }

    if (!pSample)
        return false;

    m_position = static_cast<double>(timestamp) / 10000000.0;

    // バッファ取得
    IMFMediaBuffer* pBuffer = nullptr;
    hr = pSample->ConvertToContiguousBuffer(&pBuffer);
    if (FAILED(hr))
    {
        pSample->Release();
        return false;
    }

    BYTE* pData = nullptr;
    DWORD maxLen = 0, curLen = 0;
    hr = pBuffer->Lock(&pData, &maxLen, &curLen);
    if (FAILED(hr))
    {
        pBuffer->Release();
        pSample->Release();
        return false;
    }

    // Media FoundationのRGB32はBGRAかつ上下反転なので、RGBA上向きに変換する。
    // 初心者向け: 画像の並び順が違うため、そのままだと色や上下がずれる。
    std::vector<uint8_t> rgba(m_width * m_height * 4);
    for (uint32_t y = 0; y < m_height; ++y)
    {
        // 縦反転（MFは下から上の並び）
        const uint8_t* srcRow = pData + (m_height - 1 - y) * m_width * 4;
        uint8_t* dstRow = rgba.data() + y * m_width * 4;
        for (uint32_t x = 0; x < m_width; ++x)
        {
            dstRow[x * 4 + 0] = srcRow[x * 4 + 2]; // R <- B
            dstRow[x * 4 + 1] = srcRow[x * 4 + 1]; // G
            dstRow[x * 4 + 2] = srcRow[x * 4 + 0]; // B <- R
            dstRow[x * 4 + 3] = 255;                // A
        }
    }

    pBuffer->Unlock();
    pBuffer->Release();
    pSample->Release();

    // テクスチャを作成/更新
    if (m_textureHandle < 0)
    {
        m_textureHandle = m_texManager->CreateTextureFromMemory(
            rgba.data(), m_width, m_height);
    }
    else
    {
        // 旧テクスチャを解放して作り直す（簡単だが少し重い）
        m_texManager->ReleaseTexture(m_textureHandle);
        m_textureHandle = m_texManager->CreateTextureFromMemory(
            rgba.data(), m_width, m_height);
    }

    return true;
}

} // namespace GX
