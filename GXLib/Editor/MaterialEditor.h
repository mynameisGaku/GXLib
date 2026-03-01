#pragma once
/// @file MaterialEditor.h
/// @brief マテリアルエディタ — ノードベースでシェーダーの見た目を編集する
///
/// テクスチャスロット・PBRパラメータ・シェーダーモデル（PBR/Toon/Unlit等）を
/// ノードグラフで組み合わせてマテリアルを作成・保存する。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace gx
{

/// @brief マテリアルエディタ用ノード種別
enum class MENodeType
{
    Output,         ///< 最終出力ノード
    TextureSample,  ///< テクスチャサンプリングノード
    ConstantFloat,  ///< 定数float値ノード
    ConstantFloat3, ///< 定数float3値ノード
    Multiply,       ///< 乗算ノード
    Add,            ///< 加算ノード
    Lerp,           ///< 線形補間ノード
    NormalMap       ///< 法線マップ変換ノード
};

/// @brief シェーダモデル種別
enum class ShaderModelType
{
    PBR,        ///< 物理ベースレンダリング
    Phong,      ///< Phongシェーディング
    Toon,       ///< トゥーン（セル）シェーディング
    ClearCoat,  ///< クリアコート（車の塗装等）
    Subsurface, ///< サブサーフェススキャタリング（肌等）
    Unlit       ///< ライティングなし
};

/// @brief テクスチャスロット情報
struct TextureSlotInfo
{
    std::string name;      ///< スロット名 ("Albedo", "Normal", etc.)
    std::string path;      ///< テクスチャパス
    uint32_t slotIndex = 0; ///< テクスチャレジスタスロット番号
};

/// @brief 編集可能パラメータ
struct MaterialParam
{
    std::string name;        ///< パラメータ名
    std::string category;   ///< カテゴリ ("Base", "PBR", "Emission" 等)
    enum class Type { Float, Float3, Bool, Texture } type = Type::Float; ///< パラメータの型
    float floatValue = 0.0f;                ///< float値
    float float3Value[3] = { 0, 0, 0 };    ///< float3値（色やベクトル）
    bool boolValue = false;                 ///< bool値
    std::string texturePath;                ///< テクスチャパス
};

/// @brief マテリアルプリセット
struct MaterialPreset
{
    std::string name;                                ///< プリセット名
    ShaderModelType shaderModel = ShaderModelType::PBR; ///< シェーダモデル
    std::vector<MaterialParam> params;               ///< パラメータリスト
};

/// @brief マテリアルエディタ用ノード（グラフ用）
struct MENode
{
    uint32_t id = 0;                                 ///< ノードID
    MENodeType type = MENodeType::Output;            ///< ノード種別
    std::string name;                                 ///< ノード名（表示用）
    float value = 0.0f;                               ///< float定数値
    float value3[3] = { 0, 0, 0 };                    ///< float3定数値
    std::string texturePath;                           ///< テクスチャパス（TextureSample時）
};

/// @brief マテリアルエディタ
class MaterialEditor
{
public:
    MaterialEditor() = default;
    ~MaterialEditor() = default;

    /// @brief マテリアル名を設定
    void SetMaterial(const std::string& materialName);

    /// @brief マテリアルをクリア
    void ClearMaterial();

    /// @brief 現在のマテリアル名取得
    const std::string& GetMaterialName() const { return m_materialName; }

    /// @brief マテリアルがセットされているか
    bool HasMaterial() const { return !m_materialName.empty(); }

    /// @brief シェーダモデルを設定
    void SetShaderModel(ShaderModelType model);

    /// @brief シェーダモデルを取得
    ShaderModelType GetShaderModel() const { return m_shaderModel; }

    // --- パラメータ編集 ---
    void SetBaseColor(float r, float g, float b);
    void GetBaseColor(float& r, float& g, float& b) const;

    void SetMetallic(float value);
    float GetMetallic() const { return m_metallic; }

    void SetRoughness(float value);
    float GetRoughness() const { return m_roughness; }

    void SetEmissiveColor(float r, float g, float b);
    void GetEmissiveColor(float& r, float& g, float& b) const;

    void SetEmissiveIntensity(float intensity);
    float GetEmissiveIntensity() const { return m_emissiveIntensity; }

    // --- テクスチャスロット ---
    void SetTextureSlot(const std::string& slotName, const std::string& path);
    std::string GetTextureSlot(const std::string& slotName) const;
    std::vector<TextureSlotInfo> GetTextureSlots() const;
    uint32_t GetTextureSlotCount() const { return static_cast<uint32_t>(m_textureSlots.size()); }

    // --- ノード操作 ---
    uint32_t AddNode(MENodeType type, const std::string& name = "");
    void RemoveNode(uint32_t nodeId);
    uint32_t GetNodeCount() const { return static_cast<uint32_t>(m_nodes.size()); }
    const MENode* GetNode(uint32_t nodeId) const;

    // --- 接続 ---
    void ConnectNodes(uint32_t srcNodeId, uint32_t srcPin, uint32_t dstNodeId, uint32_t dstPin);
    void DisconnectNodes(uint32_t srcNodeId, uint32_t srcPin, uint32_t dstNodeId, uint32_t dstPin);
    uint32_t GetConnectionCount() const { return static_cast<uint32_t>(m_connections.size()); }

    // --- パラメータリスト ---
    std::vector<MaterialParam> GetEditableParams() const;

    // --- プリセット ---
    MaterialPreset SaveAsPreset(const std::string& name) const;
    void LoadPreset(const MaterialPreset& preset);
    static std::vector<std::string> GetBuiltinPresetNames();

    // --- コールバック ---
    using ChangeCallback = std::function<void(const std::string& paramName)>;
    void SetOnChanged(ChangeCallback cb) { m_onChange = std::move(cb); }

    // --- ダーティフラグ ---
    bool IsDirty() const { return m_dirty; }
    void ClearDirty() { m_dirty = false; }

private:
    void NotifyChange(const std::string& paramName);

    std::string m_materialName;                          ///< 編集中のマテリアル名
    ShaderModelType m_shaderModel = ShaderModelType::PBR; ///< シェーダモデル

    float m_baseColor[3] = { 1.0f, 1.0f, 1.0f };        ///< ベースカラー（RGB）
    float m_metallic = 0.0f;                              ///< メタリック（0=非金属、1=金属）
    float m_roughness = 0.5f;                             ///< ラフネス（0=鏡面、1=粗面）
    float m_emissiveColor[3] = { 0.0f, 0.0f, 0.0f };     ///< 発光色（RGB）
    float m_emissiveIntensity = 0.0f;                     ///< 発光強度

    std::unordered_map<std::string, TextureSlotInfo> m_textureSlots; ///< テクスチャスロットマップ
    std::vector<MENode> m_nodes;                           ///< ノードグラフのノード一覧
    uint32_t m_nextNodeId = 0;                            ///< 次に割り当てるノードID

    struct NodeConnection
    {
        uint32_t srcNode, srcPin; ///< 接続元のノードIDとピン番号
        uint32_t dstNode, dstPin; ///< 接続先のノードIDとピン番号
    };
    std::vector<NodeConnection> m_connections;             ///< ノード間の接続リスト

    ChangeCallback m_onChange;                             ///< パラメータ変更時コールバック
    bool m_dirty = false;                                 ///< 未保存の変更があるか
};

} // namespace gx
/// @}
