/// @file LensFlare.hlsl
/// @brief 物理ベースレンズフレア — 光束追跡（Ray Bundle Tracing）
///
/// カメラレンズを複数の球面/平面インターフェースとしてモデル化し、
/// 各ゴーストパス（2回反射）をコンピュートシェーダーでレイトレースする。
/// Sellmeier分散による波長依存屈折率で自然な色収差を再現。
///
/// Pass 1 [CS]: 光束追跡 — レンズ系を通るレイをトレースし GhostVertex を生成
/// Pass 2 [VS/PS]: ゴーストクアッド描画 — 加算ブレンドでフレアRTに描画
/// Pass 3 [Fullscreen PS]: シーン合成 — HDRシーン + フレアRT × 強度

#include "Fullscreen.hlsli"

// ============================================================================
// 共通構造体
// ============================================================================

/// レンズインターフェース（球面/平面の屈折面）
struct LensInterface
{
    float centerZ;          // Z座標（光軸上、mm）
    float radius;           // 曲率半径（正=凸, 負=凹, 0=平面/絞り）
    float apertureRadius;   // 開口半径（mm）
    float n1;               // 入射側屈折率（基準: d線 587.6nm）
    float n2;               // 出射側屈折率
    float coatingN;         // ARコーティング屈折率（0=なし）
    float coatingD;         // コーティング厚（nm）
    int   isAperture;       // 1=絞り（反射/屈折なし、遮蔽のみ）
};

/// ゴースト頂点（CS出力 → VS入力）
struct GhostVertex
{
    float2 posR;            // Red   センサー位置 (NDC)
    float2 posG;            // Green センサー位置 (NDC)
    float2 posB;            // Blue  センサー位置 (NDC)
    float  intensityR;      // Red   強度（Fresnel累積）
    float  intensityG;      // Green 強度
    float  intensityB;      // Blue  強度
    float  valid;           // 1.0=有効, 0.0=遮蔽/全反射で無効
};

// ============================================================================
// 定数バッファ
// ============================================================================

cbuffer LensFlareCB : register(b0)
{
    float2 lightScreenUV;       // 光源のスクリーンUV [0,1]
    float  lightBrightness;     // 光源輝度
    float  globalIntensity;     // グローバル強度乗数
    uint   numInterfaces;       // レンズインターフェース数
    uint   gridSize;            // グリッド解像度 (32)
    uint   numGhosts;           // ゴースト数
    float  sensorSize;          // センサーサイズ (mm)
    float  apertureBlades;      // 絞り羽根数（スターバースト用）
    float  starburstStrength;   // スターバースト強度
    float2 screenSize;          // スクリーン解像度
};

// ============================================================================
// Pass 1: Compute Shader リソース
// ============================================================================

StructuredBuffer<LensInterface> gLensInterfaces : register(t0);
StructuredBuffer<uint2>         gGhostPairs     : register(t1);
RWStructuredBuffer<GhostVertex> gGhostVertices  : register(u0);

// ============================================================================
// 物理光学ユーティリティ
// ============================================================================

/// Cauchy分散式: n(λ) = A + B/λ² + C/λ⁴
/// BK7ガラス (一般的な光学ガラス)
float CauchyBK7(float lambdaNm)
{
    float L2 = lambdaNm * lambdaNm * 1e-6; // nm² → μm²
    float L4 = L2 * L2;
    return 1.5168 + 0.00420 / L2 + 0.0 / L4;
}

/// SF11ガラス (高屈折率)
float CauchySF11(float lambdaNm)
{
    float L2 = lambdaNm * lambdaNm * 1e-6;
    return 1.7847 + 0.01080 / L2;
}

/// 屈折率を波長で補正
/// @param nBase d線 (587.6nm) での基準屈折率
/// @param lambdaNm 波長 (nm)
/// @return 補正された屈折率
float DispersionCorrect(float nBase, float lambdaNm)
{
    // 基準波長からの分散量を推定
    // nBase が 1.0（空気）の場合はそのまま返す
    if (nBase <= 1.001) return 1.0;

    // Cauchy近似: n(λ) ≈ nBase + B * (1/λ² - 1/λd²)
    // B ≈ 0.00420 (BK7相当) or 0.01080 (SF11相当)
    float lambdaD = 587.6;  // d線 (基準波長, nm)
    float B = (nBase > 1.65) ? 0.01080 : 0.00420;
    float invL2 = 1.0 / (lambdaNm * lambdaNm * 1e-6);
    float invLd2 = 1.0 / (lambdaD * lambdaD * 1e-6);
    return nBase + B * (invL2 - invLd2);
}

/// Fresnel反射係数（無偏光）
/// @param cosI 入射角の余弦
/// @param n1 入射側屈折率
/// @param n2 出射側屈折率
/// @return 反射率 R [0, 1]
float FresnelReflectance(float cosI, float n1, float n2)
{
    // Snellの法則で出射角を計算
    float sinI2 = max(1.0 - cosI * cosI, 0.0);
    float sinT2 = (n1 * n1 / (n2 * n2)) * sinI2;

    // 全反射チェック
    if (sinT2 >= 1.0)
        return 1.0;

    float cosT = sqrt(max(1.0 - sinT2, 0.0));

    // s偏光 (TE)
    float Rs = (n1 * cosI - n2 * cosT) / (n1 * cosI + n2 * cosT);
    Rs = Rs * Rs;

    // p偏光 (TM)
    float Rp = (n2 * cosI - n1 * cosT) / (n2 * cosI + n1 * cosT);
    Rp = Rp * Rp;

    return 0.5 * (Rs + Rp);
}

/// 球面との交差判定
/// @param origin レイ原点
/// @param dir レイ方向（正規化済み）
/// @param centerZ 球面中心のZ座標
/// @param radius 曲率半径
/// @param[out] hitT ヒット距離
/// @param[out] hitPos ヒット位置
/// @param[out] hitNormal ヒット法線（界面の外向き）
/// @return true if hit
bool IntersectSphere(float3 origin, float3 dir,
                     float centerZ, float radius,
                     out float hitT, out float3 hitPos, out float3 hitNormal)
{
    hitT = 0;
    hitPos = float3(0, 0, 0);
    hitNormal = float3(0, 0, 0);

    float3 center = float3(0, 0, centerZ);
    float3 oc = origin - center;

    float a = dot(dir, dir);
    float b = 2.0 * dot(oc, dir);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0)
        return false;

    float sqrtD = sqrt(discriminant);
    float t1 = (-b - sqrtD) / (2.0 * a);
    float t2 = (-b + sqrtD) / (2.0 * a);

    // 正しい交差点を選択: レイ方向に最も近い面
    // radius > 0 (凸): 近い面を選択
    // radius < 0 (凹): 遠い面を選択
    if (radius > 0)
        hitT = (t1 > 0.001) ? t1 : t2;
    else
        hitT = (t2 > 0.001) ? t2 : t1;

    if (hitT < 0.001)
        return false;

    hitPos = origin + dir * hitT;
    hitNormal = normalize(hitPos - center);

    // radius < 0 の場合、法線を反転
    if (radius < 0)
        hitNormal = -hitNormal;

    return true;
}

/// 平面との交差判定（Z=定数の平面）
bool IntersectPlane(float3 origin, float3 dir, float planeZ,
                    out float hitT, out float3 hitPos)
{
    hitT = 0;
    hitPos = float3(0, 0, 0);

    if (abs(dir.z) < 1e-8)
        return false;

    hitT = (planeZ - origin.z) / dir.z;
    if (hitT < 0.001)
        return false;

    hitPos = origin + dir * hitT;
    return true;
}

/// Snellの法則による屈折方向計算
/// @param incident 入射方向（正規化済み）
/// @param normal 界面法線（入射側を向く）
/// @param n1 入射側屈折率
/// @param n2 出射側屈折率
/// @param[out] refracted 屈折方向
/// @return true if refraction (not total internal reflection)
bool SnellRefract(float3 incident, float3 normal, float n1, float n2,
                  out float3 refracted)
{
    refracted = float3(0, 0, 0);

    float cosI = -dot(incident, normal);
    // 法線が入射側を向くように調整
    if (cosI < 0)
    {
        normal = -normal;
        cosI = -cosI;
    }

    float eta = n1 / n2;
    float sinT2 = eta * eta * (1.0 - cosI * cosI);

    if (sinT2 > 1.0)
        return false; // 全反射

    float cosT = sqrt(max(1.0 - sinT2, 0.0));
    refracted = normalize(eta * incident + (eta * cosI - cosT) * normal);
    return true;
}

/// 反射方向
float3 Reflect(float3 incident, float3 normal)
{
    return incident - 2.0 * dot(incident, normal) * normal;
}

// ============================================================================
// レイトレース: 1界面を通過（屈折）
// ============================================================================

/// 界面を屈折通過する
/// @param rayPos レイ位置（in/out）
/// @param rayDir レイ方向（in/out）
/// @param iface レンズインターフェース
/// @param n1 入射側屈折率
/// @param n2 出射側屈折率
/// @param[in,out] intensity 累積強度（Fresnel透過率を乗算）
/// @return true if passed (not blocked)
bool TraceRefract(inout float3 rayPos, inout float3 rayDir,
                  LensInterface iface, float n1, float n2,
                  inout float intensity)
{
    // 絞りの場合: 平面交差 + 開口チェックのみ
    if (iface.isAperture)
    {
        float hitT;
        float3 hitPos;
        if (!IntersectPlane(rayPos, rayDir, iface.centerZ, hitT, hitPos))
            return false;

        float r = length(hitPos.xy);
        if (r > iface.apertureRadius)
            return false;

        rayPos = hitPos;
        return true;
    }

    // 球面の場合
    if (abs(iface.radius) > 0.001)
    {
        float hitT;
        float3 hitPos, hitNormal;
        if (!IntersectSphere(rayPos, rayDir, iface.centerZ, iface.radius,
                             hitT, hitPos, hitNormal))
            return false;

        // 開口チェック
        float r = length(hitPos.xy);
        if (r > iface.apertureRadius)
            return false;

        // 法線が入射方向と反対を向くようにする
        float cosI = -dot(rayDir, hitNormal);
        if (cosI < 0)
        {
            hitNormal = -hitNormal;
            cosI = -cosI;
        }

        // Fresnel透過率
        float R = FresnelReflectance(cosI, n1, n2);
        intensity *= (1.0 - R);

        // 屈折
        float3 refracted;
        if (!SnellRefract(rayDir, hitNormal, n1, n2, refracted))
            return false; // 全反射

        rayPos = hitPos;
        rayDir = refracted;
        return true;
    }
    else
    {
        // 平面（radius == 0、非aperture）
        float hitT;
        float3 hitPos;
        if (!IntersectPlane(rayPos, rayDir, iface.centerZ, hitT, hitPos))
            return false;

        float r = length(hitPos.xy);
        if (r > iface.apertureRadius)
            return false;

        // 平面屈折（法線 = Z+方向）
        float3 normal = float3(0, 0, 1);
        float cosI = -dot(rayDir, normal);
        if (cosI < 0)
        {
            normal = -normal;
            cosI = -cosI;
        }

        float R = FresnelReflectance(cosI, n1, n2);
        intensity *= (1.0 - R);

        float3 refracted;
        if (!SnellRefract(rayDir, normal, n1, n2, refracted))
            return false;

        rayPos = hitPos;
        rayDir = refracted;
        return true;
    }
}

// ============================================================================
// レイトレース: 1界面で反射
// ============================================================================

bool TraceReflect(inout float3 rayPos, inout float3 rayDir,
                  LensInterface iface, float n1, float n2,
                  inout float intensity)
{
    if (iface.isAperture)
        return false; // 絞りでは反射しない

    float3 hitNormal;

    if (abs(iface.radius) > 0.001)
    {
        // 球面
        float hitT;
        float3 hitPos;
        if (!IntersectSphere(rayPos, rayDir, iface.centerZ, iface.radius,
                             hitT, hitPos, hitNormal))
            return false;

        float r = length(hitPos.xy);
        if (r > iface.apertureRadius)
            return false;

        rayPos = hitPos;
    }
    else
    {
        // 平面
        float hitT;
        float3 hitPos;
        if (!IntersectPlane(rayPos, rayDir, iface.centerZ, hitT, hitPos))
            return false;

        float r = length(hitPos.xy);
        if (r > iface.apertureRadius)
            return false;

        hitNormal = float3(0, 0, 1);
        rayPos = hitPos;
    }

    // 法線方向を入射側に合わせる
    float cosI = -dot(rayDir, hitNormal);
    if (cosI < 0)
    {
        hitNormal = -hitNormal;
        cosI = -cosI;
    }

    // Fresnel反射率 → 強度に乗算
    float R = FresnelReflectance(cosI, n1, n2);
    intensity *= R;

    // 反射方向
    rayDir = Reflect(rayDir, hitNormal);

    return true;
}

// ============================================================================
// Pass 1: Compute Shader — 光束追跡
// ============================================================================

/// 1スレッド = 1グリッド頂点 × 1ゴースト
/// Dispatch: ceil(numGhosts * gridSize * gridSize / 256)
[numthreads(256, 1, 1)]
void CSTraceGhosts(uint3 dtid : SV_DispatchThreadID)
{
    uint totalVertices = numGhosts * gridSize * gridSize;
    if (dtid.x >= totalVertices)
        return;

    // インデックス分解
    uint vertexIdx = dtid.x;
    uint ghostIdx  = vertexIdx / (gridSize * gridSize);
    uint gridIdx   = vertexIdx % (gridSize * gridSize);
    uint gridX     = gridIdx % gridSize;
    uint gridY     = gridIdx / gridSize;

    // ゴーストペア取得: 反射面A (シーン側), 反射面B (センサー側)
    uint2 ghostPair = gGhostPairs[ghostIdx];
    uint reflA = ghostPair.x;  // シーン側の反射面（インデックス大=Z大）
    uint reflB = ghostPair.y;  // センサー側の反射面（インデックス小=Z小）

    // 出力先
    uint outIdx = vertexIdx;

    // 光源位置からグリッド座標を計算
    // 光源スクリーンUV → 入射面上の位置にマッピング
    float2 gridUV = (float2(gridX, gridY) + 0.5) / float(gridSize); // [0,1]

    // 入射面（最もシーン側の面）上のサンプル位置
    LensInterface firstIface = gLensInterfaces[numInterfaces - 1];
    float entranceR = firstIface.apertureRadius;

    // グリッドを入射面の開口上に配置
    float2 gridPos = (gridUV * 2.0 - 1.0) * entranceR; // [-R, R]

    // 光源方向: lightScreenUV → レイ方向
    // UV [0,1] → 角度（小角度近似）
    float2 lightAngle = (lightScreenUV - 0.5) * 0.8; // FOV ≈ ±0.4 rad

    // 3波長のレイトレース
    float wavelengths[3] = { 650.0, 550.0, 450.0 }; // R, G, B (nm)
    float2 sensorPos[3];
    float  intensities[3] = { 0, 0, 0 };
    bool   allValid = true;

    for (int ch = 0; ch < 3; ++ch)
    {
        float lambda = wavelengths[ch];
        float3 rayPos = float3(gridPos.x, gridPos.y, firstIface.centerZ);
        float3 rayDir = normalize(float3(-lightAngle.x, -lightAngle.y, -1.0));
        float  rayIntensity = 1.0;
        bool   valid = true;

        // ========== フェーズ1: シーン側 → 反射面A まで屈折で進む ==========
        // インターフェースをシーン側から順に (numInterfaces-1 → reflA+1)
        for (int i = (int)numInterfaces - 1; i > (int)reflA; --i)
        {
            LensInterface iface = gLensInterfaces[i];
            float n1_corr = DispersionCorrect(iface.n2, lambda); // シーン→センサー方向: n2が入射側
            float n2_corr = DispersionCorrect(iface.n1, lambda);

            if (!TraceRefract(rayPos, rayDir, iface, n1_corr, n2_corr, rayIntensity))
            {
                valid = false;
                break;
            }
        }

        if (!valid) { allValid = false; continue; }

        // ========== フェーズ2: 反射面A で反射 ==========
        {
            LensInterface ifaceA = gLensInterfaces[reflA];
            float nInside = DispersionCorrect(ifaceA.n1, lambda);
            float nOutside = DispersionCorrect(ifaceA.n2, lambda);
            // レイはシーン側から来ているので n2→n1 方向で反射面に到達
            if (!TraceReflect(rayPos, rayDir, ifaceA, nInside, nOutside, rayIntensity))
            {
                valid = false;
            }
        }

        if (!valid) { allValid = false; continue; }

        // ========== フェーズ3: 反射面A → 反射面B まで屈折で戻る ==========
        // 反射後はセンサー方向に戻るが反射面Bまで
        for (int j = (int)reflA + 1; j < (int)reflB; ++j)
        {
            // 逆方向の屈折: n1→n2 (元の定義通り)
            LensInterface iface = gLensInterfaces[j];
            float n1_corr = DispersionCorrect(iface.n1, lambda);
            float n2_corr = DispersionCorrect(iface.n2, lambda);

            // 反射後は逆向き（センサー→シーン方向）に進むので
            // n1/n2は反転しない（TraceRefractが法線方向で自動調整）
            if (!TraceRefract(rayPos, rayDir, iface, n1_corr, n2_corr, rayIntensity))
            {
                valid = false;
                break;
            }
        }

        if (!valid) { allValid = false; continue; }

        // ========== フェーズ4: 反射面B で反射 ==========
        {
            LensInterface ifaceB = gLensInterfaces[reflB];
            float nInside = DispersionCorrect(ifaceB.n2, lambda);
            float nOutside = DispersionCorrect(ifaceB.n1, lambda);
            if (!TraceReflect(rayPos, rayDir, ifaceB, nInside, nOutside, rayIntensity))
            {
                valid = false;
            }
        }

        if (!valid) { allValid = false; continue; }

        // ========== フェーズ5: 反射面B → センサーまで屈折で進む ==========
        for (int k = (int)reflB - 1; k >= 0; --k)
        {
            LensInterface iface = gLensInterfaces[k];
            float n1_corr = DispersionCorrect(iface.n2, lambda);
            float n2_corr = DispersionCorrect(iface.n1, lambda);

            if (!TraceRefract(rayPos, rayDir, iface, n1_corr, n2_corr, rayIntensity))
            {
                valid = false;
                break;
            }
        }

        if (!valid) { allValid = false; continue; }

        // ========== センサー面（Z=0）に投影 ==========
        float hitT;
        float3 hitPos;
        if (!IntersectPlane(rayPos, rayDir, 0.0, hitT, hitPos))
        {
            allValid = false;
            continue;
        }

        // センサー座標をNDCに変換
        sensorPos[ch] = hitPos.xy / (sensorSize * 0.5); // [-1, 1]
        intensities[ch] = rayIntensity * lightBrightness;
    }

    // 結果書き込み
    GhostVertex v;
    v.posR = sensorPos[0];
    v.posG = sensorPos[1];
    v.posB = sensorPos[2];
    v.intensityR = intensities[0];
    v.intensityG = intensities[1];
    v.intensityB = intensities[2];
    v.valid = allValid ? 1.0 : 0.0;

    gGhostVertices[outIdx] = v;
}

// ============================================================================
// Pass 2: Ghost Quad Rendering — VS/PS
// ============================================================================

// ゴースト頂点バッファ（SRV として読み取り）
StructuredBuffer<GhostVertex> gGhostVertexSRV : register(t0);

cbuffer GhostRenderCB : register(b0)
{
    uint  gGridSizeRender;      // gridSize
    uint  gNumGhostsRender;     // numGhosts
    uint  gChannelIndex;        // 0=R, 1=G, 2=B
    float gGlobalIntensity;     // 強度乗数
    float gStarburstStrengthR;  // スターバースト強度
    float gApertureBladesR;     // 絞り羽根数
    float2 gScreenSizeR;        // スクリーン解像度
    float2 gLightScreenUVR;     // 光源UV
    float2 gPaddingR;
};

struct GhostVSOutput
{
    float4 pos   : SV_Position;
    float3 color : COLOR;
};

/// ゴーストクアッド頂点シェーダー
/// インデックスバッファ: 1ゴースト = (gridSize-1)² × 6 indices
/// SV_InstanceID でゴーストを選択
GhostVSOutput VSGhost(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    GhostVSOutput output;
    output.pos = float4(0, 0, 0, 1);
    output.color = float3(0, 0, 0);

    uint ghostIdx = instanceID;
    uint gs = gGridSizeRender;
    uint quadsPerRow = gs - 1;

    // インデックスバッファからクアッド頂点を展開
    // vertexID はインデックスバッファ経由のフラットインデックス
    // 6頂点/クアッド: (0,1,2) + (2,1,3) のトライアングルリスト
    uint quadIdx = vertexID / 6;
    uint cornerIdx = vertexID % 6;

    // クアッドインデックス → グリッド座標
    uint qx = quadIdx % quadsPerRow;
    uint qy = quadIdx / quadsPerRow;

    // 6頂点のコーナーオフセット: (0,0), (1,0), (0,1), (0,1), (1,0), (1,1)
    static const uint2 cornerOffsets[6] = {
        uint2(0, 0), uint2(1, 0), uint2(0, 1),
        uint2(0, 1), uint2(1, 0), uint2(1, 1)
    };
    uint2 corner = cornerOffsets[cornerIdx];
    uint gx = qx + corner.x;
    uint gy = qy + corner.y;

    // グリッド頂点 → GhostVertex バッファインデックス
    uint vertBufIdx = ghostIdx * (gs * gs) + gy * gs + gx;
    GhostVertex gv = gGhostVertexSRV[vertBufIdx];

    if (gv.valid < 0.5)
    {
        // 無効頂点: 退化三角形
        output.pos = float4(0, 0, 0, 0);
        return output;
    }

    // チャンネルに応じた位置と強度を選択
    float2 ndcPos;
    float  intensity;

    if (gChannelIndex == 0)
    {
        ndcPos = gv.posR;
        intensity = gv.intensityR;
    }
    else if (gChannelIndex == 1)
    {
        ndcPos = gv.posG;
        intensity = gv.intensityG;
    }
    else
    {
        ndcPos = gv.posB;
        intensity = gv.intensityB;
    }

    // スターバーストフィルタ
    float2 toCenter = ndcPos;
    float angle = atan2(toCenter.y, toCenter.x);
    float starburst = cos(angle * gApertureBladesR) * 0.5 + 0.5;
    starburst = lerp(1.0, pow(starburst, 4.0), gStarburstStrengthR);

    intensity *= starburst * gGlobalIntensity;

    // NDC → クリップ空間（Z=0, W=1）
    output.pos = float4(ndcPos.x, ndcPos.y, 0.0, 1.0);

    // 色: 単一チャンネル
    float3 col = float3(0, 0, 0);
    if (gChannelIndex == 0) col.r = intensity;
    else if (gChannelIndex == 1) col.g = intensity;
    else col.b = intensity;

    output.color = col;
    return output;
}

/// ゴースト描画ピクセルシェーダー（加算ブレンド）
float4 PSGhost(GhostVSOutput input) : SV_Target
{
    // 強度が極小なら破棄
    float lum = dot(input.color, float3(0.2126, 0.7152, 0.0722));
    if (lum < 1e-6)
        discard;

    return float4(input.color, 1.0);
}

// ============================================================================
// Pass 3: Composite — フルスクリーン合成
// ============================================================================

Texture2D<float4> gSceneTexture : register(t0);
Texture2D<float4> gFlareTexture : register(t1);
SamplerState gCompositeSampler  : register(s0);

cbuffer CompositeCB : register(b0)
{
    float gCompositeIntensity;
    float3 gCompositePadding;
};

float4 PSComposite(FullscreenVSOutput input) : SV_Target
{
    float3 scene = gSceneTexture.Sample(gCompositeSampler, input.uv).rgb;
    float3 flare = gFlareTexture.Sample(gCompositeSampler, input.uv).rgb;

    // フレアエネルギーをReinhard でソフトクランプ（ホワイトアウト防止）
    flare = flare / (1.0 + flare);

    float3 result = scene + flare * gCompositeIntensity;
    return float4(result, 1.0);
}
