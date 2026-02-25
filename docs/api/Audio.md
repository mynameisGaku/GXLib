# Audio API リファレンス

名前空間: `GX`。XAudio2 ベースのオーディオシステム。ハンドル管理、3D 空間音響対応。

## AudioManager

ハンドルベースのサウンド管理クラス。同一パスの二重読み込みはキャッシュで防止される。

### 初期化・終了

| メソッド | 説明 |
|---|---|
| `bool Initialize()` | オーディオシステム全体を初期化する |
| `void Shutdown()` | 全サウンドを解放してデバイスを破棄する |
| `void Update(float deltaTime)` | BGM フェード、3D 計算、SE クリーンアップを行う |

### サウンド読み込み・解放

| メソッド | 説明 |
|---|---|
| `int LoadSound(const std::wstring& filePath)` | WAV を読み込みハンドルを返す。失敗時 -1 |
| `void ReleaseSound(int handle)` | サウンドハンドルを解放する |

### SE 再生

| メソッド | 説明 |
|---|---|
| `void PlaySound(int handle, float volume=1.0f, float pan=0.0f)` | 効果音を再生する。同じ音を複数同時に鳴らせる |
| `void PlaySoundOnBus(int handle, AudioBus& bus, float volume=1.0f)` | 指定バスに出力して再生する |
| `void SetSoundVolume(int handle, float volume)` | 音量を設定する (0.0-1.0) |

### BGM 再生

| メソッド | 説明 |
|---|---|
| `void PlayMusic(int handle, bool loop=true, float volume=1.0f)` | BGM を再生する |
| `void StopMusic()` | BGM を停止する |
| `void PauseMusic()` / `void ResumeMusic()` | BGM の一時停止・再開 |
| `void FadeInMusic(float seconds)` | BGM フェードイン |
| `void FadeOutMusic(float seconds)` | BGM フェードアウト (完了後に自動停止) |
| `bool IsMusicPlaying()` | BGM が再生中か |

### 3D サウンド

| メソッド | 説明 |
|---|---|
| `int PlaySound3D(int handle, AudioEmitter& emitter, float volume=1.0f)` | 3D 空間内で再生する。ボイス ID を返す |
| `void StopSound3D(int voiceId)` | 3D サウンドを停止する |
| `void SetListener(const AudioListener& listener)` | リスナー位置を設定する (毎フレーム更新推奨) |
| `void SetMasterVolume(float volume)` | マスターボリュームを設定する (0.0-1.0) |

### サブシステムアクセス

| メソッド | 説明 |
|---|---|
| `AudioDevice& GetDevice()` | XAudio2 デバイスを取得する |
| `SoundPlayer& GetSoundPlayer()` | SE プレイヤーを取得する |
| `MusicPlayer& GetMusicPlayer()` | BGM プレイヤーを取得する |
| `AudioMixer& GetMixer()` | ミキサーを取得する (BGM/SE/Voice バス) |

## AudioEmitter

3D 空間内の音源を定義する。

| メソッド | 説明 |
|---|---|
| `void SetPosition(const XMFLOAT3& pos)` | 音源位置を設定する |
| `void SetVelocity(const XMFLOAT3& vel)` | 速度を設定する (ドップラー効果用) |
| `void SetDirection(const XMFLOAT3& front)` | 向きを設定する (指向性コーン用) |
| `void SetInnerRadius(float radius)` | 内側半径を設定する (この範囲内はフル音量) |
| `void SetMaxDistance(float distance)` | 最大距離を設定する (この距離以遠は無音) |
| `void SetCone(innerAngle, outerAngle, outerVolume)` | 指向性コーンを設定する (ラジアン) |
| `void DisableCone()` | コーンを無効化する (全方向均等) |

## AudioListener

3D 空間内の聴取者を定義する。通常は Camera3D と連動させる。

| メソッド | 説明 |
|---|---|
| `void SetPosition(const XMFLOAT3& pos)` | リスナー位置を設定する |
| `void SetOrientation(const XMFLOAT3& front, const XMFLOAT3& up)` | 向きを設定する |
| `void SetVelocity(const XMFLOAT3& vel)` | 速度を設定する (ドップラー効果用) |
| `void UpdateFromCamera(const Camera3D& camera, float deltaTime)` | カメラから位置・方向を自動更新する |

### 使用例

```cpp
// 初期化
AudioManager audio;
audio.Initialize();
int seFire = audio.LoadSound(L"Assets/fire.wav");
int bgm    = audio.LoadSound(L"Assets/bgm.wav");

// BGM 再生
audio.PlayMusic(bgm, true, 0.7f);

// SE 再生
audio.PlaySound(seFire);

// 3D サウンド
AudioEmitter emitter;
emitter.SetPosition({10.0f, 0.0f, 5.0f});
emitter.SetMaxDistance(50.0f);
int voiceId = audio.PlaySound3D(seFire, emitter);

AudioListener listener;
listener.UpdateFromCamera(camera, deltaTime);
audio.SetListener(listener);

// 毎フレーム
audio.Update(deltaTime);

// 終了
audio.Shutdown();
```
