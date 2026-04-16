#pragma once
/// @file TerrainPanel.h
/// @brief テレイン編集パネル（プレースホルダー）
///
/// ハイトマップ読み込み、高さスケール、スプラットテクスチャ4層、LODレベルの
/// UIを用意している。実際のTerrainオブジェクトにはまだ接続されていない。

#include <string>

/// @brief テレイン設定のプレースホルダーUI（将来的にTerrainクラスと接続予定）
class TerrainPanel
{
public:
    /// @brief テレインパネルを描画する
    void Draw();

private:
    std::string m_heightmapPath;       ///< ハイトマップファイルパス
    float m_heightScale = 10.0f;       ///< 高さスケール倍率
    std::string m_splatTextures[4];    ///< スプラットマップのテクスチャ（4レイヤー）
    int m_lodLevel = 3;                ///< LODレベル（0=最低〜6=最高）
};
