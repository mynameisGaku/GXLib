/// @file ActionMapping.cpp
/// @brief アクションマッピングの実装
#include "pch.h"
#include "Input/ActionMapping.h"
#include "Input/Keyboard.h"
#include "Input/Mouse.h"
#include "Input/Gamepad.h"
#include "Core/Logger.h"

// JSON読み書き用
#include "ThirdParty/json.hpp"

namespace GX
{

ActionState ActionMapping::s_emptyState = {};

void ActionMapping::DefineAction(const std::string& name, const std::vector<InputBinding>& bindings)
{
    auto& def = m_actions[name];
    def.name = name;
    def.bindings = bindings;
    def.state = {};
    def.prevState = {};
}

void ActionMapping::RemoveAction(const std::string& name)
{
    m_actions.erase(name);
}

void ActionMapping::Update(const Keyboard& keyboard, const Mouse& mouse, const Gamepad& gamepad)
{
    for (auto& [name, def] : m_actions)
    {
        def.prevState = def.state;

        // 全バインディングの値を評価し、最大の絶対値を採用する
        float maxValue = 0.0f;
        bool anyPressed = false;

        for (const auto& binding : def.bindings)
        {
            float val = EvaluateBinding(binding, keyboard, mouse, gamepad);

            // 最も大きい絶対値を採用（複数バインドの中から最も強い入力を使う）
            if (std::abs(val) > std::abs(maxValue))
            {
                maxValue = val;
            }

            if (EvaluateBindingDigital(binding, keyboard, mouse, gamepad))
            {
                anyPressed = true;
            }
        }

        def.state.value = maxValue;
        def.state.pressed = anyPressed || (std::abs(maxValue) > 0.001f);
        def.state.triggered = def.state.pressed && !def.prevState.pressed;
        def.state.released  = !def.state.pressed && def.prevState.pressed;
    }
}

float ActionMapping::EvaluateBinding(const InputBinding& binding,
                                      const Keyboard& keyboard, const Mouse& mouse,
                                      const Gamepad& gamepad) const
{
    switch (binding.type)
    {
    case InputBindingType::KeyboardKey:
        return keyboard.IsKeyDown(binding.keyCode) ? binding.scale : 0.0f;

    case InputBindingType::MouseButton:
        return mouse.IsButtonDown(binding.keyCode) ? binding.scale : 0.0f;

    case InputBindingType::GamepadButton:
        return gamepad.IsButtonDown(binding.padIndex, binding.keyCode) ? binding.scale : 0.0f;

    case InputBindingType::GamepadAxis:
    {
        float raw = 0.0f;
        switch (binding.axisId)
        {
        case GamepadAxisId::LeftStickX:   raw = gamepad.GetLeftStickX(binding.padIndex); break;
        case GamepadAxisId::LeftStickY:   raw = gamepad.GetLeftStickY(binding.padIndex); break;
        case GamepadAxisId::RightStickX:  raw = gamepad.GetRightStickX(binding.padIndex); break;
        case GamepadAxisId::RightStickY:  raw = gamepad.GetRightStickY(binding.padIndex); break;
        case GamepadAxisId::LeftTrigger:  raw = gamepad.GetLeftTrigger(binding.padIndex); break;
        case GamepadAxisId::RightTrigger: raw = gamepad.GetRightTrigger(binding.padIndex); break;
        }
        // カスタムデッドゾーン（Gamepadクラスのデッドゾーンとは別に追加適用）
        if (std::abs(raw) < binding.deadZone) raw = 0.0f;
        return raw * binding.scale;
    }

    case InputBindingType::MouseAxis:
        // keyCode: 0=X, 1=Y
        if (binding.keyCode == 0)
            return static_cast<float>(mouse.GetDeltaX()) * binding.scale;
        else
            return static_cast<float>(mouse.GetDeltaY()) * binding.scale;
    }

    return 0.0f;
}

bool ActionMapping::EvaluateBindingDigital(const InputBinding& binding,
                                            const Keyboard& keyboard, const Mouse& mouse,
                                            const Gamepad& gamepad) const
{
    switch (binding.type)
    {
    case InputBindingType::KeyboardKey:
        return keyboard.IsKeyDown(binding.keyCode);
    case InputBindingType::MouseButton:
        return mouse.IsButtonDown(binding.keyCode);
    case InputBindingType::GamepadButton:
        return gamepad.IsButtonDown(binding.padIndex, binding.keyCode);
    case InputBindingType::GamepadAxis:
    {
        float val = std::abs(EvaluateBinding(binding, keyboard, mouse, gamepad));
        return val > binding.deadZone;
    }
    case InputBindingType::MouseAxis:
        return false;  // マウス軸はデジタル判定しない
    }
    return false;
}

const ActionState& ActionMapping::GetAction(const std::string& name) const
{
    auto it = m_actions.find(name);
    if (it != m_actions.end()) return it->second.state;
    return s_emptyState;
}

bool ActionMapping::IsActionPressed(const std::string& name) const
{
    return GetAction(name).pressed;
}

bool ActionMapping::IsActionTriggered(const std::string& name) const
{
    return GetAction(name).triggered;
}

bool ActionMapping::IsActionReleased(const std::string& name) const
{
    return GetAction(name).released;
}

float ActionMapping::GetActionValue(const std::string& name) const
{
    return GetAction(name).value;
}

bool ActionMapping::LoadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        GX_LOG_WARN("ActionMapping: failed to open '%s'", path.c_str());
        return false;
    }

    try
    {
        nlohmann::json root;
        file >> root;

        if (!root.contains("actions")) return false;

        for (auto& [actionName, bindingsJson] : root["actions"].items())
        {
            std::vector<InputBinding> bindings;
            for (auto& bj : bindingsJson)
            {
                InputBinding b;
                std::string typeName = bj.value("type", "KeyboardKey");
                if (typeName == "KeyboardKey")       b.type = InputBindingType::KeyboardKey;
                else if (typeName == "MouseButton")  b.type = InputBindingType::MouseButton;
                else if (typeName == "GamepadButton") b.type = InputBindingType::GamepadButton;
                else if (typeName == "GamepadAxis")  b.type = InputBindingType::GamepadAxis;
                else if (typeName == "MouseAxis")    b.type = InputBindingType::MouseAxis;

                b.keyCode = bj.value("key", 0);
                b.axisId = static_cast<GamepadAxisId>(bj.value("axis", 0));
                b.deadZone = bj.value("deadZone", 0.2f);
                b.scale = bj.value("scale", 1.0f);
                b.padIndex = bj.value("pad", 0);
                bindings.push_back(b);
            }
            DefineAction(actionName, bindings);
        }

        GX_LOG_INFO("ActionMapping: loaded %zu actions from '%s'",
                     m_actions.size(), path.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        GX_LOG_ERROR("ActionMapping: JSON parse error: %s", e.what());
        return false;
    }
}

bool ActionMapping::SaveToFile(const std::string& path) const
{
    nlohmann::json root;
    nlohmann::json actionsJson;

    for (const auto& [name, def] : m_actions)
    {
        nlohmann::json bindingsJson = nlohmann::json::array();
        for (const auto& b : def.bindings)
        {
            nlohmann::json bj;
            switch (b.type)
            {
            case InputBindingType::KeyboardKey:   bj["type"] = "KeyboardKey"; break;
            case InputBindingType::MouseButton:   bj["type"] = "MouseButton"; break;
            case InputBindingType::GamepadButton:  bj["type"] = "GamepadButton"; break;
            case InputBindingType::GamepadAxis:   bj["type"] = "GamepadAxis"; break;
            case InputBindingType::MouseAxis:     bj["type"] = "MouseAxis"; break;
            }
            bj["key"] = b.keyCode;
            if (b.type == InputBindingType::GamepadAxis)
                bj["axis"] = static_cast<int>(b.axisId);
            if (b.deadZone != 0.2f)
                bj["deadZone"] = b.deadZone;
            if (b.scale != 1.0f)
                bj["scale"] = b.scale;
            if (b.padIndex != 0)
                bj["pad"] = b.padIndex;
            bindingsJson.push_back(bj);
        }
        actionsJson[name] = bindingsJson;
    }

    root["actions"] = actionsJson;

    std::ofstream file(path);
    if (!file.is_open())
    {
        GX_LOG_ERROR("ActionMapping: failed to write '%s'", path.c_str());
        return false;
    }

    file << root.dump(2);
    GX_LOG_INFO("ActionMapping: saved %zu actions to '%s'",
                 m_actions.size(), path.c_str());
    return true;
}

void ActionMapping::Clear()
{
    m_actions.clear();
}

} // namespace GX
