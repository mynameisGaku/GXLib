/// @file Samples/TilemapShowcase/main.cpp
/// @brief 2D Tilemap demo.
///
/// プロシージャルに生成したタイルマップを Camera2D でスクロール表示するサンプル。
/// Tilemap クラスの手動構築と描画を実演する。
///
/// Controls:
///   Arrow keys - Scroll map
///   Z/X        - Zoom in/out
///   ESC        - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/Rendering/Tilemap.h"
#include "Graphics/Rendering/Camera2D.h"

#include <cmath>
#include <Windows.h>

static constexpr int k_MapWidth  = 64;
static constexpr int k_MapHeight = 64;
static constexpr int k_TileSize  = 32;

class TilemapShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: 2D Tilemap";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 30; config.bgG = 50; config.bgB = 30;
        return config;
    }

    void Start() override
    {
        m_camera.SetPosition(k_MapWidth * k_TileSize / 2.0f,
                            k_MapHeight * k_TileSize / 2.0f);
        m_camera.SetZoom(1.0f);

        // プロシージャルにタイルマップを構築
        m_tilemap.Create(k_MapWidth, k_MapHeight, k_TileSize, k_TileSize);

        // 地面レイヤー
        GX::TilemapLayer groundLayer;
        groundLayer.name = "ground";
        groundLayer.width = k_MapWidth;
        groundLayer.height = k_MapHeight;
        groundLayer.tiles.resize(k_MapWidth * k_MapHeight);

        // プロシージャル地形生成（シンプルなパーリンノイズ風）
        for (int y = 0; y < k_MapHeight; ++y)
        {
            for (int x = 0; x < k_MapWidth; ++x)
            {
                float nx = static_cast<float>(x) / k_MapWidth;
                float ny = static_cast<float>(y) / k_MapHeight;
                float noise = sinf(nx * 12.0f) * cosf(ny * 8.0f)
                            + sinf(nx * 5.0f + ny * 7.0f) * 0.5f;

                int tileId;
                if (noise > 0.5f)
                    tileId = 3; // 山
                else if (noise > -0.2f)
                    tileId = 1; // 草
                else
                    tileId = 2; // 水

                groundLayer.tiles[y * k_MapWidth + x] = tileId;
            }
        }

        m_tilemap.AddLayer(groundLayer);
    }

    void Update(float dt) override
    {
        auto& kb = GX_Internal::CompatContext::Instance().inputManager.GetKeyboard();
        float scrollSpeed = 300.0f / m_camera.GetZoom();

        float cx = m_camera.GetPositionX();
        float cy = m_camera.GetPositionY();

        if (kb.IsKeyDown(VK_UP))    cy -= scrollSpeed * dt;
        if (kb.IsKeyDown(VK_DOWN))  cy += scrollSpeed * dt;
        if (kb.IsKeyDown(VK_LEFT))  cx -= scrollSpeed * dt;
        if (kb.IsKeyDown(VK_RIGHT)) cx += scrollSpeed * dt;

        m_camera.SetPosition(cx, cy);

        // ズーム
        float zoom = m_camera.GetZoom();
        if (kb.IsKeyDown('Z')) zoom *= (1.0f + 2.0f * dt);
        if (kb.IsKeyDown('X')) zoom *= (1.0f - 2.0f * dt);
        zoom = (std::max)(0.25f, (std::min)(zoom, 4.0f));
        m_camera.SetZoom(zoom);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();

        // タイルマップ描画（テクスチャなしのため、タイルIDに基づくカラー描画に代替）
        ctx.EnsurePrimitiveBatch();
        for (int ly = 0; ly < m_tilemap.GetLayerCount(); ++ly)
        {
            const auto& layer = m_tilemap.GetLayer(ly);

            float viewLeft = m_camera.GetPositionX() - 640.0f / m_camera.GetZoom();
            float viewTop  = m_camera.GetPositionY() - 360.0f / m_camera.GetZoom();
            float viewRight = m_camera.GetPositionX() + 640.0f / m_camera.GetZoom();
            float viewBottom = m_camera.GetPositionY() + 360.0f / m_camera.GetZoom();

            int startX = (std::max)(0, static_cast<int>(viewLeft / k_TileSize) - 1);
            int startY = (std::max)(0, static_cast<int>(viewTop / k_TileSize) - 1);
            int endX   = (std::min)(k_MapWidth, static_cast<int>(viewRight / k_TileSize) + 2);
            int endY   = (std::min)(k_MapHeight, static_cast<int>(viewBottom / k_TileSize) + 2);

            for (int y = startY; y < endY; ++y)
            {
                for (int x = startX; x < endX; ++x)
                {
                    int gid = m_tilemap.GetTile(ly, x, y);
                    if (gid <= 0) continue;

                    float worldX = static_cast<float>(x * k_TileSize);
                    float worldY = static_cast<float>(y * k_TileSize);

                    // ワールド→スクリーン変換
                    float screenX = (worldX - m_camera.GetPositionX()) * m_camera.GetZoom() + 640.0f;
                    float screenY = (worldY - m_camera.GetPositionY()) * m_camera.GetZoom() + 360.0f;
                    float tileScreenSize = k_TileSize * m_camera.GetZoom();

                    // タイルIDに基づくカラー
                    uint32_t color;
                    switch (gid)
                    {
                    case 1: color = 0xFF228B22; break; // 草 (緑)
                    case 2: color = 0xFF1E90FF; break; // 水 (青)
                    case 3: color = 0xFF8B7355; break; // 山 (茶)
                    default: color = 0xFF808080; break;
                    }

                    ctx.primBatch.DrawBox(
                        screenX, screenY,
                        screenX + tileScreenSize, screenY + tileScreenSize,
                        color, true);
                }
            }
        }

        // UI（SpriteBatchに切り替え）
        ctx.EnsureSpriteBatch();
        std::wstring info = std::format(
            L"Map: {}x{}  Zoom: {:.1f}  Pos: ({:.0f},{:.0f})  Arrow=Scroll Z/X=Zoom",
            k_MapWidth, k_MapHeight,
            m_camera.GetZoom(),
            m_camera.GetPositionX(), m_camera.GetPositionY());
        ctx.textRenderer.DrawString(ctx.defaultFontHandle, 10, 10, info, 0xFFFFFFFF);
    }

private:
    GX::Tilemap  m_tilemap;
    GX::Camera2D m_camera;
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    TilemapShowcaseApp app;
    return GXEasy::Run(app, app.GetConfig());
}
