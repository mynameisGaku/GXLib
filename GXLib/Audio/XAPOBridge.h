#pragma once
/// @file XAPOBridge.h
/// @brief IAudioEffect → IXAPO ブリッジ (カスタム DSP を XAudio2 パイプラインに接続)
///
/// AudioBus の insert チェーンに登録された IAudioEffect インスタンスの Process() を
/// XAudio2 SubmixVoice のエフェクトスロット経由で実際に呼び出すための XAPO 実装。
/// AudioBus::AddEffect / RemoveEffect 時に自動で SetEffectChain を更新する。

#include "pch_audio.h"
#include "Audio/AudioEffect.h"
#include <xapo.h>
#include <atomic>

namespace gx
{

class XAPOBridge : public IXAPO
{
public:
    static constexpr size_t k_MaxEffects = 4;

    XAPOBridge() : m_refCount(1), m_channels(0), m_sampleRate(0)
    {
        for (auto& e : m_effects) e = nullptr;
    }

    void SetEffectSlot(size_t index, IAudioEffect* effect)
    {
        if (index < k_MaxEffects)
            m_effects[index] = effect;
    }

    // === IUnknown ===

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IXAPO))
        {
            *ppv = static_cast<IXAPO*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return m_refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG count = m_refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (count == 0) delete this;
        return count;
    }

    // === IXAPO ===

    HRESULT STDMETHODCALLTYPE GetRegistrationProperties(XAPO_REGISTRATION_PROPERTIES** ppProps) override
    {
        if (!ppProps) return E_POINTER;
        auto* props = static_cast<XAPO_REGISTRATION_PROPERTIES*>(
            CoTaskMemAlloc(sizeof(XAPO_REGISTRATION_PROPERTIES)));
        if (!props) return E_OUTOFMEMORY;
        ZeroMemory(props, sizeof(*props));
        props->clsid = __uuidof(IUnknown);
        wcscpy_s(props->FriendlyName, L"GXLib IAudioEffect Bridge");
        wcscpy_s(props->CopyrightInfo, L"GXLib");
        props->MajorVersion = 1;
        props->MinorVersion = 0;
        // 2026-04-19 defect fix: XAudio2 が SetEffectChain で要求する標準
        // MUST_MATCH フラグ群を全部立てる。INPLACE_REQUIRED だけだと
        // SetEffectChain が XAUDIO2_E_INVALID_CALL (0x88960001) を返す。
        // Microsoft XAPO サンプル準拠 — IN_PLACE_SUPPORTED で in-place も
        // out-of-place も受け付け、実装は in-place で動作する。
        props->Flags = XAPO_FLAG_CHANNELS_MUST_MATCH
                     | XAPO_FLAG_FRAMERATE_MUST_MATCH
                     | XAPO_FLAG_BITSPERSAMPLE_MUST_MATCH
                     | XAPO_FLAG_BUFFERCOUNT_MUST_MATCH
                     | XAPO_FLAG_INPLACE_SUPPORTED;
        props->MinInputBufferCount = 1;
        props->MaxInputBufferCount = 1;
        props->MinOutputBufferCount = 1;
        props->MaxOutputBufferCount = 1;
        *ppProps = props;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE IsInputFormatSupported(
        const WAVEFORMATEX* /*pOutputFormat*/,
        const WAVEFORMATEX* pRequestedInputFormat,
        WAVEFORMATEX** ppSupportedInputFormat) override
    {
        if (ppSupportedInputFormat) *ppSupportedInputFormat = nullptr;
        if (!pRequestedInputFormat) return E_POINTER;
        if (pRequestedInputFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            return S_OK;
        return XAPO_E_FORMAT_UNSUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE IsOutputFormatSupported(
        const WAVEFORMATEX* /*pInputFormat*/,
        const WAVEFORMATEX* pRequestedOutputFormat,
        WAVEFORMATEX** ppSupportedOutputFormat) override
    {
        if (ppSupportedOutputFormat) *ppSupportedOutputFormat = nullptr;
        if (!pRequestedOutputFormat) return E_POINTER;
        if (pRequestedOutputFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            return S_OK;
        return XAPO_E_FORMAT_UNSUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE Initialize(const void* /*pData*/, UINT32 /*DataByteSize*/) override
    {
        return S_OK;
    }

    void STDMETHODCALLTYPE Reset() override
    {
        for (auto* e : m_effects)
            if (e) e->Reset();
    }

    HRESULT STDMETHODCALLTYPE LockForProcess(
        UINT32 InputLockedParameterCount,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pInputLockedParameters,
        UINT32 /*OutputLockedParameterCount*/,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* /*pOutputLockedParameters*/) override
    {
        if (InputLockedParameterCount > 0 && pInputLockedParameters)
        {
            const auto& fmt = *pInputLockedParameters->pFormat;
            m_channels   = fmt.nChannels;
            m_sampleRate = fmt.nSamplesPerSec;
        }
        return S_OK;
    }

    void STDMETHODCALLTYPE UnlockForProcess() override {}

    void STDMETHODCALLTYPE Process(
        UINT32 InputProcessParameterCount,
        const XAPO_PROCESS_BUFFER_PARAMETERS* pInputProcessParameters,
        UINT32 /*OutputProcessParameterCount*/,
        XAPO_PROCESS_BUFFER_PARAMETERS* pOutputProcessParameters,
        BOOL /*IsEnabled*/) override
    {
        if (InputProcessParameterCount == 0 || !pInputProcessParameters) return;

        const auto& inParams = pInputProcessParameters[0];
        if (inParams.BufferFlags == XAPO_BUFFER_SILENT)
        {
            if (pOutputProcessParameters)
                pOutputProcessParameters[0].BufferFlags = XAPO_BUFFER_SILENT;
            return;
        }

        auto* buffer = static_cast<float*>(inParams.pBuffer);
        uint32_t frameCount = inParams.ValidFrameCount;

        for (auto* effect : m_effects)
        {
            if (effect)
                effect->Process(buffer, frameCount, m_channels, m_sampleRate);
        }

        if (pOutputProcessParameters)
        {
            pOutputProcessParameters[0].ValidFrameCount = frameCount;
            pOutputProcessParameters[0].BufferFlags = XAPO_BUFFER_VALID;
        }
    }

    UINT32 STDMETHODCALLTYPE CalcInputFrames(UINT32 OutputFrameCount) override
    {
        return OutputFrameCount;
    }

    UINT32 STDMETHODCALLTYPE CalcOutputFrames(UINT32 InputFrameCount) override
    {
        return InputFrameCount;
    }

private:
    std::atomic<ULONG> m_refCount;
    IAudioEffect*      m_effects[k_MaxEffects];
    uint32_t           m_channels;
    uint32_t           m_sampleRate;
};

} // namespace gx
