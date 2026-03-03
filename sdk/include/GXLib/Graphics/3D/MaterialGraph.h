#pragma once
/// @file MaterialGraph.h
/// @brief ノードベースマテリアルグラフ（DAG → Material コンパイル）
///
/// ノードグラフを構築し、トポロジカルソートでシェーダー定数や
/// テクスチャスロットを決定する。サイクル検出付き。

#include "pch_graphics.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <sstream>

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

/// @brief マテリアルノードタイプ
enum class MaterialNodeType
{
    Float,      ///< スカラー定数
    Vector2,    ///< 2Dベクトル
    Vector3,    ///< 3Dベクトル
    Vector4,    ///< 4Dベクトル
    Texture,    ///< テクスチャサンプル
    Add,        ///< 加算
    Subtract,   ///< 減算
    Multiply,   ///< 乗算
    Divide,     ///< 除算
    Lerp,       ///< 線形補間
    Clamp,      ///< クランプ
    Dot,        ///< 内積
    Normalize,  ///< 正規化
    Output      ///< 出力ノード
};

/// @brief ピンの値タイプ
enum class PinType
{
    Float,    ///< スカラー
    Vec2,     ///< 2Dベクトル
    Vec3,     ///< 3Dベクトル
    Vec4,     ///< 4Dベクトル
    Texture   ///< テクスチャ
};

/// @brief マテリアルノードのピン（入力/出力端子）
struct MaterialNodePin
{
    gx::String name;                       ///< ピン名
    PinType type = PinType::Float;          ///< 値タイプ
    bool connected = false;                 ///< 接続済みか
    int connectedNodeId = -1;               ///< 接続先ノードID
    int connectedPinIndex = -1;             ///< 接続先ピンインデックス
    struct { float x = 0, y = 0, z = 0, w = 0; } defaultValue; ///< デフォルト値
};

/// @brief マテリアルグラフのノード
class MaterialNode
{
public:
    int id = -1;                            ///< ノードID
    MaterialNodeType type;                  ///< ノードタイプ
    gx::String name;                       ///< ノード名
    gx::Vector<MaterialNodePin> inputs;    ///< 入力ピン列
    gx::Vector<MaterialNodePin> outputs;   ///< 出力ピン列
    struct { float x = 0, y = 0; } position; ///< エディタ上の位置

    MaterialNode() : type(MaterialNodeType::Float) {}
    MaterialNode(int nodeId, MaterialNodeType nodeType, const gx::String& nodeName)
        : id(nodeId), type(nodeType), name(nodeName) {}
};

/// @brief マテリアルグラフコンパイル結果
struct MaterialGraphResult
{
    bool success = false;                                   ///< コンパイル成功
    gx::String error;                                      ///< エラーメッセージ
    gx::HashMap<gx::String, gx::Array<float, 4>> constants;  ///< 定数値
    gx::Vector<gx::String> textureSlots;                  ///< テクスチャスロット名
};

/// @brief ノードベースマテリアルグラフ
///
/// マテリアルをノード接続で構築し、DAGとしてコンパイルする。
/// サイクル検出、トポロジカルソート、JSON直列化に対応。
class MaterialGraph
{
public:
    MaterialGraph() = default;
    ~MaterialGraph() = default;

    /// @brief ノードを追加
    /// @param type ノードタイプ
    /// @param name ノード名
    /// @return ノードID
    int AddNode(MaterialNodeType type, const gx::String& name);

    /// @brief ノードを削除
    void RemoveNode(int id);

    /// @brief ピンを接続
    /// @return 接続成功ならtrue
    bool Connect(int srcNodeId, int srcPinIndex, int dstNodeId, int dstPinIndex);

    /// @brief ピンの接続を解除
    void Disconnect(int dstNodeId, int dstPinIndex);

    /// @brief ノードを取得
    MaterialNode* GetNode(int id);

    /// @brief ノード数を取得
    uint32_t GetNodeCount() const { return static_cast<uint32_t>(m_nodes.size()); }

    /// @brief トポロジカルソート（ノードID列）
    gx::Vector<int> TopologicalSort() const;

    /// @brief サイクル検出
    bool HasCycle() const;

    /// @brief グラフをコンパイル
    MaterialGraphResult Compile() const;

    /// @brief グラフをクリア
    void Clear();

    /// @brief JSONシリアライズ
    gx::String SerializeToJSON() const;

    /// @brief JSONデシリアライズ
    bool DeserializeFromJSON(const gx::String& json);

    /// @brief 出力ノードを設定
    void SetOutputNode(int id) { m_outputNodeId = id; }

    /// @brief 出力ノードIDを取得
    int GetOutputNode() const { return m_outputNodeId; }

private:
    /// @brief DFSサイクル検出ヘルパー
    bool DFSHasCycle(int nodeId,
                     gx::HashMap<int, int>& colorMap) const;

    /// @brief DFSトポロジカルソートヘルパー
    void DFSTopSort(int nodeId,
                    gx::HashMap<int, int>& colorMap,
                    gx::Vector<int>& result) const;

    /// @brief ノードタイプに応じたピンを初期化
    void InitializePins(MaterialNode& node) const;

    gx::HashMap<int, std::unique_ptr<MaterialNode>> m_nodes; ///< ノードマップ
    int m_nextId = 0;               ///< 次のノードID
    int m_outputNodeId = -1;        ///< 出力ノードID
};

/// @}
} // namespace gx
