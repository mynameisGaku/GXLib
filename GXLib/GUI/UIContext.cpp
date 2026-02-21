#include "pch.h"
#include "GUI/UIContext.h"
#include "Input/InputManager.h"
#include "Math/Transform2D.h"

namespace GX { namespace GUI {

// ウィジェットのCSS transformプロパティからアフィン変換を構築する
static Transform2D BuildLocalTransform(const Widget& widget, const Style& style)
{
    float tx = style.translateX;
    float ty = style.translateY;
    float sx = style.scaleX;
    float sy = style.scaleY;
    float rad = style.rotate * 0.0174532925f;

    float pivotX = widget.globalRect.x + widget.globalRect.width * style.pivotX;
    float pivotY = widget.globalRect.y + widget.globalRect.height * style.pivotY;

    Transform2D t = Transform2D::Identity();
    if (tx != 0.0f || ty != 0.0f)
        t = Multiply(t, Transform2D::Translation(tx, ty));
    t = Multiply(t, Transform2D::Translation(pivotX, pivotY));
    if (rad != 0.0f)
        t = Multiply(t, Transform2D::Rotation(rad));
    if (sx != 1.0f || sy != 1.0f)
        t = Multiply(t, Transform2D::Scale(sx, sy));
    t = Multiply(t, Transform2D::Translation(-pivotX, -pivotY));
    return t;
}

// ルートからwidgetまでのtransformを合成してワールド変換行列を得る
static Transform2D BuildWorldTransform(const Widget* widget)
{
    std::vector<const Widget*> chain;
    for (auto* w = widget; w; w = w->GetParent())
        chain.push_back(w);
    std::reverse(chain.begin(), chain.end());

    Transform2D world = Transform2D::Identity();
    for (auto* w : chain)
    {
        const Style& style = w->GetRenderStyle();
        world = Multiply(world, BuildLocalTransform(*w, style));
    }
    return world;
}

// スクリーン座標をウィジェットのローカル座標に変換する（ワールド変換の逆行列を適用）
static XMFLOAT2 ComputeLocalPoint(const Widget* widget, float x, float y)
{
    if (!widget)
        return { x, y };
    Transform2D world = BuildWorldTransform(widget);
    Transform2D inv = Inverse(world);
    return TransformPoint(inv, x, y);
}

// 座標(x,y)にあるウィジェットを再帰的に探す。子の上にあるものが優先（後方の子＝手前）
static Widget* HitTestInternal(Widget* widget, float x, float y,
                               const Transform2D& parent, XMFLOAT2* outLocal)
{
    if (!widget->visible || !widget->enabled) return nullptr;

    const Style& style = widget->GetRenderStyle();
    Transform2D local = BuildLocalTransform(*widget, style);
    Transform2D world = Multiply(parent, local);
    Transform2D inv = Inverse(world);
    XMFLOAT2 localPt = TransformPoint(inv, x, y);

    bool clipChildren = (widget->computedStyle.overflow == OverflowMode::Hidden ||
                         widget->computedStyle.overflow == OverflowMode::Scroll);
    if (clipChildren && !widget->globalRect.Contains(localPt.x, localPt.y))
        return nullptr;

    const auto& children = widget->GetChildren();
    for (int i = static_cast<int>(children.size()) - 1; i >= 0; --i)
    {
        Widget* hit = HitTestInternal(children[i].get(), x, y, world, outLocal);
        if (hit) return hit;
    }

    if (widget->globalRect.Contains(localPt.x, localPt.y))
    {
        if (outLocal) *outLocal = localPt;
        return widget;
    }

    return nullptr;
}

// ============================================================================
// 初期化
// ============================================================================

bool UIContext::Initialize(UIRenderer* renderer, uint32_t screenWidth, uint32_t screenHeight)
{
    m_renderer = renderer;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    return true;
}

// ============================================================================
// ルートウィジェット
// ============================================================================

void UIContext::SetRoot(std::unique_ptr<Widget> root)
{
    m_root = std::move(root);
    m_focusedWidget = nullptr;
    m_hoveredWidget = nullptr;
    m_pressedWidget = nullptr;
}

void UIContext::SetStyleSheet(StyleSheet* sheet)
{
    m_styleSheet = sheet;
}

Widget* UIContext::FindById(const std::string& id)
{
    if (!m_root) return nullptr;
    return m_root->FindById(id);
}

// ============================================================================
// 更新
// ============================================================================

void UIContext::Update(float deltaTime, InputManager& input)
{
    if (!m_root) return;

    // 入力イベントを処理
    ProcessInputEvents(input);

    // レイアウト計算
    ComputeLayout();

    // ウィジェット更新
    m_root->Update(deltaTime);
}

// ============================================================================
// 描画
// ============================================================================

void UIContext::Render()
{
    if (!m_root || !m_renderer) return;
    m_root->Render(*m_renderer);
    m_renderer->FlushDeferredDraws();
}

// ============================================================================
// 入力イベント処理
// ============================================================================

void UIContext::ProcessInputEvents(InputManager& input)
{
    auto& mouse = input.GetMouse();
    float mx = static_cast<float>(mouse.GetX());
    float my = static_cast<float>(mouse.GetY());

    // スクリーン座標 → デザイン座標に変換
    if (m_renderer && m_designWidth > 0 && m_designHeight > 0)
    {
        float scale = m_renderer->GetGuiScale();
        float offsetX = m_renderer->GetGuiOffsetX();
        float offsetY = m_renderer->GetGuiOffsetY();
        if (scale > 0.0f)
        {
            mx = (mx - offsetX) / scale;
            my = (my - offsetY) / scale;
        }
    }

    bool mouseDown = mouse.IsButtonDown(MouseButton::Left);
    bool mouseTriggered = mouse.IsButtonTriggered(MouseButton::Left);
    bool mouseReleased = mouse.IsButtonReleased(MouseButton::Left);
    int wheelDelta = mouse.GetWheel();

    // ヒットテスト
    XMFLOAT2 hitLocal = { mx, my };
    Widget* hitWidget = m_root
        ? HitTestInternal(m_root.get(), mx, my, Transform2D::Identity(), &hitLocal)
        : nullptr;

    // --- マウス離脱 / 進入 ---
    if (hitWidget != m_hoveredWidget)
    {
        if (m_hoveredWidget)
        {
            m_hoveredWidget->hovered = false;
            UIEvent leaveEvent;
            leaveEvent.type = UIEventType::MouseLeave;
            leaveEvent.mouseX = mx;
            leaveEvent.mouseY = my;
            XMFLOAT2 leaveLocal = ComputeLocalPoint(m_hoveredWidget, mx, my);
            leaveEvent.localX = leaveLocal.x;
            leaveEvent.localY = leaveLocal.y;
            leaveEvent.target = m_hoveredWidget;
            DispatchEvent(leaveEvent);
        }
        if (hitWidget)
        {
            hitWidget->hovered = true;
            UIEvent enterEvent;
            enterEvent.type = UIEventType::MouseEnter;
            enterEvent.mouseX = mx;
            enterEvent.mouseY = my;
            enterEvent.localX = hitLocal.x;
            enterEvent.localY = hitLocal.y;
            enterEvent.target = hitWidget;
            DispatchEvent(enterEvent);
        }
        m_hoveredWidget = hitWidget;
    }

    // --- マウス移動 ---
    if (mx != m_prevMouseX || my != m_prevMouseY)
    {
        if (hitWidget)
        {
            UIEvent moveEvent;
            moveEvent.type = UIEventType::MouseMove;
            moveEvent.mouseX = mx;
            moveEvent.mouseY = my;
            moveEvent.localX = hitLocal.x;
            moveEvent.localY = hitLocal.y;
            moveEvent.target = hitWidget;
            DispatchEvent(moveEvent);
        }
    }

    // --- マウス押下 ---
    if (mouseTriggered)
    {
        if (hitWidget)
        {
            hitWidget->pressed = true;
            m_pressedWidget = hitWidget;

            // フォーカス変更
            if (hitWidget != m_focusedWidget)
                SetFocus(hitWidget);

            UIEvent downEvent;
            downEvent.type = UIEventType::MouseDown;
            downEvent.mouseX = mx;
            downEvent.mouseY = my;
            downEvent.localX = hitLocal.x;
            downEvent.localY = hitLocal.y;
            downEvent.mouseButton = MouseButton::Left;
            downEvent.target = hitWidget;
            DispatchEvent(downEvent);
        }
        else
        {
            // 空クリック→フォーカスクリア
            SetFocus(nullptr);
        }
    }

    // --- マウス解放 ---
    if (mouseReleased)
    {
        if (m_pressedWidget)
        {
            m_pressedWidget->pressed = false;

            XMFLOAT2 pressedLocal = ComputeLocalPoint(m_pressedWidget, mx, my);

            UIEvent upEvent;
            upEvent.type = UIEventType::MouseUp;
            upEvent.mouseX = mx;
            upEvent.mouseY = my;
            upEvent.localX = pressedLocal.x;
            upEvent.localY = pressedLocal.y;
            upEvent.mouseButton = MouseButton::Left;
            upEvent.target = m_pressedWidget;
            DispatchEvent(upEvent);

            // Click判定（ボタンを押した場所と離した場所が同じウィジェット）
            if (hitWidget == m_pressedWidget && m_pressedWidget->enabled)
            {
                UIEvent clickEvent;
                clickEvent.type = UIEventType::Click;
                clickEvent.mouseX = mx;
                clickEvent.mouseY = my;
                clickEvent.localX = pressedLocal.x;
                clickEvent.localY = pressedLocal.y;
                clickEvent.target = m_pressedWidget;
                DispatchEvent(clickEvent);

                if (m_pressedWidget->onClick)
                    m_pressedWidget->onClick();
            }

            m_pressedWidget = nullptr;
        }
    }

    // --- マウスホイール ---
    if (wheelDelta != 0 && hitWidget)
    {
        UIEvent wheelEvent;
        wheelEvent.type = UIEventType::MouseWheel;
        wheelEvent.mouseX = mx;
        wheelEvent.mouseY = my;
        wheelEvent.localX = hitLocal.x;
        wheelEvent.localY = hitLocal.y;
        wheelEvent.wheelDelta = wheelDelta;
        wheelEvent.target = hitWidget;
        DispatchEvent(wheelEvent);
    }

    // --- キーボード ---
    auto& kb = input.GetKeyboard();
    if (m_focusedWidget)
    {
        static const int keys[] = {
            VK_LEFT, VK_RIGHT, VK_HOME, VK_END,
            VK_BACK, VK_DELETE, VK_RETURN, VK_ESCAPE,
            'A', 'C', 'V', 'X'
        };
        for (int vk : keys)
        {
            if (kb.IsKeyTriggered(vk))
            {
                UIEvent keyEvent;
                keyEvent.type = UIEventType::KeyDown;
                keyEvent.keyCode = vk;
                keyEvent.target = m_focusedWidget;
                DispatchEvent(keyEvent);
            }
        }
    }
    // Tab でフォーカス移動（将来拡張用に予約）

    m_prevMouseX = mx;
    m_prevMouseY = my;
    m_prevMouseDown = mouseDown;
}

// ============================================================================
// WM_CHAR 処理
// ============================================================================

bool UIContext::ProcessCharMessage(wchar_t ch)
{
    if (!m_focusedWidget) return false;

    UIEvent charEvent;
    charEvent.type = UIEventType::CharInput;
    charEvent.charCode = ch;
    charEvent.target = m_focusedWidget;
    DispatchEvent(charEvent);

    return charEvent.handled;
}

// ============================================================================
// フォーカス管理
// ============================================================================

void UIContext::SetFocus(Widget* widget)
{
    if (m_focusedWidget == widget) return;

    if (m_focusedWidget)
    {
        m_focusedWidget->focused = false;
        UIEvent lostEvent;
        lostEvent.type = UIEventType::FocusLost;
        lostEvent.target = m_focusedWidget;
        DispatchEvent(lostEvent);
    }

    m_focusedWidget = widget;

    if (m_focusedWidget)
    {
        m_focusedWidget->focused = true;
        UIEvent gainEvent;
        gainEvent.type = UIEventType::FocusGained;
        gainEvent.target = m_focusedWidget;
        DispatchEvent(gainEvent);
    }
}

// ============================================================================
// イベントディスパッチ（3フェーズ）
// ============================================================================

void UIContext::DispatchEvent(UIEvent& event)
{
    if (!event.target) return;

    auto applyLocalPoint = [&](Widget* w)
    {
        switch (event.type)
        {
        case UIEventType::MouseDown:
        case UIEventType::MouseUp:
        case UIEventType::MouseMove:
        case UIEventType::MouseWheel:
        case UIEventType::MouseEnter:
        case UIEventType::MouseLeave:
        case UIEventType::Click:
        {
            XMFLOAT2 local = ComputeLocalPoint(w, event.mouseX, event.mouseY);
            event.localX = local.x;
            event.localY = local.y;
            break;
        }
        default:
            break;
        }
    };

    // 祖先パスを収集（ルート→ターゲットの順）
    std::vector<Widget*> path;
    CollectAncestors(event.target, path);

    // Phase 1: キャプチャ（ルート→ターゲット）
    event.phase = UIEventPhase::Capture;
    for (auto* w : path)
    {
        if (event.stopPropagation) break;
        applyLocalPoint(w);
        w->OnEvent(event);
    }

    // Phase 2: ターゲット
    if (!event.stopPropagation)
    {
        event.phase = UIEventPhase::Target;
        applyLocalPoint(event.target);
        event.target->OnEvent(event);
    }

    // Phase 3: バブル（ターゲット→ルート）
    event.phase = UIEventPhase::Bubble;
    for (int i = static_cast<int>(path.size()) - 1; i >= 0; --i)
    {
        if (event.stopPropagation) break;
        applyLocalPoint(path[i]);
        path[i]->OnEvent(event);
    }
}

void UIContext::CollectAncestors(Widget* widget, std::vector<Widget*>& path)
{
    // ルートからターゲットの直前の親までを収集
    Widget* p = widget->GetParent();
    while (p)
    {
        path.push_back(p);
        p = p->GetParent();
    }
    // ルートが先頭になるように反転
    std::reverse(path.begin(), path.end());
}

// ============================================================================
// ヒットテスト
// ============================================================================

Widget* UIContext::HitTest(Widget* widget, float x, float y)
{
    return HitTestInternal(widget, x, y, Transform2D::Identity(), nullptr);
}

// ============================================================================
// レイアウト計算
// ============================================================================

void UIContext::ComputeLayout()
{
    if (!m_root) return;

    // スタイルシートが設定されている場合、ツリー全体に適用
    if (m_styleSheet)
        m_styleSheet->ApplyToTree(m_root.get());

    // デザイン解像度が設定されていればそれを使用（レイアウトはデザイン座標で行う）
    float sw = static_cast<float>(m_designWidth > 0 ? m_designWidth : m_screenWidth);
    float sh = static_cast<float>(m_designHeight > 0 ? m_designHeight : m_screenHeight);
    LayoutWidget(m_root.get(), 0.0f, 0.0f, sw, sh);
}

// ----------------------------------------------------------------------------
// MeasureWidget: ウィジェットの希望サイズをボトムアップで計測
// maxWidth/maxHeight はサイズ解決の基準（親のコンテンツ領域）
// ----------------------------------------------------------------------------

UIContext::WidgetSize UIContext::MeasureWidget(Widget* widget, float maxWidth, float maxHeight)
{
    auto& style = widget->computedStyle;

    // 明示的サイズを解決
    float w = style.width.IsAuto()  ? 0.0f : style.width.Resolve(maxWidth);
    float h = style.height.IsAuto() ? 0.0f : style.height.Resolve(maxHeight);

    // リーフウィジェットの内容サイズ（テキスト幅/画像サイズなど中身の自然な大きさ）
    if (w <= 0.0f) w = widget->GetIntrinsicWidth();
    if (h <= 0.0f) h = widget->GetIntrinsicHeight();

    // 子から計測が必要か判定
    auto& children = widget->GetChildren();
    bool needW = (w <= 0.0f);
    bool needH = (h <= 0.0f);

    if (!children.empty() && (needW || needH))
    {
        float padH = style.padding.HorizontalTotal() + style.borderWidth * 2.0f;
        float padV = style.padding.VerticalTotal()   + style.borderWidth * 2.0f;
        float childMaxW = (w > padH) ? (w - padH) : (maxWidth - padH);
        float childMaxH = (h > padV) ? (h - padV) : (maxHeight - padV);
        if (childMaxW < 0.0f) childMaxW = 0.0f;
        if (childMaxH < 0.0f) childMaxH = 0.0f;

        bool isColumn = (style.flexDirection == FlexDirection::Column);
        float mainTotal = 0.0f, crossMax = 0.0f;
        int count = 0;

        for (auto& child : children)
        {
            if (!child->visible) continue;
            if (child->computedStyle.position == PositionType::Absolute) continue;

            auto hint = MeasureWidget(child.get(), childMaxW, childMaxH);
            auto& cm = child->computedStyle.margin;

            if (isColumn)
            {
                mainTotal += hint.height + cm.top + cm.bottom;
                crossMax = (std::max)(crossMax, hint.width + cm.left + cm.right);
            }
            else
            {
                mainTotal += hint.width + cm.left + cm.right;
                crossMax = (std::max)(crossMax, hint.height + cm.top + cm.bottom);
            }
            count++;
        }

        float totalGap = (count > 1) ? style.gap * (count - 1) : 0.0f;

        if (isColumn)
        {
            if (needH) h = mainTotal + totalGap + padV;
            if (needW) w = crossMax + padH;
        }
        else
        {
            if (needW) w = mainTotal + totalGap + padH;
            if (needH) h = crossMax + padV;
        }
    }

    // フォールバック: 利用可能空間を使用
    if (w <= 0.0f) w = maxWidth;
    if (h <= 0.0f) h = maxHeight;

    // min/max 制約
    w = (std::max)(w, style.minWidth.Resolve(maxWidth));
    w = (std::min)(w, style.maxWidth.Resolve(maxWidth));
    h = (std::max)(h, style.minHeight.Resolve(maxHeight));
    h = (std::min)(h, style.maxHeight.Resolve(maxHeight));

    return { w, h };
}

// ----------------------------------------------------------------------------
// LayoutWidget: 確定位置・サイズで再帰レイアウト
// posX, posY: このウィジェットの確定位置（左上角）
// allocWidth, allocHeight: 割り当てサイズ（Auto解決の基準）
// ----------------------------------------------------------------------------

void UIContext::LayoutWidget(Widget* widget, float posX, float posY,
                              float allocWidth, float allocHeight)
{
    auto& style = widget->computedStyle;

    // --- サイズ解決 ---
    // Auto: 割り当てサイズをそのまま使用
    // （Flexbox呼び出しの場合、MeasureWidget測定済みサイズが渡される）
    float w = style.width.IsAuto()  ? allocWidth  : style.width.Resolve(allocWidth);
    float h = style.height.IsAuto() ? allocHeight : style.height.Resolve(allocHeight);

    // min/max 制約
    w = (std::max)(w, style.minWidth.Resolve(allocWidth));
    w = (std::min)(w, style.maxWidth.Resolve(allocWidth));
    h = (std::max)(h, style.minHeight.Resolve(allocHeight));
    h = (std::min)(h, style.maxHeight.Resolve(allocHeight));

    // --- 位置設定 ---
    // posX/posY はこのウィジェットの確定位置（margin は呼び出し元が処理済み）
    widget->globalRect = { posX, posY, w, h };
    widget->layoutRect = { posX - (widget->GetParent() ? widget->GetParent()->globalRect.x : 0.0f),
                           posY - (widget->GetParent() ? widget->GetParent()->globalRect.y : 0.0f),
                           w, h };

    // --- 子レイアウト ---
    auto& children = widget->GetChildren();
    if (children.empty()) return;

    // コンテンツ領域（padding + border の内側）
    float contentX = posX + style.padding.left + style.borderWidth;
    float contentY = posY + style.padding.top  + style.borderWidth;
    float contentW = w - style.padding.HorizontalTotal() - style.borderWidth * 2.0f;
    float contentH = h - style.padding.VerticalTotal()   - style.borderWidth * 2.0f;
    if (contentW < 0.0f) contentW = 0.0f;
    if (contentH < 0.0f) contentH = 0.0f;

    bool isColumn = (style.flexDirection == FlexDirection::Column);

    // === Pass 1: 子の希望サイズを計算 ===
    float totalFixed    = 0.0f;
    float totalFlexGrow = 0.0f;
    int   numVisible    = 0;

    struct ChildInfo
    {
        Widget* widget;
        float   mainSize;
        float   crossSize;
        float   flexGrow;
        bool    crossAuto;  // 交差軸が Auto か
    };
    std::vector<ChildInfo> infos;

    for (auto& child : children)
    {
        if (!child->visible) continue;
        if (child->computedStyle.position == PositionType::Absolute) continue;

        auto& cs = child->computedStyle;

        // MeasureWidget で子の希望サイズを計測
        auto hint = MeasureWidget(child.get(), contentW, contentH);

        float childMainSize, childCrossSize;
        bool crossAuto;

        if (isColumn)
        {
            childMainSize  = cs.height.IsAuto() ? hint.height : cs.height.Resolve(contentH);
            childCrossSize = cs.width.IsAuto()  ? hint.width  : cs.width.Resolve(contentW);
            crossAuto = cs.width.IsAuto();
            // 主軸方向の margin を加算
            totalFixed += cs.margin.top + cs.margin.bottom;
        }
        else
        {
            childMainSize  = cs.width.IsAuto()  ? hint.width  : cs.width.Resolve(contentW);
            childCrossSize = cs.height.IsAuto() ? hint.height : cs.height.Resolve(contentH);
            crossAuto = cs.height.IsAuto();
            totalFixed += cs.margin.left + cs.margin.right;
        }

        totalFixed    += childMainSize;
        totalFlexGrow += cs.flexGrow;
        infos.push_back({ child.get(), childMainSize, childCrossSize, cs.flexGrow, crossAuto });
        numVisible++;
    }

    // ギャップ合計
    float totalGap     = (numVisible > 1) ? style.gap * (numVisible - 1) : 0.0f;
    float mainAxisSize = isColumn ? contentH : contentW;
    float crossAxisSize= isColumn ? contentW : contentH;
    float freeSpace    = mainAxisSize - totalFixed - totalGap;

    // === Pass 2: flex-grow で残りスペースを分配 ===
    if (freeSpace > 0.0f && totalFlexGrow > 0.0f)
    {
        for (auto& info : infos)
        {
            if (info.flexGrow > 0.0f)
                info.mainSize += freeSpace * (info.flexGrow / totalFlexGrow);
        }
        freeSpace = 0.0f;
    }

    // === justify-content オフセット計算 ===
    float mainStart = 0.0f;
    float mainGap   = style.gap;

    switch (style.justifyContent)
    {
    case JustifyContent::Start:
        break;
    case JustifyContent::Center:
        mainStart = (std::max)(freeSpace * 0.5f, 0.0f);
        break;
    case JustifyContent::End:
        mainStart = (std::max)(freeSpace, 0.0f);
        break;
    case JustifyContent::SpaceBetween:
        if (numVisible > 1)
            mainGap = (freeSpace + totalGap) / (numVisible - 1);
        break;
    case JustifyContent::SpaceAround:
        if (numVisible > 0)
        {
            float spacePer = (freeSpace + totalGap) / numVisible;
            mainStart = spacePer * 0.5f;
            mainGap   = spacePer;
        }
        break;
    }

    // === Pass 3: 位置確定 ===
    // スクロールオフセットを適用
    float scrollX = widget->scrollOffsetX;
    float scrollY = widget->scrollOffsetY;

    float cursor = mainStart;
    for (auto& info : infos)
    {
        auto& cs = info.widget->computedStyle;

        // margin 分離
        float mms = isColumn ? cs.margin.top    : cs.margin.left;   // main-start
        float mme = isColumn ? cs.margin.bottom  : cs.margin.right;  // main-end
        float mcs = isColumn ? cs.margin.left    : cs.margin.top;    // cross-start
        float mce = isColumn ? cs.margin.right   : cs.margin.bottom; // cross-end

        // 主軸方向のマージンをカーソルに加算
        cursor += mms;

        // align-items（Stretch は交差軸 Auto の場合のみ有効）
        float crossPos = 0.0f;
        switch (style.alignItems)
        {
        case AlignItems::Start:
            crossPos = 0.0f;
            break;
        case AlignItems::Center:
            crossPos = (crossAxisSize - info.crossSize) * 0.5f;
            break;
        case AlignItems::End:
            crossPos = crossAxisSize - info.crossSize;
            break;
        case AlignItems::Stretch:
            if (info.crossAuto)
            {
                info.crossSize = crossAxisSize - mcs - mce;
                if (info.crossSize < 0.0f) info.crossSize = 0.0f;
                crossPos = 0.0f;
            }
            break;
        }

        // cross-axis margin を加算
        crossPos += mcs;

        float childX, childY, childW, childH;
        if (isColumn)
        {
            childX = contentX + crossPos - scrollX;
            childY = contentY + cursor  - scrollY;
            childW = info.crossSize;
            childH = info.mainSize;
        }
        else
        {
            childX = contentX + cursor  - scrollX;
            childY = contentY + crossPos - scrollY;
            childW = info.mainSize;
            childH = info.crossSize;
        }

        // 再帰レイアウト（位置は margin 込みで確定済み）
        LayoutWidget(info.widget, childX, childY, childW, childH);

        // cursor 進行（main-end margin + gap）
        cursor += info.mainSize + mme + mainGap;
    }

    // === Absolute 子の処理 ===
    for (auto& child : children)
    {
        if (!child->visible) continue;
        if (child->computedStyle.position != PositionType::Absolute) continue;

        auto& cs = child->computedStyle;
        float absX = contentX + cs.margin.left;
        float absY = contentY + cs.margin.top;

        if (!cs.posLeft.IsAuto())
            absX = contentX + cs.posLeft.Resolve(contentW);
        if (!cs.posTop.IsAuto())
            absY = contentY + cs.posTop.Resolve(contentH);

        LayoutWidget(child.get(), absX, absY, contentW, contentH);
    }
}

// ============================================================================
// リサイズ
// ============================================================================

void UIContext::OnResize(uint32_t width, uint32_t height)
{
    m_screenWidth = width;
    m_screenHeight = height;
    if (m_root) m_root->layoutDirty = true;
}

void UIContext::SetDesignResolution(uint32_t width, uint32_t height)
{
    m_designWidth = width;
    m_designHeight = height;
    if (m_renderer)
        m_renderer->SetDesignResolution(width, height);
    if (m_root) m_root->layoutDirty = true;
}

}} // namespace GX::GUI
