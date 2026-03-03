/// @file DataBinding.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "GUI/DataBinding.h"

namespace gx { namespace GUI {

// ============================================================================
// DataContext
// ============================================================================

void DataContext::SetProperty(const gx::String& name, const gx::String& value)
{
    auto it = m_properties.find(name);
    if (it == m_properties.end())
    {
        DataProperty prop(name);
        prop.SetSilent(value);
        m_properties.emplace(name, std::move(prop));
    }
    else
    {
        it->second.Set(value);
    }

    if (OnPropertyChanged)
        OnPropertyChanged(name, value);
}

gx::String DataContext::GetProperty(const gx::String& name) const
{
    auto it = m_properties.find(name);
    if (it == m_properties.end())
        return "";
    return it->second.Get();
}

bool DataContext::HasProperty(const gx::String& name) const
{
    return m_properties.find(name) != m_properties.end();
}

bool DataContext::RemoveProperty(const gx::String& name)
{
    return m_properties.erase(name) > 0;
}

size_t DataContext::GetPropertyCount() const
{
    return m_properties.size();
}

gx::Vector<gx::String> DataContext::GetPropertyNames() const
{
    gx::Vector<gx::String> names;
    names.reserve(m_properties.size());
    for (const auto& [name, prop] : m_properties)
        names.push_back(name);
    return names;
}

DataProperty* DataContext::GetDataProperty(const gx::String& name)
{
    auto it = m_properties.find(name);
    if (it == m_properties.end())
        return nullptr;
    return &it->second;
}

// ============================================================================
// DataBindingManager
// ============================================================================

uint32_t DataBindingManager::Bind(const gx::String& sourceProperty, const gx::String& targetId,
                                   const gx::String& targetProperty, BindingMode mode)
{
    Binding binding;
    binding.sourceProperty = sourceProperty;
    binding.targetId = targetId;
    binding.targetProperty = targetProperty;
    binding.mode = mode;
    binding.active = true;

    uint32_t id = m_nextBindingId++;
    m_bindings[id] = std::move(binding);
    return id;
}

uint32_t DataBindingManager::BindWithConverter(const gx::String& sourceProperty, const gx::String& targetId,
                                                const gx::String& targetProperty, const BindingConverter& converter,
                                                BindingMode mode)
{
    Binding binding;
    binding.sourceProperty = sourceProperty;
    binding.targetId = targetId;
    binding.targetProperty = targetProperty;
    binding.mode = mode;
    binding.converter = converter;
    binding.active = true;

    uint32_t id = m_nextBindingId++;
    m_bindings[id] = std::move(binding);
    return id;
}

bool DataBindingManager::Unbind(uint32_t bindingId)
{
    return m_bindings.erase(bindingId) > 0;
}

void DataBindingManager::UnbindAll(const gx::String& targetId)
{
    for (auto it = m_bindings.begin(); it != m_bindings.end(); )
    {
        if (it->second.targetId == targetId)
            it = m_bindings.erase(it);
        else
            ++it;
    }
}

size_t DataBindingManager::GetBindingCount() const
{
    return m_bindings.size();
}

const Binding* DataBindingManager::GetBinding(uint32_t bindingId) const
{
    auto it = m_bindings.find(bindingId);
    if (it == m_bindings.end())
        return nullptr;
    return &it->second;
}

void DataBindingManager::SetDataContext(DataContext* context)
{
    m_dataContext = context;
}

DataContext* DataBindingManager::GetDataContext() const
{
    return m_dataContext;
}

void DataBindingManager::SetActive(uint32_t bindingId, bool active)
{
    auto it = m_bindings.find(bindingId);
    if (it != m_bindings.end())
        it->second.active = active;
}

bool DataBindingManager::IsActive(uint32_t bindingId) const
{
    auto it = m_bindings.find(bindingId);
    if (it == m_bindings.end())
        return false;
    return it->second.active;
}

void DataBindingManager::UpdateBindings()
{
    if (!m_dataContext)
        return;

    // Process all dirty properties
    for (const auto& propName : m_dirtyProperties)
    {
        for (auto& [id, binding] : m_bindings)
        {
            if (!binding.active)
                continue;
            if (binding.sourceProperty != propName)
                continue;

            // OneTime bindings are only applied once then deactivated
            if (binding.mode == BindingMode::OneTime)
            {
                binding.active = false;
            }

            // The actual target update would happen through widget lookup
            // For now, we track that the binding was evaluated
        }
    }

    m_dirtyProperties.clear();
}

void DataBindingManager::NotifyPropertyChanged(const gx::String& propertyName)
{
    // Add to dirty list (avoid duplicates)
    for (const auto& name : m_dirtyProperties)
    {
        if (name == propertyName)
            return;
    }
    m_dirtyProperties.push_back(propertyName);
}

gx::Vector<uint32_t> DataBindingManager::GetBindingsForProperty(const gx::String& propertyName) const
{
    gx::Vector<uint32_t> result;
    for (const auto& [id, binding] : m_bindings)
    {
        if (binding.sourceProperty == propertyName)
            result.push_back(id);
    }
    return result;
}

gx::Vector<uint32_t> DataBindingManager::GetBindingsForTarget(const gx::String& targetId) const
{
    gx::Vector<uint32_t> result;
    for (const auto& [id, binding] : m_bindings)
    {
        if (binding.targetId == targetId)
            result.push_back(id);
    }
    return result;
}

void DataBindingManager::Clear()
{
    m_bindings.clear();
    m_dirtyProperties.clear();
    m_nextBindingId = 1;
}

}} // namespace gx::GUI
