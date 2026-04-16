#pragma once
/// @file TypeInfo.h
/// @brief 型のリフレクション情報（プロパティ一覧・名前・ファクトリ）
///
/// 登録した型のプロパティを名前で取得・設定したり、
/// インスタンスを動的生成したりできる。エディタの Inspector や
/// スクリプトバインディングが型情報を得るための窓口。
/// @addtogroup grp_reflection/// @{

#include "pch_common.h"
#include "Core/Reflect/PropertyMeta.h"

namespace gx
{

/// @brief リフレクション対象型の記述: 名前、サイズ、ハッシュ、プロパティ、ファクトリ
class TypeInfo
{
public:
    /// @brief TypeInfo を構築する
    /// @param name 型名（例: "MyStruct"）
    /// @param typeSize sizeof(T) で得られる型のバイトサイズ
    TypeInfo(const gx::String& name, uint32_t typeSize);

    /// @brief 型名を取得する
    /// @return 登録時に指定された型名
    const gx::String& GetName() const { return m_name; }

    /// @brief 型のバイトサイズを取得する
    /// @return sizeof で得られるサイズ
    uint32_t GetTypeSize() const { return m_typeSize; }

    /// @brief 型名から算出された一意のハッシュ値を取得する
    /// @return FNV-1a ハッシュ値
    uint32_t GetTypeHash() const { return m_typeHash; }

    /// @brief この型にプロパティを登録する
    /// @param prop 登録するプロパティのメタデータ
    void AddProperty(const PropertyMeta& prop);

    /// @brief 登録済みの全プロパティを取得する
    /// @return プロパティメタデータの配列への参照
    const gx::Vector<PropertyMeta>& GetProperties() const { return m_properties; }

    /// @brief 名前でプロパティを検索する（見つからない場合nullptrを返す）
    /// @param name 検索するプロパティ名
    /// @return 見つかったプロパティへのポインタ。見つからない場合は nullptr
    const PropertyMeta* FindProperty(const gx::String& name) const;

    /// @brief 登録済みプロパティ数を取得する
    /// @return プロパティの数
    uint32_t GetPropertyCount() const { return static_cast<uint32_t>(m_properties.size()); }

    // --- 名前でプロパティ値を取得 ---

    /// @brief bool プロパティの値を取得する
    /// @param obj 対象オブジェクトへのポインタ
    /// @param propName プロパティ名
    /// @return プロパティの値
    bool GetBool(const void* obj, const gx::String& propName) const;

    /// @brief int プロパティの値を取得する
    /// @param obj 対象オブジェクトへのポインタ
    /// @param propName プロパティ名
    /// @return プロパティの値
    int GetInt(const void* obj, const gx::String& propName) const;

    /// @brief float プロパティの値を取得する
    /// @param obj 対象オブジェクトへのポインタ
    /// @param propName プロパティ名
    /// @return プロパティの値
    float GetFloat(const void* obj, const gx::String& propName) const;

    /// @brief string プロパティの値を取得する
    /// @param obj 対象オブジェクトへのポインタ
    /// @param propName プロパティ名
    /// @return プロパティの値
    gx::String GetString(const void* obj, const gx::String& propName) const;

    // --- 名前でプロパティ値を設定 ---

    /// @brief bool プロパティの値を設定する
    /// @param obj 対象オブジェクトへのポインタ
    /// @param propName プロパティ名
    /// @param value 設定する値
    void SetBool(void* obj, const gx::String& propName, bool value) const;

    /// @brief int プロパティの値を設定する
    /// @param obj 対象オブジェクトへのポインタ
    /// @param propName プロパティ名
    /// @param value 設定する値
    void SetInt(void* obj, const gx::String& propName, int value) const;

    /// @brief float プロパティの値を設定する
    /// @param obj 対象オブジェクトへのポインタ
    /// @param propName プロパティ名
    /// @param value 設定する値
    void SetFloat(void* obj, const gx::String& propName, float value) const;

    /// @brief string プロパティの値を設定する
    /// @param obj 対象オブジェクトへのポインタ
    /// @param propName プロパティ名
    /// @param value 設定する値
    void SetString(void* obj, const gx::String& propName, const gx::String& value) const;

    // --- ファクトリ ---

    /// @brief インスタンス生成用のファクトリ関数型
    using FactoryFn = std::function<void*()>;

    /// @brief ファクトリ関数を設定する
    /// @param fn インスタンスを new して返す関数
    void SetFactory(FactoryFn fn) { m_factory = std::move(fn); }

    /// @brief ファクトリ関数を使ってインスタンスを動的生成する
    /// @return 生成されたオブジェクトへのポインタ。ファクトリ未設定時は nullptr
    void* CreateInstance() const { return m_factory ? m_factory() : nullptr; }

    // --- 基底型 ---

    /// @brief 基底型の名前を設定する（継承チェーンの追跡用）
    /// @param baseName 基底型名
    void SetBaseType(const gx::String& baseName) { m_baseName = baseName; }

    /// @brief 基底型の名前を取得する
    /// @return 基底型名（未設定の場合は空文字列）
    const gx::String& GetBaseTypeName() const { return m_baseName; }

private:
    gx::String m_name;                    ///< 型名
    gx::String m_baseName;                ///< 基底型名（継承追跡用）
    uint32_t m_typeSize = 0;               ///< sizeof で得られるバイトサイズ
    uint32_t m_typeHash = 0;               ///< 型名の FNV-1a ハッシュ値
    gx::Vector<PropertyMeta> m_properties; ///< 登録済みプロパティ一覧
    FactoryFn m_factory;                   ///< インスタンス生成用ファクトリ関数

    /// @brief FNV-1a文字列ハッシュ
    /// @param s ハッシュ対象の文字列
    /// @return 32ビットハッシュ値
    static uint32_t HashString(const gx::String& s);
};

} // namespace gx
/// @}
