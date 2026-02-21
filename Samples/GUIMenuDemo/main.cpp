/// @file Samples/GUIMenuDemo/main.cpp
/// @brief GUIメニューのデモ。XML/CSSのホットリロード対応。
///
/// GXLibのGUIシステムを使って、ゲームのメインメニューを構築する例。
///
/// 特徴:
///   - XML+CSSでレイアウトを定義（WebフロントエンドのようなUI定義）
///   - ファイルの更新を監視し、CSSやXMLを変更すると即座に反映される
///   - Slider/CheckBox/RadioButton/Dialogなどのウィジェットを組み合わせ
///   - UIRendererはSpriteBatch+TextRenderer上に構築されている
///
/// DxLibにはGUI相当のAPIがないため、これはGXLib独自の機能。
#include "GXEasy.h"
#include "Compat/CompatContext.h"

#include "Core/Logger.h"
#include "GUI/GUILoader.h"             // XMLからウィジェットツリーを構築
#include "GUI/UIContext.h"             // UIの更新/描画を管理
#include "GUI/UIRenderer.h"            // UIの描画バックエンド（SpriteBatch上）
#include "GUI/StyleSheet.h"            // CSSスタイルシート
#include "GUI/Widgets/Dialog.h"        // モーダルダイアログ
#include "GUI/Widgets/Slider.h"        // スライダー
#include "GUI/Widgets/CheckBox.h"      // チェックボックス
#include "GUI/Widgets/RadioButton.h"   // ラジオボタン

#include <filesystem>
#include <format>
#include <string>

#ifdef UNICODE
using TChar = wchar_t;
#else
using TChar = char;
#endif

using TString = std::basic_string<TChar>;

template <class... Args>
TString FormatT(const TChar* fmt, Args&&... args)
{
#ifdef UNICODE
    return std::vformat(fmt, std::make_wformat_args(std::forward<Args>(args)...));
#else
    return std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
#endif
}

/// @brief GUIメニューデモのメインクラス
///
/// メイン画面と設定画面をMenuScreenで切り替え、
/// XML+CSSで定義したUIレイアウトをリアルタイムにリロードできる。
class GUIMenuApp : public GXEasy::App
{
public:
    /// @brief ウィンドウ設定
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title = L"GXLib Sample: GUI Menu Demo";
        config.width = 1280;
        config.height = 720;
        config.bgR = 8;
        config.bgG = 10;
        config.bgB = 20;
        return config;
    }

    /// @brief UIシステムの初期化（UIRenderer/UIContext/フォント/レイアウト）
    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();

        m_screenW = ctx.swapChain.GetWidth();
        m_screenH = ctx.swapChain.GetHeight();
        m_lastW = m_screenW;
        m_lastH = m_screenH;
        m_designW = ctx.app.GetWindow().GetWidth();
        m_designH = ctx.app.GetWindow().GetHeight();

        // UIRendererはSpriteBatch/TextRendererに依存しており、
        // デバイスとコマンドキュー+画面サイズで初期化する。
        if (!m_uiRenderer.Initialize(ctx.graphicsDevice.GetDevice(),
                                     ctx.commandQueue.GetQueue(),
                                     m_screenW, m_screenH,
                                     &ctx.spriteBatch, &ctx.textRenderer, &ctx.fontManager))
        {
            GX_LOG_ERROR("UIRenderer initialization failed");
        }

        // UIContextはウィジェットの更新・レイアウト計算・描画を統括する
        if (!m_uiContext.Initialize(&m_uiRenderer, m_screenW, m_screenH))
        {
            GX_LOG_ERROR("UIContext initialization failed");
        }
        // デザイン解像度を指定すると、画面サイズに応じて自動スケーリングされる
        m_uiContext.SetDesignResolution(m_designW, m_designH);

        // フォント生成（環境依存なので複数フォールバックする）
        m_fontHandle = ctx.fontManager.CreateFont(L"Segoe UI", 22);
        if (m_fontHandle < 0) m_fontHandle = ctx.fontManager.CreateFont(L"MS Gothic", 22);
        if (m_fontHandle < 0) m_fontHandle = ctx.defaultFontHandle;
        m_fontLarge = ctx.fontManager.CreateFont(L"Segoe UI", 40);
        if (m_fontLarge < 0) m_fontLarge = m_fontHandle;
        if (m_fontLarge < 0) m_fontLarge = ctx.defaultFontHandle;

        // GUILoaderのイベント登録→パス解決→初回ロード
        SetupUILoader();
        ResolveUIPaths();
        ReloadUI(true);  // force=trueで必ず読み込む

        // テキスト入力をUIに渡す
        ctx.app.GetWindow().AddMessageCallback([this](HWND, UINT msg, WPARAM wParam, LPARAM) -> bool {
            if (msg == WM_CHAR)
                return m_uiContext.ProcessCharMessage(static_cast<wchar_t>(wParam));
            return false;
        });
    }

    /// @brief UI更新（ホットリロード判定+ウィジェット更新+リサイズ対応）
    void Update(float dt) override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        m_lastDt = dt;

        // F2: レイアウトデバッグ表示（ウィジェットの矩形を可視化）
        if (CheckHitKey(KEY_INPUT_F2))
            m_debugLayout = !m_debugLayout;

        // F5: UIの強制リロード。それ以外はファイル変更を自動検出してリロード。
        if (CheckHitKey(KEY_INPUT_F5))
            ReloadUI(true);
        else
            ReloadUI(false);

        // ESC: 設定画面→メインに戻る、メイン画面→アプリ終了
        if (CheckHitKey(KEY_INPUT_ESCAPE))
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

        // UIコンテキスト更新（マウス/キーボード入力処理+レイアウト再計算）
        m_uiContext.Update(dt, ctx.inputManager);

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

        uint32_t w = ctx.swapChain.GetWidth();
        uint32_t h = ctx.swapChain.GetHeight();
        uint32_t designW = ctx.app.GetWindow().GetWidth();
        uint32_t designH = ctx.app.GetWindow().GetHeight();

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

    /// @brief UI描画（UIRenderer→UIContext→デバッグ矩形→HUD）
    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();

        // GXEasyの2DバッチをFlush（UIRendererがSpriteBatchを使うため先にクリア）
        ctx.FlushAll();
        // UIRenderer.Begin〜End内でUIContext.Render()を呼ぶ
        m_uiRenderer.Begin(ctx.cmdList, ctx.frameIndex);
        m_uiContext.Render();
        m_uiRenderer.End();

        if (m_debugLayout)
        {
            DrawWidgetRect("btnStart", 0xFF00FF00);
            DrawWidgetRect("btnSettings", 0xFF00FF00);
            DrawWidgetRect("btnAbout", 0xFF00FF00);
            DrawWidgetRect("btnExit", 0xFF00FF00);
            DrawWidgetRect("btnBack", 0xFFFFAA00);
        }

        // HUD
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        const TString fpsText = FormatT(TEXT("FPS: {:.0f}"), fps);
        DrawString(10, 10, fpsText.c_str(), 0xFF888888);
        DrawString(10, (int)m_screenH - 30,
                   TEXT("ESC: Back/Quit"), 0xFF666666);
    }

private:
    /// メニュー画面の状態
    enum class MenuScreen { Main, Settings, About };

    // --- GUIシステムのコアオブジェクト ---
    GX::GUI::UIRenderer m_uiRenderer;   ///< 描画バックエンド
    GX::GUI::UIContext  m_uiContext;     ///< 更新・レイアウト・描画管理
    GX::GUI::StyleSheet m_styleSheet;   ///< CSSスタイル定義

    GX::GUI::Dialog* m_aboutDialog = nullptr;
    MenuScreen m_currentScreen = MenuScreen::Main;

    // --- 設定画面の値（UIと双方向バインド） ---
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
    float m_lastDt = 0.0f;

    // --- ホットリロード関連 ---
    GX::GUI::GUILoader m_uiLoader;      ///< XMLからUIツリーを構築するローダー
    std::string m_uiXmlPath = "Assets/ui/guimenu_demo.xml";  ///< UIレイアウトXML
    std::string m_uiCssPath = "Assets/ui/guimenu_demo.css";  ///< UIスタイルCSS
    std::filesystem::file_time_type m_uiXmlTime;   ///< 前回のXMLタイムスタンプ
    std::filesystem::file_time_type m_uiCssTime;   ///< 前回のCSSタイムスタンプ
    bool m_aboutVisible = false;

    static constexpr const char* k_UiXmlRelPath = "Assets/ui/guimenu_demo.xml";
    static constexpr const char* k_UiCssRelPath = "Assets/ui/guimenu_demo.css";

    /// @brief GUILoaderの初期設定（フォント登録・イベントバインド・描画コールバック）
    ///
    /// GUILoaderはXMLのonClick属性やonChanged属性に対応するC++コールバックを
    /// RegisterEvent/RegisterValueChangedEventで事前登録しておく仕組み。
    void SetupUILoader()
    {
        m_uiLoader = GX::GUI::GUILoader();
        m_uiLoader.SetRenderer(&m_uiRenderer);
        // XMLで font="default" / font="title" と指定するとこのフォントが使われる
        m_uiLoader.RegisterFont("default", m_fontHandle);
        m_uiLoader.RegisterFont("title", m_fontLarge);
        // Canvasウィジェット用の描画コールバック（背景グラデーション）
        m_uiLoader.RegisterDrawCallback("DrawBackground",
            [this](GX::GUI::UIRenderer& renderer, const GX::GUI::LayoutRect& rect)
            {
                DrawBackground(renderer, rect);
            });

        // --- ボタンイベント ---
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

        // --- 設定画面のスライダー/チェックボックス/ラジオボタンのイベント ---
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

    /// @brief アセットパスの解決（リポジトリルートからの相対パスを絶対パスに変換）
    void ResolveUIPaths()
    {
        m_uiXmlPath = ResolveAssetPath(k_UiXmlRelPath);
        m_uiCssPath = ResolveAssetPath(k_UiCssRelPath);
        GX_LOG_INFO("UI XML: %s", m_uiXmlPath.c_str());
        GX_LOG_INFO("UI CSS: %s", m_uiCssPath.c_str());
    }

    /// @brief UIのリロード（force=trueで強制、falseならファイル変更時のみ）
    ///
    /// XML/CSSの更新タイムスタンプを毎フレームチェックし、
    /// 変更があれば自動的にリロードする（ホットリロード）。
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

    /// @brief CSSスタイルシートの読み込み
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

    /// @brief XMLレイアウトの読み込み（ウィジェットツリーを再構築）
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

    /// @brief 現在のMenuScreen状態をUIウィジェットに反映する
    ///
    /// メインメニュー/設定画面の可視性切り替えや、
    /// スライダー値/チェックボックス値のUIへの同期を行う。
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

    /// @brief Aboutダイアログを表示する
    void ShowAboutDialog()
    {
        m_aboutVisible = true;
        if (m_aboutDialog) m_aboutDialog->visible = true;
    }

    /// @brief CMakeLists.txtを手がかりにリポジトリルートを探す
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

    /// @brief 相対パスをリポジトリルート基準の絶対パスに解決する
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

    /// @brief ファイルの更新日時が前回と異なるか判定する（ホットリロード用）
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

    /// @brief ウィジェットの矩形情報をログに出力する（デバッグ用）
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

    /// @brief ウィジェットの矩形を枠線で描画する（デバッグ用）
    void DrawWidgetRect(const char* id, uint32_t color)
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
        DrawBox((int)x1, (int)y1, (int)x2, (int)y2, color, FALSE);
    }

    /// @brief 背景グラデーション描画（Canvasウィジェットのコールバック）
    ///
    /// CSSから色と方向を取得し、DrawGradientRectで描画する。
    /// CanvasはUIRenderer経由で自由な描画ができるウィジェット。
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

// エントリーポイント
GX_EASY_APP(GUIMenuApp)


