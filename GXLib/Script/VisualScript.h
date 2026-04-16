#pragma once
/// @file VisualScript.h
/// @brief ビジュアルスクリプティングシステム（Blueprint風）
///
/// ノードベースのビジュアルスクリプティング環境を提供する。
/// 実行フローピンとデータピンによるノード間接続、
/// サイクル検出、変数管理、組み込みノードタイプ登録、
/// Save/Loadによるグラフの永続化をサポートする。
/// @addtogroup grp_script/// @{

#include "pch_common.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace gx
{

/// @brief ノードカテゴリ
enum class VSNodeCategory
{
    Flow,       ///< フロー制御
    Math,       ///< 数学演算
    Logic,      ///< 論理演算
    String,     ///< 文字列操作
    Entity,     ///< エンティティ操作
    Component,  ///< コンポーネント操作
    Physics,    ///< 物理
    Audio,      ///< オーディオ
    Input,      ///< 入力
    Event,      ///< イベント
    Variable,   ///< 変数
    Custom      ///< カスタム
};

/// @brief ピン型
enum class VSPinType
{
    Exec,       ///< 実行フロー
    Bool,       ///< 真偽値
    Int,        ///< 整数
    Float,      ///< 浮動小数点
    String,     ///< 文字列
    Vector2,    ///< 2Dベクトル
    Vector3,    ///< 3Dベクトル
    Color,      ///< 色
    Entity,     ///< エンティティ
    Any         ///< 任意型
};

/// @brief ピン定義
struct VSPin
{
    gx::String name;                       ///< ピン名
    VSPinType type = VSPinType::Any;        ///< ピン型
    uint32_t nodeId = 0;                    ///< 所属ノードID
    uint32_t pinIndex = 0;                  ///< ピン番号
    bool isOutput = false;                  ///< 出力ピンか（false=入力）
    gx::String defaultValue;               ///< 未接続時の既定値
    bool connected = false;                 ///< 接続済みフラグ
};

/// @brief ノード間接続
struct VSConnection
{
    uint32_t fromNode = 0; ///< 接続元ノードID
    uint32_t fromPin = 0;  ///< 接続元ピン番号
    uint32_t toNode = 0;   ///< 接続先ノードID
    uint32_t toPin = 0;    ///< 接続先ピン番号
};

/// @brief ノード型定義
struct VSNodeDef
{
    gx::String typeName;                           ///< ノード型名（一意識別子）
    VSNodeCategory category = VSNodeCategory::Custom; ///< カテゴリ
    gx::String displayName;                         ///< エディタ上の表示名
    gx::Vector<VSPin> inputPins;                    ///< 入力ピン定義リスト
    gx::Vector<VSPin> outputPins;                   ///< 出力ピン定義リスト
    bool isEvent = false;                            ///< イベントノードか
    bool isPure = false; ///< trueならExecピン不要（副作用なし）
};

/// @brief 変数定義
struct VSVariable
{
    gx::String name;                   ///< 変数名
    VSPinType type = VSPinType::Any;    ///< 変数型
    gx::String value;                  ///< 現在の値（文字列表現）
    bool isPublic = false;              ///< 外部から参照可能か
};

/// @brief ノードインスタンス
struct VSNodeInstance
{
    uint32_t id = 0;                   ///< ノードID
    gx::String typeName;              ///< ノード型名
    float posX = 0.0f;                ///< エディタ上のX座標
    float posY = 0.0f;                ///< エディタ上のY座標
    gx::HashMap<gx::String, gx::String> properties; ///< ノードプロパティ
};

/// @brief 実行コンテキスト
struct VSExecutionContext
{
    gx::HashMap<gx::String, gx::String> variables; ///< ランタイム変数テーブル
    uint32_t executionCount = 0;  ///< 実行されたノード数
    gx::String lastError;        ///< 最後のエラーメッセージ
    gx::Vector<uint32_t> callStack; ///< ノードID列
};

/// @brief ビジュアルスクリプティングシステム
class VisualScript
{
public:
    VisualScript() = default;
    ~VisualScript() = default;

    /// @brief ノード型を登録する
    void RegisterNodeType(const VSNodeDef& def);

    /// @brief 登録済みノード型の数を取得する
    size_t GetRegisteredTypeCount() const { return m_nodeTypes.size(); }

    /// @brief ノード型定義を取得する
    const VSNodeDef* GetNodeType(const gx::String& typeName) const;

    /// @brief ノード型が存在するか判定する
    bool HasNodeType(const gx::String& typeName) const;

    /// @brief ノードを追加する
    /// @return ノードID（失敗時は0xFFFFFFFF）
    uint32_t AddNode(const gx::String& typeName, float posX = 0.0f, float posY = 0.0f);

    /// @brief ノードを削除する
    bool RemoveNode(uint32_t nodeId);

    /// @brief ノードを取得する
    const VSNodeInstance* GetNode(uint32_t nodeId) const;

    /// @brief ノード数を取得する
    size_t GetNodeCount() const { return m_nodes.size(); }

    /// @brief ピンを接続する
    bool Connect(uint32_t fromNode, uint32_t fromPin, uint32_t toNode, uint32_t toPin);

    /// @brief ピンの接続を切断する
    bool Disconnect(uint32_t fromNode, uint32_t fromPin, uint32_t toNode, uint32_t toPin);

    /// @brief 接続数を取得する
    size_t GetConnectionCount() const { return m_connections.size(); }

    /// @brief 全接続を取得する
    gx::Vector<VSConnection> GetConnections() const { return m_connections; }

    /// @brief 変数を追加する
    bool AddVariable(const gx::String& name, VSPinType type, const gx::String& defaultValue = "");

    /// @brief 変数を削除する
    bool RemoveVariable(const gx::String& name);

    /// @brief 変数値を設定する
    void SetVariable(const gx::String& name, const gx::String& value);

    /// @brief 変数値を取得する
    gx::String GetVariable(const gx::String& name) const;

    /// @brief 変数数を取得する
    size_t GetVariableCount() const { return m_variables.size(); }

    /// @brief 変数が存在するか判定する
    bool HasVariable(const gx::String& name) const;

    /// @brief 変数型を取得する
    VSPinType GetVariableType(const gx::String& name) const;

    /// @brief グラフを検証する
    /// @return エラーメッセージのリスト（空なら問題なし）
    gx::Vector<gx::String> Validate() const;

    /// @brief サイクルが存在するか判定する
    bool HasCycle() const;

    /// @brief グラフを実行する
    /// @param entryNodeId 開始ノードID
    /// @param context 実行コンテキスト
    /// @return 成功ならtrue
    bool Execute(uint32_t entryNodeId, VSExecutionContext& context);

    /// @brief ノードハンドラを設定する
    using NodeHandler = std::function<bool(VSNodeInstance&, VSExecutionContext&)>;
    void SetNodeHandler(const gx::String& typeName, NodeHandler handler);

    /// @brief ファイルに保存する
    bool Save(const gx::String& filePath) const;

    /// @brief ファイルから読み込む
    bool Load(const gx::String& filePath);

    /// @brief 全データをクリアする
    void Clear();

    /// @brief エントリーノード（Exec入力のないイベントノード）を取得する
    gx::Vector<uint32_t> GetEntryNodes() const;

    /// @brief 組み込みノード型を登録する
    void RegisterBuiltinTypes();

private:
    bool HasCycleDFS(uint32_t nodeId,
                     gx::HashMap<uint32_t, int>& visited) const;
    gx::Vector<uint32_t> GetExecOutputConnections(uint32_t nodeId, uint32_t pinIndex) const;
    void ResolveInputs(uint32_t nodeId, VSExecutionContext& context);

    gx::HashMap<gx::String, VSNodeDef> m_nodeTypes;      ///< 登録済みノード型定義
    gx::HashMap<uint32_t, VSNodeInstance> m_nodes;       ///< ノードインスタンスマップ
    gx::Vector<VSConnection> m_connections;                     ///< 接続リスト
    gx::HashMap<gx::String, VSVariable> m_variables;    ///< 変数マップ
    gx::HashMap<gx::String, NodeHandler> m_nodeHandlers; ///< ノード実行ハンドラ
    uint32_t m_nextNodeId = 1;                                   ///< 次に割り当てるノードID
};

} // namespace gx
/// @}
