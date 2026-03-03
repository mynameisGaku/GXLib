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
/// アニメーションドット付きテキストとプログレスバーを描画する。
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
    void Render(ID3D12GraphicsCommandList* cmdList,
                D3D12_CPU_DESCRIPTOR_HANDLE rtv,
                ID3D12Resource* backBuffer,
                uint32_t frameIndex,
                float progress = -1.0f,
                const wchar_t* status = L"Now Loading");

private:
    SpriteBatch    m_spriteBatch;
    PrimitiveBatch m_primitiveBatch;
    FontManager    m_fontManager;
    TextRenderer   m_textRenderer;
    int            m_font = -1;
    Timer          m_timer;
    uint32_t       m_width  = 0;
    uint32_t       m_height = 0;
};

} // namespace gx
