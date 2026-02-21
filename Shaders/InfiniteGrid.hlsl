// Y=0平面上に無限に広がるグリッドをシェーダーだけで描画する。
// GXModelViewerのシーンエディタで使用。フルスクリーン三角形+レイ平面交差方式。

/// @file InfiniteGrid.hlsl
/// @brief 無限グリッドシェーダー（Y=0平面）
///
/// フルスクリーン三角形のPS内でレイ-平面交差を計算してグリッドを生成。
/// マイナーグリッド(1m)、メジャーグリッド(10m)、軸の色分け(X=赤, Z=青)、
/// 距離フェード、正確な深度出力に対応。

#include "Fullscreen.hlsli"

cbuffer GridCB : register(b0)
{
    float4x4 gViewProjectionInverse; // 逆VP行列（レイ生成用）
    float4x4 gViewProjection;        // VP行列（深度値計算用）
    float3   gCameraPos;             // カメラのワールド座標
    float    gGridScale;             // グリッドの基本間隔 (デフォルト1.0m)
    float    gFadeDistance;          // グリッドが消え始める距離
    float    gMajorLineEvery;        // メジャーラインの間隔 (マイナーN本ごと)
    float    _pad0;
    float    _pad1;
};

struct PSOutput
{
    float4 color : SV_Target;
    float  depth : SV_Depth;
};

/// @brief グリッドPS — レイ平面交差でY=0上にグリッドを描画し、正確な深度も出力
PSOutput GridPS(FullscreenVSOutput input)
{
    PSOutput output;

    // スクリーンUVからワールド空間レイを復元
    float2 ndc = input.uv * float2(2.0, -2.0) + float2(-1.0, 1.0);

    float4 nearPoint = mul(float4(ndc, 0.0, 1.0), gViewProjectionInverse);
    float4 farPoint  = mul(float4(ndc, 1.0, 1.0), gViewProjectionInverse);
    nearPoint /= nearPoint.w;
    farPoint  /= farPoint.w;

    float3 rayDir = farPoint.xyz - nearPoint.xyz;

    // Y=0平面との交差を計算
    float t = -nearPoint.y / rayDir.y;

    // レイが平面に当たらない場合は破棄（カメラの後ろ or 平行）
    if (t < 0.0)
        discard;

    // ワールド空間の交差点
    float3 worldPos = nearPoint.xyz + rayDir * t;

    // スクリーン空間微分によるアンチエイリアス付きグリッド計算
    float2 coord = worldPos.xz / gGridScale;
    float2 derivative = fwidth(coord);

    // マイナーグリッドライン（基本間隔）
    float2 grid = abs(frac(coord - 0.5) - 0.5) / derivative;
    float minorLine = min(grid.x, grid.y);
    float minorAlpha = 1.0 - min(minorLine, 1.0);

    // メジャーグリッドライン（N本ごとの太い線）
    float2 coordMajor = coord / gMajorLineEvery;
    float2 derivativeMajor = derivative / gMajorLineEvery;
    float2 gridMajor = abs(frac(coordMajor - 0.5) - 0.5) / derivativeMajor;
    float majorLine = min(gridMajor.x, gridMajor.y);
    float majorAlpha = 1.0 - min(majorLine, 1.0);

    // 軸の色分け: X軸=赤, Z軸=青
    float3 lineColor = float3(0.0, 0.0, 0.0); // 通常のグリッド線は黒

    // Z軸付近（worldPos.x≈0）→ X軸ラインを赤で表示
    float xAxisDist = abs(worldPos.z) / (derivative.y * gGridScale);
    float xAxisAlpha = 1.0 - min(xAxisDist, 1.0);

    // X軸付近（worldPos.z≈0）→ Z軸ラインを青で表示
    float zAxisDist = abs(worldPos.x) / (derivative.x * gGridScale);
    float zAxisAlpha = 1.0 - min(zAxisDist, 1.0);

    // 色合成
    float3 color = lineColor;
    float alpha = minorAlpha * 0.25; // minor lines are subtle

    // メジャーライン（より濃く、強いアルファ）
    color = lerp(color, float3(0.0, 0.0, 0.0), majorAlpha);
    alpha = max(alpha, majorAlpha * 0.5);

    // 軸ライン（最優先で上書き）
    color = lerp(color, float3(0.8, 0.15, 0.15), zAxisAlpha * 0.9); // X軸=赤（X方向に延伸, z=0）
    alpha = max(alpha, zAxisAlpha * 0.8);

    color = lerp(color, float3(0.15, 0.15, 0.8), xAxisAlpha * 0.9); // Z軸=青（Z方向に延伸, x=0）
    alpha = max(alpha, xAxisAlpha * 0.8);

    // 距離ベースのフェードアウト
    float dist = length(worldPos - gCameraPos);
    float fade = 1.0 - smoothstep(gFadeDistance * 0.5, gFadeDistance, dist);
    alpha *= fade;

    // 完全透明ピクセルは破棄
    if (alpha < 0.001)
        discard;

    output.color = float4(color, alpha);

    // シーンオブジェクトとの深度テスト用に正確な深度値を計算
    float4 clipPos = mul(float4(worldPos, 1.0), gViewProjection);
    output.depth = clipPos.z / clipPos.w;

    return output;
}
