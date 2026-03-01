#pragma once
/// @file ShaderGraph.h
/// @brief ビジュアルシェーダグラフエディタ — ノードを繋いでHLSLコードを自動生成する
///
/// テクスチャサンプル・定数・演算ノードをピンで接続し、最終的な Output ノードへ
/// 流れる計算式から HLSL コードを生成する。Unreal の Material Editor に近い概念。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace gx
{

/// @brief シェーダノード種別
enum class ShaderNodeType : uint32_t
{
    Output = 0,
    TextureSample,
    ConstFloat,
    ConstFloat2,
    ConstFloat3,
    ConstFloat4,
    ConstColor,
    Multiply,
    Add,
    Subtract,
    Divide,
    Dot,
    Cross,
    Normalize,
    Lerp,
    Clamp,
    Saturate,
    Power,
    SquareRoot,
    Abs,
    Frac,
    Floor,
    Ceil,
    Sin,
    Cos,
    Time,
    UV,
    WorldPos,
    WorldNormal,
    ViewDir,
    Fresnel,
    NormalMap,
    Parallax
};

/// @brief シェーダピン型
enum class ShaderPinType : uint32_t
{
    Float = 0,
    Float2,
    Float3,
    Float4,
    Texture2D,
    Sampler
};

/// @brief シェーダピン記述
struct ShaderPin
{
    std::string name;                               ///< ピン名
    ShaderPinType type  = ShaderPinType::Float;      ///< ピン型
    uint32_t nodeId     = 0;                         ///< 所属ノードID
    uint32_t pinIndex   = 0;                         ///< ピン番号
    bool isOutput       = false;                     ///< 出力ピンか（false=入力）
    float defaultValue[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; ///< 未接続時の既定値
};

/// @brief シェーダノード間接続
struct ShaderConnection
{
    uint32_t fromNode = 0; ///< 接続元ノードID
    uint32_t fromPin  = 0; ///< 接続元ピン番号
    uint32_t toNode   = 0; ///< 接続先ノードID
    uint32_t toPin    = 0; ///< 接続先ピン番号
};

/// @brief シェーダノード
struct ShaderNode
{
    uint32_t id         = 0;                         ///< ノードID
    ShaderNodeType type = ShaderNodeType::Output;    ///< ノード種別
    std::string name;                                ///< ノード名（表示用）
    float posX = 0.0f;                               ///< エディタ上のX座標
    float posY = 0.0f;                               ///< エディタ上のY座標
    std::vector<ShaderPin> pins;                     ///< ピン一覧（入力+出力）
    std::unordered_map<std::string, std::string> properties; ///< ノードプロパティ
};

/// @brief シェーダグラフデータ（シリアライズ用）
struct ShaderGraphData
{
    std::vector<ShaderNode> nodes;             ///< 全ノード
    std::vector<ShaderConnection> connections;  ///< 全接続
    std::string name;                           ///< グラフ名
    std::string targetModel = "PBR";            ///< ターゲットシェーダモデル
};

/// @brief HLSLコンパイル結果
struct CompileResult
{
    bool success = false;                   ///< コンパイル成功フラグ
    std::string hlslCode;                   ///< 生成されたHLSLコード
    std::vector<std::string> errors;        ///< エラーメッセージ一覧
    std::vector<std::string> warnings;      ///< 警告メッセージ一覧
};

/// @brief ノードベースビジュアルシェーダグラフエディタ
///
/// ノードの追加・接続・トポロジカルソート・HLSL コード生成を行う。
/// 構築時に Output ノード (id=0) が自動的に追加される。
class ShaderGraph
{
public:
    ShaderGraph();
    ~ShaderGraph() = default;

    // --- ノード操作 ---

    /// @brief ノードを追加する
    /// @return 新しいノードの ID
    uint32_t AddNode(ShaderNodeType type, const std::string& name = "",
                     float posX = 0.0f, float posY = 0.0f);

    /// @brief ノードを削除する (Output ノードは削除不可)
    bool RemoveNode(uint32_t nodeId);

    /// @brief ノードを取得する
    const ShaderNode* GetNode(uint32_t nodeId) const;

    /// @brief ノード数を返す
    uint32_t GetNodeCount() const { return static_cast<uint32_t>(m_nodes.size()); }

    // --- 接続 ---

    /// @brief ノード間を接続する（型互換チェック付き）
    bool Connect(uint32_t fromNode, uint32_t fromPin, uint32_t toNode, uint32_t toPin);

    /// @brief 接続を切断する
    bool Disconnect(uint32_t fromNode, uint32_t fromPin, uint32_t toNode, uint32_t toPin);

    /// @brief 接続数を返す
    uint32_t GetConnectionCount() const { return static_cast<uint32_t>(m_connections.size()); }

    // --- 検証 / コンパイル ---

    /// @brief サイクルが存在するかチェックする (DFS)
    bool HasCycle() const;

    /// @brief グラフをコンパイルして HLSL コードを生成する
    CompileResult Compile() const;

    /// @brief グラフを検証し、エラー一覧を返す
    std::vector<std::string> Validate() const;

    // --- ターゲットモデル ---
    void SetTargetModel(const std::string& model) { m_targetModel = model; }
    const std::string& GetTargetModel() const { return m_targetModel; }

    // --- シリアライズ ---
    bool Save(const std::string& filePath) const;
    bool Load(const std::string& filePath);

    // --- その他 ---
    void Clear();

    /// @brief Output ノードを取得する (常に存在)
    const ShaderNode* GetOutputNode() const;

    /// @brief 指定ノードのピン一覧を返す
    std::vector<ShaderPin> GetNodePins(uint32_t nodeId) const;

    /// @brief ノードプロパティを設定する
    void SetNodeProperty(uint32_t nodeId, const std::string& key, const std::string& value);

    /// @brief ノードプロパティを取得する
    std::string GetNodeProperty(uint32_t nodeId, const std::string& key) const;

private:
    /// @brief ノード種別に応じたデフォルトピンを生成する
    void CreateDefaultPins(ShaderNode& node) const;

    /// @brief 接続の型互換性を検証する
    bool ValidateConnection(uint32_t fromNode, uint32_t fromPin,
                             uint32_t toNode, uint32_t toPin) const;

    /// @brief トポロジカルソートを行う
    std::vector<uint32_t> TopologicalSort() const;

    /// @brief ノードの HLSL 変数宣言コードを生成する
    std::string GenerateHLSLForNode(const ShaderNode& node, uint32_t index) const;

    /// @brief ピン型名を HLSL 型文字列に変換する
    static std::string PinTypeToHLSL(ShaderPinType type);

    /// @brief 2つのピン型が接続互換か判定する
    static bool AreTypesCompatible(ShaderPinType from, ShaderPinType to);

    ShaderNode* FindNode(uint32_t nodeId);
    const ShaderNode* FindNodeConst(uint32_t nodeId) const;

    std::vector<ShaderNode>       m_nodes;          ///< ノード一覧
    std::vector<ShaderConnection> m_connections;    ///< 接続一覧
    std::string                   m_targetModel = "PBR"; ///< ターゲットシェーダモデル
    uint32_t                      m_nextNodeId  = 0;     ///< 次に割り当てるノードID
};

} // namespace gx
/// @}
