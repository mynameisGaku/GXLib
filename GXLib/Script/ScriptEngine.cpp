/// @file ScriptEngine.cpp
/// @brief Lua スクリプトエンジンの実装
#include "pch_common.h"
#include "Script/ScriptEngine.h"
#include "Script/ScriptBindings.h"
#include "Core/Logger.h"

#ifdef GX_ENABLE_LUA

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

namespace gx
{

struct ScriptEngine::Impl
{
    sol::state lua;
    SpriteBatch* spriteBatch = nullptr;
    TextureManager* texManager = nullptr;
};

ScriptEngine::ScriptEngine() = default;

ScriptEngine::~ScriptEngine()
{
    Shutdown();
}

bool ScriptEngine::Initialize()
{
    m_impl = std::make_unique<Impl>();
    m_impl->lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table,
        sol::lib::io,
        sol::lib::os
    );

    // 数学バインディングを登録（入力は SetContext で登録）
    ScriptBindings::RegisterMath(m_impl->lua);

    return true;
}

void ScriptEngine::SetContext(SpriteBatch* spriteBatch, TextureManager* texManager,
                              InputManager* inputMgr)
{
    if (!m_impl) return;
    m_impl->spriteBatch = spriteBatch;
    m_impl->texManager = texManager;

    ScriptBindings::RegisterInput(m_impl->lua, inputMgr);
    ScriptBindings::RegisterDrawing(m_impl->lua, spriteBatch, texManager);
}

bool ScriptEngine::ExecuteFile(const std::string& path)
{
    if (!m_impl)
    {
        m_lastError = "ScriptEngine not initialized";
        return false;
    }

    auto result = m_impl->lua.safe_script_file(path, sol::script_pass_on_error);
    if (!result.valid())
    {
        sol::error err = result;
        m_lastError = err.what();
        GX_LOG_ERROR("Lua error in file '{}': {}", path, m_lastError);
        return false;
    }
    return true;
}

bool ScriptEngine::ExecuteString(const std::string& code)
{
    if (!m_impl)
    {
        m_lastError = "ScriptEngine not initialized";
        return false;
    }

    auto result = m_impl->lua.safe_script(code, sol::script_pass_on_error);
    if (!result.valid())
    {
        sol::error err = result;
        m_lastError = err.what();
        GX_LOG_ERROR("Lua error: {}", m_lastError);
        return false;
    }
    return true;
}

bool ScriptEngine::CallFunction(const std::string& funcName)
{
    if (!m_impl) return false;

    sol::protected_function fn = m_impl->lua[funcName];
    if (!fn.valid())
    {
        m_lastError = "Function not found: " + funcName;
        return false;
    }

    auto result = fn();
    if (!result.valid())
    {
        sol::error err = result;
        m_lastError = err.what();
        GX_LOG_ERROR("Lua call error '{}': {}", funcName, m_lastError);
        return false;
    }
    return true;
}

bool ScriptEngine::CallFunction(const std::string& funcName, float arg1)
{
    if (!m_impl) return false;

    sol::protected_function fn = m_impl->lua[funcName];
    if (!fn.valid())
    {
        m_lastError = "Function not found: " + funcName;
        return false;
    }

    auto result = fn(arg1);
    if (!result.valid())
    {
        sol::error err = result;
        m_lastError = err.what();
        GX_LOG_ERROR("Lua call error '{}': {}", funcName, m_lastError);
        return false;
    }
    return true;
}

void ScriptEngine::SetGlobal(const std::string& name, float value)
{
    if (m_impl) m_impl->lua[name] = value;
}

void ScriptEngine::SetGlobal(const std::string& name, int value)
{
    if (m_impl) m_impl->lua[name] = value;
}

void ScriptEngine::SetGlobal(const std::string& name, const std::string& value)
{
    if (m_impl) m_impl->lua[name] = value;
}

float ScriptEngine::GetGlobalFloat(const std::string& name, float defaultValue) const
{
    if (!m_impl) return defaultValue;
    sol::object obj = m_impl->lua[name];
    if (obj.is<float>()) return obj.as<float>();
    if (obj.is<double>()) return static_cast<float>(obj.as<double>());
    return defaultValue;
}

int ScriptEngine::GetGlobalInt(const std::string& name, int defaultValue) const
{
    if (!m_impl) return defaultValue;
    sol::object obj = m_impl->lua[name];
    if (obj.is<int>()) return obj.as<int>();
    return defaultValue;
}

void ScriptEngine::Shutdown()
{
    m_impl.reset();
}

} // namespace gx

#else // !GX_ENABLE_LUA

// Lua 無効ビルド時のスタブ実装
namespace gx
{

struct ScriptEngine::Impl {};

ScriptEngine::ScriptEngine() = default;
ScriptEngine::~ScriptEngine() = default;

bool ScriptEngine::Initialize()
{
    m_lastError = "Lua scripting is not enabled in this build";
    return false;
}

void ScriptEngine::SetContext(SpriteBatch*, TextureManager*, InputManager*) {}
bool ScriptEngine::ExecuteFile(const std::string&) { return false; }
bool ScriptEngine::ExecuteString(const std::string&) { return false; }
bool ScriptEngine::CallFunction(const std::string&) { return false; }
bool ScriptEngine::CallFunction(const std::string&, float) { return false; }
void ScriptEngine::SetGlobal(const std::string&, float) {}
void ScriptEngine::SetGlobal(const std::string&, int) {}
void ScriptEngine::SetGlobal(const std::string&, const std::string&) {}
float ScriptEngine::GetGlobalFloat(const std::string&, float d) const { return d; }
int ScriptEngine::GetGlobalInt(const std::string&, int d) const { return d; }
void ScriptEngine::Shutdown() {}

} // namespace gx

#endif // GX_ENABLE_LUA
