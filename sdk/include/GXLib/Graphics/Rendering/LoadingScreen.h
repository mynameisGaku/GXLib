/// @file LoadingScreen.h
/// @brief ローディング画面描画クラス
///
/// バックグラウンド初期化中にメインスレッドで軽量なローディング画面を表示する。
/// 独自の SpriteBatch/FontManager/TextRenderer/PrimitiveBatch を所有し、
/// 他のレンダラーが未初期化でも単独で動作する。

#pragma once
#include "pch_graphics.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include "Graphics/Rendering/PrimitiveBatch.h"
#include "Graphics/Rendering/FontManager.h"
#include "Graphics/Rendering/TextRenderer.h"
#include "Core/Timer.h"

namespace gx {

/// @brief ローディング画面
///
/// メインスレッドのローディングループで毎フレーム Render() を呼び出す。
/// 幾何学的アニメーション（ヘキサゴン、周回ドット、プログレス弧）を描画する。
class LoadingScreen
{
public:
    /// @brief 初期化
    /// @param device D3D12 デバイス
    /// @param queue コマンドキュー (フォントアトラスアップロード用)
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功時 true
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* queue,
                    uint32_t width, uint32_t height);

    /// @brief リソース解放
    void Shutdown();

    /// @brief ローディング画面を1フレーム描画する
    /// @param cmdList 描画先コマンドリスト
    /// @param rtv バックバッファ RTV
    /// @param backBuffer バックバッファリソース (バリア用)
    /// @param frameIndex フレームインデックス (0 or 1)
    /// @param progress 進捗 (0.0〜1.0、負数で不定モード)
    /// @param status 表示テキスト
    /// @param alpha 全体のアルファ (0.0〜1.0、フェードアウト用)
    void Render(ID3D12GraphicsCommandList* cmdList,
                D3D12_CPU_DESCRIPTOR_HANDLE rtv,
                ID3D12Resource* backBuffer,
                uint32_t frameIndex,
                float progress = -1.0f,
                const wchar_t* status = L"Now Loading",
                float alpha = 1.0f);

private:
    /// @brief アルファ適用済みの ARGB カラーを生成する
    static uint32_t ApplyAlpha(uint32_t baseColor, float alpha);

    /// @brief 回転するヘキサゴンを描画する
    void DrawHexagon(float cx, float cy, float radius, float angle, float alpha);

    /// @brief ヘキサゴン周囲を周回するドットを描画する
    void DrawOrbitingDots(float cx, float cy, float orbitRadius, float time, float alpha);

    /// @brief プログレス弧を描画する
    void DrawProgressArc(float cx, float cy, float radius, float progress, float time, float alpha);

    SpriteBatch    m_spriteBatch;
    PrimitiveBatch m_primitiveBatch;
    FontManager    m_fontManager;
    TextRenderer   m_textRenderer;
    int            m_font = -1;
    Timer          m_timer;
    uint32_t       m_width  = 0;
    uint32_t       m_height = 0;
    float          m_displayProgress = 0.0f; ///< 表示用の補間済みプログレス値
};

} // namespace gx
