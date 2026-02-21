/// @file Samples/IKShowcase/main.cpp
/// @brief インバースキネマティクス（IK）デモ — 足IK
///
/// Two-Bone Analytical IK + ポールベクトルで脚（太もも→すね→足先）が
/// 段差に接地する様子を可視化。膝が常に手前方向（-Z）に曲がる。
/// マウスX座標で足先ターゲットが左右に移動し、段差の上面に自動スナップ。
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"

#include <cmath>
#include <algorithm>
#include <Windows.h>

/// @brief Two-Bone Analytical IK（余弦定理 + ポールベクトル）
/// @param hip        股関節位置
/// @param target     足先ターゲット位置
/// @param poleTarget ポールベクトルターゲット（膝が向くべき方向の参照点）
/// @param thighLen   太もも長さ (a)
/// @param shinLen    すね長さ (b)
/// @param outKnee    [out] 膝位置
/// @param outAnkle   [out] 足首位置（クランプ後のターゲット）
static void SolveTwoBoneIK(
    const XMFLOAT3& hip,
    const XMFLOAT3& target,
    const XMFLOAT3& poleTarget,
    float thighLen,
    float shinLen,
    XMFLOAT3& outKnee,
    XMFLOAT3& outAnkle)
{
    XMVECTOR vHip    = XMLoadFloat3(&hip);
    XMVECTOR vTarget = XMLoadFloat3(&target);
    XMVECTOR vPole   = XMLoadFloat3(&poleTarget);

    XMVECTOR vToTarget = XMVectorSubtract(vTarget, vHip);
    float dist = XMVectorGetX(XMVector3Length(vToTarget));

    float chainLen = thighLen + shinLen;
    float minLen   = std::abs(thighLen - shinLen);

    // 距離をチェーン長の範囲にクランプ
    float c = (std::max)(minLen + 0.001f, (std::min)(dist, chainLen - 0.001f));

    // hip→target 方向の正規化ベクトル
    XMVECTOR vDir = XMVector3Normalize(vToTarget);

    // ターゲットが届かない場合はhip→target方向にチェーン長分伸ばす
    if (dist >= chainLen - 0.001f)
    {
        XMVECTOR vAnkle = XMVectorAdd(vHip, XMVectorScale(vDir, chainLen - 0.001f));
        XMStoreFloat3(&outAnkle, vAnkle);
        c = chainLen - 0.001f;
    }
    else
    {
        outAnkle = target;
    }

    // 余弦定理: cos(α) = (a² + c² - b²) / (2ac)
    float a = thighLen;
    float b = shinLen;
    float cosAlpha = (a * a + c * c - b * b) / (2.0f * a * c);
    cosAlpha = (std::max)(-1.0f, (std::min)(1.0f, cosAlpha));
    float alpha = std::acos(cosAlpha);

    // hip→target 軸上の射影距離と垂直距離
    float proj = a * std::cos(alpha);
    float perp = a * std::sin(alpha);

    // ポールベクトルから垂直方向を決定
    // poleTarget を hip→target 軸に直交射影
    XMVECTOR vToPole = XMVectorSubtract(vPole, vHip);
    float poleDot = XMVectorGetX(XMVector3Dot(vToPole, vDir));
    XMVECTOR vPoleOnAxis = XMVectorScale(vDir, poleDot);
    XMVECTOR vPerpDir = XMVectorSubtract(vToPole, vPoleOnAxis);

    float perpLen = XMVectorGetX(XMVector3Length(vPerpDir));
    if (perpLen > 0.0001f)
    {
        vPerpDir = XMVector3Normalize(vPerpDir);
    }
    else
    {
        // ポールベクトルが軸上 → フォールバック: -Z方向
        vPerpDir = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
    }

    // 膝位置 = hip + dir * proj + perpDir * perp
    XMVECTOR vKnee = XMVectorAdd(vHip,
        XMVectorAdd(XMVectorScale(vDir, proj), XMVectorScale(vPerpDir, perp)));
    XMStoreFloat3(&outKnee, vKnee);
}

/// @brief 段差ブロックのAABB定義
struct BoxAABB
{
    float minX, maxX;
    float topY;
};

class IKShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Foot IK (Two-Bone)";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 10; config.bgG = 12; config.bgB = 25;
        return config;
    }

    void Start() override
    {
        auto& ctx      = GX_Internal::CompatContext::Instance();
        auto& renderer  = ctx.renderer3D;
        auto& camera    = ctx.camera;
        auto& postFX    = ctx.postEffect;

        renderer.SetShadowEnabled(false);
        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // 床
        m_floorMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(20.0f, 20.0f, 1, 1));
        m_floorTransform.SetPosition(0, 0, 0);
        m_floorMat.constants.albedoFactor    = { 0.25f, 0.25f, 0.28f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.9f;

        // 段差ブロック（階段状）
        m_boxMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f));

        // 段差1: 右の低い段
        m_boxTransforms[0].SetPosition(2.0f, 0.35f, 0.0f);
        m_boxTransforms[0].SetScale(2.5f, 0.7f, 3.0f);
        m_boxMats[0].constants.albedoFactor    = { 0.45f, 0.38f, 0.32f, 1.0f };
        m_boxMats[0].constants.roughnessFactor = 0.7f;
        m_boxAABBs[0] = { 2.0f - 1.25f, 2.0f + 1.25f, 0.7f };

        // 段差2: 中央の中段
        m_boxTransforms[1].SetPosition(-0.3f, 0.65f, 0.0f);
        m_boxTransforms[1].SetScale(2.0f, 1.3f, 3.0f);
        m_boxMats[1].constants.albedoFactor    = { 0.32f, 0.38f, 0.45f, 1.0f };
        m_boxMats[1].constants.roughnessFactor = 0.7f;
        m_boxAABBs[1] = { -0.3f - 1.0f, -0.3f + 1.0f, 1.3f };

        // 段差3: 左の高い段
        m_boxTransforms[2].SetPosition(-2.5f, 1.0f, 0.0f);
        m_boxTransforms[2].SetScale(2.5f, 2.0f, 3.0f);
        m_boxMats[2].constants.albedoFactor    = { 0.38f, 0.32f, 0.42f, 1.0f };
        m_boxMats[2].constants.roughnessFactor = 0.7f;
        m_boxAABBs[2] = { -2.5f - 1.25f, -2.5f + 1.25f, 2.0f };

        // ターゲット球（足先マーカー）
        m_targetMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.1f, 12, 6));
        m_targetMat.constants.albedoFactor    = { 1.0f, 0.3f, 0.1f, 1.0f };
        m_targetMat.constants.metallicFactor  = 0.8f;

        // ルート球（股関節マーカー）
        m_rootMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.12f, 12, 6));
        m_rootMat.constants.albedoFactor    = { 0.8f, 0.4f, 1.0f, 1.0f };

        // ライト
        GX::LightData lights[2];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
        lights[1] = GX::Light::CreatePoint({ 0.0f, 6.0f, -4.0f }, 25.0f, { 0.8f, 0.9f, 1.0f }, 3.0f);
        renderer.SetLights(lights, 2, { 0.1f, 0.1f, 0.12f });
        renderer.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);
        renderer.GetSkybox().SetColors({ 0.3f, 0.35f, 0.5f }, { 0.5f, 0.55f, 0.6f });

        // カメラ（横から全体を見る）
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(0.0f, 2.5f, -10.0f);
        camera.SetPitch(0.05f);
        camera.SetYaw(0.0f);
    }

    float QueryTerrainHeight(float x) const
    {
        float height = 0.0f;
        for (int i = 0; i < k_NumBoxes; ++i)
        {
            const auto& b = m_boxAABBs[i];
            if (x >= b.minX && x <= b.maxX)
                height = (std::max)(height, b.topY);
        }
        return height;
    }

    void Update(float dt) override
    {
        auto& ctx   = GX_Internal::CompatContext::Instance();
        auto& mouse = ctx.inputManager.GetMouse();

        m_totalTime += dt;
        m_lastDt = dt;

        // マウスX → ワールドX
        float screenW = static_cast<float>(ctx.screenWidth);
        float mx = static_cast<float>(mouse.GetX());
        float ndcX = (mx / screenW) * 2.0f - 1.0f;
        float worldX = ndcX * 5.0f;

        // 足先ターゲット: 地形高さにスナップ
        float groundY = QueryTerrainHeight(worldX);
        m_targetPos = { worldX, groundY, 0.0f };

        // 股関節（ルート）: 固定位置
        m_hipPos = { 0.0f, k_HipHeight, 0.0f };

        // ポールベクトルターゲット: 膝が前方向に曲がるよう股関節より上に設定
        m_poleTarget = { 0.0f, k_HipHeight + 1.0f, -3.0f };

        // Two-Bone IK 解析解
        SolveTwoBoneIK(m_hipPos, m_targetPos, m_poleTarget,
                       k_ThighLen, k_ShinLen,
                       m_kneePos, m_anklePos);

        // 膝角度を計算（HUD表示用）
        XMVECTOR vHip   = XMLoadFloat3(&m_hipPos);
        XMVECTOR vKnee  = XMLoadFloat3(&m_kneePos);
        XMVECTOR vAnkle = XMLoadFloat3(&m_anklePos);
        XMVECTOR vThigh = XMVector3Normalize(XMVectorSubtract(vHip, vKnee));
        XMVECTOR vShin  = XMVector3Normalize(XMVectorSubtract(vAnkle, vKnee));
        float cosKnee = XMVectorGetX(XMVector3Dot(vThigh, vShin));
        cosKnee = (std::max)(-1.0f, (std::min)(1.0f, cosKnee));
        m_kneeAngleDeg = XMConvertToDegrees(std::acos(cosKnee));

        m_targetTransform.SetPosition(m_targetPos.x, m_targetPos.y, m_targetPos.z);
        m_rootTransform.SetPosition(m_hipPos.x, m_hipPos.y, m_hipPos.z);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t frameIndex = ctx.frameIndex;

        ctx.FlushAll();

        ctx.postEffect.BeginScene(cmd, frameIndex,
                                  ctx.renderer3D.GetDepthBuffer().GetDSVHandle(),
                                  ctx.camera);
        ctx.renderer3D.Begin(cmd, frameIndex, ctx.camera, m_totalTime);

        // 床
        ctx.renderer3D.SetMaterial(m_floorMat);
        ctx.renderer3D.DrawMesh(m_floorMesh, m_floorTransform);

        // 段差
        for (int i = 0; i < k_NumBoxes; ++i)
        {
            ctx.renderer3D.SetMaterial(m_boxMats[i]);
            ctx.renderer3D.DrawMesh(m_boxMesh, m_boxTransforms[i]);
        }

        // ターゲット球
        ctx.renderer3D.SetMaterial(m_targetMat);
        ctx.renderer3D.DrawMesh(m_targetMesh, m_targetTransform);

        // ルート球
        ctx.renderer3D.SetMaterial(m_rootMat);
        ctx.renderer3D.DrawMesh(m_rootMesh, m_rootTransform);

        ctx.renderer3D.End();

        // ボーンをラインで描画
        auto& primBatch = ctx.renderer3D.GetPrimitiveBatch3D();
        XMFLOAT4X4 vp;
        XMStoreFloat4x4(&vp, XMMatrixTranspose(ctx.camera.GetViewProjectionMatrix()));
        primBatch.Begin(cmd, frameIndex, vp);

        // 太もも（股関節→膝）
        primBatch.DrawLine(m_hipPos, m_kneePos, { 1.0f, 0.9f, 0.7f, 1.0f });
        // すね（膝→足首）
        primBatch.DrawLine(m_kneePos, m_anklePos, { 0.7f, 0.9f, 1.0f, 1.0f });

        // ジョイント球: 股関節=ピンク、膝=黄、足首=緑
        primBatch.DrawWireSphere(m_hipPos, 0.1f, { 1.0f, 0.5f, 1.0f, 1.0f }, 8);
        primBatch.DrawWireSphere(m_kneePos, 0.12f, { 1.0f, 1.0f, 0.3f, 1.0f }, 8);
        primBatch.DrawWireSphere(m_anklePos, 0.08f, { 0.3f, 1.0f, 0.3f, 1.0f }, 8);

        // 足首→ターゲットへのガイドライン
        primBatch.DrawLine(m_anklePos, m_targetPos, { 1.0f, 0.3f, 0.1f, 0.5f });

        primBatch.DrawGrid(10.0f, 10, { 0.2f, 0.2f, 0.2f, 0.3f });
        primBatch.End();

        ctx.postEffect.EndScene();

        auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               depthBuffer, ctx.camera, m_lastDt);
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // HUD
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(10, 10, FormatT(TEXT("FPS: {:.1f}"), fps).c_str(), GetColor(255, 255, 255));
        DrawString(10, 35,
                   FormatT(TEXT("Two-Bone IK: thigh={:.1f} shin={:.1f}  target=({:.1f}, {:.1f})  knee={:.0f}\xB0"),
                           k_ThighLen, k_ShinLen,
                           m_targetPos.x, m_targetPos.y,
                           m_kneeAngleDeg).c_str(),
                   GetColor(120, 180, 255));
        DrawString(10, 60, TEXT("Mouse left/right: move foot along terrain  ESC: Quit"),
                   GetColor(136, 136, 136));
    }

private:
    static constexpr int   k_NumBoxes  = 3;
    static constexpr float k_HipHeight = 3.0f;
    static constexpr float k_ThighLen  = 1.5f;
    static constexpr float k_ShinLen   = 1.4f;

    float m_totalTime    = 0.0f;
    float m_lastDt       = 0.0f;
    float m_kneeAngleDeg = 0.0f;

    XMFLOAT3 m_hipPos     = {};
    XMFLOAT3 m_kneePos    = {};
    XMFLOAT3 m_anklePos   = {};
    XMFLOAT3 m_targetPos  = {};
    XMFLOAT3 m_poleTarget = {};

    GX::GPUMesh     m_floorMesh;
    GX::Transform3D m_floorTransform;
    GX::Material    m_floorMat;

    GX::GPUMesh     m_boxMesh;
    GX::Transform3D m_boxTransforms[k_NumBoxes];
    GX::Material    m_boxMats[k_NumBoxes];
    BoxAABB         m_boxAABBs[k_NumBoxes];

    GX::GPUMesh     m_targetMesh;
    GX::Transform3D m_targetTransform;
    GX::Material    m_targetMat;

    GX::GPUMesh     m_rootMesh;
    GX::Transform3D m_rootTransform;
    GX::Material    m_rootMat;
};

GX_EASY_APP(IKShowcaseApp)
