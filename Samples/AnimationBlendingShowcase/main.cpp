/// @file Samples/AnimationBlendingShowcase/main.cpp
/// @brief AnimatorStateMachine + BlendStack + BlendTree + AnimationEvent demo.
///
/// Demonstrates the animation blending system using procedurally generated
/// animation clips on a simple skeleton. A "character" (sphere) moves via
/// RootMotion while states transition through a state machine.
///
/// Controls:
///   WASD         - Movement input (sets speed parameter for BlendTree)
///   Space        - Jump (trigger)
///   1/2/3/4      - Force state: Idle/Walk/Run/Jump
///   Tab          - Toggle RootMotion
///   WASD/QE      - Camera (when Shift held)
///   Right Click  - Toggle mouse capture for camera look
///   ESC          - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/PrimitiveBatch3D.h"
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/AnimatorStateMachine.h"
#include "Graphics/3D/BlendStack.h"
#include "Graphics/3D/BlendTree.h"

#include <Windows.h>
#include <cmath>

class AnimationBlendingShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Animation Blending";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 8; config.bgG = 10; config.bgB = 20;
        return config;
    }

    void Start() override
    {
        auto& ctx      = GX_Internal::CompatContext::Instance();
        auto& renderer = ctx.renderer3D;
        auto& camera   = ctx.camera;
        auto& postFX   = ctx.postEffect;

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
        camera.LookAt({ 0.0f, 1.0f, 0.0f });

        // Floor
        m_planeMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(50.0f, 50.0f, 10, 10));
        m_floorMat.constants.albedoFactor    = { 0.35f, 0.38f, 0.4f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.9f;

        // Character meshes (body = sphere, joints = small spheres)
        m_bodyMesh   = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.4f, 16, 8));
        m_jointMesh  = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.12f, 8, 4));
        m_bodyMat.constants.albedoFactor    = { 0.2f, 0.6f, 1.0f, 1.0f };
        m_bodyMat.constants.roughnessFactor = 0.3f;
        m_bodyMat.constants.metallicFactor  = 0.6f;
        m_jointMat.constants.albedoFactor    = { 1.0f, 0.7f, 0.2f, 1.0f };
        m_jointMat.constants.roughnessFactor = 0.4f;

        BuildSkeleton();
        BuildAnimations();
        SetupStateMachine();
        SetupAnimator();
    }

    void Update(float dt) override
    {
        auto& ctx    = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& mouse  = ctx.inputManager.GetMouse();
        auto& kb     = ctx.inputManager.GetKeyboard();

        m_totalTime += dt;
        m_lastDt = dt;

        // Camera controls
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
        if (CheckHitKey(KEY_INPUT_LSHIFT))
        {
            float speed = 10.0f * dt;
            if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(speed);
            if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-speed);
            if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(speed);
            if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-speed);
            if (CheckHitKey(KEY_INPUT_E)) camera.MoveUp(speed);
            if (CheckHitKey(KEY_INPUT_Q)) camera.MoveUp(-speed);
        }

        // Movement input (sets speed for state machine)
        float inputX = 0.0f, inputZ = 0.0f;
        if (!CheckHitKey(KEY_INPUT_LSHIFT))
        {
            if (CheckHitKey(KEY_INPUT_W)) inputZ += 1.0f;
            if (CheckHitKey(KEY_INPUT_S)) inputZ -= 1.0f;
            if (CheckHitKey(KEY_INPUT_D)) inputX += 1.0f;
            if (CheckHitKey(KEY_INPUT_A)) inputX -= 1.0f;
        }
        float inputLen = std::sqrt(inputX * inputX + inputZ * inputZ);
        m_speedParam = (std::min)(1.0f, inputLen);
        m_stateMachine.SetFloat("speed", m_speedParam);

        // Jump trigger
        if (kb.IsKeyTriggered(VK_SPACE))
            m_stateMachine.SetTrigger("jump");

        // Force state
        if (kb.IsKeyTriggered('1')) m_stateMachine.SetCurrentState(0);
        if (kb.IsKeyTriggered('2')) m_stateMachine.SetCurrentState(1);
        if (kb.IsKeyTriggered('3')) m_stateMachine.SetCurrentState(2);
        if (kb.IsKeyTriggered('4')) m_stateMachine.SetCurrentState(3);

        // Toggle root motion
        if (kb.IsKeyTriggered(VK_TAB))
            m_rootMotionEnabled = !m_rootMotionEnabled;

        // Update animation
        m_animator.Update(dt);

        // Apply root motion
        if (m_rootMotionEnabled)
        {
            XMFLOAT3 delta = m_animator.GetRootMotionDelta();
            m_charX += delta.x;
            m_charZ += delta.z;
        }
        else if (m_speedParam > 0.01f)
        {
            // Manual movement
            float moveSpeed = m_speedParam * 3.0f * dt;
            m_charX += inputX * moveSpeed;
            m_charZ += inputZ * moveSpeed;
        }

        // Collect events
        m_lastEvents.clear();
        const auto* clip = m_animator.GetCurrentClip();
        if (clip)
        {
            float curTime = m_animator.GetCurrentTime();
            float prevTime = curTime - dt * m_animator.GetSpeed();
            if (prevTime < 0.0f) prevTime += clip->GetDuration();
            std::vector<const GX::AnimationEvent*> events;
            clip->CollectEvents(prevTime, curTime, events);
            for (auto* e : events)
                m_lastEvents.push_back(e->name);
        }
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

        // Character body
        GX::Transform3D bodyT;
        bodyT.SetPosition(m_charX, 0.5f, m_charZ);
        ctx.renderer3D.SetMaterial(m_bodyMat);
        ctx.renderer3D.DrawMesh(m_bodyMesh, bodyT);

        // Draw skeleton joints as small spheres
        const auto& globals = m_animator.GetGlobalTransforms();
        ctx.renderer3D.SetMaterial(m_jointMat);
        for (uint32_t i = 0; i < m_skeleton.GetJointCount() && i < globals.size(); ++i)
        {
            XMFLOAT4X4 g = globals[i];
            GX::Transform3D jt;
            jt.SetPosition(m_charX + g._41, 0.5f + g._42, m_charZ + g._43);
            ctx.renderer3D.DrawMesh(m_jointMesh, jt);
        }

        ctx.renderer3D.End();
        ctx.postEffect.EndScene();

        auto& db = ctx.renderer3D.GetDepthBuffer();
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(), db, ctx.camera, m_lastDt);
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // HUD
        float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        const auto* state = m_stateMachine.GetCurrentState();
        TString stateName = state ? TString(state->name.begin(), state->name.end()) : TEXT("???");
        bool trans = m_stateMachine.IsTransitioning();

        DrawString(10, 10, FormatT(TEXT("FPS: {:.1f}"), fps).c_str(), GetColor(255, 255, 255));
        DrawString(10, 35,
            FormatT(TEXT("State: {} {}  Speed: {:.2f}  RootMotion: {}"),
                    stateName, trans ? TEXT("[TRANSITIONING]") : TEXT(""),
                    m_speedParam, m_rootMotionEnabled ? TEXT("ON") : TEXT("OFF")).c_str(),
            GetColor(100, 220, 255));
        DrawString(10, 60,
            FormatT(TEXT("Position: ({:.1f}, {:.1f})"), m_charX, m_charZ).c_str(),
            GetColor(180, 180, 180));

        // Event log
        if (!m_lastEvents.empty())
        {
            TString eventStr = TEXT("Events: ");
            for (auto& e : m_lastEvents)
            {
                eventStr += TString(e.begin(), e.end());
                eventStr += TEXT(" ");
            }
            DrawString(10, 85, eventStr.c_str(), GetColor(255, 200, 80));
        }

        DrawString(10, 110,
            TEXT("WASD: Move  Space: Jump  1-4: Force state  Tab: RootMotion  Shift+WASD: Camera"),
            GetColor(136, 136, 136));
    }

private:
    void BuildSkeleton()
    {
        // Simple 5-joint skeleton: Root, Spine, Head, LeftArm, RightArm
        auto AddJoint = [&](const std::string& name, int parent, float x, float y, float z)
        {
            GX::Joint joint;
            joint.name = name;
            joint.parentIndex = parent;
            XMMATRIX localMat = XMMatrixTranslation(x, y, z);
            XMStoreFloat4x4(&joint.localTransform, localMat);
            XMStoreFloat4x4(&joint.inverseBindMatrix, XMMatrixIdentity());
            m_skeleton.AddJoint(joint);
        };

        AddJoint("Root",     -1, 0.0f, 0.0f, 0.0f);
        AddJoint("Spine",     0, 0.0f, 0.5f, 0.0f);
        AddJoint("Head",      1, 0.0f, 0.5f, 0.0f);
        AddJoint("LeftArm",   1, -0.4f, 0.0f, 0.0f);
        AddJoint("RightArm",  1, 0.4f, 0.0f, 0.0f);
    }

    void BuildAnimations()
    {
        const uint32_t jointCount = m_skeleton.GetJointCount();

        // Idle clip: gentle breathing motion on Spine
        {
            m_idleClip.SetName("Idle");
            m_idleClip.SetDuration(2.0f);
            GX::AnimationChannel ch;
            ch.jointIndex = 1; // Spine
            ch.translationKeys.push_back({ 0.0f, { 0.0f, 0.5f, 0.0f } });
            ch.translationKeys.push_back({ 1.0f, { 0.0f, 0.53f, 0.0f } });
            ch.translationKeys.push_back({ 2.0f, { 0.0f, 0.5f, 0.0f } });
            ch.rotationKeys.push_back({ 0.0f, { 0.0f, 0.0f, 0.0f, 1.0f } });
            ch.scaleKeys.push_back({ 0.0f, { 1.0f, 1.0f, 1.0f } });
            m_idleClip.AddChannel(ch);
        }

        // Walk clip: root motion + arm swing
        {
            m_walkClip.SetName("Walk");
            m_walkClip.SetDuration(1.0f);

            // Root: forward motion
            GX::AnimationChannel rootCh;
            rootCh.jointIndex = 0;
            rootCh.translationKeys.push_back({ 0.0f, { 0.0f, 0.0f, 0.0f } });
            rootCh.translationKeys.push_back({ 1.0f, { 0.0f, 0.0f, 1.5f } });
            rootCh.rotationKeys.push_back({ 0.0f, { 0.0f, 0.0f, 0.0f, 1.0f } });
            rootCh.scaleKeys.push_back({ 0.0f, { 1.0f, 1.0f, 1.0f } });
            m_walkClip.AddChannel(rootCh);

            // LeftArm swing
            GX::AnimationChannel laCh;
            laCh.jointIndex = 3;
            laCh.translationKeys.push_back({ 0.0f, { -0.4f, 0.0f, 0.2f } });
            laCh.translationKeys.push_back({ 0.5f, { -0.4f, 0.0f, -0.2f } });
            laCh.translationKeys.push_back({ 1.0f, { -0.4f, 0.0f, 0.2f } });
            laCh.rotationKeys.push_back({ 0.0f, { 0.0f, 0.0f, 0.0f, 1.0f } });
            laCh.scaleKeys.push_back({ 0.0f, { 1.0f, 1.0f, 1.0f } });
            m_walkClip.AddChannel(laCh);

            // RightArm opposite swing
            GX::AnimationChannel raCh;
            raCh.jointIndex = 4;
            raCh.translationKeys.push_back({ 0.0f, { 0.4f, 0.0f, -0.2f } });
            raCh.translationKeys.push_back({ 0.5f, { 0.4f, 0.0f, 0.2f } });
            raCh.translationKeys.push_back({ 1.0f, { 0.4f, 0.0f, -0.2f } });
            raCh.rotationKeys.push_back({ 0.0f, { 0.0f, 0.0f, 0.0f, 1.0f } });
            raCh.scaleKeys.push_back({ 0.0f, { 1.0f, 1.0f, 1.0f } });
            m_walkClip.AddChannel(raCh);

            // Footstep events
            m_walkClip.AddEvent({ 0.25f, "footstep_left" });
            m_walkClip.AddEvent({ 0.75f, "footstep_right" });
        }

        // Run clip: faster motion
        {
            m_runClip.SetName("Run");
            m_runClip.SetDuration(0.6f);

            GX::AnimationChannel rootCh;
            rootCh.jointIndex = 0;
            rootCh.translationKeys.push_back({ 0.0f, { 0.0f, 0.0f, 0.0f } });
            rootCh.translationKeys.push_back({ 0.6f, { 0.0f, 0.0f, 3.0f } });
            rootCh.rotationKeys.push_back({ 0.0f, { 0.0f, 0.0f, 0.0f, 1.0f } });
            rootCh.scaleKeys.push_back({ 0.0f, { 1.0f, 1.0f, 1.0f } });
            m_runClip.AddChannel(rootCh);

            // Bigger arm swing
            GX::AnimationChannel laCh;
            laCh.jointIndex = 3;
            laCh.translationKeys.push_back({ 0.0f, { -0.4f, 0.0f, 0.4f } });
            laCh.translationKeys.push_back({ 0.3f, { -0.4f, 0.0f, -0.4f } });
            laCh.translationKeys.push_back({ 0.6f, { -0.4f, 0.0f, 0.4f } });
            laCh.rotationKeys.push_back({ 0.0f, { 0.0f, 0.0f, 0.0f, 1.0f } });
            laCh.scaleKeys.push_back({ 0.0f, { 1.0f, 1.0f, 1.0f } });
            m_runClip.AddChannel(laCh);

            m_runClip.AddEvent({ 0.15f, "footstep_left" });
            m_runClip.AddEvent({ 0.45f, "footstep_right" });
        }

        // Jump clip: one-shot upward
        {
            m_jumpClip.SetName("Jump");
            m_jumpClip.SetDuration(0.8f);

            GX::AnimationChannel rootCh;
            rootCh.jointIndex = 0;
            rootCh.translationKeys.push_back({ 0.0f, { 0.0f, 0.0f, 0.0f } });
            rootCh.translationKeys.push_back({ 0.3f, { 0.0f, 1.5f, 0.0f } });
            rootCh.translationKeys.push_back({ 0.8f, { 0.0f, 0.0f, 0.0f } });
            rootCh.rotationKeys.push_back({ 0.0f, { 0.0f, 0.0f, 0.0f, 1.0f } });
            rootCh.scaleKeys.push_back({ 0.0f, { 1.0f, 1.0f, 1.0f } });
            m_jumpClip.AddChannel(rootCh);

            m_jumpClip.AddEvent({ 0.0f, "jump_start" });
            m_jumpClip.AddEvent({ 0.8f, "jump_land" });
        }
    }

    void SetupStateMachine()
    {
        // States
        GX::AnimState idleState;
        idleState.name = "Idle";
        idleState.clip = &m_idleClip;
        idleState.loop = true;
        idleState.speed = 1.0f;
        m_stateMachine.AddState(idleState);

        GX::AnimState walkState;
        walkState.name = "Walk";
        walkState.clip = &m_walkClip;
        walkState.loop = true;
        walkState.speed = 1.0f;
        m_stateMachine.AddState(walkState);

        GX::AnimState runState;
        runState.name = "Run";
        runState.clip = &m_runClip;
        runState.loop = true;
        runState.speed = 1.0f;
        m_stateMachine.AddState(runState);

        GX::AnimState jumpState;
        jumpState.name = "Jump";
        jumpState.clip = &m_jumpClip;
        jumpState.loop = false;
        jumpState.speed = 1.0f;
        m_stateMachine.AddState(jumpState);

        // Transitions: Idle <-> Walk <-> Run, Any -> Jump, Jump -> Idle
        GX::AnimTransition t;

        // Idle -> Walk (speed > 0.1)
        t = {}; t.fromState = 0; t.toState = 1; t.duration = 0.2f;
        m_stateMachine.AddTransition(t);

        // Walk -> Idle (speed < 0.1)
        t = {}; t.fromState = 1; t.toState = 0; t.duration = 0.2f;
        m_stateMachine.AddTransition(t);

        // Walk -> Run (speed > 0.7)
        t = {}; t.fromState = 1; t.toState = 2; t.duration = 0.15f;
        m_stateMachine.AddTransition(t);

        // Run -> Walk (speed < 0.7)
        t = {}; t.fromState = 2; t.toState = 1; t.duration = 0.15f;
        m_stateMachine.AddTransition(t);

        // Any -> Jump (trigger)
        t = {}; t.fromState = 0; t.toState = 3; t.duration = 0.1f; t.triggerName = "jump";
        m_stateMachine.AddTransition(t);
        t.fromState = 1;
        m_stateMachine.AddTransition(t);
        t.fromState = 2;
        m_stateMachine.AddTransition(t);

        // Jump -> Idle (exit time)
        t = {}; t.fromState = 3; t.toState = 0; t.duration = 0.2f;
        t.hasExitTime = true; t.exitTimeNorm = 0.95f;
        m_stateMachine.AddTransition(t);
    }

    void SetupAnimator()
    {
        m_animator.SetSkeleton(&m_skeleton);
        m_animator.SetStateMachine(&m_stateMachine);
        m_animator.SetRootMotionEnabled(true);
        m_animator.SetLockRootPosition(false);
        m_animator.SetLockRootRotation(false);

        m_animator.SetEventCallback([this](const GX::AnimationEvent& evt)
        {
            m_lastEvents.push_back(evt.name);
        });

        m_animator.Play(&m_idleClip, true);
    }

    // Skeleton & Animation
    GX::Skeleton              m_skeleton;
    GX::AnimationClip         m_idleClip, m_walkClip, m_runClip, m_jumpClip;
    GX::AnimatorStateMachine  m_stateMachine;
    GX::Animator              m_animator;

    // Character state
    float m_charX = 0.0f, m_charZ = 0.0f;
    float m_speedParam = 0.0f;
    bool  m_rootMotionEnabled = true;
    std::vector<std::string> m_lastEvents;

    // Rendering
    GX::GPUMesh  m_planeMesh, m_bodyMesh, m_jointMesh;
    GX::Material m_floorMat, m_bodyMat, m_jointMat;

    float m_totalTime = 0.0f, m_lastDt = 0.0f;
    bool  m_mouseCaptured = false;
    int   m_lastMX = 0, m_lastMY = 0;
};

GX_EASY_APP(AnimationBlendingShowcaseApp)
