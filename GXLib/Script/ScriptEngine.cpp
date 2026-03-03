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

bool ScriptEngine::ExecuteFile(const gx::String& path)
{
    if (!m_impl)
    {
        m_lastError = "ScriptEngine not initialized";
        return false;
    }

    auto result = m_impl->lua.safe_script_file(std::string(path.c_str()), sol::script_pass_on_error);
    if (!result.valid())
    {
        sol::error err = result;
        m_lastError = err.what();
        GX_LOG_ERROR("Lua error in file '{}': {}", path, m_lastError);
        return false;
    }
    return true;
}

bool ScriptEngine::ExecuteString(const gx::String& code)
{
    if (!m_impl)
    {
        m_lastError = "ScriptEngine not initialized";
        return false;
    }

    auto result = m_impl->lua.safe_script(std::string(code.c_str()), sol::script_pass_on_error);
    if (!result.valid())
    {
        sol::error err = result;
        m_lastError = err.what();
        GX_LOG_ERROR("Lua error: {}", m_lastError);
        return false;
    }
    return true;
}

bool ScriptEngine::CallFunction(const gx::String& funcName)
{
    if (!m_impl) return false;

    std::string sfn(funcName.c_str());
    sol::protected_function fn = m_impl->lua[sfn];
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

bool ScriptEngine::CallFunction(const gx::String& funcName, float arg1)
{
    if (!m_impl) return false;

    std::string sfn(funcName.c_str());
    sol::protected_function fn = m_impl->lua[sfn];
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

void ScriptEngine::SetGlobal(const gx::String& name, float value)
{
    if (m_impl) m_impl->lua[std::string(name.c_str())] = value;
}

void ScriptEngine::SetGlobal(const gx::String& name, int value)
{
    if (m_impl) m_impl->lua[std::string(name.c_str())] = value;
}

void ScriptEngine::SetGlobal(const gx::String& name, const gx::String& value)
{
    if (m_impl) m_impl->lua[std::string(name.c_str())] = std::string(value.c_str());
}

float ScriptEngine::GetGlobalFloat(const gx::String& name, float defaultValue) const
{
    if (!m_impl) return defaultValue;
    sol::object obj = m_impl->lua[std::string(name.c_str())];
    if (obj.is<float>()) return obj.as<float>();
    if (obj.is<double>()) return static_cast<float>(obj.as<double>());
    return defaultValue;
}

int ScriptEngine::GetGlobalInt(const gx::String& name, int defaultValue) const
{
    if (!m_impl) return defaultValue;
    sol::object obj = m_impl->lua[std::string(name.c_str())];
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
bool ScriptEngine::ExecuteFile(const gx::String&) { return false; }
bool ScriptEngine::ExecuteString(const gx::String&) { return false; }
bool ScriptEngine::CallFunction(const gx::String&) { return false; }
bool ScriptEngine::CallFunction(const gx::String&, float) { return false; }
void ScriptEngine::SetGlobal(const gx::String&, float) {}
void ScriptEngine::SetGlobal(const gx::String&, int) {}
void ScriptEngine::SetGlobal(const gx::String&, const gx::String&) {}
float ScriptEngine::GetGlobalFloat(const gx::String&, float d) const { return d; }
int ScriptEngine::GetGlobalInt(const gx::String&, int d) const { return d; }
void ScriptEngine::Shutdown() {}

} // namespace gx

#endif // GX_ENABLE_LUA
