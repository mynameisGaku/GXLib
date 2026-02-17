/// @file Samples/EasyHello/main.cpp
/// @brief GXLib の最小サンプル。矢印キーで円を動かす。
#include "GXEasy.h"

class EasyHelloApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title = L"GXLib Sample: EasyHello";
        config.width = 1280;
        config.height = 720;
        config.bgR = 12;
        config.bgG = 12;
        config.bgB = 28;
        return config;
    }

    void Start() override
    {
        m_x = 200.0f;
        m_y = 180.0f;
    }

    void Update(float dt) override
    {
        const float speed = 220.0f;
        if (CheckHitKey(KEY_INPUT_LEFT))  m_x -= speed * dt;
        if (CheckHitKey(KEY_INPUT_RIGHT)) m_x += speed * dt;
        if (CheckHitKey(KEY_INPUT_UP))    m_y -= speed * dt;
        if (CheckHitKey(KEY_INPUT_DOWN))  m_y += speed * dt;
    }

    void Draw() override
    {
        DrawString(20, 20, TEXT("EasyHello: Arrow keys to move"), GetColor(220, 220, 240));
        DrawCircle(static_cast<int>(m_x), static_cast<int>(m_y), 30, GetColor(255, 200, 80), TRUE);
        DrawString(20, 50, TEXT("ESC: Quit"), GetColor(140, 180, 220));
    }

private:
    float m_x = 0.0f;
    float m_y = 0.0f;
};

GX_EASY_APP(EasyHelloApp)

