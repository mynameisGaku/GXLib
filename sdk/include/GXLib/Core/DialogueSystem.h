#pragma once
/// @file DialogueSystem.h
/// @brief 台詞システム — キャラクターの会話を行送り・選択肢付きで進行する
///
/// DialogueSequence に台詞行と選択肢を登録し、
/// StartSequence() で再生を開始する。選択肢を選ぶと
/// 別のシーケンスに分岐できる。RPGやADVの会話シーンに使う。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>

namespace gx
{

/// @brief 台詞の1行
struct DialogueLine
{
    gx::String speaker;     ///< キャラクター名
    gx::String text;        ///< 台詞テキスト
    float displayDuration = 0.0f;  ///< 自動送りまでの秒数（0 = 手動）
};

/// @brief 台詞中の選択肢
struct DialogueChoice
{
    gx::String text;                ///< プレイヤーに表示される選択肢テキスト
    gx::String nextSequenceId;     ///< 選択時にジャンプするシーケンス（空 = 続行）
    std::function<void()> callback; ///< 選択時のオプションコールバック
};

/// @brief 選択肢付きの台詞シーケンス
struct DialogueSequence
{
    gx::String id;                    ///< 一意の識別子
    gx::Vector<DialogueLine> lines;   ///< 順番に表示される行
    gx::Vector<DialogueChoice> choices; ///< 最後の行の後に表示される選択肢（空 = なし）
    gx::String nextSequenceId;        ///< 次のシーケンスへの自動チェーン（空 = 停止）
};

/// @brief 台詞イベントコールバック
struct DialogueCallbacks
{
    std::function<void(const gx::String& speaker, const gx::String& text)> onLineStarted;   ///< 台詞行が表示開始した時のコールバック
    std::function<void(const gx::Vector<DialogueChoice>& choices)> onChoicesPresented;       ///< 選択肢が提示された時のコールバック
    std::function<void()> onSequenceEnded;                                                     ///< シーケンスが終了した時のコールバック
};

/// @brief 台詞の状態
enum class DialogueState
{
    Inactive,         ///< 台詞が非アクティブ
    ShowingLine,      ///< 台詞行を表示中
    WaitingForInput,  ///< プレイヤーの入力待ち
    ShowingChoices,   ///< 選択肢を表示中
};

/// @brief NPC会話を管理する台詞システム
class DialogueSystem
{
public:
    DialogueSystem() = default;
    ~DialogueSystem() = default;

    /// @brief 台詞シーケンスを登録する
    /// @param sequence 登録するシーケンス
    void RegisterSequence(const DialogueSequence& sequence);

    /// @brief IDで台詞シーケンスを開始する
    /// @return シーケンスが見つかり開始された場合true
    bool StartSequence(const gx::String& sequenceId);

    /// @brief 次の行に進む（末尾の場合は選択肢を表示）
    void AdvanceLine();

    /// @brief インデックスで選択肢を選択する
    /// @param index 選択肢のインデックス（0始まり）
    void SelectChoice(int index);

    /// @brief 台詞システムを更新する（自動送りタイミング用）
    /// @param deltaTime 前フレームからの経過秒数
    void Update(float deltaTime);

    /// @brief 現在の状態を取得する
    /// @return 台詞の状態
    DialogueState GetState() const { return m_state; }

    /// @brief 現在表示中の行を取得する
    /// @return 現在の行へのポインタ（アクティブでなければnullptr）
    const DialogueLine* GetCurrentLine() const;

    /// @brief シーケンス内の現在の行インデックスを取得する
    /// @return 行インデックス（-1は未開始）
    int GetCurrentLineIndex() const { return m_currentLineIndex; }

    /// @brief 現在の選択肢を取得する（ShowingChoices状態の場合）
    /// @return 選択肢の配列への参照（選択肢がなければ空の配列）
    const gx::Vector<DialogueChoice>& GetCurrentChoices() const;

    /// @brief 台詞がアクティブか確認する
    /// @return Inactive以外の状態ならtrue
    bool IsActive() const { return m_state != DialogueState::Inactive; }

    /// @brief 台詞を即座に終了する
    void EndDialogue();

    /// @brief イベントコールバックを設定する
    /// @param cb コールバック群
    void SetCallbacks(const DialogueCallbacks& cb) { m_callbacks = cb; }

    /// @brief 登録済みシーケンス数を取得する
    /// @return シーケンス数
    int GetSequenceCount() const { return static_cast<int>(m_sequences.size()); }

    /// @brief シーケンスが存在するか確認する
    /// @param id シーケンスID
    /// @return 登録済みならtrue
    bool HasSequence(const gx::String& id) const { return m_sequences.count(id) > 0; }

private:
    /// @brief 現在の行を表示しコールバックを発火する
    void ShowCurrentLine();

    gx::HashMap<gx::String, DialogueSequence> m_sequences; ///< 登録済みシーケンス
    const DialogueSequence* m_currentSequence = nullptr;           ///< 現在再生中のシーケンス
    int m_currentLineIndex = -1;                                    ///< 現在の行インデックス
    float m_lineTimer = 0.0f;                                       ///< 自動送りタイマー（秒）
    DialogueState m_state = DialogueState::Inactive;                ///< 現在の状態
    DialogueCallbacks m_callbacks;                                  ///< イベントコールバック
    static const gx::Vector<DialogueChoice> s_emptyChoices;       ///< 空の選択肢（参照返却用）
};

} // namespace gx
/// @}
