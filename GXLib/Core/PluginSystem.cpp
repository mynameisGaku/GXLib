/// @file PluginSystem.cpp
/// @brief プラグイン管理システムの実装

#include "pch_common.h"
#include "Core/PluginSystem.h"

#include <algorithm>

namespace gx
{

// =============================================================================
// 初期化 / シャットダウン
// =============================================================================

void PluginSystem::Initialize()
{
    m_initialized = true;
}

void PluginSystem::Shutdown()
{
    ShutdownAll();
    m_plugins.clear();
    m_initialized = false;
}

// =============================================================================
// プラグイン登録 / 解除
// =============================================================================

bool PluginSystem::RegisterPlugin(std::unique_ptr<IPlugin> plugin)
{
    if (!plugin)
    {
        return false;
    }

    const gx::String name = plugin->GetInfo().name;

    // 同名チェック
    for (const auto& entry : m_plugins)
    {
        if (entry.plugin && entry.plugin->GetInfo().name == name)
        {
            return false; // 同名プラグインが既に存在
        }
    }

    PluginEntry entry;
    entry.plugin      = std::move(plugin);
    entry.enabled     = true;
    entry.initialized = false;

    m_plugins.push_back(std::move(entry));

    // コールバック呼び出し
    if (OnPluginLoaded)
    {
        OnPluginLoaded(name);
    }

    return true;
}

bool PluginSystem::UnregisterPlugin(const gx::String& name)
{
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it)
    {
        if (it->plugin && it->plugin->GetInfo().name == name)
        {
            // 初期化済みならシャットダウン
            if (it->initialized)
            {
                it->plugin->Shutdown();
            }

            m_plugins.erase(it);

            // コールバック呼び出し
            if (OnPluginUnloaded)
            {
                OnPluginUnloaded(name);
            }

            return true;
        }
    }
    return false;
}

IPlugin* PluginSystem::GetPlugin(const gx::String& name) const
{
    for (const auto& entry : m_plugins)
    {
        if (entry.plugin && entry.plugin->GetInfo().name == name)
        {
            return entry.plugin.get();
        }
    }
    return nullptr;
}

bool PluginSystem::HasPlugin(const gx::String& name) const
{
    return GetPlugin(name) != nullptr;
}

// =============================================================================
// 一括操作
// =============================================================================

void PluginSystem::InitializeAll()
{
    SortByPriority();

    for (auto& entry : m_plugins)
    {
        if (entry.plugin && entry.enabled && !entry.initialized)
        {
            if (entry.plugin->Initialize())
            {
                entry.initialized = true;
            }
        }
    }
}

void PluginSystem::ShutdownAll()
{
    // 逆順でシャットダウン（優先度の逆）
    for (size_t i = m_plugins.size(); i > 0; --i)
    {
        auto& entry = m_plugins[i - 1];
        if (entry.plugin && entry.initialized)
        {
            entry.plugin->Shutdown();
            entry.initialized = false;
        }
    }
}

void PluginSystem::UpdateAll(float deltaTime)
{
    for (auto& entry : m_plugins)
    {
        if (entry.plugin && entry.enabled && entry.initialized)
        {
            entry.plugin->Update(deltaTime);
        }
    }
}

// =============================================================================
// 情報取得
// =============================================================================

gx::Vector<PluginInfo> PluginSystem::GetLoadedPlugins() const
{
    gx::Vector<PluginInfo> infos;
    infos.reserve(m_plugins.size());
    for (const auto& entry : m_plugins)
    {
        if (entry.plugin)
        {
            infos.push_back(entry.plugin->GetInfo());
        }
    }
    return infos;
}

size_t PluginSystem::GetPluginCount() const
{
    return m_plugins.size();
}

bool PluginSystem::IsInitialized() const
{
    return m_initialized;
}

// =============================================================================
// 有効/無効
// =============================================================================

void PluginSystem::SetPluginEnabled(const gx::String& name, bool enabled)
{
    for (auto& entry : m_plugins)
    {
        if (entry.plugin && entry.plugin->GetInfo().name == name)
        {
            entry.enabled = enabled;
            return;
        }
    }
}

bool PluginSystem::IsPluginEnabled(const gx::String& name) const
{
    for (const auto& entry : m_plugins)
    {
        if (entry.plugin && entry.plugin->GetInfo().name == name)
        {
            return entry.enabled;
        }
    }
    return false;
}

// =============================================================================
// Private ヘルパー
// =============================================================================

void PluginSystem::SortByPriority()
{
    std::stable_sort(m_plugins.begin(), m_plugins.end(),
        [](const PluginEntry& a, const PluginEntry& b)
        {
            int pa = a.plugin ? a.plugin->GetPriority() : 0;
            int pb = b.plugin ? b.plugin->GetPriority() : 0;
            return pa < pb;
        });
}

} // namespace gx
