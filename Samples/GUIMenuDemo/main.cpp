/// @file Samples/GUIMenuDemo/main.cpp
/// @brief GXFrameworkでGUIメニューを作るデモ。UIの組み立て手順が分かる。

#include "FrameworkApp.h"
#include "GameScene.h"

#include "Core/Logger.h"
#include "GUI/GUILoader.h"
#include "GUI/UIContext.h"
#include "GUI/UIRenderer.h"
#include "GUI/StyleSheet.h"
#include "GUI/Widgets/Dialog.h"
#include "GUI/Widgets/Slider.h"
#include "GUI/Widgets/CheckBox.h"
#include "GUI/Widgets/RadioButton.h"

#include <filesystem>

namespace
{

class GUIMenuScene : public GXFW::GameScene
{
public:
    const char* GetName() const override { return "GUIMenuDemo"; }

protected:
    void OnSceneEnter(GXFW::SceneContext& ctx) override
    {
        m_screenW = ctx.swapChain->GetWidth();
        m_screenH = ctx.swapChain->GetHeight();
        m_lastW = m_screenW;
        m_lastH = m_screenH;
        if (ctx.app)
        {
            m_designW = ctx.app->GetWindow().GetWidth();
            m_designH = ctx.app->GetWindow().GetHeight();
        }

        if (!m_uiRenderer.Initialize(ctx.graphics->GetDevice(),
                                     ctx.commandQueue->GetQueue(),
                                     m_screenW, m_screenH,
                                     ctx.spriteBatch, ctx.textRenderer, ctx.fontManager))
        {
            GX_LOG_ERROR("UIRenderer initialization failed");
        }

        if (!m_uiContext.Initialize(&m_uiRenderer, m_screenW, m_screenH))
        {
            GX_LOG_ERROR("UIContext initialization failed");
        }
        m_uiContext.SetDesignResolution(m_designW, m_designH);

        m_fontHandle = ctx.fontManager->CreateFont(L"Segoe UI", 22);
        if (m_fontHandle < 0) m_fontHandle = ctx.fontManager->CreateFont(L"MS Gothic", 22);
        if (m_fontHandle < 0 && ctx.defaultFont >= 0) m_fontHandle = ctx.defaultFont;
        m_fontLarge = ctx.fontManager->CreateFont(L"Segoe UI", 40);
        if (m_fontLarge < 0) m_fontLarge = m_fontHandle;
        if (m_fontLarge < 0 && ctx.defaultFont >= 0) m_fontLarge = ctx.defaultFont;

        SetupUILoader();
        ResolveUIPaths();
        ReloadUI(true);

        ctx.app->GetWindow().AddMessageCallback([this](HWND, UINT msg, WPARAM wParam, LPARAM) -> bool {
            if (msg == WM_CHAR)
                return m_uiContext.ProcessCharMessage(static_cast<wchar_t>(wParam));
            return false;
        });
    }

    void OnSceneUpdate(GXFW::SceneContext& ctx, float dt) override
    {
        if (ctx.input->CheckHitKey(VK_F2))
            m_debugLayout = !m_debugLayout;

        if (ctx.input->CheckHitKey(VK_F5))
            ReloadUI(true);
        else
            ReloadUI(false);

        if (ctx.input->CheckHitKey(VK_ESCAPE))
        {
            if (m_currentScreen == MenuScreen::Settings)
            {
                m_currentScreen = MenuScreen::Main;
                ApplyUIState();
            }
            else
            {
                PostQuitMessage(0);
            }
            return;
        }

        m_uiContext.Update(dt, *ctx.input);

        if (m_debugLayout && !m_layoutLogged)
        {
            LogWidgetRect("root", "Root");
            LogWidgetRect("title", "Title");
            LogWidgetRect("btnStart", "Start");
            LogWidgetRect("btnSettings", "Settings");
            LogWidgetRect("btnAbout", "About");
            LogWidgetRect("btnExit", "Exit");
            LogWidgetRect("btnBack", "Back");
            m_layoutLogged = true;
        }

        uint32_t w = ctx.swapChain->GetWidth();
        uint32_t h = ctx.swapChain->GetHeight();
        uint32_t designW = m_designW;
        uint32_t designH = m_designH;
        if (ctx.app)
        {
            designW = ctx.app->GetWindow().GetWidth();
            designH = ctx.app->GetWindow().GetHeight();
        }

        if (designW != m_designW || designH != m_designH)
        {
            m_designW = designW;
            m_designH = designH;
            m_uiContext.SetDesignResolution(m_designW, m_designH);
        }
        if (w != m_lastW || h != m_lastH)
        {
            m_lastW = w;
            m_lastH = h;
            m_screenW = w;
            m_screenH = h;
            m_uiRenderer.OnResize(w, h);
            m_uiContext.OnResize(w, h);
            m_uiContext.SetDesignResolution(m_designW, m_designH);
        }
    }

    void OnSceneRenderUI(GXFW::SceneContext& ctx) override
    {
        // UI描画（UIRendererがSpriteBatchを使う前にフラッシュ）。
        // 2D描画の順序を保つため、途中でバッチを閉じるのがポイント。
        ctx.Flush2D();
        m_uiRenderer.Begin(ctx.cmd, ctx.frameIndex);
        m_uiContext.Render();
        m_uiRenderer.End();

        if (m_debugLayout)
        {
            DrawWidgetRect(ctx, "btnStart", 0xFF00FF00);
            DrawWidgetRect(ctx, "btnSettings", 0xFF00FF00);
            DrawWidgetRect(ctx, "btnAbout", 0xFF00FF00);
            DrawWidgetRect(ctx, "btnExit", 0xFF00FF00);
            DrawWidgetRect(ctx, "btnBack", 0xFFFFAA00);
        }

        // HUD表示。FPSなどの補助情報を描く。
        if (ctx.app)
        {
            ctx.DrawString((float)10, 10.0f,
                           std::format(L"FPS: {:.0f}", ctx.app->GetTimer().GetFPS()),
                           0xFF888888);
            ctx.DrawString(10.0f, (float)m_screenH - 30.0f,
                           L"ESC: Back/Quit", 0xFF666666);
        }
    }

private:
    enum class MenuScreen { Main, Settings, About };

    GX::GUI::UIRenderer m_uiRenderer;
    GX::GUI::UIContext  m_uiContext;
    GX::GUI::StyleSheet m_styleSheet;

    GX::GUI::Dialog* m_aboutDialog = nullptr;
    MenuScreen m_currentScreen = MenuScreen::Main;

    float m_volume = 0.8f;
    float m_brightness = 0.5f;
    bool  m_fullscreen = false;
    bool  m_vsync = true;
    std::string m_difficulty = "Normal";

    int m_fontHandle = -1;
    int m_fontLarge = -1;
    bool m_debugLayout = false;
    bool m_layoutLogged = false;
    uint32_t m_designW = 1280;
    uint32_t m_designH = 720;
    uint32_t m_screenW = 1280;
    uint32_t m_screenH = 720;
    uint32_t m_lastW = 1280;
    uint32_t m_lastH = 720;
    GX::GUI::GUILoader m_uiLoader;
    std::string m_uiXmlPath = "Assets/ui/guimenu_demo.xml";
    std::string m_uiCssPath = "Assets/ui/guimenu_demo.css";
    std::filesystem::file_time_type m_uiXmlTime;
    std::filesystem::file_time_type m_uiCssTime;
    bool m_aboutVisible = false;

    static constexpr const char* k_UiXmlRelPath = "Assets/ui/guimenu_demo.xml";
    static constexpr const char* k_UiCssRelPath = "Assets/ui/guimenu_demo.css";

    void SetupUILoader()
    {
        m_uiLoader = GX::GUI::GUILoader();
        m_uiLoader.SetRenderer(&m_uiRenderer);
        m_uiLoader.RegisterFont("default", m_fontHandle);
        m_uiLoader.RegisterFont("title", m_fontLarge);
        m_uiLoader.RegisterDrawCallback("DrawBackground",
            [this](GX::GUI::UIRenderer& renderer, const GX::GUI::LayoutRect& rect)
            {
                DrawBackground(renderer, rect);
            });

        m_uiLoader.RegisterEvent("StartGame", []() { GX_LOG_INFO("Start Game clicked!"); });
        m_uiLoader.RegisterEvent("OpenSettings", [this]() {
            m_currentScreen = MenuScreen::Settings;
            ApplyUIState();
        });
        m_uiLoader.RegisterEvent("OpenAbout", [this]() { ShowAboutDialog(); });
        m_uiLoader.RegisterEvent("ExitGame", []() { PostQuitMessage(0); });
        m_uiLoader.RegisterEvent("BackToMain", [this]() {
            m_currentScreen = MenuScreen::Main;
            ApplyUIState();
        });
        m_uiLoader.RegisterEvent("CloseAbout", [this]() {
            m_aboutVisible = false;
            if (m_aboutDialog) m_aboutDialog->visible = false;
        });

        m_uiLoader.RegisterValueChangedEvent("VolumeChanged", [this](const std::string& v) {
            m_volume = std::stof(v);
            GX_LOG_INFO("Volume: %.2f", m_volume);
        });
        m_uiLoader.RegisterValueChangedEvent("BrightnessChanged", [this](const std::string& v) {
            m_brightness = std::stof(v);
            GX_LOG_INFO("Brightness: %.2f", m_brightness);
        });
        m_uiLoader.RegisterValueChangedEvent("FullscreenChanged", [this](const std::string& v) {
            m_fullscreen = (v == "1");
            GX_LOG_INFO("Fullscreen: %s", m_fullscreen ? "ON" : "OFF");
        });
        m_uiLoader.RegisterValueChangedEvent("VSyncChanged", [this](const std::string& v) {
            m_vsync = (v == "1");
            GX_LOG_INFO("VSync: %s", m_vsync ? "ON" : "OFF");
        });
        m_uiLoader.RegisterValueChangedEvent("DifficultyChanged", [this](const std::string& v) {
            m_difficulty = v;
            GX_LOG_INFO("Difficulty: %s", v.c_str());
        });
    }

    void ResolveUIPaths()
    {
        m_uiXmlPath = ResolveAssetPath(k_UiXmlRelPath);
        m_uiCssPath = ResolveAssetPath(k_UiCssRelPath);
        GX_LOG_INFO("UI XML: %s", m_uiXmlPath.c_str());
        GX_LOG_INFO("UI CSS: %s", m_uiCssPath.c_str());
    }

    void ReloadUI(bool force)
    {
        if (force)
        {
            LoadStyleSheet();
            LoadUILayout();
            UpdateFileTimestamp(m_uiCssPath, m_uiCssTime);
            UpdateFileTimestamp(m_uiXmlPath, m_uiXmlTime);
            ApplyUIState();
            m_layoutLogged = false;
            return;
        }

        bool cssChanged = CheckFileChanged(m_uiCssPath, m_uiCssTime);
        bool xmlChanged = CheckFileChanged(m_uiXmlPath, m_uiXmlTime);
        if (!cssChanged && !xmlChanged)
            return;

        if (cssChanged)
            LoadStyleSheet();
        if (xmlChanged)
            LoadUILayout();
        ApplyUIState();
        m_layoutLogged = false;
    }

    bool LoadStyleSheet()
    {
        if (!m_styleSheet.LoadFromFile(m_uiCssPath))
        {
            GX_LOG_WARN("GUIMenuDemo: Failed to load CSS: %s", m_uiCssPath.c_str());
            return false;
        }
        m_uiContext.SetStyleSheet(&m_styleSheet);
        GX_LOG_INFO("GUIMenuDemo: CSS reloaded");
        return true;
    }

    bool LoadUILayout()
    {
        auto root = m_uiLoader.BuildFromFile(m_uiXmlPath);
        if (!root)
        {
            GX_LOG_WARN("GUIMenuDemo: Failed to load XML: %s", m_uiXmlPath.c_str());
            return false;
        }
        m_uiContext.SetRoot(std::move(root));
        m_aboutDialog = dynamic_cast<GX::GUI::Dialog*>(m_uiContext.FindById("aboutDialog"));
        GX_LOG_INFO("GUIMenuDemo: XML reloaded");
        return true;
    }

    void ApplyUIState()
    {
        if (auto* menuCard = m_uiContext.FindById("menuCard"))
            menuCard->visible = (m_currentScreen == MenuScreen::Main);
        if (auto* settingsPanel = m_uiContext.FindById("settingsPanel"))
            settingsPanel->visible = (m_currentScreen == MenuScreen::Settings);

        if (auto* slider = dynamic_cast<GX::GUI::Slider*>(m_uiContext.FindById("sliderVolume")))
            slider->SetValue(m_volume);
        if (auto* slider = dynamic_cast<GX::GUI::Slider*>(m_uiContext.FindById("sliderBrightness")))
            slider->SetValue(m_brightness);
        if (auto* cb = dynamic_cast<GX::GUI::CheckBox*>(m_uiContext.FindById("chkFullscreen")))
            cb->SetChecked(m_fullscreen);
        if (auto* cb = dynamic_cast<GX::GUI::CheckBox*>(m_uiContext.FindById("chkVsync")))
            cb->SetChecked(m_vsync);

        if (auto* rb = dynamic_cast<GX::GUI::RadioButton*>(m_uiContext.FindById("rbEasy")))
            rb->SetSelected(m_difficulty == "Easy");
        if (auto* rb = dynamic_cast<GX::GUI::RadioButton*>(m_uiContext.FindById("rbNormal")))
            rb->SetSelected(m_difficulty == "Normal");
        if (auto* rb = dynamic_cast<GX::GUI::RadioButton*>(m_uiContext.FindById("rbHard")))
            rb->SetSelected(m_difficulty == "Hard");

        if (m_aboutDialog)
            m_aboutDialog->visible = m_aboutVisible;
    }

    void ShowAboutDialog()
    {
        m_aboutVisible = true;
        if (m_aboutDialog) m_aboutDialog->visible = true;
    }

    static std::filesystem::path FindRepoRoot()
    {
        std::filesystem::path cur = std::filesystem::current_path();
        for (int i = 0; i < 6; ++i)
        {
            if (std::filesystem::exists(cur / "CMakeLists.txt"))
                return cur;
            if (!cur.has_parent_path())
                break;
            cur = cur.parent_path();
        }
        return {};
    }

    static std::string ResolveAssetPath(const std::string& relative)
    {
        std::filesystem::path repo = FindRepoRoot();
        if (!repo.empty())
        {
            auto p = repo / relative;
            if (std::filesystem::exists(p))
                return p.string();
        }

        auto p = std::filesystem::current_path() / relative;
        if (std::filesystem::exists(p))
            return p.string();

        return relative;
    }

    static bool CheckFileChanged(const std::string& path, std::filesystem::file_time_type& lastTime)
    {
        std::error_code ec;
        auto t = std::filesystem::last_write_time(path, ec);
        if (ec) return false;

        if (lastTime == std::filesystem::file_time_type{})
        {
            lastTime = t;
            return false;
        }
        if (t != lastTime)
        {
            lastTime = t;
            return true;
        }
        return false;
    }

    static void UpdateFileTimestamp(const std::string& path, std::filesystem::file_time_type& outTime)
    {
        std::error_code ec;
        auto t = std::filesystem::last_write_time(path, ec);
        if (!ec)
            outTime = t;
    }

    void LogWidgetRect(const char* id, const char* label)
    {
        auto* w = m_uiContext.FindById(id);
        if (!w)
        {
            GX_LOG_WARN("UI debug: %s not found", label);
            return;
        }
        const auto& r = w->globalRect;
        const auto& s = w->computedStyle;
        GX_LOG_INFO("UI debug: %s rect x=%.1f y=%.1f w=%.1f h=%.1f bgA=%.2f textA=%.2f",
                    label, r.x, r.y, r.width, r.height, s.backgroundColor.a, s.color.a);
    }

    void DrawWidgetRect(GXFW::SceneContext& ctx, const char* id, uint32_t color)
    {
        auto* w = m_uiContext.FindById(id);
        if (!w) return;
        const auto& r = w->globalRect;
        float scale = m_uiRenderer.GetGuiScale();
        float ox = m_uiRenderer.GetGuiOffsetX();
        float oy = m_uiRenderer.GetGuiOffsetY();
        float x1 = r.x * scale + ox;
        float y1 = r.y * scale + oy;
        float x2 = (r.x + r.width) * scale + ox;
        float y2 = (r.y + r.height) * scale + oy;
        ctx.DrawBox(x1, y1, x2, y2, color, false);
    }

    static uint32_t RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        return (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
    }

    void DrawBackground(GX::GUI::UIRenderer& renderer, const GX::GUI::LayoutRect& rect)
    {
        GX::GUI::StyleColor top = { 0.17f, 0.23f, 0.45f, 1.0f };
        GX::GUI::StyleColor bottom = { 0.06f, 0.08f, 0.15f, 1.0f };
        float dirX = 0.0f;
        float dirY = 1.0f;

        if (auto* bg = m_uiContext.FindById("bgCanvas"))
        {
            if (!bg->computedStyle.backgroundColor.IsTransparent())
                top = bg->computedStyle.backgroundColor;
            else if (!bg->computedStyle.shadowColor.IsTransparent())
                top = bg->computedStyle.shadowColor;
            if (!bg->computedStyle.borderColor.IsTransparent())
                bottom = bg->computedStyle.borderColor;
            else
                bottom = top;

            if (bg->computedStyle.shadowOffsetX != 0.0f || bg->computedStyle.shadowOffsetY != 0.0f)
            {
                dirX = bg->computedStyle.shadowOffsetX;
                dirY = bg->computedStyle.shadowOffsetY;
            }
        }

        renderer.DrawGradientRect(rect.x, rect.y, rect.width, rect.height,
                                  top, bottom, dirX, dirY);
    }
};

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    GXFW::FrameworkApp app;
    GXFW::AppConfig config;
    config.title = L"GXLib Sample: GUI Menu Demo";
    config.width = 1280;
    config.height = 720;
    config.enableDebug = true;

    if (!app.Initialize(config))
        return -1;

    app.SetScene(std::make_unique<GUIMenuScene>());
    app.Run();
    app.Shutdown();
    return 0;
}
