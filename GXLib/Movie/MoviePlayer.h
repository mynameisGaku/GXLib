#pragma once
/// @file MoviePlayer.h
/// @brief 動画プレイヤー — Media Foundation ソースリーダーによるデコード
///
/// 動画ファイルをデコードし、毎フレームテクスチャとして取得できる。
/// SpriteBatch で描画することで動画再生が可能。

#include "Graphics/Device/GraphicsDevice.h"
#include "Graphics/Resource/TextureManager.h"

struct IMFSourceReader;

namespace GX {

/// @brief 動画の再生状態
enum class MovieState {
    Stopped,    ///< 停止中
    Playing,    ///< 再生中
    Paused      ///< 一時停止中
};

/// @brief 動画プレイヤー
///
/// Media Foundation を使用して動画ファイルをデコードし、
/// フレームごとにテクスチャとして取得する。
/// Update() を毎フレーム呼び出し、GetTextureHandle() で描画用テクスチャを取得する。
class MoviePlayer
{
public:
    MoviePlayer();
    ~MoviePlayer();

    /// @brief 動画ファイルを開く
    /// @param filePath 動画ファイルパス (MP4, WMV, AVI など)
    /// @param device GraphicsDevice への参照
    /// @param texManager テクスチャマネージャーへの参照
    /// @return 成功した場合true
    bool Open(const std::string& filePath, GraphicsDevice& device, TextureManager& texManager);

    /// @brief 動画を閉じてリソースを解放する
    void Close();

    /// @brief 再生を開始する
    void Play();

    /// @brief 再生を一時停止する
    void Pause();

    /// @brief 再生を停止し、先頭に戻る
    void Stop();

    /// @brief 指定時刻にシークする
    /// @param timeSeconds シーク先の時刻 (秒)
    void Seek(double timeSeconds);

    /// @brief 毎フレーム呼び出してデコードを進める
    /// @param device GraphicsDevice への参照
    /// @return 新しいフレームがデコードされた場合true
    bool Update(GraphicsDevice& device);

    /// @brief 再生状態を取得する
    /// @return 現在の再生状態
    MovieState GetState() const { return m_state; }

    /// @brief 動画の総再生時間を取得する
    /// @return 総再生時間 (秒)
    double GetDuration() const { return m_duration; }

    /// @brief 現在の再生位置を取得する
    /// @return 現在の再生位置 (秒)
    double GetPosition() const { return m_position; }

    /// @brief 現在のフレームのテクスチャハンドルを取得する
    /// @return TextureManagerのハンドル (SpriteBatchで描画に使用)
    int GetTextureHandle() const { return m_textureHandle; }

    /// @brief 動画の幅を取得する
    /// @return 幅 (ピクセル)
    uint32_t GetWidth() const { return m_width; }

    /// @brief 動画の高さを取得する
    /// @return 高さ (ピクセル)
    uint32_t GetHeight() const { return m_height; }

    /// @brief 動画が最後まで再生されたか判定する
    /// @return 再生完了の場合true
    bool IsFinished() const { return m_finished; }

private:
    IMFSourceReader* m_reader = nullptr;
    int m_textureHandle = -1;
    TextureManager* m_texManager = nullptr;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    double m_duration = 0.0;
    double m_position = 0.0;
    MovieState m_state = MovieState::Stopped;
    bool m_finished = false;
    bool m_mfInitialized = false;

    LARGE_INTEGER m_lastFrameTime{};
    double m_frameInterval = 0.0;

    bool DecodeNextFrame(GraphicsDevice& device);
};

} // namespace GX
