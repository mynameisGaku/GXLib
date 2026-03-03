#pragma once
/// @file PropertyMeta.h
/// @brief リフレクションシステムのプロパティ記述データ
///
/// 型の各メンバ変数（名前・型・オフセット・範囲など）を
/// 実行時に参照できる形で保持する。エディタの Inspector 欄や
/// JSON シリアライズで、メンバ名を文字列として扱うために使う。
/// @addtogroup grp_reflection/// @{

#include "pch_common.h"

namespace gx
{

/// @brief リフレクションシステムのプロパティ型識別子
enum class PropertyType : uint32_t
{
    Bool,       ///< 真偽値 (bool)
    Int,        ///< 整数 (int)
    Float,      ///< 浮動小数点 (float)
    String,     ///< 文字列 (gx::String)
    Vec3,       ///< 3次元ベクトル (Vector3)
    Vec4,       ///< 4次元ベクトル (Vector4)
    Quaternion, ///< 四元数 (Quaternion)
    Color,      ///< 色 (Color)
    Enum,       ///< 列挙型
    Custom,     ///< ユーザー定義型
};

/// @brief 単一のリフレクションプロパティを記述するメタデータ
struct PropertyMeta
{
    gx::String name;                       ///< 内部名（検索用）
    gx::String displayName;                ///< 表示名（エディタ用）
    PropertyType type = PropertyType::Custom;  ///< プロパティの型識別子
    uint32_t offset = 0;                    ///< オブジェクト先頭からのバイトオフセット
    uint32_t size = 0;                      ///< プロパティのバイトサイズ
    bool readOnly = false;                  ///< 読み取り専用かどうか
    float minValue = 0.0f;                  ///< 最小値（範囲付きfloat/int用）
    float maxValue = 1.0f;                  ///< 最大値（範囲付きfloat/int用）
    bool hasRange = false;                  ///< 最小/最大範囲が有効かどうか

    /// @brief 非トリビアルコピー型（例: gx::String）用のゲッター
    std::function<void(const void* obj, void* outValue)> getter;

    /// @brief 非トリビアルコピー型（例: gx::String）用のセッター
    std::function<void(void* obj, const void* value)> setter;
};

} // namespace gx
/// @}
