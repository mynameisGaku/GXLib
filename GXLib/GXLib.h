#pragma once
/// @file GXLib.h
/// @brief GXLib 統合ヘッダー — これ1つで全機能にアクセス可能
///
/// 手続き型API（DrawGraph, DrawString 等）、クラス型API（gx::App）、
/// 中級者向けアクセサ（GetRenderer3D, GetPostEffects 等）を全て含む。
/// @addtogroup grp_graphics
/// @{

// コンテナ型 (PCHなしの外部ターゲット用)
#include "Container/ContainerFwd.h"

// 手続き型API（DXLib互換）
#include "Compat/GXLib.h"

// クラス型API
#include "GX/App.h"

// ポストエフェクトマスク
#include "Graphics/PostEffect/PostFXMask.h"

// 数学型
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Quaternion.h"
#include "Math/Matrix4x4.h"
#include "Math/Color.h"
#include "Math/ColorCode.h"
#include "Math/Tween.h"
#include "Math/Random.h"

// よく使うC++ヘルパー
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/AnimationPlayer.h"
#include "Graphics/3D/Humanoid.h"
#include "Physics/PhysicsWorld3D.h"
#include "Physics/RigidBody3D.h"
#include "Physics/MeshCollider.h"
/// @}
