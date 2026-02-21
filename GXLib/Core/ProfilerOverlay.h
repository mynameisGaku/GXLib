#pragma once
/// @file ProfilerOverlay.h
/// @brief ゲーム内プロファイラオーバーレイ

#include "pch.h"

namespace GX
{

class SpriteBatch;
class PrimitiveBatch;
class TextRenderer;
class GPUProfiler;

/// @brief ゲーム内パフォーマンスオーバーレイ
class ProfilerOverlay
{
public:
    /// @brief 表示モード
    enum class Mode { Minimal, Detailed, Graph };

    /// @brief 初期化
    void Initialize(SpriteBatch* spriteBatch, PrimitiveBatch* primitiveBatch,
                     TextRenderer* textRenderer, int fontHandle);

    /// @brief GPUProfilerからデータを取得して描画
    void Draw(const GPUProfiler& gpuProfiler, float deltaTime);

    /// @brief 表示/非表示切替
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    void ToggleVisible() { m_visible = !m_visible; }

    /// @brief 表示モード設定
    void SetMode(Mode mode) { m_mode = mode; }
    Mode GetMode() const { return m_mode; }
    void CycleMode();

private:
    void DrawMinimal(float deltaTime);
    void DrawDetailed(const GPUProfiler& gpuProfiler, float deltaTime);
    void DrawGraph(float deltaTime);

    SpriteBatch* m_spriteBatch = nullptr;
    PrimitiveBatch* m_primitiveBatch = nullptr;
    TextRenderer* m_textRenderer = nullptr;
    int m_fontHandle = -1;
    bool m_visible = false;
    Mode m_mode = Mode::Minimal;

    static constexpr int k_HistorySize = 120;
    float m_fpsHistory[k_HistorySize] = {};
    float m_frameTimeHistory[k_HistorySize] = {};
    int m_historyIndex = 0;
};

} // namespace GX
