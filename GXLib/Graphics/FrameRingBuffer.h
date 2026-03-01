#pragma once
/// @file FrameRingBuffer.h
/// @brief フレームをリングバッファで保持する巻き戻し録画バッファ
///
/// 直近N秒分のフレームデータ（RGBA画素）をリング状のバッファに蓄積し続け、
/// 任意のタイミングで VideoRecorder に渡して動画として書き出せる。
/// 「いつでも過去に戻って保存できる」録画機能の実装に使う。
/// @addtogroup grp_graphics/// @{
#include "pch_graphics.h"

namespace gx
{

class VideoRecorder;

class FrameRingBuffer
{
public:
    FrameRingBuffer() = default;
    ~FrameRingBuffer() = default;

    /// @brief リングバッファを初期化する
    /// @param maxSeconds 保持する最大秒数
    /// @param width フレーム幅（ピクセル）
    /// @param height フレーム高さ（ピクセル）
    /// @param fps フレームレート
    void Initialize(float maxSeconds, uint32_t width, uint32_t height, uint32_t fps = 30);
    /// @brief フレームデータをリングバッファに追加する
    /// @param rgbaData RGBAピクセルデータ
    /// @param dataSize データサイズ（バイト）
    void PushFrame(const uint8_t* rgbaData, uint32_t dataSize);
    /// @brief バッファ内容を動画ファイルとして書き出す
    /// @param recorder 使用するVideoRecorder
    /// @param outputPath 出力ファイルパス
    /// @return 成功時true
    bool ExportToVideo(VideoRecorder& recorder, const std::wstring& outputPath);

    /// @brief 初期化済みか判定する
    /// @return 初期化済みならtrue
    bool IsInitialized() const { return m_initialized; }
    /// @brief 現在のフレーム数を取得する
    /// @return バッファに格納されたフレーム数
    uint32_t GetFrameCount() const { return m_frameCount; }
    /// @brief 最大フレーム数を取得する
    /// @return 保持可能な最大フレーム数
    uint32_t GetMaxFrames() const { return m_maxFrames; }
    /// @brief フレーム幅を取得する
    /// @return ピクセル単位の幅
    uint32_t GetWidth() const { return m_width; }
    /// @brief フレーム高さを取得する
    /// @return ピクセル単位の高さ
    uint32_t GetHeight() const { return m_height; }
    /// @brief FPSを取得する
    /// @return フレームレート
    uint32_t GetFPS() const { return m_fps; }
    /// @brief バッファ内の秒数を取得する
    /// @return 秒数
    float GetBufferedSeconds() const;
    /// @brief バッファをクリアする
    void Clear();

private:
    bool m_initialized = false;      ///< 初期化済みフラグ
    uint32_t m_width = 0;            ///< フレーム幅（ピクセル）
    uint32_t m_height = 0;           ///< フレーム高さ（ピクセル）
    uint32_t m_fps = 30;             ///< フレームレート
    uint32_t m_maxFrames = 0;        ///< リングバッファの最大フレーム数
    uint32_t m_frameCount = 0;       ///< 現在格納されているフレーム数
    uint32_t m_writeIndex = 0;       ///< 次の書き込み先インデックス
    uint32_t m_frameSize = 0;        ///< 1フレームのバイトサイズ
    std::vector<std::vector<uint8_t>> m_frames;  ///< フレームデータリング
};

} // namespace gx
/// @}
