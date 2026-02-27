#pragma once
/// @file GX/Audio.h
/// @brief オーディオ関数

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

inline int LoadSound(const char* path) { return LoadSoundMem(path); }
inline int PlaySound(int handle, int type, int top = 1) { return PlaySoundMem(handle, type, top); }
inline int StopSound(int handle) { return StopSoundMem(handle); }
inline int DeleteSound(int handle) { return DeleteSoundMem(handle); }
inline int SetSoundVolume(int vol, int handle) { return ChangeVolumeSoundMem(vol, handle); }
inline int IsSoundPlaying(int handle) { return CheckSoundMem(handle); }

using ::PlayMusic;
using ::StopMusic;
using ::CheckMusic;

} // namespace gx
