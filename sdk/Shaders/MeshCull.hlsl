/// @file MeshCull.hlsl
/// @brief メッシュシェーダ + アンプリフィケーションシェーダ
///
/// Amplification Shader: メッシュレット単位のフラスタムカリング
/// Mesh Shader: 可視メッシュレットの頂点/プリミティブ出力
///
/// メッシュレットは最大64頂点・最大126三角形のメッシュ部分集合。
/// ASで可視判定し、MSで実際の頂点変換と出力を行う。

// --- 構造体定義 ---

struct Meshlet
{
    uint vertexCount;
    uint vertexOffset;
    uint primitiveCount;
    uint primitiveOffset;
};

struct MeshletBounds
{
    float3 center;
    float  radius;
    float3 coneAxis;    // 法線コーン軸
    float  coneCutoff;  // 法線コーンカットオフ (cos値)
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
};

// --- 定数バッファ ---

cbuffer SceneConstants : register(b0)
{
    float4x4 g_ViewProj;
    float4x4 g_World;
    float4   g_FrustumPlanes[6]; // フラスタムの6平面 (xyz=法線, w=距離)
    float3   g_CameraPosition;
    uint     g_MeshletCount;
};

// --- リソース ---

StructuredBuffer<Meshlet>       g_Meshlets       : register(t0);
StructuredBuffer<MeshletBounds> g_MeshletBounds  : register(t1);
ByteAddressBuffer               g_VertexBuffer   : register(t2);
ByteAddressBuffer               g_UniqueVertices : register(t3);
ByteAddressBuffer               g_PrimitiveIndices : register(t4);

// --- ペイロード (AS→MS間のデータ受け渡し) ---

struct AmplificationPayload
{
    uint meshletIndices[32]; // 可視メッシュレットのインデックス（グループあたり最大32）
};

groupshared AmplificationPayload s_Payload;

// --- フラスタムカリング判定 ---

bool IsMeshletVisible(uint meshletIndex)
{
    if (meshletIndex >= g_MeshletCount)
        return false;

    MeshletBounds bounds = g_MeshletBounds[meshletIndex];

    // ワールド空間に変換
    float3 worldCenter = mul(float4(bounds.center, 1.0), g_World).xyz;

    // フラスタムの各平面に対してバウンディングスフィアをテスト
    [unroll]
    for (uint i = 0; i < 6; ++i)
    {
        float dist = dot(g_FrustumPlanes[i].xyz, worldCenter) + g_FrustumPlanes[i].w;
        if (dist < -bounds.radius)
            return false; // 完全に平面の外側
    }

    // 法線コーンカリング (バックフェースメッシュレットの除去)
    if (bounds.coneCutoff < 1.0)
    {
        float3 coneAxisWorld = normalize(mul(float4(bounds.coneAxis, 0.0), g_World).xyz);
        float3 viewDir = normalize(worldCenter - g_CameraPosition);
        if (dot(viewDir, coneAxisWorld) > bounds.coneCutoff)
            return false; // 全プリミティブがバックフェース
    }

    return true;
}

// ============================================================
// Amplification Shader
// ============================================================
// 各スレッドが1つのメッシュレットの可視判定を行う。
// 可視メッシュレットのインデックスをペイロードに詰めてMSにディスパッチする。

[numthreads(32, 1, 1)]
void ASMain(
    uint gtid : SV_GroupThreadID,
    uint dtid : SV_DispatchThreadID,
    uint gid  : SV_GroupID)
{
    bool visible = IsMeshletVisible(dtid);

    // ウェーブ内で可視メッシュレットをコンパクトにまとめる
    // WavePrefixCountBitsで各スレッドの前に何個の可視メッシュレットがあるかを算出
    uint visibleCount = WaveActiveCountBits(visible);
    uint visibleIndex = WavePrefixCountBits(visible);

    if (visible)
    {
        s_Payload.meshletIndices[visibleIndex] = dtid;
    }

    // 可視メッシュレット数分のMesh Shaderスレッドグループをディスパッチ
    DispatchMesh(visibleCount, 1, 1, s_Payload);
}

// ============================================================
// Mesh Shader
// ============================================================
// ペイロードから受け取ったメッシュレットインデックスを使い、
// メッシュレットの頂点を変換してプリミティブを出力する。

#define MAX_VERTICES   64
#define MAX_PRIMITIVES 126

[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void MSMain(
    uint gtid : SV_GroupThreadID,
    uint gid  : SV_GroupID,
    in payload AmplificationPayload payload,
    out vertices VertexOutput verts[MAX_VERTICES],
    out indices uint3 tris[MAX_PRIMITIVES])
{
    uint meshletIndex = payload.meshletIndices[gid];
    Meshlet m = g_Meshlets[meshletIndex];

    // メッシュレットの頂点数とプリミティブ数を出力に設定
    SetMeshOutputCounts(m.vertexCount, m.primitiveCount);

    // 頂点処理: 各スレッドが1頂点を担当
    if (gtid < m.vertexCount)
    {
        // ユニーク頂点インデックスを取得
        uint vertexIndex = g_UniqueVertices.Load((m.vertexOffset + gtid) * 4);

        // 頂点データを読み込み (position: float3, normal: float3, uv: float2 = 32 bytes)
        uint byteOffset = vertexIndex * 32;
        float3 position = asfloat(g_VertexBuffer.Load3(byteOffset));
        float3 normal   = asfloat(g_VertexBuffer.Load3(byteOffset + 12));
        float2 uv       = asfloat(g_VertexBuffer.Load2(byteOffset + 24));

        // ワールド→クリップ空間変換
        float4 worldPos = mul(float4(position, 1.0), g_World);
        float4 clipPos  = mul(worldPos, g_ViewProj);

        VertexOutput vo;
        vo.position = clipPos;
        vo.normal   = normalize(mul(float4(normal, 0.0), g_World).xyz);
        vo.texcoord = uv;

        verts[gtid] = vo;
    }

    // プリミティブ処理: 各スレッドが1三角形を担当
    if (gtid < m.primitiveCount)
    {
        // プリミティブインデックスを10ビットパッキングで読み込み
        // 3インデックスが1つのuint32に格納: bits[0:9]=v0, [10:19]=v1, [20:29]=v2
        uint packed = g_PrimitiveIndices.Load((m.primitiveOffset + gtid) * 4);
        uint i0 = (packed >>  0) & 0x3FF;
        uint i1 = (packed >> 10) & 0x3FF;
        uint i2 = (packed >> 20) & 0x3FF;

        tris[gtid] = uint3(i0, i1, i2);
    }
}

// ============================================================
// Pixel Shader (基本的なPBR出力)
// ============================================================

float4 PSMain(VertexOutput input) : SV_Target
{
    // 簡易ライティング（完全な実装はPBR.hlslで行う）
    float3 lightDir = normalize(float3(0.3, -1.0, 0.5));
    float  ndotl    = saturate(dot(input.normal, -lightDir));
    float3 ambient  = float3(0.15, 0.15, 0.18);
    float3 color    = ambient + float3(0.8, 0.8, 0.8) * ndotl;

    return float4(color, 1.0);
}
