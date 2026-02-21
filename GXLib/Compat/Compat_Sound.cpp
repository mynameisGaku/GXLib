/// @file Compat_Sound.cpp
/// @brief 簡易API サウンド関数の実装
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatContext.h"
#include "Audio/AudioEmitter.h"
#include "Audio/AudioListener.h"

using Ctx = GX_Internal::CompatContext;

/// TCHAR文字列をstd::wstringに変換するヘルパー
static std::wstring ToWString(const TCHAR* str)
{
#ifdef UNICODE
    return std::wstring(str);
#else
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str, -1, result.data(), len);
    return result;
#endif
}

// ============================================================================
// SE (効果音)
// ============================================================================
int LoadSoundMem(const TCHAR* filePath)
{
    return Ctx::Instance().audioManager.LoadSound(ToWString(filePath));
}

// GX_PLAYTYPE_LOOPの場合はBGMチャネル（PlayMusic）で再生する。
// GX_PLAYTYPE_NORMALとGX_PLAYTYPE_BACKはどちらもSE再生（PlaySound）に統一。
int PlaySoundMem(int handle, int playType, int resumeFlag)
{
    (void)resumeFlag;
    auto& ctx = Ctx::Instance();
    if (playType == GX_PLAYTYPE_LOOP)
    {
        ctx.audioManager.PlayMusic(handle, true);
    }
    else
    {
        ctx.audioManager.PlaySound(handle);
    }
    return 0;
}

int StopSoundMem(int handle)
{
    (void)handle;
    return 0;
}

int DeleteSoundMem(int handle)
{
    Ctx::Instance().audioManager.ReleaseSound(handle);
    return 0;
}

// DxLibの音量は0〜255の整数。GXLibは0.0〜1.0のfloatなので変換する。
int ChangeVolumeSoundMem(int volume, int handle)
{
    float vol = volume / 255.0f;
    Ctx::Instance().audioManager.SetSoundVolume(handle, vol);
    return 0;
}

int CheckSoundMem(int handle)
{
    (void)handle;
    return 0;
}

// ============================================================================
// BGM（背景音楽）
// ============================================================================
int PlayMusic(const TCHAR* filePath, int playType)
{
    auto& ctx = Ctx::Instance();
    int handle = ctx.audioManager.LoadSound(ToWString(filePath));
    if (handle < 0) return -1;
    bool loop = (playType == GX_PLAYTYPE_LOOP);
    ctx.audioManager.PlayMusic(handle, loop);
    return 0;
}

int StopMusic()
{
    Ctx::Instance().audioManager.StopMusic();
    return 0;
}

int CheckMusic()
{
    return Ctx::Instance().audioManager.IsMusicPlaying() ? 1 : 0;
}

// ============================================================================
// 3Dサウンド
// ============================================================================
int Set3DPositionSoundMem(VECTOR pos, int handle)
{
    auto& ctx = Ctx::Instance();
    ctx.audioEmitter3D.SetPosition({ pos.x, pos.y, pos.z });
    return 0;
}

int Set3DRadiusSoundMem(float radius, int handle)
{
    auto& ctx = Ctx::Instance();
    ctx.audioEmitter3D.SetMaxDistance(radius);
    return 0;
}

int SetListenerPosition(VECTOR pos, VECTOR front, VECTOR up)
{
    auto& ctx = Ctx::Instance();
    ctx.audioListener3D.SetPosition({ pos.x, pos.y, pos.z });
    ctx.audioListener3D.SetOrientation({ front.x, front.y, front.z },
                                        { up.x, up.y, up.z });
    ctx.audioManager.SetListener(ctx.audioListener3D);
    return 0;
}
