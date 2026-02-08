/// @file Fullscreen.hlsli
/// @brief フルスクリーン三角形の共通頂点シェーダー
///
/// SV_VertexIDから画面全体を覆う1枚の三角形を生成する。
/// 頂点バッファ不要（VBなしで Draw(3,1,0,0) するだけ）。

struct FullscreenVSOutput
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

FullscreenVSOutput FullscreenVS(uint id : SV_VertexID)
{
    FullscreenVSOutput o;
    // id=0: (0,0), id=1: (2,0), id=2: (0,2) → 画面全体を覆う三角形
    o.uv = float2((id << 1) & 2, id & 2);
    o.pos = float4(o.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return o;
}
