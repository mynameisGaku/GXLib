#pragma once
/// @file TerrainSculptor.h
/// @brief 地形スカルプトツール — ブラシでハイトマップを彫刻・ペイントする
///
/// Raise/Lower/Flatten/Smooth/Noise/Paint/Erode の各モードでブラシを当て、
/// ハイトマップデータを編集する。テクスチャペイントレイヤーにも対応。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include <vector>
#include <utility>
#include <cstdint>
#include <string>

namespace gx
{

/// @brief ブラシ形状
enum class BrushType : uint32_t
{
    Circle = 0, ///< 円形ブラシ
    Square,     ///< 矩形ブラシ
    Diamond,    ///< ダイヤモンド形ブラシ
    Gaussian    ///< ガウシアン（滑らかな減衰）ブラシ
};

/// @brief スカルプトモード
enum class SculptMode : uint32_t
{
    Raise = 0, ///< 隆起（地形を持ち上げる）
    Lower,     ///< 陥没（地形を掘り下げる）
    Flatten,   ///< 平坦化（指定高さに均す）
    Smooth,    ///< 平滑化（凸凹をなだらかにする）
    Noise,     ///< ノイズ（ランダムな凹凸を加える）
    Paint,     ///< テクスチャペイント
    Erode      ///< 侵食（自然な浸食効果を加える）
};

/// @brief ブラシ設定
struct BrushSettings
{
    float     radius   = 5.0f;              ///< ブラシ半径（セル単位）
    float     strength = 0.5f;              ///< ブラシ強度（0.0～1.0）
    float     falloff  = 0.5f;              ///< 減衰率（0=均一、1=端で0）
    float     opacity  = 1.0f;              ///< 不透明度（0.0～1.0）
    BrushType type     = BrushType::Circle; ///< ブラシ形状
};

/// @brief ペイントレイヤー
struct PaintLayer
{
    uint32_t    layerIndex = 0;  ///< レイヤーインデックス
    int         textureId  = -1; ///< テクスチャハンドル（-1=未設定）
    std::string name;            ///< レイヤー名
};

/// @brief スカルプトストローク（一連のブラシ適用記録）
struct SculptStroke
{
    SculptMode mode;                                ///< 使用されたスカルプトモード
    std::vector<std::pair<float, float>> positions; ///< ブラシ適用座標リスト (x, y)
    BrushSettings brush;                            ///< ストローク時のブラシ設定
    uint64_t timestamp = 0;                         ///< ストロークのタイムスタンプ
};

/// @brief スカルプトアクション（Undo用）
struct SculptAction
{
    SculptMode mode = SculptMode::Raise;              ///< 操作モード
    std::vector<std::pair<int, int>> affectedCells;    ///< 影響を受けたセル座標 (col, row)
    std::vector<float> oldValues;                      ///< 変更前の値
    std::vector<float> newValues;                      ///< 変更後の値
};

/// @brief 地形スカルプトツール
///
/// ハイトマップデータに対してブラシベースの編集操作を行う。
/// Raise/Lower/Flatten/Smooth/Noise/Paint/Erode モードをサポートし、
/// 操作履歴 (Undo) を管理する。
class TerrainSculptor
{
public:
    TerrainSculptor() = default;
    ~TerrainSculptor() = default;

    // --- ブラシ ---
    void SetBrush(const BrushSettings& settings) { m_brush = settings; }
    const BrushSettings& GetBrush() const { return m_brush; }

    // --- モード ---
    void SetMode(SculptMode mode) { m_mode = mode; }
    SculptMode GetMode() const { return m_mode; }

    // --- ターゲットハイトマップ ---

    /// @brief 編集対象のハイトマップデータを設定する
    /// @param data  float 配列 (width * height)、書き込み可
    /// @param width  ハイトマップ幅
    /// @param height ハイトマップ高さ
    void SetTargetHeightmap(float* data, int width, int height);

    int GetTargetWidth() const { return m_width; }
    int GetTargetHeight() const { return m_height; }
    bool HasTarget() const { return m_heightmap != nullptr; }
    void ClearTarget();

    // --- ブラシ適用 ---

    /// @brief 指定位置にブラシを適用し、変更をSculptActionとして返す
    SculptAction ApplyBrush(float centerX, float centerY);

    /// @brief ストローク全体を適用する
    void ApplyStroke(const SculptStroke& stroke);

    /// @brief ブラシマスク（各セルの位置と重み）を取得する
    std::vector<std::pair<std::pair<int, int>, float>> GetBrushMask(float centerX, float centerY) const;

    // --- Flatten ---
    void SetFlattenHeight(float h) { m_flattenHeight = h; }
    float GetFlattenHeight() const { return m_flattenHeight; }

    // --- Paint ---
    void SetPaintLayer(const PaintLayer& layer) { m_paintLayer = layer; }
    const PaintLayer& GetPaintLayer() const { return m_paintLayer; }

    // --- Undo ---
    void Undo(const SculptAction& action);
    bool CanUndo() const { return !m_history.empty(); }
    uint32_t GetHistoryCount() const { return static_cast<uint32_t>(m_history.size()); }
    void ClearHistory() { m_history.clear(); }

    // --- ユーティリティ ---

    /// @brief ハイトマップから法線を再計算する
    static std::vector<DirectX::XMFLOAT3> ComputeNormals(const float* heightmap, int width, int height);

    /// @brief ハイトマップからバイリニアサンプリングする
    float SampleHeight(float x, float y) const;

    /// @brief ブラシ形状に基づく重みを評価する (0~1)
    static float EvaluateBrush(float distance, const BrushSettings& settings);

private:
    BrushSettings  m_brush;                           ///< 現在のブラシ設定
    SculptMode     m_mode         = SculptMode::Raise; ///< 現在のスカルプトモード
    float*         m_heightmap    = nullptr;           ///< 編集対象のハイトマップデータ
    int            m_width        = 0;                 ///< ハイトマップ幅
    int            m_height       = 0;                 ///< ハイトマップ高さ
    float          m_flattenHeight = 0.0f;             ///< Flattenモードの目標高さ
    PaintLayer     m_paintLayer;                       ///< 現在のペイントレイヤー
    std::vector<SculptAction> m_history;               ///< 操作履歴（Undo用）
};

} // namespace gx
/// @}
