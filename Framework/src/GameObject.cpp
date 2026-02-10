/// @file GameObject.cpp
/// @brief GameObject の実装。基本動作だけをまとめる。

#include "GameObject.h"
#include "FrameworkApp.h"

#include <cassert>

namespace GXFW
{

SceneContext& GameObject::Ctx()
{
    assert(m_ctx != nullptr && "GameObject has no SceneContext. Did you add it to a GameScene?");
    return *m_ctx;
}

} // namespace GXFW
