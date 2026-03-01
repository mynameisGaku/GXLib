#pragma once
/// @file SettingsManager.h
/// @brief 設定マネージャー（セクション＋Key-Valueの永続化設定）
///
/// ゲームの設定（画面解像度、音量、キーコンフィグ等）を
/// セクション付きKey-Valueで管理し、JSONファイルに保存/読み込みする。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx
{

/// @brief 設定マネージャー（シングルトン）
class SettingsManager
{
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return SettingsManager のシングルトン参照
    static SettingsManager& Instance();

    // --- 値の設定 ---

    /// @brief 整数値を設定する
    /// @param section セクション名
    /// @param key キー名
    /// @param value 設定する整数値
    void SetInt(const std::string& section, const std::string& key, int value);

    /// @brief 浮動小数点値を設定する
    /// @param section セクション名
    /// @param key キー名
    /// @param value 設定する浮動小数点値
    void SetFloat(const std::string& section, const std::string& key, float value);

    /// @brief 文字列値を設定する
    /// @param section セクション名
    /// @param key キー名
    /// @param value 設定する文字列値
    void SetString(const std::string& section, const std::string& key, const std::string& value);

    /// @brief 真偽値を設定する
    /// @param section セクション名
    /// @param key キー名
    /// @param value 設定する真偽値
    void SetBool(const std::string& section, const std::string& key, bool value);

    // --- 値の取得 ---

    /// @brief 整数値を取得する
    /// @param section セクション名
    /// @param key キー名
    /// @param defaultValue キーが存在しない場合の既定値
    /// @return キーに対応する整数値（未設定時は defaultValue）
    int         GetInt(const std::string& section, const std::string& key, int defaultValue = 0) const;

    /// @brief 浮動小数点値を取得する
    /// @param section セクション名
    /// @param key キー名
    /// @param defaultValue キーが存在しない場合の既定値
    /// @return キーに対応する浮動小数点値（未設定時は defaultValue）
    float       GetFloat(const std::string& section, const std::string& key, float defaultValue = 0.0f) const;

    /// @brief 文字列値を取得する
    /// @param section セクション名
    /// @param key キー名
    /// @param defaultValue キーが存在しない場合の既定値
    /// @return キーに対応する文字列値（未設定時は defaultValue）
    std::string GetString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;

    /// @brief 真偽値を取得する
    /// @param section セクション名
    /// @param key キー名
    /// @param defaultValue キーが存在しない場合の既定値
    /// @return キーに対応する真偽値（未設定時は defaultValue）
    bool        GetBool(const std::string& section, const std::string& key, bool defaultValue = false) const;

    /// @brief ファイルに保存する
    /// @param filePath ファイルパス
    /// @return 成功でtrue
    bool SaveToFile(const std::string& filePath = "settings.json");

    /// @brief ファイルから読み込む
    /// @param filePath ファイルパス
    /// @return 成功でtrue
    bool LoadFromFile(const std::string& filePath = "settings.json");

    /// @brief 全設定をクリアする
    void Clear();

private:
    SettingsManager() = default;

    /// @brief 内部ストアから値を取得する
    /// @param section セクション名
    /// @param key キー名
    /// @return 値文字列（未設定時は空文字列）
    std::string GetValue(const std::string& section, const std::string& key) const;

    /// @brief 内部ストアに値を設定する
    /// @param section セクション名
    /// @param key キー名
    /// @param value 値文字列
    void SetValue(const std::string& section, const std::string& key, const std::string& value);

    /// section -> (key -> value) の二重マップ
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_data; ///< 設定データ
};

} // namespace gx
/// @}
