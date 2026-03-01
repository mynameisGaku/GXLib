#pragma once
/// @file SaveSystem.h
/// @brief セーブシステム（スロット管理 + Key-Value ストレージ）
///
/// RPGのセーブ画面のような「スロット式」セーブを提供する。
/// SetInt/SetFloat/SetString/SetBool でデータを設定し、
/// SaveToSlot() でファイルに書き出す。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx
{

/// @brief セーブスロット情報
struct SaveSlotInfo
{
    uint32_t    slotIndex = 0;          ///< スロット番号
    std::string label;                  ///< セーブラベル（"Chapter 3" 等）
    std::string dateTime;               ///< 保存日時文字列
    float       playTime    = 0.0f;     ///< プレイ時間（秒）
    bool        exists      = false;    ///< スロットにデータがあるか
};

/// @brief セーブシステム
///
/// Key-Value方式でゲームデータを保持し、スロット単位で
/// ファイルに保存/読み込みする。
class SaveSystem
{
public:
    /// @brief コンストラクタ
    /// @param saveDirectory セーブファイルの保存先ディレクトリ
    explicit SaveSystem(const std::string& saveDirectory = "saves");

    // --- Key-Value アクセス ---

    /// @brief 整数値を設定する
    /// @param key キー名
    /// @param value 設定する整数値
    void SetInt(const std::string& key, int value);

    /// @brief 浮動小数点値を設定する
    /// @param key キー名
    /// @param value 設定する浮動小数点値
    void SetFloat(const std::string& key, float value);

    /// @brief 文字列値を設定する
    /// @param key キー名
    /// @param value 設定する文字列値
    void SetString(const std::string& key, const std::string& value);

    /// @brief 真偽値を設定する
    /// @param key キー名
    /// @param value 設定する真偽値
    void SetBool(const std::string& key, bool value);

    /// @brief 整数値を取得する
    /// @param key キー名
    /// @param defaultValue キーが存在しない場合の既定値
    /// @return キーに対応する整数値（未設定時は defaultValue）
    int         GetInt(const std::string& key, int defaultValue = 0) const;

    /// @brief 浮動小数点値を取得する
    /// @param key キー名
    /// @param defaultValue キーが存在しない場合の既定値
    /// @return キーに対応する浮動小数点値（未設定時は defaultValue）
    float       GetFloat(const std::string& key, float defaultValue = 0.0f) const;

    /// @brief 文字列値を取得する
    /// @param key キー名
    /// @param defaultValue キーが存在しない場合の既定値
    /// @return キーに対応する文字列値（未設定時は defaultValue）
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;

    /// @brief 真偽値を取得する
    /// @param key キー名
    /// @param defaultValue キーが存在しない場合の既定値
    /// @return キーに対応する真偽値（未設定時は defaultValue）
    bool        GetBool(const std::string& key, bool defaultValue = false) const;

    /// @brief キーが存在するか判定する
    /// @param key キー名
    /// @return キーが存在すれば true
    bool HasKey(const std::string& key) const;

    /// @brief キーを削除する
    /// @param key 削除するキー名
    void RemoveKey(const std::string& key);

    /// @brief 全データをクリアする
    void ClearAll();

    // --- スロット操作 ---

    /// @brief 指定スロットに保存する
    /// @param slotIndex スロット番号
    /// @param label セーブラベル
    /// @param playTime プレイ時間（秒）
    /// @return 成功でtrue
    bool SaveToSlot(uint32_t slotIndex, const std::string& label = "", float playTime = 0.0f);

    /// @brief 指定スロットから読み込む
    /// @param slotIndex スロット番号
    /// @return 成功でtrue
    bool LoadFromSlot(uint32_t slotIndex);

    /// @brief 指定スロットを削除する
    /// @param slotIndex スロット番号
    /// @return 成功でtrue
    bool DeleteSlot(uint32_t slotIndex);

    /// @brief スロット情報一覧を取得する
    /// @param maxSlots 取得するスロット数
    /// @return スロット情報配列
    std::vector<SaveSlotInfo> GetSlotList(uint32_t maxSlots = 10) const;

private:
    /// @brief スロットのデータファイルパスを取得する
    /// @param slotIndex スロット番号
    /// @return データファイルパス
    std::string GetSlotPath(uint32_t slotIndex) const;

    /// @brief スロットのメタファイルパスを取得する
    /// @param slotIndex スロット番号
    /// @return メタファイルパス
    std::string GetMetaPath(uint32_t slotIndex) const;

    std::string m_saveDirectory;                             ///< セーブファイル保存先ディレクトリ
    std::unordered_map<std::string, std::string> m_data;     ///< Key-Valueデータ（全て文字列で保持）
};

} // namespace gx
/// @}
