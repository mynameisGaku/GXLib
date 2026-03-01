#pragma once
/// @file GameState.h
/// @brief ゲームステートマシン（シーンとは独立した状態管理）
///
/// タイトル画面 → ゲームプレイ → ポーズメニュー → リザルト画面 等の
/// ゲームフロー管理に使う。スタック構造により、ポーズメニューを
/// 重ねて PushState/PopState できる。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx
{

/// @brief ゲームステート定義
struct GameState
{
    std::string name;                         ///< ステート名
    std::function<void()>      onEnter;       ///< ステート開始時
    std::function<void(float)> onUpdate;      ///< 毎フレーム更新
    std::function<void()>      onDraw;        ///< 描画
    std::function<void()>      onExit;        ///< ステート終了時
};

/// @brief ゲームステートマシン
///
/// RegisterState() でステートを登録し、ChangeState/PushState/PopState で遷移する。
/// Update/Draw はスタックトップのステートに対して呼ばれる。
class GameStateMachine
{
public:
    /// @brief ステートを登録する
    /// @param state ステート定義
    /// @return ステートID
    uint32_t RegisterState(const GameState& state);

    /// @brief ステートを名前で検索する
    /// @param name ステート名
    /// @return ステートID（見つからない場合 UINT32_MAX）
    uint32_t FindState(const std::string& name) const;

    /// @brief ステートを変更する（スタックを全クリア→Push）
    /// @param stateId RegisterState()で取得したID
    void ChangeState(uint32_t stateId);

    /// @brief ステートをスタックに積む（ポーズメニュー等）
    /// @param stateId ステートID
    void PushState(uint32_t stateId);

    /// @brief スタックトップのステートを取り除く
    void PopState();

    /// @brief 毎フレーム更新（スタックトップ）
    /// @param deltaTime デルタタイム（秒）
    void Update(float deltaTime);

    /// @brief 描画（スタックトップ）
    void Draw();

    /// @brief 現在のステート名を取得する
    /// @return ステート名（スタック空の場合空文字列）
    const std::string& GetCurrentStateName() const;

    /// @brief ステートスタックが空か判定する
    /// @return スタックが空なら true
    bool IsEmpty() const { return m_stack.empty(); }

    /// @brief スタックの深さを取得する
    /// @return 現在のスタック深さ
    size_t GetStackDepth() const { return m_stack.size(); }

private:
    std::vector<GameState> m_states;       ///< 登録済みステート一覧
    std::vector<uint32_t>  m_stack;        ///< アクティブなステートIDスタック
    static const std::string s_emptyName;  ///< 空文字列定数（スタック空時の戻り値）
};

} // namespace gx
/// @}
