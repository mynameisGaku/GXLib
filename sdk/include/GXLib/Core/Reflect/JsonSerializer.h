#pragma once
/// @file JsonSerializer.h
/// @brief リフレクション情報を使った軽量 JSON シリアライザ
///
/// 外部ライブラリ不要の手書きパーサーで、TypeInfo に登録済みの
/// プロパティを JSON 文字列へ変換したり、JSON から復元したりする。
/// セーブデータや設定ファイルの読み書きに使える。
/// @addtogroup grp_reflection/// @{

#include "pch_common.h"
#include "Core/Reflect/TypeInfo.h"

namespace gx
{

/// @brief リフレクション対象オブジェクトのJSON文字列へのシリアライズ/デシリアライズ
///
/// 外部JSONライブラリに依存しない軽量シリアライザ。
/// Bool、Int、Float、String、Vec3、Vec4、Quaternion、Colorプロパティ型を処理する。
class JsonSerializer
{
public:
    /// @brief オブジェクトの全プロパティをJSON文字列にシリアライズする
    /// @param type 対象オブジェクトの型情報
    /// @param obj シリアライズ対象オブジェクトへのポインタ
    /// @return JSON文字列
    static gx::String Serialize(const TypeInfo& type, const void* obj);

    /// @brief JSON文字列をオブジェクトのプロパティにデシリアライズする
    /// @param type 対象オブジェクトの型情報
    /// @param obj デシリアライズ先オブジェクトへのポインタ
    /// @param json JSON文字列
    /// @return 成功時 true
    static bool Deserialize(const TypeInfo& type, void* obj, const gx::String& json);

    /// @brief 単一プロパティをJSONのkey:valueフラグメントにシリアライズする
    /// @param prop シリアライズ対象のプロパティメタデータ
    /// @param obj プロパティを保持するオブジェクトへのポインタ
    /// @return JSON フラグメント文字列（例: "\"speed\": 1.5"）
    static gx::String SerializeProperty(const PropertyMeta& prop, const void* obj);

    /// @brief 生の文字列値を単一プロパティにデシリアライズする
    /// @param prop デシリアライズ先のプロパティメタデータ
    /// @param obj プロパティを保持するオブジェクトへのポインタ
    /// @param value デシリアライズする値の文字列
    /// @return 成功時 true
    static bool DeserializeProperty(const PropertyMeta& prop, void* obj, const gx::String& value);
};

} // namespace gx
/// @}
