#pragma once
/// @file LightingPanel.h
/// @brief シーンライティング編集パネル
///
/// Directional/Point/Spotライトの追加・削除・パラメータ編集と
/// アンビエントカラーの設定を行い、変更があればRenderer3Dに即時反映する。

#include <vector>
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Renderer3D.h"

/// @brief ライト（最大16個）とアンビエントカラーを編集するパネル
class LightingPanel
{
public:
    /// @brief デフォルトライト（ディレクショナル+ポイント）をセットアップする
    void Initialize();

    /// @brief ライティングパネルを独立ウィンドウとして描画する
    /// @param renderer ライト設定の反映先
    void Draw(GX::Renderer3D& renderer);

    /// @brief タブコンテナ内埋め込み用（Begin/Endなし）
    /// @param renderer ライト設定の反映先
    void DrawContent(GX::Renderer3D& renderer);

    /// @brief 現在のライトデータ配列を取得する
    /// @return ライトデータの読み取り専用参照
    const std::vector<GX::LightData>& GetLights() const { return m_lights; }

    /// @brief アンビエントカラーを取得する
    /// @return RGB float[3]へのポインタ
    const float* GetAmbientColor() const { return m_ambientColor; }

private:
    static constexpr int k_MaxLights = 16;  ///< ライト最大数

    std::vector<GX::LightData> m_lights;
    float m_ambientColor[3] = { 0.15f, 0.15f, 0.18f };
    bool  m_dirty = true;  ///< パラメータ変更時にtrueにし、次のDrawContentでRenderer3Dに反映
};
