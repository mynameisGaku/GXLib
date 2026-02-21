#pragma once
/// @file ActionMapping.h
/// @brief アクションマッピング（入力抽象化レイヤー）
///
/// ゲーム内の論理アクション（"Jump", "Attack"等）と物理入力（キーボードキー、
/// ゲームパッドボタン/軸）を分離し、リマッピング可能にする。
/// 1つのアクションに複数の入力バインディングを割り当てでき、
/// JSONファイルへの保存/読み込みでキーコンフィグ設定を永続化できる。
///
/// DxLibには直接相当するAPIはないが、ゲーム開発では標準的なパターン。
/// 典型的な使い方:
///   actionMapping.DefineAction("Jump", { KeyBinding(VK_SPACE), PadBinding(PadButton::A) });
///   if (actionMapping.IsActionTriggered("Jump")) { /* ジャンプ処理 */ }

#include "pch.h"

namespace GX
{

class Keyboard;
class Mouse;
class Gamepad;

/// @brief 入力バインディングの種類
enum class InputBindingType
{
    KeyboardKey,    ///< キーボードキー（VK_*）
    MouseButton,    ///< マウスボタン（0=左, 1=右, 2=中）
    GamepadButton,  ///< ゲームパッドボタン（PadButton::*）
    GamepadAxis,    ///< ゲームパッドアナログ軸
    MouseAxis,      ///< マウス移動量
};

/// @brief ゲームパッド軸の識別子
enum class GamepadAxisId
{
    LeftStickX  = 0,
    LeftStickY  = 1,
    RightStickX = 2,
    RightStickY = 3,
    LeftTrigger = 4,
    RightTrigger = 5,
};

/// @brief 1つの入力バインディング
struct InputBinding
{
    InputBindingType type = InputBindingType::KeyboardKey;
    int keyCode = 0;             ///< VK_*, PadButton::*, MouseButton::* のキーコード
    GamepadAxisId axisId = GamepadAxisId::LeftStickX;  ///< 軸ID（GamepadAxis時のみ）
    float deadZone = 0.2f;       ///< アナログ軸のデッドゾーン
    float scale = 1.0f;          ///< 軸のスケール（-1.0で反転）
    int padIndex = 0;            ///< ゲームパッドインデックス（0〜3）

    /// @brief キーボードキーバインディングを作成する
    static InputBinding Key(int vk) {
        InputBinding b; b.type = InputBindingType::KeyboardKey; b.keyCode = vk; return b;
    }
    /// @brief マウスボタンバインディングを作成する
    static InputBinding MouseBtn(int btn) {
        InputBinding b; b.type = InputBindingType::MouseButton; b.keyCode = btn; return b;
    }
    /// @brief ゲームパッドボタンバインディングを作成する
    static InputBinding PadBtn(int btn, int pad = 0) {
        InputBinding b; b.type = InputBindingType::GamepadButton; b.keyCode = btn; b.padIndex = pad; return b;
    }
    /// @brief ゲームパッド軸バインディングを作成する
    static InputBinding PadAxis(GamepadAxisId axis, float s = 1.0f, float dz = 0.2f, int pad = 0) {
        InputBinding b; b.type = InputBindingType::GamepadAxis; b.axisId = axis; b.scale = s; b.deadZone = dz; b.padIndex = pad; return b;
    }
    /// @brief キーボードキーを軸として使う（スケール指定付き）
    static InputBinding KeyAxis(int vk, float s) {
        InputBinding b; b.type = InputBindingType::KeyboardKey; b.keyCode = vk; b.scale = s; return b;
    }
};

/// @brief アクションの状態
struct ActionState
{
    bool  pressed   = false;   ///< 現在押されているか
    bool  triggered = false;   ///< このフレームで押されたか
    bool  released  = false;   ///< このフレームで離されたか
    float value     = 0.0f;    ///< アナログ値（デジタル入力では0.0 or 1.0）
};

/// @brief アクションマッピング
///
/// 論理アクションと入力バインディングの対応を管理する。
/// 毎フレームUpdate()を呼ぶことで、全バインディングを評価し
/// アクション状態（pressed/triggered/released/value）を更新する。
class ActionMapping
{
public:
    ActionMapping() = default;
    ~ActionMapping() = default;

    /// @brief アクションを定義する（既存の同名アクションは上書き）
    /// @param name アクション名（"Jump", "MoveX"等）
    /// @param bindings 入力バインディング配列（複数バインド可能）
    void DefineAction(const std::string& name, const std::vector<InputBinding>& bindings);

    /// @brief アクションを削除する
    /// @param name アクション名
    void RemoveAction(const std::string& name);

    /// @brief 全アクション状態を更新する（InputManager::Update()の後に呼ぶ）
    /// @param keyboard キーボード入力
    /// @param mouse マウス入力
    /// @param gamepad ゲームパッド入力
    void Update(const Keyboard& keyboard, const Mouse& mouse, const Gamepad& gamepad);

    /// @brief アクション状態を取得する
    /// @param name アクション名
    /// @return アクション状態への参照（未定義のアクションは空の状態を返す）
    const ActionState& GetAction(const std::string& name) const;

    /// @brief アクションが押されているか
    /// @param name アクション名
    /// @return 押されていればtrue
    bool IsActionPressed(const std::string& name) const;

    /// @brief アクションがこのフレームで押されたか
    /// @param name アクション名
    /// @return トリガーされていればtrue
    bool IsActionTriggered(const std::string& name) const;

    /// @brief アクションがこのフレームで離されたか
    /// @param name アクション名
    /// @return リリースされていればtrue
    bool IsActionReleased(const std::string& name) const;

    /// @brief アクションのアナログ値を取得する
    /// @param name アクション名
    /// @return アナログ値（-1.0〜1.0 or 0.0〜1.0）
    float GetActionValue(const std::string& name) const;

    /// @brief JSONファイルからバインディングを読み込む
    /// @param path ファイルパス
    /// @return 成功でtrue
    bool LoadFromFile(const std::string& path);

    /// @brief JSONファイルにバインディングを保存する
    /// @param path ファイルパス
    /// @return 成功でtrue
    bool SaveToFile(const std::string& path) const;

    /// @brief 全アクションをクリアする
    void Clear();

private:
    /// @brief アクション定義
    struct ActionDef
    {
        std::string name;
        std::vector<InputBinding> bindings;
        ActionState state;
        ActionState prevState;
    };

    /// @brief 1バインディングの値を評価する
    float EvaluateBinding(const InputBinding& binding,
                           const Keyboard& keyboard, const Mouse& mouse,
                           const Gamepad& gamepad) const;

    /// @brief 1バインディングのデジタル押下状態を評価する
    bool EvaluateBindingDigital(const InputBinding& binding,
                                 const Keyboard& keyboard, const Mouse& mouse,
                                 const Gamepad& gamepad) const;

    std::unordered_map<std::string, ActionDef> m_actions;
    static ActionState s_emptyState;
};

} // namespace GX
