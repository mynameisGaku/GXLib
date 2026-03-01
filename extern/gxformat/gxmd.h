#pragma once
/// @file gxmd.h
/// @brief GXMDバイナリモデル形式の定義
///
/// .gxmdファイルはメッシュ・マテリアル・スケルトン・アニメーションを
/// 単一バイナリにまとめた3Dモデル形式。gxconvで生成し、gxloaderで読み込む。

#include "types.h"
#include "shader_model.h"

namespace gxfmt
{

// ============================================================
// 定数
// ============================================================

static constexpr uint32_t k_GxmdMagic   = 0x444D5847; ///< ファイル識別子 'GXMD'
static constexpr uint32_t k_GxmdVersion = 2;           ///< 現在のフォーマットバージョン

// ============================================================
// 頂点フォーマットフラグ
// ============================================================

/// @brief 頂点に含まれる属性をビットフラグで表す
enum VertexFormat : uint32_t
{
    VF_Position  = 1 << 0,  ///< 位置 (float3)
    VF_Normal    = 1 << 1,  ///< 法線 (float3)
    VF_UV0       = 1 << 2,  ///< テクスチャ座標0 (float2)
    VF_Tangent   = 1 << 3,  ///< 接線 (float4, w=従法線の符号)
    VF_Joints    = 1 << 4,  ///< ボーンインデックス (uint32x4)
    VF_Weights   = 1 << 5,  ///< ボーンウェイト (float4)
    VF_UV1       = 1 << 6,  ///< テクスチャ座標1 (float2)
    VF_Color     = 1 << 7,  ///< 頂点カラー

    VF_Standard  = VF_Position | VF_Normal | VF_UV0 | VF_Tangent, ///< 標準頂点 (48B)
    VF_Skinned   = VF_Standard | VF_Joints | VF_Weights,          ///< スキニング頂点 (80B)
};

/// @brief インデックスバッファのフォーマット
enum class IndexFormat : uint8_t
{
    UInt16 = 0,  ///< 16bit (65535頂点まで)
    UInt32 = 1,  ///< 32bit
};

/// @brief プリミティブトポロジー
enum class PrimitiveTopology : uint8_t
{
    TriangleList = 0,   ///< 三角形リスト
    TriangleStrip = 1,  ///< 三角形ストリップ
};

// ============================================================
// 頂点構造体
// ============================================================

/// @brief 標準頂点 (48B) — GX::Vertex3D_PBRとバイナリ互換
struct VertexStandard
{
    float    position[3];  ///< 位置 (xyz)
    float    normal[3];    ///< 法線 (xyz)
    float    uv0[2];       ///< テクスチャ座標 (uv)
    float    tangent[4];   ///< 接線 (xyzw), w=従法線の符号
};

static_assert(sizeof(VertexStandard) == 48, "VertexStandard must be 48 bytes");

/// @brief スキニング頂点 (80B) — GX::Vertex3D_Skinnedとバイナリ互換
struct VertexSkinned
{
    float    position[3];  ///< 位置 (xyz)
    float    normal[3];    ///< 法線 (xyz)
    float    uv0[2];       ///< テクスチャ座標 (uv)
    float    tangent[4];   ///< 接線 (xyzw), w=従法線の符号
    uint32_t joints[4];    ///< ボーンインデックス (最大4影響)
    float    weights[4];   ///< ボーンウェイト (合計1.0に正規化済み)
};

static_assert(sizeof(VertexSkinned) == 80, "VertexSkinned must be 80 bytes");

// ============================================================
// ファイルヘッダ (128B)
// ============================================================

/// @brief GXMDファイルの先頭に配置されるヘッダ (128B固定)
/// @details 各データチャンクへのオフセットとサイズ情報を保持する。
struct FileHeader
{
    uint32_t magic;               ///< ファイル識別子 0x444D5847 ('GXMD')
    uint32_t version;             ///< フォーマットバージョン (現在2)
    uint32_t flags;               ///< 予約フラグ
    uint32_t meshCount;           ///< メッシュ数
    uint32_t materialCount;       ///< マテリアル数
    uint32_t boneCount;           ///< ボーン数 (0=スケルトンなし)
    uint32_t animationCount;      ///< アニメーション数
    uint32_t blendShapeCount;     ///< ブレンドシェイプ数

    uint64_t stringTableOffset;   ///< 文字列テーブルのファイル先頭からのオフセット
    uint64_t meshChunkOffset;     ///< MeshChunk配列のオフセット
    uint64_t materialChunkOffset; ///< MaterialChunk配列のオフセット
    uint64_t vertexDataOffset;    ///< 頂点データブロックのオフセット
    uint64_t indexDataOffset;     ///< インデックスデータブロックのオフセット
    uint64_t boneDataOffset;      ///< BoneData配列のオフセット
    uint64_t animationDataOffset; ///< アニメーションデータのオフセット
    uint64_t blendShapeDataOffset;///< ブレンドシェイプデータのオフセット

    uint32_t stringTableSize;     ///< 文字列テーブルのバイト数
    uint32_t vertexDataSize;      ///< 頂点データのバイト数
    uint32_t indexDataSize;       ///< インデックスデータのバイト数
    uint8_t  _reserved[20];       ///< 128Bパディング用予約
};

static_assert(sizeof(FileHeader) == 128, "FileHeader must be 128 bytes");

// ============================================================
// チャンク
// ============================================================

/// @brief メッシュ1つ分のメタ情報
/// @details 頂点/インデックスデータブロック内のオフセットとサイズ、AABB等を保持する。
struct MeshChunk
{
    uint32_t nameIndex;           ///< メッシュ名 (文字列テーブルのバイトオフセット)
    uint32_t vertexCount;         ///< 頂点数
    uint32_t indexCount;          ///< インデックス数
    uint32_t materialIndex;       ///< 対応するMaterialChunkのインデックス
    uint32_t vertexFormatFlags;   ///< 頂点属性のビットマスク (VertexFormat)
    uint32_t vertexStride;        ///< 1頂点あたりのバイト数
    uint64_t vertexOffset;        ///< 頂点データブロック内のバイトオフセット
    uint64_t indexOffset;         ///< インデックスデータブロック内のバイトオフセット
    IndexFormat     indexFormat;   ///< インデックスフォーマット (16bit/32bit)
    PrimitiveTopology topology;   ///< プリミティブトポロジー
    uint8_t  _pad[2];
    float    aabbMin[3];          ///< バウンディングボックス最小値
    float    aabbMax[3];          ///< バウンディングボックス最大値
};

/// @brief マテリアル1つ分の定義
struct MaterialChunk
{
    uint32_t          nameIndex;     ///< マテリアル名 (文字列テーブルのバイトオフセット)
    ShaderModel       shaderModel;   ///< シェーダーモデル種別
    ShaderModelParams params;        ///< シェーダーパラメータ (256B)
};

// ============================================================
// スケルトン
// ============================================================

/// @brief ボーン1つ分のデータ
/// @details 逆バインド行列とローカルTRS (バインドポーズ) を保持する。
struct BoneData
{
    uint32_t nameIndex;              ///< ボーン名 (文字列テーブルのバイトオフセット)
    int32_t  parentIndex;            ///< 親ボーンのインデックス (-1=ルート)
    float    inverseBindMatrix[16];  ///< 逆バインド行列 (行ベクトル=DirectXMath互換)
    float    localTranslation[3];    ///< ローカル平行移動
    float    localRotation[4];       ///< ローカル回転 (クォータニオン x,y,z,w)
    float    localScale[3];          ///< ローカルスケール
};

// ============================================================
// アニメーション (GXMD内埋め込み)
// ============================================================

/// @brief アニメーションチャネルのターゲット種別
enum class AnimChannelTarget : uint8_t
{
    Translation = 0,  ///< 平行移動
    Rotation    = 1,  ///< 回転
    Scale       = 2,  ///< スケール
};

/// @brief アニメーション1つ分のヘッダ
struct AnimationChunk
{
    uint32_t nameIndex;         ///< アニメーション名 (文字列テーブルのバイトオフセット)
    float    duration;          ///< 再生時間 (秒)
    uint32_t channelCount;      ///< チャネル数
    uint32_t _pad;
};

/// @brief アニメーションチャネルの記述子
/// @details 1チャネル = 1ボーンの1プロパティ (T/R/S) に対するキーフレーム列
struct AnimationChannelDesc
{
    uint32_t        boneIndex;      ///< 対象ボーンのインデックス (BoneData配列のインデックス)
    AnimChannelTarget target;       ///< ターゲット種別 (T/R/S)
    uint8_t         interpolation;  ///< 補間方法 (0=Linear, 1=Step, 2=CubicSpline)
    uint8_t         _pad[2];
    uint32_t        keyCount;       ///< キーフレーム数
    uint32_t        dataOffset;     ///< キーデータへのバイトオフセット (アニメーションデータ先頭から)
};

/// @brief float3キーフレーム (平行移動/スケール用)
struct VectorKey
{
    float time;       ///< 時刻 (秒)
    float value[3];   ///< 値 (xyz)
};

/// @brief クォータニオンキーフレーム (回転用)
struct QuatKey
{
    float time;       ///< 時刻 (秒)
    float value[4];   ///< 値 (x,y,z,w)
};

// ============================================================
// ブレンドシェイプ
// ============================================================

/// @brief ブレンドシェイプターゲット1つ分のメタ情報
struct BlendShapeTarget
{
    uint32_t nameIndex;         ///< ターゲット名 (文字列テーブルのバイトオフセット)
    uint32_t meshIndex;         ///< 対象メッシュのインデックス
    uint32_t deltaCount;        ///< デルタ頂点数
    uint32_t deltaOffset;       ///< デルタデータへのバイトオフセット (ブレンドシェイプデータ先頭から)
};

/// @brief ブレンドシェイプ1頂点分の差分
struct BlendShapeDelta
{
    uint32_t vertexIndex;       ///< 元メッシュの頂点インデックス
    float    positionDelta[3];  ///< 位置の差分 (xyz)
    float    normalDelta[3];    ///< 法線の差分 (xyz)
};

// ============================================================
// 文字列テーブル
// ============================================================

/// @brief 文字列テーブルの構造:
///   先頭4B = uint32_t byteCount (テーブル全体のバイト数)
///   以降 = null終端UTF-8文字列の連結
///   各nameIndexはbyteCountの直後からのバイトオフセット
static constexpr uint32_t k_InvalidStringIndex = 0xFFFFFFFF; ///< 文字列未設定を示す値

// ============================================================
// Binary layout:
//   [FileHeader 128B]
//   [StringTable: uint32_t byteCount + UTF-8 strings]
//   [MeshChunk x meshCount]
//   [MaterialChunk x materialCount]
//   [VertexData (contiguous)]
//   [IndexData (uint16 or uint32)]
//   [BoneData x boneCount]        (if boneCount > 0)
//   [AnimationData]                (if animationCount > 0)
//   [BlendShapeData]               (if blendShapeCount > 0)
// ============================================================

} // namespace gxfmt
