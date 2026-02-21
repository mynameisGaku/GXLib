#include "pch.h"
#include "GUI/GUILoader.h"
#include "GUI/UIRenderer.h"
#include "GUI/StyleSheet.h"
#include "GUI/Widgets/Panel.h"
#include "GUI/Widgets/TextWidget.h"
#include "GUI/Widgets/Button.h"
#include "GUI/Widgets/Spacer.h"
#include "GUI/Widgets/ProgressBar.h"
#include "GUI/Widgets/Image.h"
#include "GUI/Widgets/CheckBox.h"
#include "GUI/Widgets/Slider.h"
#include "GUI/Widgets/ScrollView.h"
#include "GUI/Widgets/RadioButton.h"
#include "GUI/Widgets/DropDown.h"
#include "GUI/Widgets/ListView.h"
#include "GUI/Widgets/TabView.h"
#include "GUI/Widgets/Dialog.h"
#include "GUI/Widgets/Canvas.h"
#include "GUI/Widgets/TextInput.h"
#include "Core/Logger.h"

namespace GX { namespace GUI {

// ============================================================================
// UTF-8 → wstring 変換
// ============================================================================

static std::wstring Utf8ToWstring(const std::string& utf8)
{
    if (utf8.empty()) return L"";

    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                    static_cast<int>(utf8.size()), nullptr, 0);
    if (wlen <= 0) return L"";

    std::wstring result(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                         static_cast<int>(utf8.size()), &result[0], wlen);
    return result;
}

// カンマ区切り文字列をパース
static std::vector<std::string> SplitComma(const std::string& str)
{
    std::vector<std::string> result;
    size_t start = 0;
    while (start < str.size())
    {
        size_t end = str.find(',', start);
        if (end == std::string::npos) end = str.size();
        // トリム
        size_t s = start, e = end;
        while (s < e && str[s] == ' ') ++s;
        while (e > s && str[e - 1] == ' ') --e;
        result.push_back(str.substr(s, e - s));
        start = end + 1;
    }
    return result;
}

// ============================================================================
// 公開API
// ============================================================================

void GUILoader::SetRenderer(UIRenderer* renderer)
{
    m_renderer = renderer;
}

void GUILoader::RegisterFont(const std::string& name, int fontHandle)
{
    m_fontMap[name] = fontHandle;
}

void GUILoader::RegisterEvent(const std::string& name, std::function<void()> handler)
{
    m_eventMap[name] = std::move(handler);
}

void GUILoader::RegisterTexture(const std::string& name, int textureHandle)
{
    m_textureMap[name] = textureHandle;
}

void GUILoader::RegisterValueChangedEvent(const std::string& name,
                                           std::function<void(const std::string&)> handler)
{
    m_valueChangedMap[name] = std::move(handler);
}

void GUILoader::RegisterDrawCallback(const std::string& name,
                                      std::function<void(UIRenderer&, const LayoutRect&)> handler)
{
    m_drawCallbackMap[name] = std::move(handler);
}

std::unique_ptr<Widget> GUILoader::BuildFromFile(const std::string& xmlPath)
{
    XMLDocument doc;
    if (!doc.LoadFromFile(xmlPath))
    {
        GX_LOG_ERROR("GUILoader: Failed to load XML: %s", xmlPath.c_str());
        return nullptr;
    }
    return BuildFromDocument(doc);
}

std::unique_ptr<Widget> GUILoader::BuildFromDocument(const XMLDocument& doc)
{
    auto* root = doc.GetRoot();
    if (!root)
    {
        GX_LOG_ERROR("GUILoader: XML document has no root node");
        return nullptr;
    }
    return BuildWidget(root);
}

// ============================================================================
// BuildWidget — 再帰的にウィジェットツリーを構築
// ============================================================================

std::unique_ptr<Widget> GUILoader::BuildWidget(const XMLNode* node)
{
    if (!node) return nullptr;

    // タグ名 → ウィジェット生成
    std::unique_ptr<Widget> widget;
    const std::string& tag = node->tag;

    if (tag == "Panel")
    {
        widget = std::make_unique<Panel>();
    }
    else if (tag == "Text")
    {
        widget = std::make_unique<TextWidget>();
    }
    else if (tag == "Button")
    {
        widget = std::make_unique<Button>();
    }
    else if (tag == "Spacer")
    {
        widget = std::make_unique<Spacer>();
    }
    else if (tag == "ProgressBar")
    {
        widget = std::make_unique<ProgressBar>();
    }
    else if (tag == "Image")
    {
        widget = std::make_unique<Image>();
    }
    else if (tag == "CheckBox")
    {
        widget = std::make_unique<CheckBox>();
    }
    else if (tag == "Slider")
    {
        widget = std::make_unique<Slider>();
    }
    else if (tag == "ScrollView")
    {
        widget = std::make_unique<ScrollView>();
    }
    else if (tag == "RadioButton")
    {
        widget = std::make_unique<RadioButton>();
    }
    else if (tag == "DropDown")
    {
        widget = std::make_unique<DropDown>();
    }
    else if (tag == "ListView")
    {
        widget = std::make_unique<ListView>();
    }
    else if (tag == "TabView")
    {
        widget = std::make_unique<TabView>();
    }
    else if (tag == "Dialog")
    {
        widget = std::make_unique<Dialog>();
    }
    else if (tag == "Canvas")
    {
        widget = std::make_unique<Canvas>();
    }
    else if (tag == "TextInput")
    {
        widget = std::make_unique<TextInput>();
    }
    else
    {
        GX_LOG_WARN("GUILoader: Unknown tag '%s', using Panel fallback", tag.c_str());
        widget = std::make_unique<Panel>();
    }

    // --- 特殊属性の処理 ---

    // id（識別子）
    if (node->HasAttribute("id"))
        widget->id = node->GetAttribute("id");

    // class（スタイル用クラス名）
    if (node->HasAttribute("class"))
        widget->className = node->GetAttribute("class");

    // enabled（操作可否）
    if (node->HasAttribute("enabled"))
    {
        const std::string& val = node->GetAttribute("enabled");
        widget->enabled = !(val == "false" || val == "0");
    }

    // visible（表示/非表示）
    if (node->HasAttribute("visible"))
    {
        const std::string& val = node->GetAttribute("visible");
        widget->visible = !(val == "false" || val == "0");
    }

    // --- 汎用イベント（任意のウィジェットで使用可能） ---
    auto bindEvent = [&](const char* attrName, std::function<void()>& target)
    {
        if (!node->HasAttribute(attrName)) return;
        const std::string& eventName = node->GetAttribute(attrName);
        auto it = m_eventMap.find(eventName);
        if (it != m_eventMap.end())
            target = it->second;
        else
            GX_LOG_WARN("GUILoader: Unregistered event '%s'", eventName.c_str());
    };

    bindEvent("onHover", widget->onHover);
    bindEvent("onLeave", widget->onLeave);
    bindEvent("onPress", widget->onPress);
    bindEvent("onRelease", widget->onRelease);
    bindEvent("onFocus", widget->onFocus);
    bindEvent("onBlur", widget->onBlur);
    bindEvent("onSubmit", widget->onSubmit);

    // --- TextWidget / Button 固有 ---
    bool isText   = (tag == "Text");
    bool isButton = (tag == "Button");

    if (isText || isButton)
    {
        // font 属性
        std::string fontName = "default";
        if (node->HasAttribute("font"))
            fontName = node->GetAttribute("font");

        int fontHandle = ResolveFontHandle(fontName);

        // テキスト内容: text 属性 > テキストコンテント
        std::wstring textContent;
        if (node->HasAttribute("text"))
            textContent = Utf8ToWstring(node->GetAttribute("text"));
        else if (!node->text.empty())
            textContent = Utf8ToWstring(node->text);

        if (isText)
        {
            auto* tw = static_cast<TextWidget*>(widget.get());
            tw->SetRenderer(m_renderer);
            tw->SetFontHandle(fontHandle);
            if (!textContent.empty())
                tw->SetText(textContent);
        }
        else // isButton
        {
            auto* btn = static_cast<Button*>(widget.get());
            btn->SetRenderer(m_renderer);
            btn->SetFontHandle(fontHandle);
            if (!textContent.empty())
                btn->SetText(textContent);

            // onClick イベント
            if (node->HasAttribute("onClick"))
            {
                const std::string& eventName = node->GetAttribute("onClick");
                auto it = m_eventMap.find(eventName);
                if (it != m_eventMap.end())
                    btn->onClick = it->second;
                else
                    GX_LOG_WARN("GUILoader: Unregistered event '%s'", eventName.c_str());
            }
        }
    }

    // --- Image 固有 ---
    if (tag == "Image")
    {
        auto* img = static_cast<Image*>(widget.get());

        if (node->HasAttribute("src"))
        {
            int texHandle = ResolveTextureHandle(node->GetAttribute("src"));
            img->SetTextureHandle(texHandle);
        }

        if (node->HasAttribute("fit"))
        {
            const std::string& fitStr = node->GetAttribute("fit");
            if (fitStr == "contain") img->SetFit(ImageFit::Contain);
            else if (fitStr == "cover") img->SetFit(ImageFit::Cover);
            else img->SetFit(ImageFit::Stretch);
        }

        if (node->HasAttribute("naturalWidth") && node->HasAttribute("naturalHeight"))
        {
            float nw = static_cast<float>(atof(node->GetAttribute("naturalWidth").c_str()));
            float nh = static_cast<float>(atof(node->GetAttribute("naturalHeight").c_str()));
            img->SetNaturalSize(nw, nh);
        }
    }

    // --- Slider 固有 ---
    if (tag == "Slider")
    {
        auto* slider = static_cast<Slider*>(widget.get());

        float minVal = 0.0f, maxVal = 1.0f;
        if (node->HasAttribute("min"))
            minVal = static_cast<float>(atof(node->GetAttribute("min").c_str()));
        if (node->HasAttribute("max"))
            maxVal = static_cast<float>(atof(node->GetAttribute("max").c_str()));
        slider->SetRange(minVal, maxVal);

        if (node->HasAttribute("step"))
            slider->SetStep(static_cast<float>(atof(node->GetAttribute("step").c_str())));

        if (node->HasAttribute("value"))
            slider->SetValue(static_cast<float>(atof(node->GetAttribute("value").c_str())));

        if (node->HasAttribute("onValueChanged"))
        {
            const std::string& evtName = node->GetAttribute("onValueChanged");
            auto it = m_valueChangedMap.find(evtName);
            if (it != m_valueChangedMap.end())
                slider->onValueChanged = it->second;
            else
                GX_LOG_WARN("GUILoader: Unregistered valueChanged event '%s'", evtName.c_str());
        }
    }

    // --- CheckBox 固有 ---
    if (tag == "CheckBox")
    {
        auto* cb = static_cast<CheckBox*>(widget.get());
        cb->SetRenderer(m_renderer);

        // font 属性
        std::string fontName = "default";
        if (node->HasAttribute("font"))
            fontName = node->GetAttribute("font");
        cb->SetFontHandle(ResolveFontHandle(fontName));

        // テキスト内容
        std::wstring textContent;
        if (node->HasAttribute("text"))
            textContent = Utf8ToWstring(node->GetAttribute("text"));
        else if (!node->text.empty())
            textContent = Utf8ToWstring(node->text);
        if (!textContent.empty())
            cb->SetText(textContent);

        // checked
        if (node->HasAttribute("checked"))
        {
            const std::string& val = node->GetAttribute("checked");
            cb->SetChecked(val == "true" || val == "1");
        }

        if (node->HasAttribute("onValueChanged"))
        {
            const std::string& evtName = node->GetAttribute("onValueChanged");
            auto it = m_valueChangedMap.find(evtName);
            if (it != m_valueChangedMap.end())
                cb->onValueChanged = it->second;
            else
                GX_LOG_WARN("GUILoader: Unregistered valueChanged event '%s'", evtName.c_str());
        }
    }

    // --- ProgressBar 固有 ---
    if (tag == "ProgressBar")
    {
        auto* pb = static_cast<ProgressBar*>(widget.get());

        if (node->HasAttribute("value"))
            pb->SetValue(static_cast<float>(atof(node->GetAttribute("value").c_str())));

        if (node->HasAttribute("barColor"))
            pb->SetBarColor(StyleColor::FromHex(node->GetAttribute("barColor")));
    }

    // --- Spacer 固有 ---
    if (tag == "Spacer")
    {
        auto* spacer = static_cast<Spacer*>(widget.get());
        float sw = 0.0f, sh = 0.0f;
        if (node->HasAttribute("width"))
            sw = static_cast<float>(atof(node->GetAttribute("width").c_str()));
        if (node->HasAttribute("height"))
            sh = static_cast<float>(atof(node->GetAttribute("height").c_str()));
        spacer->SetSize(sw, sh);
    }

    // --- RadioButton 固有 ---
    if (tag == "RadioButton")
    {
        auto* rb = static_cast<RadioButton*>(widget.get());
        rb->SetRenderer(m_renderer);

        std::string fontName = "default";
        if (node->HasAttribute("font"))
            fontName = node->GetAttribute("font");
        rb->SetFontHandle(ResolveFontHandle(fontName));

        // テキスト内容
        std::wstring textContent;
        if (node->HasAttribute("text"))
            textContent = Utf8ToWstring(node->GetAttribute("text"));
        else if (!node->text.empty())
            textContent = Utf8ToWstring(node->text);
        if (!textContent.empty())
            rb->SetText(textContent);

        if (node->HasAttribute("value"))
            rb->SetValue(node->GetAttribute("value"));

        if (node->HasAttribute("selected"))
        {
            const std::string& val = node->GetAttribute("selected");
            if (val == "true" || val == "1")
                rb->SetSelected(true);
        }
    }

    // --- DropDown 固有 ---
    if (tag == "DropDown")
    {
        auto* dd = static_cast<DropDown*>(widget.get());
        dd->SetRenderer(m_renderer);

        std::string fontName = "default";
        if (node->HasAttribute("font"))
            fontName = node->GetAttribute("font");
        dd->SetFontHandle(ResolveFontHandle(fontName));

        if (node->HasAttribute("items"))
            dd->SetItems(SplitComma(node->GetAttribute("items")));

        if (node->HasAttribute("selectedIndex"))
            dd->SetSelectedIndex(atoi(node->GetAttribute("selectedIndex").c_str()));

        if (node->HasAttribute("onValueChanged"))
        {
            const std::string& evtName = node->GetAttribute("onValueChanged");
            auto it = m_valueChangedMap.find(evtName);
            if (it != m_valueChangedMap.end())
                dd->onValueChanged = it->second;
            else
                GX_LOG_WARN("GUILoader: Unregistered valueChanged event '%s'", evtName.c_str());
        }
    }

    // --- ListView 固有 ---
    if (tag == "ListView")
    {
        auto* lv = static_cast<ListView*>(widget.get());
        lv->SetRenderer(m_renderer);

        std::string fontName = "default";
        if (node->HasAttribute("font"))
            fontName = node->GetAttribute("font");
        lv->SetFontHandle(ResolveFontHandle(fontName));

        if (node->HasAttribute("items"))
            lv->SetItems(SplitComma(node->GetAttribute("items")));

        if (node->HasAttribute("selectedIndex"))
            lv->SetSelectedIndex(atoi(node->GetAttribute("selectedIndex").c_str()));

        if (node->HasAttribute("onValueChanged"))
        {
            const std::string& evtName = node->GetAttribute("onValueChanged");
            auto it = m_valueChangedMap.find(evtName);
            if (it != m_valueChangedMap.end())
                lv->onValueChanged = it->second;
            else
                GX_LOG_WARN("GUILoader: Unregistered valueChanged event '%s'", evtName.c_str());
        }
    }

    // --- TabView 固有 ---
    if (tag == "TabView")
    {
        auto* tv = static_cast<TabView*>(widget.get());
        tv->SetRenderer(m_renderer);

        std::string fontName = "default";
        if (node->HasAttribute("font"))
            fontName = node->GetAttribute("font");
        tv->SetFontHandle(ResolveFontHandle(fontName));

        if (node->HasAttribute("tabs"))
            tv->SetTabNames(SplitComma(node->GetAttribute("tabs")));

        if (node->HasAttribute("activeTab"))
            tv->SetActiveTab(atoi(node->GetAttribute("activeTab").c_str()));
    }

    // --- Dialog 固有 ---
    if (tag == "Dialog")
    {
        auto* dlg = static_cast<Dialog*>(widget.get());
        dlg->SetRenderer(m_renderer);

        std::string fontName = "default";
        if (node->HasAttribute("font"))
            fontName = node->GetAttribute("font");
        dlg->SetFontHandle(ResolveFontHandle(fontName));

        if (node->HasAttribute("title"))
            dlg->SetTitle(Utf8ToWstring(node->GetAttribute("title")));

        // Dialog はデフォルト非表示
        if (!node->HasAttribute("visible"))
            widget->visible = false;

        if (node->HasAttribute("onClose"))
        {
            const std::string& eventName = node->GetAttribute("onClose");
            auto it = m_eventMap.find(eventName);
            if (it != m_eventMap.end())
                dlg->onClose = it->second;
        }
    }

    // --- Canvas 固有 ---
    if (tag == "Canvas")
    {
        auto* canvas = static_cast<Canvas*>(widget.get());

        if (node->HasAttribute("onDraw"))
        {
            const std::string& cbName = node->GetAttribute("onDraw");
            auto it = m_drawCallbackMap.find(cbName);
            if (it != m_drawCallbackMap.end())
                canvas->onDraw = it->second;
            else
                GX_LOG_WARN("GUILoader: Unregistered draw callback '%s'", cbName.c_str());
        }
    }

    // --- TextInput 固有 ---
    if (tag == "TextInput")
    {
        auto* ti = static_cast<TextInput*>(widget.get());
        ti->SetRenderer(m_renderer);

        std::string fontName = "default";
        if (node->HasAttribute("font"))
            fontName = node->GetAttribute("font");
        ti->SetFontHandle(ResolveFontHandle(fontName));

        if (node->HasAttribute("placeholder"))
            ti->SetPlaceholder(Utf8ToWstring(node->GetAttribute("placeholder")));

        if (node->HasAttribute("value"))
            ti->SetText(Utf8ToWstring(node->GetAttribute("value")));

        if (node->HasAttribute("maxLength"))
            ti->SetMaxLength(atoi(node->GetAttribute("maxLength").c_str()));

        if (node->HasAttribute("password"))
        {
            const std::string& val = node->GetAttribute("password");
            ti->SetPasswordMode(val == "true" || val == "1");
        }

        if (node->HasAttribute("onValueChanged"))
        {
            const std::string& evtName = node->GetAttribute("onValueChanged");
            auto it = m_valueChangedMap.find(evtName);
            if (it != m_valueChangedMap.end())
                ti->onValueChanged = it->second;
            else
                GX_LOG_WARN("GUILoader: Unregistered valueChanged event '%s'", evtName.c_str());
        }

        if (node->HasAttribute("onSubmit"))
        {
            const std::string& eventName = node->GetAttribute("onSubmit");
            auto it = m_eventMap.find(eventName);
            if (it != m_eventMap.end())
                ti->onSubmit = it->second;
            else
                GX_LOG_WARN("GUILoader: Unregistered event '%s'", eventName.c_str());
        }
    }

    // --- その他の属性 → インラインスタイルとして直接適用 ---
    // id/class/onClick等の特殊属性以外は全てCSSプロパティとして解釈する
    for (const auto& [attrName, attrValue] : node->attributes)
    {
        // 特殊属性はスキップ
        if (attrName == "id" || attrName == "class" || attrName == "enabled" ||
            attrName == "visible" || attrName == "font" || attrName == "text" ||
            attrName == "onClick" || attrName == "onValueChanged" || attrName == "onClose" ||
            attrName == "onHover" || attrName == "onLeave" || attrName == "onPress" ||
            attrName == "onRelease" || attrName == "onFocus" || attrName == "onBlur" ||
            attrName == "onSubmit" ||
            attrName == "src" || attrName == "fit" ||
            attrName == "naturalWidth" || attrName == "naturalHeight" ||
            attrName == "min" || attrName == "max" || attrName == "step" ||
            attrName == "value" || attrName == "checked" || attrName == "barColor" ||
            attrName == "items" || attrName == "tabs" || attrName == "activeTab" ||
            attrName == "selectedIndex" || attrName == "selected" ||
            attrName == "title" || attrName == "onDraw" ||
            attrName == "placeholder" || attrName == "maxLength" ||
            attrName == "password" || attrName == "onSubmit")
            continue;

        // インラインスタイルとして適用
        StyleSheet::ApplyProperty(widget->computedStyle, attrName, attrValue);
    }

    // --- 子要素の再帰構築 ---
    for (const auto& child : node->children)
    {
        auto childWidget = BuildWidget(child.get());
        if (childWidget)
            widget->AddChild(std::move(childWidget));
    }

    // --- RadioButton: 親パネルの onValueChanged を接続 ---
    // 親パネルに onValueChanged が設定される前にRadioButtonが構築されるため、
    // 親パネル自体の onValueChanged は XML の onValueChanged 属性で設定
    if (tag == "Panel" && node->HasAttribute("onValueChanged"))
    {
        const std::string& evtName = node->GetAttribute("onValueChanged");
        auto it = m_valueChangedMap.find(evtName);
        if (it != m_valueChangedMap.end())
            widget->onValueChanged = it->second;
    }

    return widget;
}

// ============================================================================
// ResolveFontHandle / ResolveTextureHandle
// ============================================================================

int GUILoader::ResolveFontHandle(const std::string& fontName) const
{
    auto it = m_fontMap.find(fontName);
    if (it != m_fontMap.end())
        return it->second;

    GX_LOG_WARN("GUILoader: Unknown font '%s'", fontName.c_str());
    return -1;
}

int GUILoader::ResolveTextureHandle(const std::string& texName) const
{
    auto it = m_textureMap.find(texName);
    if (it != m_textureMap.end())
        return it->second;

    GX_LOG_WARN("GUILoader: Unknown texture '%s'", texName.c_str());
    return -1;
}

}} // namespace GX::GUI
