/// @file Model.cpp
/// @brief モデルクラスの実装
#include "pch.h"
#include "Graphics/3D/Model.h"

namespace GX
{

int Model::FindAnimationIndex(const std::string& name) const
{
    for (uint32_t i = 0; i < m_animations.size(); ++i)
    {
        if (m_animations[i].GetName() == name)
            return static_cast<int>(i);
    }
    return -1;
}

} // namespace GX
