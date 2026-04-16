#pragma once
/// @file ReflectMacros.h
/// @brief 型・プロパティをリフレクションシステムへ登録するマクロ集
///
/// GX_REFLECT_BEGIN / GX_REFLECT_END ブロックで囲み、
/// GX_REFLECT_FLOAT / GX_REFLECT_BOOL などで各メンバを列挙するだけで
/// 実行時にプロパティを名前・型付きで参照できるようになる。
///
/// 使用例:
///   GX_REFLECT_BEGIN(MyStruct)
///       GX_REFLECT_FLOAT("speed", speed)
///       GX_REFLECT_BOOL("active", active)
///       GX_REFLECT_STRING("name", name)
///       GX_REFLECT_FLOAT_RANGE("volume", volume, 0.0f, 1.0f)
///   GX_REFLECT_END(MyStruct)
/// @addtogroup grp_reflection/// @{

#include "Core/Reflect/TypeRegistry.h"
#include "Core/Reflect/PropertyMeta.h"

// マクロはプリプロセッサレベルのため、意図的にnamespace gxの外に配置。
// 内部でgx::型を完全修飾しているため、任意のスコープで使用可能。

/// @brief 型内のメンバのバイトオフセットを計算する
#define GX_OFFSET_OF(Type, Member) \
    static_cast<uint32_t>(reinterpret_cast<size_t>( \
        &reinterpret_cast<const volatile char&>(((Type*)0)->Member)))

/// @brief 型のリフレクション登録ブロックを開始する
#define GX_REFLECT_BEGIN(Type) \
    namespace { \
    struct Type##_Registrar { \
        Type##_Registrar() { \
            using _ReflType = Type; \
            auto typeInfo = std::make_unique<gx::TypeInfo>(#Type, sizeof(_ReflType)); \
            typeInfo->SetFactory([]() -> void* { return new _ReflType(); });

/// @brief 明示的なPropertyTypeでプロパティを登録する
#define GX_REFLECT_PROPERTY(PropName, Member, PropType) \
            { \
                gx::PropertyMeta prop; \
                prop.name = PropName; \
                prop.displayName = PropName; \
                prop.type = PropType; \
                prop.offset = GX_OFFSET_OF(_ReflType, Member); \
                prop.size = sizeof(decltype(std::declval<_ReflType>().Member)); \
                typeInfo->AddProperty(prop); \
            }

/// @brief boolプロパティを登録する
#define GX_REFLECT_BOOL(PropName, Member) \
    GX_REFLECT_PROPERTY(PropName, Member, gx::PropertyType::Bool)

/// @brief intプロパティを登録する
#define GX_REFLECT_INT(PropName, Member) \
    GX_REFLECT_PROPERTY(PropName, Member, gx::PropertyType::Int)

/// @brief floatプロパティを登録する
#define GX_REFLECT_FLOAT(PropName, Member) \
    GX_REFLECT_PROPERTY(PropName, Member, gx::PropertyType::Float)

/// @brief Vec3プロパティを登録する
#define GX_REFLECT_VEC3(PropName, Member) \
    GX_REFLECT_PROPERTY(PropName, Member, gx::PropertyType::Vec3)

/// @brief Vec4プロパティを登録する
#define GX_REFLECT_VEC4(PropName, Member) \
    GX_REFLECT_PROPERTY(PropName, Member, gx::PropertyType::Vec4)

/// @brief Quaternionプロパティを登録する
#define GX_REFLECT_QUATERNION(PropName, Member) \
    GX_REFLECT_PROPERTY(PropName, Member, gx::PropertyType::Quaternion)

/// @brief Colorプロパティを登録する
#define GX_REFLECT_COLOR(PropName, Member) \
    GX_REFLECT_PROPERTY(PropName, Member, gx::PropertyType::Color)

/// @brief gx::Stringプロパティを登録する（安全なコピーのためゲッター/セッターを使用）
#define GX_REFLECT_STRING(PropName, Member) \
            { \
                gx::PropertyMeta prop; \
                prop.name = PropName; \
                prop.displayName = PropName; \
                prop.type = gx::PropertyType::String; \
                prop.offset = GX_OFFSET_OF(_ReflType, Member); \
                prop.size = sizeof(gx::String); \
                prop.getter = [](const void* obj, void* out) { \
                    *static_cast<gx::String*>(out) = \
                        static_cast<const _ReflType*>(obj)->Member; \
                }; \
                prop.setter = [](void* obj, const void* val) { \
                    static_cast<_ReflType*>(obj)->Member = \
                        *static_cast<const gx::String*>(val); \
                }; \
                typeInfo->AddProperty(prop); \
            }

/// @brief 最小/最大範囲付きfloatプロパティを登録する
#define GX_REFLECT_FLOAT_RANGE(PropName, Member, Min, Max) \
            { \
                gx::PropertyMeta prop; \
                prop.name = PropName; \
                prop.displayName = PropName; \
                prop.type = gx::PropertyType::Float; \
                prop.offset = GX_OFFSET_OF(_ReflType, Member); \
                prop.size = sizeof(float); \
                prop.hasRange = true; \
                prop.minValue = Min; \
                prop.maxValue = Max; \
                typeInfo->AddProperty(prop); \
            }

/// @brief 読み取り専用プロパティを登録する
#define GX_REFLECT_READONLY(PropName, Member, PropType) \
            { \
                gx::PropertyMeta prop; \
                prop.name = PropName; \
                prop.displayName = PropName; \
                prop.type = PropType; \
                prop.offset = GX_OFFSET_OF(_ReflType, Member); \
                prop.size = sizeof(decltype(std::declval<_ReflType>().Member)); \
                prop.readOnly = true; \
                typeInfo->AddProperty(prop); \
            }

/// @brief 継承追跡用の基底型名を設定する
#define GX_REFLECT_BASE(BaseName) \
            typeInfo->SetBaseType(#BaseName);

/// @brief リフレクション登録ブロックを終了する
#define GX_REFLECT_END(Type) \
            gx::TypeRegistry::Instance().RegisterType(std::move(typeInfo)); \
        } \
    }; \
    static Type##_Registrar s_##Type##_registrar; \
    }
/// @}
