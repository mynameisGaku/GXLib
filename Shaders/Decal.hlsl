/// @file Decal.hlsl
/// @brief デカールシェーダー（Deferred Box Projection）
///
/// ユニットキューブをデカールのワールド空間に配置し、
/// 深度バッファからワールド座標を復元してデカールテクスチャを投影する。

// ============================================================================
// 定数バッファ
// ============================================================================
cbuffer DecalCB : register(b0)
{
    float4x4 gInvViewProj;     // 逆ビュープロジェクション行列
    float4x4 gDecalWorldVP;    // デカールワールド * ビュープロジェクション行列
    float4x4 gDecalInvWorld;   // デカール逆ワールド行列
    float4   gDecalColor;      // デカールカラー (RGBA)
    float    gFadeDistance;     // エッジフェード距離
    float    gNormalThreshold;  // 法線方向しきい値
    float2   gScreenSize;      // 画面サイズ (width, height)
};

// ============================================================================
// テクスチャ・サンプラー
// ============================================================================
Texture2D<float>  gDepthTexture  : register(t0);  // 深度バッファ
Texture2D<float4> gDecalTexture  : register(t1);  // デカールテクスチャ
SamplerState      gLinearClamp   : register(s0);   // LINEAR/CLAMPサンプラー

// ============================================================================
// 構造体
// ============================================================================
struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 posCS : SV_Position;   // クリップ空間座標
    float4 posPS : TEXCOORD0;     // 射影座標（w除算前）
};

// ============================================================================
// 頂点シェーダー
// ============================================================================
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    // ユニットキューブ頂点をデカールワールド→ビュー→プロジェクションに変換
    output.posCS = mul(float4(input.position, 1.0f), gDecalWorldVP);
    output.posPS = output.posCS;
    return output;
}

// ============================================================================
// ピクセルシェーダー
// ============================================================================
float4 PSMain(VSOutput input) : SV_Target
{
    // 1. スクリーンUVを計算（SV_Positionからピクセル座標→UV）
    float2 screenUV = input.posCS.xy / gScreenSize;

    // 2. 深度バッファをサンプリング
    float depth = gDepthTexture.Sample(gLinearClamp, screenUV);

    // 深度が0（何も描画されていない=遠方）の場合はスキップ
    if (depth <= 0.0f)
        discard;

    // 3. NDC座標を構築（D3D12: Z = [0, 1], XY = [-1, 1]）
    float2 ndc = screenUV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    float4 clipPos = float4(ndc, depth, 1.0f);

    // 4. 逆ビュープロジェクションでワールド座標を復元
    float4 worldPos = mul(clipPos, gInvViewProj);
    worldPos /= worldPos.w;

    // 5. ワールド座標をデカールのローカル空間に変換
    float4 localPos = mul(worldPos, gDecalInvWorld);

    // 6. ユニットキューブ内判定 (-0.5 〜 +0.5)
    float3 absLocal = abs(localPos.xyz);
    if (absLocal.x > 0.5f || absLocal.y > 0.5f || absLocal.z > 0.5f)
        discard;

    // 7. XZ平面をUVとして使用（Y軸方向に投影）
    //    localPos.xz を [0, 1] にマッピング
    float2 decalUV = localPos.xz + 0.5f;

    // 8. デカールテクスチャをサンプリング
    float4 texColor = gDecalTexture.Sample(gLinearClamp, decalUV);

    // 9. エッジフェード — キューブ境界付近で滑らかにフェードアウト
    float fadeX = smoothstep(0.5f, 0.5f - gFadeDistance, absLocal.x);
    float fadeY = smoothstep(0.5f, 0.5f - gFadeDistance, absLocal.y);
    float fadeZ = smoothstep(0.5f, 0.5f - gFadeDistance, absLocal.z);
    float edgeFade = fadeX * fadeY * fadeZ;

    // 10. 深度ベースの法線近似（中心差分法）
    //     深度のみから法線を推定し、デカールの投影方向との角度でフェード
    float2 texelSize = 1.0f / gScreenSize;
    float depthL = gDepthTexture.Sample(gLinearClamp, screenUV + float2(-texelSize.x, 0.0f));
    float depthR = gDepthTexture.Sample(gLinearClamp, screenUV + float2( texelSize.x, 0.0f));
    float depthU = gDepthTexture.Sample(gLinearClamp, screenUV + float2(0.0f, -texelSize.y));
    float depthD = gDepthTexture.Sample(gLinearClamp, screenUV + float2(0.0f,  texelSize.y));

    // NDC空間での深度勾配からサーフェス法線を推定
    float3 dpdx_approx = float3(2.0f * texelSize.x, 0.0f, depthR - depthL);
    float3 dpdy_approx = float3(0.0f, 2.0f * texelSize.y, depthD - depthU);
    float3 surfaceNormal = normalize(cross(dpdx_approx, dpdy_approx));

    // デカールのY軸（投影方向）をローカル空間のup方向として使用
    // 法線とのdot productでフェード
    float normalFade = saturate(abs(surfaceNormal.z) - (1.0f - gNormalThreshold));
    normalFade = saturate(normalFade / gNormalThreshold);

    // 11. 最終カラー合成
    float4 finalColor = texColor * gDecalColor;
    finalColor.a *= edgeFade * normalFade;

    return finalColor;
}
