#pragma once
/// @file Profiler.h
/// @brief CPUプロファイラ（セクション計測 + メモリ使用量 + フレーム履歴）
///
/// BeginSection/EndSection でコード区間のCPU時間を計測する。
/// RAIIの CPUProfileScope でスコープベースの計測も可能。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx
{

/// @brief セクション計測結果
struct SectionResult
{
    gx::String name;       ///< セクション名
    float       timeMs;     ///< 処理時間（ミリ秒）
    int         depth;      ///< ネスト深度
    uint32_t    callCount;  ///< 同一フレーム内の呼び出し回数
};

/// @brief CPUプロファイラ（シングルトン）
///
/// BeginFrame/EndFrame の間に BeginSection/EndSection で計測区間を定義する。
/// GetResults() で前フレームの結果を取得できる（ダブルバッファ方式）。
class Profiler
{
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return Profilerインスタンスへの参照
    static Profiler& Instance();

    /// @brief フレーム計測開始
    void BeginFrame();

    /// @brief フレーム計測終了
    void EndFrame();

    /// @brief セクション計測開始
    /// @param name セクション名
    void BeginSection(const char* name);

    /// @brief セクション計測終了
    void EndSection(const char* name);

    /// @brief 前フレームの計測結果を取得する
    /// @return セクション結果の配列への参照
    const gx::Vector<SectionResult>& GetResults() const;

    /// @brief フレーム全体のCPU時間（ミリ秒）を取得する
    /// @return CPU処理時間（ミリ秒）
    float GetFrameCPUTimeMs() const { return m_lastFrameTimeMs; }

    /// @brief 過去フレームの平均フレーム時間（ミリ秒）を取得する
    /// @return 平均フレーム時間（ミリ秒）
    float GetAverageFrameTimeMs() const;

    /// @brief 現在のプロセスメモリ使用量（バイト）を取得する
    /// @return メモリ使用量（バイト）
    static size_t GetMemoryUsage();

    /// @brief プロファイリングの有効/無効を切り替える
    /// @param enabled trueで有効化
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /// @brief プロファイリングが有効かどうか
    /// @return 有効ならtrue
    bool IsEnabled() const { return m_enabled; }

private:
    Profiler();

    /// @brief 計測中のセクション情報
    struct SectionEntry
    {
        gx::String name;            ///< セクション名
        LARGE_INTEGER startTime;     ///< 計測開始カウンタ値
        int depth;                   ///< ネスト深度
    };

    LARGE_INTEGER m_frequency;              ///< カウンタ周波数
    LARGE_INTEGER m_frameStartTime;         ///< フレーム開始カウンタ値
    float m_lastFrameTimeMs = 0.0f;         ///< 直近フレームのCPU時間（ミリ秒）

    gx::Vector<SectionEntry>  m_openSections;     ///< 計測中のセクションスタック
    gx::Vector<SectionResult> m_currentResults;    ///< 現フレームの結果（書き込み中）
    gx::Vector<SectionResult> m_lastResults;       ///< 前フレームの結果（読み取り用）
    int m_currentDepth = 0;                         ///< 現在のネスト深度
    bool m_enabled = true;                          ///< プロファイリング有効フラグ

    static constexpr int k_FrameHistorySize = 300;  ///< フレーム履歴の最大サイズ
    float m_frameHistory[300] = {};                  ///< フレーム時間の履歴リングバッファ
    int m_frameHistoryIndex = 0;                     ///< 履歴の書き込み位置
    int m_frameHistoryCount = 0;                     ///< 履歴に記録済みのフレーム数
};

/// @brief RAIIスコープベースのCPU計測
class CPUProfileScope
{
public:
    /// @param name 計測するセクション名
    explicit CPUProfileScope(const char* name)
        : m_name(name)
    {
        Profiler::Instance().BeginSection(name);
    }

    ~CPUProfileScope()
    {
        Profiler::Instance().EndSection(m_name);
    }

private:
    const char* m_name;  ///< セクション名（EndSectionで使用）
};

} // namespace gx

/// @brief CPUプロファイルスコープマクロ
#define GX_CPU_PROFILE_SCOPE(name) gx::CPUProfileScope _gx_profile_scope_##__LINE__(name)

/// @brief 現在の関数名でプロファイルスコープを自動生成するマクロ
#define GX_PROFILE_FUNCTION() GX_CPU_PROFILE_SCOPE(__FUNCTION__)
/// @}
