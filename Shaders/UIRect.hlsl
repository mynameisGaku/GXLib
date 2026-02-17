/// @file UIRect.hlsl
/// @brief SDF 角丸矩形シェーダー
///
/// UIRenderer が使用する角丸矩形の描画シェーダー。
/// SDF（Signed Distance Function）を使って角丸、枠線、影を
/// ピクセルシェーダーだけで描画する。

// 定数バッファ
cbuffer UIRectCB : register(b0)
{
    float4x4 cb_projection;
    float2   cb_rectSize;
    float    cb_cornerRadius;
    float    cb_borderWidth;
    float4   cb_fillColor;
    float4   cb_borderColor;
    float2   cb_shadowOffset;
    float    cb_shadowBlur;
    float    cb_shadowAlpha;
    float    cb_opacity;
    float3   cb_pad;
    float4   cb_gradientColor;
    float2   cb_gradientDir;
    float    cb_gradientEnabled;
    float    cb_pad2;
    float2   cb_effectCenter;
    float    cb_effectTime;
    float    cb_effectDuration;
    float    cb_effectStrength;
    float    cb_effectWidth;
    float    cb_effectType;
    float    cb_pad3;
};

struct VSInput
{
    float2 position : POSITION;
    float2 localUV  : TEXCOORD;
};

struct VSOutput
{
    float4 pos     : SV_Position;
    float2 localUV : TEXCOORD;
};

// ============================================================================
// 頂点シェーダー
// ============================================================================
VSOutput VSMain(VSInput input)
{
    VSOutput o;
    o.pos = mul(float4(input.position, 0.0f, 1.0f), cb_projection);
    o.localUV = input.localUV;
    return o;
}

// ============================================================================
// SDF 距離関数
// ============================================================================

/// 角丸矩形のSDF
/// p: 矩形中心からの座標
/// b: 矩形の半径（幅/2, 高さ/2）
/// r: 角丸半径
float sdRoundedBox(float2 p, float2 b, float r)
{
    float2 q = abs(p) - b + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

// ============================================================================
// ピクセルシェーダー
// ============================================================================
float4 PSMain(VSOutput input) : SV_Target
{
    float2 halfSize = cb_rectSize * 0.5;
    float2 center = halfSize;
    float2 p = input.localUV - center;

    // 角丸半径をサイズに合わせてクランプ
    float radius = min(cb_cornerRadius, min(halfSize.x, halfSize.y));

    // SDF計算
    float dist = sdRoundedBox(p, halfSize, radius);

    // アンチエイリアス（fwidthでピクセル幅を取得）
    float aa = fwidth(dist);

    // 塗り（dist < 0 = 内部）
    float fillMask = 1.0 - smoothstep(-aa, aa, dist);

    // 枠線（|dist| < borderWidth の領域）
    float borderMask = 0.0;
    if (cb_borderWidth > 0.0)
    {
        float innerDist = dist + cb_borderWidth;
        borderMask = fillMask * smoothstep(-aa, aa, innerDist);
    }

    // 影
    float shadowMask = 0.0;
    if (cb_shadowBlur > 0.0 || (cb_shadowOffset.x != 0.0 || cb_shadowOffset.y != 0.0))
    {
        float2 shadowP = p - cb_shadowOffset;
        float shadowDist = sdRoundedBox(shadowP, halfSize, radius);
        float blurWidth = max(cb_shadowBlur, 1.0);
        shadowMask = 1.0 - smoothstep(-blurWidth, blurWidth, shadowDist);
        shadowMask *= cb_shadowAlpha;
        // 本体部分では影を消す
        shadowMask *= (1.0 - fillMask);
    }

    // 色合成
    float4 color = float4(0, 0, 0, 0);

    // 影レイヤー
    color = float4(0, 0, 0, shadowMask);

    // グラデーション対応の塗り色
    float4 fillColor = cb_fillColor;
    if (cb_gradientEnabled > 0.5)
    {
        float2 dir = cb_gradientDir;
        float len = length(dir);
        if (len < 0.0001f)
            dir = float2(0.0, 1.0);
        else
            dir /= len;

        float2 uv = input.localUV / cb_rectSize;
        uv = saturate(uv);
        float t = saturate(dot(uv - 0.5, dir) + 0.5);
        fillColor = lerp(cb_fillColor, cb_gradientColor, t);
    }

    // Ripple effect
    if (cb_effectType > 0.5 && cb_effectDuration > 0.0f)
    {
        float2 uv = input.localUV / cb_rectSize;
        uv = saturate(uv);
        float rippleDist = length(uv - cb_effectCenter);
        float t = saturate(cb_effectTime / cb_effectDuration);
        float radius = t;
        float w = max(cb_effectWidth, 0.0001f);
        float ring = smoothstep(radius - w, radius, rippleDist) *
                     (1.0f - smoothstep(radius, radius + w, rippleDist));
        float fade = 1.0f - t;
        float ripple = ring * cb_effectStrength * fade;
        fillColor.rgb = lerp(fillColor.rgb, float3(1.0f, 1.0f, 1.0f), ripple);
    }

    // 塗りレイヤー（影の上に重ねる）
    float fillAlpha = (fillMask - borderMask) * fillColor.a;
    color.rgb = color.rgb * (1.0 - fillAlpha) + fillColor.rgb * fillAlpha;
    color.a = color.a * (1.0 - fillAlpha) + fillAlpha;

    // 枠線レイヤー
    float borderAlpha = borderMask * cb_borderColor.a;
    color.rgb = color.rgb * (1.0 - borderAlpha) + cb_borderColor.rgb * borderAlpha;
    color.a = color.a * (1.0 - borderAlpha) + borderAlpha;

    // 全体透明度
    color.a *= cb_opacity;

    // α=0なら破棄（不要ピクセルを削減）
    if (color.a < 0.001) discard;

    return color;
}
