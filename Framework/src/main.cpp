/// @file main.cpp
/// @brief GXLib Framework サンプルゲーム
///
/// GameBase を継承するだけで、エンジン初期化・デフォルトシーン
/// （カメラ + DirectionalLight）・レンダリングパイプラインが全て動作する。

#include "GameFramework.h"
#include <GXLib.h>

class SampleGame : public gx::GameBase
{
public:

    class MeshObject
    {
    public:
        GPUMesh mesh;
        Transform3D transform;
    };

    GPUMesh m_cube;
    Vector<MeshObject> m_spheres;

    void OnStart() override
    {
        // デフォルトシーンにはカメラと DirectionalLight が既に配置済み
        auto& cam = GetMainCamera();
        cam.SetPosition(0.0f, 5.0f, -15.0f);
        cam.LookAt({ 0.0f, 0.0f, 0.0f });

        ToggleDebugOverlay();

        MeshData meshData = MeshGenerator::CreateBox(2.0f, 2.0f, 2.0f);
        m_cube = GetRenderer().CreateGPUMesh(meshData);

        for(int i = 0; i < 10; ++i)
        {
            MeshData sphereData = MeshGenerator::CreateSphere(1.0f, 16, 16);
            MeshObject obj;
            obj.mesh = GetRenderer().CreateGPUMesh(sphereData);
            float rx = GetRandF(0.0f, 1.0f);
            float ry = GetRandF(0.0f, 1.0f);
            float rz = GetRandF(0.0f, 1.0f);
            float prx = rx - .5f;
            float prz = rz - .5f;
            obj.transform.SetPosition({ prx * 10.f, ry * 5.f, prz * 10.f });
            m_spheres.push_back(std::move(obj));
        }
    }

    void OnUpdate(float dt) override
    {
        auto& input = GetInput();
        auto& cam   = GetMainCamera();
        auto& mouse = input.GetMouse();

        // 右クリックでマウスキャプチャのトグル
        if (mouse.IsButtonTriggered(gx::MouseButton::Right))
        {
            m_mouseCaptured = !m_mouseCaptured;
            if (m_mouseCaptured)
            {
                ShowCursor(FALSE);
                CenterCursor();
            }
            else
            {
                ShowCursor(TRUE);
            }
        }

        // キャプチャ中: カーソル位置と中央の差分で視点回転 → 中央に戻す
        if (m_mouseCaptured)
        {
            HWND hwnd = GetWindow().GetHWND();
            RECT rc;
            GetClientRect(hwnd, &rc);
            int cx = (rc.right - rc.left) / 2;
            int cy = (rc.bottom - rc.top) / 2;

            POINT cur;
            GetCursorPos(&cur);
            ScreenToClient(hwnd, &cur);

            float dx = static_cast<float>(cur.x - cx);
            float dy = static_cast<float>(cur.y - cy);

            if (dx != 0.0f || dy != 0.0f)
            {
                cam.Rotate(-dy * 0.003f, dx * 0.003f);
                CenterCursor();
            }
        }

        // WASD でカメラ移動 (VK_* は Windows 仮想キーコード)
        float speed = 50.0f * dt;
        if (input.GetKeyboard().IsKeyDown('W'))        cam.MoveForward(speed);
        if (input.GetKeyboard().IsKeyDown('S'))        cam.MoveForward(-speed);
        if (input.GetKeyboard().IsKeyDown('A'))        cam.MoveRight(-speed);
        if (input.GetKeyboard().IsKeyDown('D'))        cam.MoveRight(speed);
        if (input.GetKeyboard().IsKeyDown(VK_SPACE))   cam.MoveUp(speed);
        if (input.GetKeyboard().IsKeyDown(VK_LSHIFT))  cam.MoveUp(-speed);
    }

    void OnDraw3D() override
    {
        Transform3D t;
        t.SetPosition({ 0.0f, -0.5f, 0.0f });
        t.SetScale({15.0f, 1.0f, 15.0f});
        GetRenderer().DrawMesh(m_cube, t);

        for (const auto& obj : m_spheres)
        {
            GetRenderer().DrawMesh(obj.mesh, obj.transform);
        }
    }

    void OnDraw2D() override
    {
        unsigned int white = 0xFFFFFFFF;
        GetText().DrawFormatString(0, 10, 10, white,
                                   L"FPS: %.1f", 1.0f / GetDeltaTime());

        auto pos = GetMainCamera().GetPosition();
        GetText().DrawFormatString(0, 10, 35, 0xFF88BBFF,
                                   L"Camera: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
    }

private:
    void CenterCursor()
    {
        HWND hwnd = GetWindow().GetHWND();
        RECT rc;
        GetClientRect(hwnd, &rc);
        POINT center = { (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
        ClientToScreen(hwnd, &center);
        SetCursorPos(center.x, center.y);
    }

    bool m_mouseCaptured = false;
};

GX_GAME_MAIN(SampleGame)
