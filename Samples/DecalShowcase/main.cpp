/// @file Samples/DecalShowcase/main.cpp
/// @brief Deferred decal box-projection demo.
///
/// Click on surfaces to place decals (projected from camera direction).
/// Decals are rendered using depth-buffer world-position reconstruction.
///
/// Controls:
///   Left Click   - Place decal at cursor
///   1/2/3        - Switch decal color (white/red/blue)
///   R            - Remove all decals
///   WASD / QE    - Camera movement
///   Right Click  - Toggle mouse capture for look
///   ESC          - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Decal.h"
#include "Graphics/Resource/SoftImage.h"

#include <Windows.h>

class DecalShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Decals";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 6; config.bgG = 8; config.bgB = 18;
        return config;
    }

    void Start() override
    {
        auto& ctx      = GX_Internal::CompatContext::Instance();
        auto& renderer = ctx.renderer3D;
        auto& camera   = ctx.camera;
        auto& postFX   = ctx.postEffect;
        auto& texMgr   = renderer.GetTextureManager();

        renderer.SetShadowEnabled(false);
        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // Lights
        GX::LightData lights[1];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.5f);
        renderer.SetLights(lights, 1, { 0.1f, 0.1f, 0.12f });
        renderer.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);

        // Camera
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 200.0f);
        camera.SetPosition(0.0f, 5.0f, -8.0f);
        camera.LookAt({ 0.0f, 0.0f, 0.0f });

        // Meshes
        m_planeMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(30.0f, 30.0f, 1, 1));
        m_cubeMesh  = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f));

        m_floorMat.constants.albedoFactor    = { 0.4f, 0.4f, 0.42f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.8f;

        m_wallMat.constants.albedoFactor    = { 0.5f, 0.45f, 0.4f, 1.0f };
        m_wallMat.constants.roughnessFactor = 0.7f;

        // Create a procedural circle decal texture using SoftImage
        GX::SoftImage decalImg;
        decalImg.Create(64, 64);
        for (int y = 0; y < 64; ++y)
        {
            for (int x = 0; x < 64; ++x)
            {
                float dx = (x - 31.5f) / 31.5f;
                float dy = (y - 31.5f) / 31.5f;
                float dist = std::sqrt(dx * dx + dy * dy);
                float alpha = (std::max)(0.0f, 1.0f - dist);
                alpha = alpha * alpha; // softer falloff
                uint8_t a = static_cast<uint8_t>(alpha * 255.0f);
                // DrawPixel expects 0xAARRGGBB
                uint32_t color = (static_cast<uint32_t>(a) << 24) | 0x00FFFFFF;
                decalImg.DrawPixel(x, y, color);
            }
        }
        m_decalTexHandle = decalImg.CreateTexture(texMgr);

        // Decal system
        m_decalSystem.Initialize(ctx.device, ctx.screenWidth, ctx.screenHeight);

        // Setup wall cubes
        for (int i = -3; i <= 3; ++i)
        {
            GX::Transform3D t;
            t.SetPosition(static_cast<float>(i) * 2.5f, 1.0f, 5.0f);
            t.SetScale(2.0f, 2.0f, 0.5f);
            m_wallTransforms.push_back(t);
        }
    }

    void Update(float dt) override
    {
        auto& ctx    = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& mouse  = ctx.inputManager.GetMouse();
        auto& kb     = ctx.inputManager.GetKeyboard();

        m_totalTime += dt;
        m_lastDt = dt;

        // Camera look
        if (mouse.IsButtonTriggered(GX::MouseButton::Right))
        {
            m_mouseCaptured = !m_mouseCaptured;
            m_lastMX = mouse.GetX();
            m_lastMY = mouse.GetY();
            ShowCursor(m_mouseCaptured ? FALSE : TRUE);
        }
        if (m_mouseCaptured)
        {
            int mx = mouse.GetX(), my = mouse.GetY();
            camera.Rotate(static_cast<float>(my - m_lastMY) * 0.003f,
                          static_cast<float>(mx - m_lastMX) * 0.003f);
            m_lastMX = mx; m_lastMY = my;
        }

        float speed = 8.0f * dt;
        if (CheckHitKey(KEY_INPUT_LSHIFT)) speed *= 3.0f;
        if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(speed);
        if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-speed);
        if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(speed);
        if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-speed);
        if (CheckHitKey(KEY_INPUT_E)) camera.MoveUp(speed);
        if (CheckHitKey(KEY_INPUT_Q)) camera.MoveUp(-speed);

        // Decal color presets
        if (kb.IsKeyTriggered('1')) m_decalColor = GX::Color(1.0f, 1.0f, 1.0f, 0.8f);
        if (kb.IsKeyTriggered('2')) m_decalColor = GX::Color(1.0f, 0.2f, 0.1f, 0.8f);
        if (kb.IsKeyTriggered('3')) m_decalColor = GX::Color(0.2f, 0.4f, 1.0f, 0.8f);

        // Place decal via raycast (ground plane + wall boxes)
        if (mouse.IsButtonTriggered(GX::MouseButton::Left) && !m_mouseCaptured)
        {
            XMFLOAT3 hitPos;
            XMFLOAT3 hitNormal;
            if (RaycastScene(mouse.GetX(), mouse.GetY(), hitPos, hitNormal))
            {
                GX::DecalData decal;
                decal.transform.SetPosition(hitPos.x + hitNormal.x * 0.01f,
                                            hitPos.y + hitNormal.y * 0.01f,
                                            hitPos.z + hitNormal.z * 0.01f);
                decal.transform.SetScale(1.5f, 1.5f, 1.5f);

                // デカールの投影方向を面法線に合わせて回転
                // デフォルトはY軸投影なので、法線がY以外なら回転が必要
                if (std::abs(hitNormal.x) > 0.5f)
                    decal.transform.SetRotation(0.0f, 0.0f, hitNormal.x > 0 ? -XM_PIDIV2 : XM_PIDIV2);
                else if (std::abs(hitNormal.z) > 0.5f)
                    decal.transform.SetRotation(hitNormal.z > 0 ? XM_PIDIV2 : -XM_PIDIV2, 0.0f, 0.0f);

                decal.textureHandle = m_decalTexHandle;
                decal.color = m_decalColor;
                decal.lifetime = 15.0f;
                m_decalSystem.AddDecal(decal);
            }
        }

        // Remove all decals
        if (kb.IsKeyTriggered('R'))
        {
            m_decalSystem.Shutdown();
            m_decalSystem.Initialize(ctx.device, ctx.screenWidth, ctx.screenHeight);
        }

        m_decalSystem.Update(dt);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t fi = ctx.frameIndex;

        ctx.FlushAll();
        ctx.postEffect.BeginScene(cmd, fi, ctx.renderer3D.GetDepthBuffer().GetDSVHandle(), ctx.camera);
        ctx.renderer3D.Begin(cmd, fi, ctx.camera, m_totalTime);

        // Floor
        GX::Transform3D floorT;
        ctx.renderer3D.SetMaterial(m_floorMat);
        ctx.renderer3D.DrawMesh(m_planeMesh, floorT);

        // Wall cubes
        ctx.renderer3D.SetMaterial(m_wallMat);
        for (auto& t : m_wallTransforms)
            ctx.renderer3D.DrawMesh(m_cubeMesh, t);

        ctx.renderer3D.End();

        // Render decals
        auto& db = ctx.renderer3D.GetDepthBuffer();
        m_decalSystem.Render(cmd, ctx.camera, db,
                             ctx.postEffect.GetHDRRTVHandle(),
                             ctx.renderer3D.GetTextureManager(), fi);

        ctx.postEffect.EndScene();

        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(), db, ctx.camera, m_lastDt);
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // HUD
        float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(10, 10,
            FormatT(TEXT("FPS: {:.1f}  Decals: {}"), fps, m_decalSystem.GetDecalCount()).c_str(),
            GetColor(255, 255, 255));
        DrawString(10, 35,
            TEXT("LClick: Place decal  1/2/3: Color (W/R/B)  R: Clear  WASD: Camera  RClick: Look"),
            GetColor(120, 180, 255));
    }

    void Release() override
    {
        m_decalSystem.Shutdown();
    }

private:
    /// @brief Ray-AABB交差判定（slab法）
    static bool RayAABB(const XMFLOAT3& origin, const XMFLOAT3& dir,
                        const XMFLOAT3& aabbMin, const XMFLOAT3& aabbMax,
                        float& outT, XMFLOAT3& outNormal)
    {
        float tmin = -FLT_MAX, tmax = FLT_MAX;
        XMFLOAT3 normal = { 0, 1, 0 };

        auto slabTest = [&](float o, float d, float lo, float hi, float nx, float ny, float nz) -> bool
        {
            if (std::abs(d) < 1e-8f)
                return (o >= lo && o <= hi);
            float t1 = (lo - o) / d;
            float t2 = (hi - o) / d;
            XMFLOAT3 n1 = { -nx, -ny, -nz };
            XMFLOAT3 n2 = {  nx,  ny,  nz };
            if (t1 > t2) { std::swap(t1, t2); std::swap(n1, n2); }
            if (t1 > tmin) { tmin = t1; normal = n1; }
            if (t2 < tmax)   tmax = t2;
            return tmin <= tmax;
        };

        if (!slabTest(origin.x, dir.x, aabbMin.x, aabbMax.x, 1, 0, 0)) return false;
        if (!slabTest(origin.y, dir.y, aabbMin.y, aabbMax.y, 0, 1, 0)) return false;
        if (!slabTest(origin.z, dir.z, aabbMin.z, aabbMax.z, 0, 0, 1)) return false;

        if (tmax < 0.0f) return false;
        outT = (tmin >= 0.0f) ? tmin : tmax;
        outNormal = normal;
        return true;
    }

    bool RaycastScene(int sx, int sy, XMFLOAT3& outPos, XMFLOAT3& outNormal)
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        float w = static_cast<float>(ctx.screenWidth);
        float h = static_cast<float>(ctx.screenHeight);
        float ndcX = (2.0f * sx / w) - 1.0f;
        float ndcY = 1.0f - (2.0f * sy / h);

        XMMATRIX invVP = XMMatrixInverse(nullptr, ctx.camera.GetViewProjectionMatrix());
        XMVECTOR nearPt = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), invVP);
        XMVECTOR farPt  = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), invVP);
        XMVECTOR rayDirV = XMVector3Normalize(XMVectorSubtract(farPt, nearPt));

        XMFLOAT3 origin, dir;
        XMStoreFloat3(&origin, nearPt);
        XMStoreFloat3(&dir, rayDirV);

        float bestT = FLT_MAX;
        XMFLOAT3 bestNormal = { 0, 1, 0 };
        bool hit = false;

        // Ground plane (Y=0)
        if (std::abs(dir.y) > 1e-6f)
        {
            float t = -origin.y / dir.y;
            if (t > 0.0f && t < bestT)
            {
                bestT = t;
                bestNormal = { 0.0f, 1.0f, 0.0f };
                hit = true;
            }
        }

        // Wall cubes
        for (const auto& wallT : m_wallTransforms)
        {
            auto pos = wallT.GetPosition();
            auto scl = wallT.GetScale();
            XMFLOAT3 halfExt = { scl.x * 0.5f, scl.y * 0.5f, scl.z * 0.5f };
            XMFLOAT3 aabbMin = { pos.x - halfExt.x, pos.y - halfExt.y, pos.z - halfExt.z };
            XMFLOAT3 aabbMax = { pos.x + halfExt.x, pos.y + halfExt.y, pos.z + halfExt.z };

            float t;
            XMFLOAT3 normal;
            if (RayAABB(origin, dir, aabbMin, aabbMax, t, normal) && t > 0.0f && t < bestT)
            {
                bestT = t;
                bestNormal = normal;
                hit = true;
            }
        }

        if (hit)
        {
            outPos = { origin.x + dir.x * bestT, origin.y + dir.y * bestT, origin.z + dir.z * bestT };
            outNormal = bestNormal;
        }
        return hit;
    }

    GX::DecalSystem m_decalSystem;
    int             m_decalTexHandle = -1;
    GX::Color       m_decalColor = GX::Color(1.0f, 1.0f, 1.0f, 0.8f);

    GX::GPUMesh  m_planeMesh, m_cubeMesh;
    GX::Material m_floorMat, m_wallMat;
    std::vector<GX::Transform3D> m_wallTransforms;

    float m_totalTime = 0.0f, m_lastDt = 0.0f;
    bool  m_mouseCaptured = false;
    int   m_lastMX = 0, m_lastMY = 0;
};

GX_EASY_APP(DecalShowcaseApp)
