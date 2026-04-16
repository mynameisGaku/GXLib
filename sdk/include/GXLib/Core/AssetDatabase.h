#pragma once
/// @file AssetDatabase.h
/// @brief アセットデータベース -- プロジェクトアセットのスキャン、追跡、管理
///
/// ファイルシステムスキャン、拡張子ベースの型分類、
/// パス/名前/型検索、インポート設定、メタデータ永続化をサポート
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include "Core/GUID.h"
#include "Core/AssetMeta.h"
#include <filesystem>

namespace gx
{

/// @brief ファイル拡張子で判定されるアセットの種類
enum class AssetType
{
    Texture,      ///< .png, .jpg, .jpeg, .bmp, .tga, .dds, .hdr
    Model,        ///< .gxmdl, .fbx, .obj, .gltf, .glb, .gxm
    Sound,        ///< .wav, .mp3
    Music,        ///< .ogg
    Font,         ///< .ttf, .otf, .spritefont
    Shader,       ///< .hlsl, .hlsli, .cso
    Script,       ///< .lua
    Scene,        ///< .gxscene
    Material,     ///< .gxmat
    Animation,    ///< .gxanim
    Prefab,       ///< .gxprefab
    Tilemap,      ///< .tmx, .tmj
    Config,       ///< .json, .xml, .ini
    Unknown       ///< 不明な種類
};

/// @brief アセットのインポート設定（キー値ペア）
struct ImportSettings
{
    gx::HashMap<gx::String, gx::String> values; ///< キー値ペアのマップ

    /// @brief インポート設定値を設定する
    /// @param key キー名
    /// @param value 値
    void Set(const gx::String& key, const gx::String& value) { values[key] = value; }

    /// @brief インポート設定値を取得する
    /// @param key キー名
    /// @param defaultVal キーが存在しない場合の既定値
    /// @return キーに対応する値（未設定時は defaultVal）
    gx::String Get(const gx::String& key, const gx::String& defaultVal = "") const
    {
        auto it = values.find(key);
        return it != values.end() ? it->second : defaultVal;
    }

    /// @brief キーが存在するか判定する
    /// @param key キー名
    /// @return キーが存在すれば true
    bool Has(const gx::String& key) const { return values.count(key) > 0; }
};

/// @brief データベース内のアセットエントリ
struct AssetEntry
{
    gx::String name;          ///< パスなしのファイル名
    gx::String relativePath;  ///< ルートからの相対パス（レガシーScan APIでは空）
    gx::String path;          ///< 正規化された完全ファイルパス（レガシーフィールド）
    gx::String fullPath;      ///< 絶対パス（pathのエイリアス）
    gx::String extension;     ///< ファイル拡張子（小文字、ドット付き）
    AssetType   type = AssetType::Unknown; ///< アセットの種類
    uint64_t    fileSize = 0;            ///< ファイルサイズ（バイト）
    uint64_t    lastModified = 0;        ///< 最終書き込み時刻（file_time_typeをティックで保持）
    gx::String hash;                    ///< ハッシュ（オプション、未計算時は空）
    ImportSettings importSettings;       ///< インポート設定
    bool        dirty = false;  ///< 最後のスキャン以降にファイルが変更された場合true
    GUID        guid;           ///< アセットGUID（永続参照用）
};

/// @brief プロジェクトファイルを追跡するシングルトンアセットデータベース
class AssetDatabase
{
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return AssetDatabase のシングルトン参照
    static AssetDatabase& Instance();

    // =====================================================================
    // 新API (Phase 7)
    // =====================================================================

    /// @brief ルートアセットディレクトリでデータベースを初期化しスキャンする
    /// @param rootPath アセットのルートディレクトリパス
    void Initialize(const gx::String& rootPath);

    /// @brief ルートディレクトリの全アセットをスキャンする（再帰的）
    void ScanAll();

    /// @brief 最後のスキャン以降に変更されたファイルを検出する
    /// @return 検出された変更ファイル数
    int DetectChanges();

    /// @brief 相対パスでアセットを検索する
    /// @param relativePath ルートからの相対パス
    /// @return アセットエントリへのポインタ、見つからない場合nullptr
    const AssetEntry* FindAsset(const gx::String& relativePath) const;

    /// @brief 指定した型の全アセットを検索する
    /// @param type 検索するアセットの種類
    /// @return 一致するアセットエントリのポインタ配列
    gx::Vector<const AssetEntry*> FindByType(AssetType type) const;

    /// @brief 名前パターンに一致するアセットを検索する（大文字小文字区別なし部分文字列）
    /// @param pattern 検索する名前パターン
    /// @return 一致するアセットエントリのポインタ配列
    gx::Vector<const AssetEntry*> FindByName(const gx::String& pattern) const;

    /// @brief 全アセットを取得する
    /// @return アセットエントリのマップ（キーはパス）
    const gx::HashMap<gx::String, AssetEntry>& GetAllAssets() const { return m_entries; }

    /// @brief 追跡中のアセット総数を取得する
    /// @return アセット数
    int GetAssetCount() const { return static_cast<int>(m_entries.size()); }

    /// @brief ファイル拡張子からアセット型を検出する
    /// @param extension ファイル拡張子（ドット付き、例: ".png"）
    /// @return 検出されたアセット型
    static AssetType DetectType(const gx::String& extension);

    /// @brief アセットのインポート設定を取得する
    /// @param relativePath ルートからの相対パス
    /// @return インポート設定へのポインタ（見つからない場合 nullptr）
    ImportSettings* GetImportSettings(const gx::String& relativePath);

    /// @brief アセットのインポート設定を設定する
    /// @param relativePath ルートからの相対パス
    /// @param key 設定キー名
    /// @param value 設定値
    void SetImportSetting(const gx::String& relativePath,
                          const gx::String& key, const gx::String& value);

    /// @brief 全メタデータ（インポート設定）をファイルに保存する
    /// @param filePath 保存先ファイルパス
    /// @return 保存に成功した場合 true
    bool SaveMetadata(const gx::String& filePath) const;

    /// @brief ファイルからメタデータを読み込む
    /// @param filePath 読み込むファイルパス
    /// @return 読み込みに成功した場合 true
    bool LoadMetadata(const gx::String& filePath);

    /// @brief ルートパスを取得する
    /// @return ルートディレクトリパス
    const gx::String& GetRootPath() const { return m_lastRootDirectory; }

    /// @brief GUIDでアセットを検索する
    /// @param guid 検索するGUID
    /// @return アセットエントリへのポインタ、見つからない場合nullptr
    const AssetEntry* FindByGUID(const GUID& guid) const;

    /// @brief アセットのGUIDを取得する（.metaになければ自動生成）
    /// @param relativePath ルートからの相対パス
    /// @return アセットのGUID（冪等: 同じパスには同じGUIDを返す）
    GUID GetOrCreateGUID(const gx::String& relativePath);

    /// @brief アセットファイルが移動されたことを通知する
    ///
    /// GUIDは維持したまま、パスマッピングを更新する。
    /// @param oldRelativePath 旧相対パス
    /// @param newRelativePath 新相対パス
    /// @return 移動が成功した場合 true
    bool OnAssetMoved(const gx::String& oldRelativePath, const gx::String& newRelativePath);

    /// @brief GUIDからパスへの逆引きマップを取得する
    /// @return GUID→相対パスのマップ
    const gx::HashMap<GUID, gx::String>& GetGUIDMap() const { return m_guidToPath; }

    /// @brief シャットダウンして全データをクリアする
    void Shutdown();

    // =====================================================================
    // レガシーAPI（後方互換性のために保持）
    // =====================================================================

    /// @brief ディレクトリをスキャンしてアセットを登録する
    /// @param rootDirectory スキャン対象のルートディレクトリ
    /// @param recursive trueの場合サブディレクトリも含める
    /// @return スキャンが正常に完了した場合true
    bool Scan(const gx::String& rootDirectory, bool recursive = true);

    /// @brief 完全パスでアセットを検索する
    /// @param path アセットの完全パス
    /// @return アセットエントリへのポインタ（見つからない場合 nullptr）
    const AssetEntry* Find(const gx::String& path) const;

    /// @brief パス部分文字列でアセットを検索する（大文字小文字区別なし）
    /// @param pattern 検索パターン
    /// @return 一致するアセットエントリのポインタ配列
    gx::Vector<const AssetEntry*> Search(const gx::String& pattern) const;

    /// @brief 登録済みアセット数を取得する
    /// @return アセット数
    size_t GetTotalCount() const { return m_entries.size(); }

    /// @brief 最後のディレクトリを再スキャンする
    /// @return 再スキャンに成功した場合 true
    bool Refresh();

    /// @brief 全エントリをクリアする
    void Clear();

private:
    AssetDatabase() = default;
    ~AssetDatabase() = default;
    AssetDatabase(const AssetDatabase&) = delete;
    AssetDatabase& operator=(const AssetDatabase&) = delete;

    static AssetType ClassifyExtension(const gx::String& ext);
    static gx::String NormalizePath(const gx::String& path);
    static gx::String ToLower(const gx::String& s);

    void ScanDirectory(const std::filesystem::path& dir);

    gx::HashMap<gx::String, AssetEntry> m_entries; ///< アセットエントリマップ（キーはパス）
    gx::HashMap<GUID, gx::String> m_guidToPath;         ///< GUID→相対パス逆引きマップ
    gx::String m_lastRootDirectory;                      ///< 最後にスキャンしたルートディレクトリ
    bool m_lastRecursive = true;                          ///< 最後のスキャンが再帰的だったか
};

} // namespace gx
/// @}
