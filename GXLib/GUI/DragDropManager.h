#pragma once
/// @file DragDropManager.h
/// @brief ウィジェット間のドラッグ&ドロップを仲介するマネージャー
///
/// ドラッグ元（DragSource）とドロップ先（DropTarget）を登録しておくと、
/// マウス操作でウィジェット間のデータ受け渡しを自動で行う。
/// インベントリのアイテム移動やパレットへのドロップなどに使う。
/// @addtogroup grp_gui/// @{

#include "pch_graphics.h"
#include "GUI/Widget.h"

namespace gx { namespace GUI {

/// @brief ドラッグで運搬するデータ
struct DragPayload
{
    gx::String type;                ///< ペイロードの型識別子（例: "item", "color"）
    gx::Vector<uint8_t> data;       ///< バイナリデータ本体
    gx::String text;                ///< テキスト表現（デバッグ/簡易用途）
};

/// @brief ドラッグソース情報
struct DragSource
{
    Widget* sourceWidget = nullptr;    ///< ドラッグ元ウィジェット
    DragPayload payload;               ///< 運搬データ
    float startX = 0, startY = 0;      ///< ドラッグ開始位置
    float currentX = 0, currentY = 0;  ///< 現在のドラッグ位置
    bool dragging = false;             ///< ドラッグ中か
    float dragThreshold = 5.0f;        ///< ドラッグ開始判定の移動量閾値(px)
};

/// @brief ドロップターゲット情報
struct DropTarget
{
    gx::String acceptType;   ///< 受け入れるペイロード型（""なら全て受け入れ）
    std::function<bool(const DragPayload&)> canAccept;  ///< カスタム受け入れ判定
    std::function<void(const DragPayload&)> onDrop;     ///< ドロップ時コールバック
    std::function<void()> onDragEnter;                  ///< ドラッグがターゲット上に入った
    std::function<void()> onDragLeave;                  ///< ドラッグがターゲットから出た
};

/// @brief ドラッグ&ドロップマネージャー
class DragDropManager
{
public:
    DragDropManager() = default;
    ~DragDropManager() = default;

    /// @brief ウィジェットをドラッグソースとして登録
    void RegisterDragSource(Widget* widget, const gx::String& payloadType);

    /// @brief ウィジェットをドロップターゲットとして登録
    void RegisterDropTarget(Widget* widget, const DropTarget& target);

    /// @brief 登録を解除
    void UnregisterDragSource(Widget* widget);
    void UnregisterDropTarget(Widget* widget);

    /// @brief ドラッグを開始（内部使用）
    void BeginDrag(Widget* source, const DragPayload& payload, float x, float y);

    /// @brief ドラッグ中のマウス移動
    void UpdateDrag(float x, float y);

    /// @brief ドラッグ終了（ドロップ or キャンセル）
    void EndDrag(float x, float y);

    /// @brief ドラッグをキャンセル
    void CancelDrag();

    /// @brief 現在ドラッグ中か
    bool IsDragging() const { return m_dragSource.dragging; }

    /// @brief 現在のドラッグソース
    const DragSource& GetDragSource() const { return m_dragSource; }

    /// @brief 現在ドラッグカーソルが乗っているドロップターゲット
    Widget* GetCurrentDropTarget() const { return m_currentTarget; }

    /// @brief 登録済みソース数
    uint32_t GetDragSourceCount() const { return static_cast<uint32_t>(m_sources.size()); }

    /// @brief 登録済みターゲット数
    uint32_t GetDropTargetCount() const { return static_cast<uint32_t>(m_targets.size()); }

    /// @brief プレビューウィジェットを設定（ドラッグ中に表示）
    void SetDragPreview(Widget* preview) { m_dragPreview = preview; }
    Widget* GetDragPreview() const { return m_dragPreview; }

    /// @brief 全登録をクリア
    void Clear();

private:
    Widget* FindDropTargetAt(float x, float y) const;

    DragSource m_dragSource;                                ///< 現在のドラッグソース情報
    gx::HashMap<Widget*, gx::String> m_sources;    ///< ウィジェット→ペイロード型マップ
    gx::HashMap<Widget*, DropTarget> m_targets;     ///< ウィジェット→ドロップターゲット情報マップ
    Widget* m_currentTarget = nullptr;                     ///< 現在カーソル下のドロップターゲット
    Widget* m_dragPreview = nullptr;                       ///< ドラッグプレビューウィジェット
};

}} // namespace gx::GUI
/// @}
