#pragma once
/// @file model_loader.h
/// @brief GXMDランタイムローダー (GPU非依存のCPU生データ)
///
/// .gxmdバイナリを読み込み、頂点・インデックス・マテリアル・スケルトン・
/// アニメーションをLoadedModel構造体に展開する。
/// GPUリソース作成はGxmdModelLoader (GXLib側) が担当する。

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "gxmd.h"
#include "shader_model.h"

namespace gxloader
{

/// @brief サブメッシュ1つ分の範囲情報
struct LoadedSubMesh
{
    uint32_t vertexOffset = 0;   ///< 統合頂点配列内の開始頂点
    uint32_t vertexCount  = 0;   ///< 頂点数
    uint32_t indexOffset   = 0;  ///< 統合インデックス配列内の開始インデックス
    uint32_t indexCount    = 0;  ///< インデックス数
    uint32_t materialIndex = 0;  ///< 対応マテリアルのインデックス
    float    aabbMin[3]    = {}; ///< バウンディングボックス最小値
    float    aabbMax[3]    = {}; ///< バウンディングボックス最大値
};

/// @brief 読み込み済みマテリアル
struct LoadedMaterial
{
    std::string            name;                                       ///< マテリアル名
    gxfmt::ShaderModel     shaderModel = gxfmt::ShaderModel::Standard; ///< シェーダーモデル
    gxfmt::ShaderModelParams params{};                                 ///< シェーダーパラメータ (256B)
    std::string            texturePaths[8]; ///< 文字列テーブルから解決済みのテクスチャパス
};

/// @brief 読み込み済みジョイント (ボーン)
struct LoadedJoint
{
    std::string name;                       ///< ボーン名
    int32_t     parentIndex = -1;           ///< 親ボーンインデックス (-1=ルート)
    float       inverseBindMatrix[16] = {   ///< 逆バインド行列 (行ベクトル4x4)
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    float       localTranslation[3] = {};   ///< バインドポーズのローカル平行移動
    float       localRotation[4]    = { 0, 0, 0, 1 }; ///< バインドポーズのローカル回転 (x,y,z,w)
    float       localScale[3]       = { 1, 1, 1 };    ///< バインドポーズのローカルスケール
};

/// @brief 読み込み済みアニメーションチャネル (ボーンインデックスベース)
struct LoadedAnimChannel
{
    uint32_t jointIndex = 0;    ///< 対象ジョイントのインデックス
    uint8_t  target     = 0;    ///< ターゲット (0=Translation, 1=Rotation, 2=Scale)
    uint8_t  interpolation = 0; ///< 補間方法 (0=Linear, 1=Step, 2=CubicSpline)
    std::vector<gxfmt::VectorKey> vecKeys;  ///< float3キーフレーム (T/S用)
    std::vector<gxfmt::QuatKey>   quatKeys; ///< クォータニオンキーフレーム (R用)
};

/// @brief 読み込み済みアニメーション
struct LoadedAnimation
{
    std::string name;      ///< アニメーション名
    float       duration = 0.0f; ///< 再生時間 (秒)
    std::vector<LoadedAnimChannel> channels; ///< チャネル一覧
};

/// @brief GXMDから読み込んだモデルデータ一式
/// @details GPU非依存のCPUデータ。頂点はstandardVerticesかskinnedVerticesのどちらかが使われる。
struct LoadedModel
{
    // 頂点データ (どちらか一方が使われる)
    std::vector<gxfmt::VertexStandard> standardVertices; ///< 標準頂点 (48B)
    std::vector<gxfmt::VertexSkinned>  skinnedVertices;  ///< スキニング頂点 (80B)
    bool isSkinned = false;  ///< trueならskinnedVerticesが有効

    // インデックスデータ (どちらか一方が使われる)
    std::vector<uint16_t> indices16; ///< 16bitインデックス
    std::vector<uint32_t> indices32; ///< 32bitインデックス
    bool uses16BitIndices = false;   ///< trueならindices16が有効

    std::vector<LoadedSubMesh> subMeshes;    ///< サブメッシュ一覧
    std::vector<LoadedMaterial> materials;    ///< マテリアル一覧
    std::vector<LoadedJoint> joints;          ///< スケルトンのジョイント一覧
    std::vector<LoadedAnimation> animations;  ///< アニメーション一覧
    uint32_t version = 0;  ///< ファイルバージョン
};

/// @brief ディスクからGXMDファイルを読み込む
/// @param filePath .gxmdファイルパス
/// @return 読み込み済みモデル。失敗時nullptr
std::unique_ptr<LoadedModel> LoadGxmd(const std::string& filePath);

/// @brief メモリバッファからGXMDを読み込む
/// @param data バッファ先頭ポインタ
/// @param size バッファサイズ
/// @return 読み込み済みモデル。失敗時nullptr
std::unique_ptr<LoadedModel> LoadGxmdFromMemory(const uint8_t* data, size_t size);

#ifdef _WIN32
/// @brief ワイド文字パスでGXMDファイルを読み込む (Windows非ASCIIパス対応)
/// @param filePath ワイド文字パス
/// @return 読み込み済みモデル。失敗時nullptr
std::unique_ptr<LoadedModel> LoadGxmdW(const std::wstring& filePath);
#endif

} // namespace gxloader
