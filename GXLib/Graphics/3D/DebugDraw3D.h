#pragma once
/// @file DebugDraw3D.h
/// @brief 3Dデバッグ描画（ワイヤーボックス・球・矢印・テキスト等）
///
/// 即時描画（1フレームのみ表示）と永続描画（duration秒間表示）をサポート。
/// PrimitiveBatch3D をバックエンドとして使用する。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Math/Color.h"
#include <DirectXMath.h>

namespace gx
{

class PrimitiveBatch3D;

/// @brief 3Dデバッグ描画マネージャー
class DebugDraw3D
{
public:
    /// @brief シングルトン取得
    static DebugDraw3D& Instance();

    // --- 即時描画（1フレームのみ） ---

    /// @brief 線分を描画する
    void DrawLine(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to,
                  const Color& color = Color{ 1.0f, 1.0f, 1.0f, 1.0f });

    /// @brief ワイヤーボックスを描画する
    void DrawWireBox(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& extent,
                     const Color& color);

    /// @brief ワイヤー球を描画する
    void DrawWireSphere(const DirectX::XMFLOAT3& center, float radius,
                        const Color& color, int segments = 16);

    /// @brief 矢印を描画する
    void DrawArrow(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to,
                   const Color& color, float headSize = 0.1f);

    /// @brief 3Dテキストを描画する（位置のみ記録、PrimitiveBatch3Dには未対応）
    void DrawText3D(const DirectX::XMFLOAT3& worldPos, const std::string& text,
                    const Color& color);

    // --- 永続描画（duration秒間表示） ---

    /// @brief 永続線分を描画する
    void DrawLinePersistent(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to,
                            const Color& color, float duration);

    /// @brief 永続ワイヤーボックスを描画する
    void DrawWireBoxPersistent(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& extent,
                               const Color& color, float duration);

    // --- フレーム管理 ---

    /// @brief 蓄積したプリミティブを PrimitiveBatch3D に描画する
    /// @param batch 描画先のバッチ
    /// @param deltaTime 経過時間（永続描画の更新用）
    void Render(PrimitiveBatch3D& batch, float deltaTime);

    /// @brief 即時描画バッファをクリアする（フレーム終了時に呼ぶ）
    void Clear();

    /// @brief デバッグ描画の有効/無効を設定する
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /// @brief デバッグ描画が有効かどうか
    bool IsEnabled() const { return m_enabled; }

    /// @brief ビュー・プロジェクション行列を設定する（テキスト描画のスクリーン座標変換用）
    void SetViewProjection(const DirectX::XMMATRIX& viewProj);

    /// @brief 蓄積した3Dテキストをスクリーン座標に変換して描画する
    /// @param screenWidth スクリーン幅
    /// @param screenHeight スクリーン高さ
    void RenderText(float screenWidth, float screenHeight);

    /// @brief 即時描画の線分数を取得する
    size_t GetLineCount() const { return m_lines.size(); }

    /// @brief 永続描画の線分数を取得する
    size_t GetPersistentLineCount() const { return m_persistentLines.size(); }

    /// @brief テキストエントリ数を取得する
    size_t GetTextCount() const { return m_texts.size(); }

private:
    DebugDraw3D() = default;

    struct LineEntry
    {
        DirectX::XMFLOAT3 from, to;
        Color color;
    };

    struct PersistentLine
    {
        DirectX::XMFLOAT3 from, to;
        Color color;
        float remainingTime;
    };

    struct TextEntry
    {
        DirectX::XMFLOAT3 worldPos;
        std::string text;
        Color color;
    };

    bool m_enabled = true;                                 ///< デバッグ描画有効フラグ
    std::vector<LineEntry> m_lines;                        ///< 即時描画の線分バッファ
    std::vector<PersistentLine> m_persistentLines;         ///< 永続描画の線分バッファ
    std::vector<TextEntry> m_texts;                        ///< 3Dテキストバッファ
    DirectX::XMFLOAT4X4 m_viewProj = {};                  ///< ビュープロジェクション行列（テキスト座標変換用）
};

} // namespace gx
/// @}
