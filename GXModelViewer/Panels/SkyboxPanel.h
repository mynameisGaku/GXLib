#pragma once
/// @file SkyboxPanel.h
/// @brief スカイボックス設定パネル
///
/// プロシージャルスカイ（上下グラデーション+太陽）のパラメータ編集と、
/// キューブマップ/HDR環境マップのプレースホルダーUIを提供する。

#include <string>

namespace GX { class Skybox; }

/// @brief スカイボックスのカラー・太陽・環境マップを編集するパネル
class SkyboxPanel
{
public:
    /// @brief スカイボックスパネルを独立ウィンドウとして描画する
    /// @param skybox 設定の反映先
    void Draw(GX::Skybox& skybox);

    /// @brief タブコンテナ内埋め込み用（Begin/Endなし）
    /// @param skybox 設定の反映先
    void DrawContent(GX::Skybox& skybox);

private:
    std::string m_cubemapFaces[6];     ///< キューブマップ6面パス（プレースホルダー）
    std::string m_hdrEnvMapPath;       ///< HDR環境マップパス（プレースホルダー）

    // Skyboxにgetterがないためパネル側でローカルコピーを保持
    float m_topColor[3]    = { 0.3f, 0.5f, 0.9f };    ///< 天頂色
    float m_bottomColor[3] = { 0.7f, 0.8f, 0.95f };   ///< 地平線色
    float m_sunDirection[3] = { 0.3f, -1.0f, 0.5f };   ///< 太陽方向
    float m_sunIntensity    = 5.0f;                     ///< 太陽強度
    float m_rotation        = 0.0f;                     ///< 回転角（未実装）
};
