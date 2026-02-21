/// @file Samples/ActionMappingShowcase/main.cpp
/// @brief アクションマッピングデモ
///
/// キーボード/ゲームパッド両対応のキャラクター移動とバインディング可視化。
/// ActionMapping で論理アクション(MoveX/MoveY/Jump/Dash)を定義し、
/// 円をリアルタイムで操作する。
///
/// 使用API:
///   - ActionMapping::DefineAction()
///   - IsActionTriggered() / IsActionPressed() / GetActionValue()
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Input/ActionMapping.h"

#include <cmath>
#include <algorithm>
#include <Windows.h>

class ActionMappingShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Action Mapping";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 20; config.bgG = 22; config.bgB = 30;
        return config;
    }

    void Start() override
    {
        // MoveX: A/D キー + 左スティックX
        m_actionMap.DefineAction("MoveX", {
            GX::InputBinding::KeyAxis('D', 1.0f),
            GX::InputBinding::KeyAxis('A', -1.0f),
            GX::InputBinding::PadAxis(GX::GamepadAxisId::LeftStickX),
        });

        // MoveY: W/S キー + 左スティックY
        m_actionMap.DefineAction("MoveY", {
            GX::InputBinding::KeyAxis('W', -1.0f),
            GX::InputBinding::KeyAxis('S', 1.0f),
            GX::InputBinding::PadAxis(GX::GamepadAxisId::LeftStickY, -1.0f),
        });

        // Jump: Space / ゲームパッドA
        m_actionMap.DefineAction("Jump", {
            GX::InputBinding::Key(VK_SPACE),
            GX::InputBinding::PadBtn(XINPUT_GAMEPAD_A),
        });

        // Dash: Shift / ゲームパッドB
        m_actionMap.DefineAction("Dash", {
            GX::InputBinding::Key(VK_LSHIFT),
            GX::InputBinding::PadBtn(XINPUT_GAMEPAD_B),
        });

        m_posX = 640.0f;
        m_posY = 400.0f;
    }

    void Update(float dt) override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        m_lastDt = dt;

        // アクションマッピング更新
        m_actionMap.Update(
            ctx.inputManager.GetKeyboard(),
            ctx.inputManager.GetMouse(),
            ctx.inputManager.GetGamepad()
        );

        // 移動
        float moveX = m_actionMap.GetActionValue("MoveX");
        float moveY = m_actionMap.GetActionValue("MoveY");
        float speed = m_actionMap.IsActionPressed("Dash") ? 400.0f : 200.0f;

        m_posX += moveX * speed * dt;
        m_posY += moveY * speed * dt;

        // 画面内にクランプ
        m_posX = (std::max)(30.0f, (std::min)(m_posX, 1250.0f));
        m_posY = (std::max)(30.0f, (std::min)(m_posY, 690.0f));

        // ジャンプ（Yを一時的に上に変位）
        if (m_actionMap.IsActionTriggered("Jump"))
            m_jumpVelocity = -300.0f;

        m_jumpOffset += m_jumpVelocity * dt;
        m_jumpVelocity += 1200.0f * dt;  // 重力
        if (m_jumpOffset > 0.0f)
        {
            m_jumpOffset = 0.0f;
            m_jumpVelocity = 0.0f;
        }

        // 状態保存
        m_moveXVal = moveX;
        m_moveYVal = moveY;
        m_dashActive = m_actionMap.IsActionPressed("Dash");
        m_jumpTriggered = m_actionMap.IsActionTriggered("Jump");
    }

    void Draw() override
    {
        // キャラクター（円）
        int drawY = static_cast<int>(m_posY + m_jumpOffset);
        unsigned int charColor = m_dashActive ? GetColor(255, 100, 100) : GetColor(100, 200, 255);
        DrawCircle(static_cast<int>(m_posX), drawY, 20, charColor, TRUE);

        // ジャンプ中に影
        if (m_jumpOffset < -1.0f)
        {
            float shadowScale = 1.0f + m_jumpOffset / 300.0f;
            int shadowR = static_cast<int>(20.0f * (std::max)(0.3f, shadowScale));
            DrawOval(static_cast<int>(m_posX), static_cast<int>(m_posY),
                     shadowR, shadowR / 3, GetColor(40, 40, 40), TRUE);
        }

        // 方向インジケーター
        if (std::abs(m_moveXVal) > 0.01f || std::abs(m_moveYVal) > 0.01f)
        {
            float len = std::sqrt(m_moveXVal * m_moveXVal + m_moveYVal * m_moveYVal);
            float nx = m_moveXVal / len;
            float ny = m_moveYVal / len;
            int endX = static_cast<int>(m_posX + nx * 35.0f);
            int endY = drawY + static_cast<int>(ny * 35.0f);
            DrawLine(static_cast<int>(m_posX), drawY, endX, endY, GetColor(255, 255, 100));
        }

        // --- アクション状態パネル ---
        int panelX = 10, panelY = 10;
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(panelX, panelY,
                   FormatT(TEXT("FPS: {:.1f}"), fps).c_str(),
                   GetColor(255, 255, 255));
        panelY += 25;

        DrawString(panelX, panelY, TEXT("=== Action States ==="), GetColor(180, 180, 255));
        panelY += 25;

        // MoveX
        DrawString(panelX, panelY,
                   FormatT(TEXT("MoveX: {:.2f}  [A/D or LStick X]"), m_moveXVal).c_str(),
                   GetColor(200, 200, 200));
        panelY += 20;

        // MoveX バー
        DrawBox(panelX, panelY, panelX + 200, panelY + 10, GetColor(60, 60, 60), TRUE);
        int barCenter = panelX + 100;
        int barEnd = barCenter + static_cast<int>(m_moveXVal * 100.0f);
        unsigned int barColor = m_moveXVal > 0 ? GetColor(100, 200, 100) : GetColor(200, 100, 100);
        DrawBox((std::min)(barCenter, barEnd), panelY, (std::max)(barCenter, barEnd), panelY + 10, barColor, TRUE);
        panelY += 18;

        // MoveY
        DrawString(panelX, panelY,
                   FormatT(TEXT("MoveY: {:.2f}  [W/S or LStick Y]"), m_moveYVal).c_str(),
                   GetColor(200, 200, 200));
        panelY += 20;

        // MoveY バー
        DrawBox(panelX, panelY, panelX + 200, panelY + 10, GetColor(60, 60, 60), TRUE);
        barEnd = barCenter + static_cast<int>(m_moveYVal * 100.0f);
        barColor = m_moveYVal > 0 ? GetColor(100, 200, 100) : GetColor(200, 100, 100);
        DrawBox((std::min)(barCenter, barEnd), panelY, (std::max)(barCenter, barEnd), panelY + 10, barColor, TRUE);
        panelY += 18;

        // Jump
        unsigned int jumpColor = m_jumpTriggered ? GetColor(255, 255, 100) : GetColor(200, 200, 200);
        const TChar* jumpState = m_jumpTriggered ? TEXT("TRIGGERED!") : TEXT("---");
        DrawString(panelX, panelY,
                   FormatT(TEXT("Jump: {}  [Space or Pad A]"), jumpState).c_str(),
                   jumpColor);
        panelY += 25;

        // Dash
        unsigned int dashColor = m_dashActive ? GetColor(255, 100, 100) : GetColor(200, 200, 200);
        const TChar* dashState = m_dashActive ? TEXT("ACTIVE") : TEXT("---");
        DrawString(panelX, panelY,
                   FormatT(TEXT("Dash: {}  [Shift or Pad B]"), dashState).c_str(),
                   dashColor);
        panelY += 30;

        DrawString(panelX, panelY,
                   TEXT("WASD: Move  Space: Jump  Shift: Dash  ESC: Quit"),
                   GetColor(136, 136, 136));
    }

private:
    float m_lastDt = 0.0f;

    GX::ActionMapping m_actionMap;

    float m_posX = 640.0f;
    float m_posY = 400.0f;
    float m_jumpOffset   = 0.0f;
    float m_jumpVelocity = 0.0f;

    // 表示用
    float m_moveXVal = 0.0f;
    float m_moveYVal = 0.0f;
    bool  m_dashActive    = false;
    bool  m_jumpTriggered = false;
};

GX_EASY_APP(ActionMappingShowcaseApp)
