#pragma once
/// @file GX/Audio.h
/// @brief Audio functions / オーディオ関数 — Layer 1 facade per ADR-0017
///
/// This header is a beginner-friendly entry point to GXLib's audio API.
/// All functions live in the `gx::` namespace and use cleaner names than
/// the underlying DXLib-compatible functions (e.g. `LoadSound` rather than
/// `LoadSoundMem`). The behaviour is identical — these are just nicer names
/// for the same Compat layer.
///
/// このヘッダはGXLibの音声APIへの初学者向け入り口。
/// すべての関数は `gx::` 名前空間にあり、DXLib互換関数より分かりやすい名前を
/// 使用 (例: `LoadSoundMem` の代わりに `LoadSound`)。動作は同じ — 同じ
/// Compat 層への別名のみ。
///
/// @code
/// // Hello sound — load and play
/// int sfx = gx::LoadSound("se/jump.wav");
/// if (sfx < 0) { /* check the log; gx::Logger printed why */ return -1; }
/// gx::PlaySound(sfx, GX_PLAYTYPE_BACK);
/// @endcode

// Forward declarations for existing Compat functions
int LoadSoundMem(const char* path);
int PlaySoundMem(int handle, int playType, int topPositionFlag = 1);
int StopSoundMem(int handle);
int DeleteSoundMem(int handle);
int ChangeVolumeSoundMem(int volumePal, int handle);
int CheckSoundMem(int handle);
int PlayMusic(const char* path, int playType);
void StopMusic();
int CheckMusic();

namespace gx {
/// @addtogroup grp_gx_facade
/// @{

/// @brief Load a sound asset (WAV/OGG). 音声アセットを読み込む。
/// @param path File path. ファイルパス。
/// @return Handle ≥ 0 on success, -1 on failure (error logged via gx::Logger).
///         成功時に0以上のハンドル、失敗時 -1 (gx::Logger にエラー出力)。
/// @code int bgm = gx::LoadSound("bgm/title.ogg"); @endcode
inline int LoadSound(const char* path) { return LoadSoundMem(path); }

/// @brief Play a loaded sound. 読み込んだ音声を再生する。
/// @param handle Handle from LoadSound. LoadSound の戻り値。
/// @param type GX_PLAYTYPE_BACK (one-shot) or GX_PLAYTYPE_LOOP (loop) or GX_PLAYTYPE_NORMAL (blocking).
/// @param top  1 = restart from beginning, 0 = continue. 1=先頭から、0=続きから。
/// @return 0 on success, -1 on failure.
/// @code gx::PlaySound(sfx, GX_PLAYTYPE_BACK); @endcode
inline int PlaySound(int handle, int type, int top = 1) { return PlaySoundMem(handle, type, top); }

/// @brief Stop a playing sound. 再生中の音声を停止する。
inline int StopSound(int handle) { return StopSoundMem(handle); }

/// @brief Release a sound asset. 音声アセットを解放する。
inline int DeleteSound(int handle) { return DeleteSoundMem(handle); }

/// @brief Set per-sound volume. 個別の音量を設定する。
/// @param vol 0..255 (DXLib-compatible range).
inline int SetSoundVolume(int vol, int handle) { return ChangeVolumeSoundMem(vol, handle); }

/// @brief Check if a sound is currently playing. 再生中か判定する。
/// @return 1 = playing, 0 = stopped, -1 = invalid handle.
inline int IsSoundPlaying(int handle) { return CheckSoundMem(handle); }

/// @brief Play streamed music (OGG). ストリーミング音楽 (OGG) を再生する。
/// @code gx::PlayMusic("bgm/stage1.ogg", GX_PLAYTYPE_LOOP); @endcode
using ::PlayMusic;

/// @brief Stop streamed music. ストリーミング音楽を停止する。
using ::StopMusic;

/// @brief Check streamed music state. ストリーミング音楽の状態を確認する。
using ::CheckMusic;

/// @}
} // namespace gx
