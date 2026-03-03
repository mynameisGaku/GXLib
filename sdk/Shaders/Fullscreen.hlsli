// フルスクリーン三角形の共通ヘッダ
// ポストエフェクトなどの画面全体処理で使用する。
// VBなしでDraw(3,1,0,0)を呼ぶだけで画面を覆う三角形が生成される。

/// @file Fullscreen.hlsli
/// @brief フルスクリーン三角形の共通頂点シェーダー
///
/// SV_VertexIDから画面全体を覆う1枚の三角形を生成する。
/// 頂点バッファ不要（VBなしで Draw(3,1,0,0) するだけ）。

struct FullscreenVSOutput
{
    float4 pos : SV_Position; // クリップ空間座標
    float2 uv  : TEXCOORD;   // テクスチャUV [0,1]
};

/// @brief フルスクリーン三角形VS — SV_VertexIDからUVとクリップ座標を生成
FullscreenVSOutput FullscreenVS(uint id : SV_VertexID)
{
    FullscreenVSOutput o;
    // ビットトリックで3頂点のUVを生成: (0,0), (2,0), (0,2)
    // 画面を覆う巨大三角形（はみ出し部分はクリップされる）
    o.uv = float2((id << 1) & 2, id & 2);
    o.pos = float4(o.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return o;
}
