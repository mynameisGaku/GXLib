#pragma once
/// @file AssetReloader.h
/// @brief アセットホットリロード拡張（テクスチャ・マテリアル・スクリプトのライブリロード）
///
/// HotReloadManagerと連携して、ファイル変更検知時に
/// 対応するリソースを自動的に再読み込みする。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx
{

class HotReloadManager;
class TextureManager;
class AssetRemapper;

/// @brief リロード対象アセットの種類
enum class ReloadableAssetType
{
    Texture,     ///< テクスチャ(.png, .jpg, .dds, .bmp, .tga)
    Material,    ///< マテリアルパラメータ(.mat, .json)
    Script,      ///< Luaスクリプト(.lua)
    StyleSheet,  ///< GUIスタイルシート(.css, .style)
    Config,      ///< コンフィグ(.ini, .cfg, .json)
    Audio,       ///< 音声(.wav, .ogg)
    Shader,      ///< シェーダー(.hlsl, .hlsli)
};

/// @brief リロードイベント情報
struct ReloadEvent
{
    ReloadableAssetType type;       ///< リロード対象のアセット種別
    gx::WString filePath;          ///< リロード対象のファイルパス
    bool success = false;           ///< リロードの成否
    gx::String errorMessage;       ///< エラーメッセージ（失敗時）
};

/// @brief リロードイベントコールバック
using ReloadEventCallback = std::function<void(const ReloadEvent&)>;

/// @brief アセットリローダー
class AssetReloader
{
public:
    AssetReloader() = default;
    ~AssetReloader() = default;

    /// @brief HotReloadManager と連携して初期化する
    /// @param hotReloadMgr ホットリロードマネージャー（外部所有）
    /// @return 成功で true
    bool Initialize(HotReloadManager* hotReloadMgr);

    /// @brief シャットダウンして登録を全解除する
    void Shutdown();

    /// @brief テクスチャマネージャーを登録する（テクスチャリロード用）
    /// @param texMgr テクスチャマネージャー（外部所有）
    void SetTextureManager(TextureManager* texMgr) { m_textureManager = texMgr; }

    /// @brief AssetRemapper を設定する（GUID ベースホットリロード用）
    /// @param remapper AssetRemapper（外部所有）
    void SetRemapper(AssetRemapper* remapper) { m_remapper = remapper; }

    /// @brief テクスチャのファイルパスとハンドルを関連付けて登録する
    /// @param filePath テクスチャファイルパス
    /// @param textureHandle テクスチャハンドル
    void RegisterTexture(const gx::WString& filePath, int textureHandle);

    /// @brief マテリアルのファイルパスと名前を関連付けて登録する
    /// @param filePath マテリアルファイルパス
    /// @param materialName マテリアル名
    void RegisterMaterial(const gx::WString& filePath, const gx::String& materialName);

    /// @brief スクリプトのファイルパスと名前を関連付けて登録する
    /// @param filePath スクリプトファイルパス
    /// @param scriptName スクリプト名
    void RegisterScript(const gx::WString& filePath, const gx::String& scriptName);

    /// @brief リロードイベントコールバックを設定する
    /// @param callback リロード完了時に呼ばれるコールバック
    void SetOnReload(ReloadEventCallback callback) { m_onReload = std::move(callback); }

    /// @brief テクスチャを手動リロードする
    /// @param filePath テクスチャファイルパス
    /// @return リロード成功で true
    bool ReloadTexture(const gx::WString& filePath);

    /// @brief マテリアルを手動リロードする
    /// @param filePath マテリアルファイルパス
    /// @return リロード成功で true
    bool ReloadMaterial(const gx::WString& filePath);

    /// @brief スクリプトを手動リロードする
    /// @param filePath スクリプトファイルパス
    /// @return リロード成功で true
    bool ReloadScript(const gx::WString& filePath);

    /// @brief 登録済みテクスチャ数を取得する
    /// @return テクスチャ登録数
    uint32_t GetRegisteredTextureCount() const { return static_cast<uint32_t>(m_textureMap.size()); }

    /// @brief 登録済みマテリアル数を取得する
    /// @return マテリアル登録数
    uint32_t GetRegisteredMaterialCount() const { return static_cast<uint32_t>(m_materialMap.size()); }

    /// @brief 登録済みスクリプト数を取得する
    /// @return スクリプト登録数
    uint32_t GetRegisteredScriptCount() const { return static_cast<uint32_t>(m_scriptMap.size()); }

    /// @brief リロード成功回数を取得する
    /// @return 成功回数
    uint32_t GetReloadSuccessCount() const { return m_successCount; }

    /// @brief リロード失敗回数を取得する
    /// @return 失敗回数
    uint32_t GetReloadFailureCount() const { return m_failureCount; }

    /// @brief 自動リロードの有効/無効を設定する
    /// @param enabled true で有効
    void SetAutoReloadEnabled(bool enabled) { m_autoReload = enabled; }

    /// @brief 自動リロードが有効かどうかを取得する
    /// @return 有効なら true
    bool IsAutoReloadEnabled() const { return m_autoReload; }

private:
    /// @brief ファイル変更通知ハンドラ
    /// @param filePath 変更されたファイルパス
    void OnFileChanged(const gx::WString& filePath);

    /// @brief ファイルパスからリロード対象アセット種別を判定する
    /// @param path ファイルパス
    /// @return 判定されたアセット種別
    ReloadableAssetType ClassifyAsset(const gx::WString& path) const;

    HotReloadManager* m_hotReloadMgr = nullptr;   ///< ホットリロードマネージャー（非所有）
    TextureManager* m_textureManager = nullptr;    ///< テクスチャマネージャー（非所有）
    AssetRemapper* m_remapper = nullptr;           ///< AssetRemapper（非所有）

    gx::HashMap<gx::WString, int> m_textureMap;          ///< テクスチャパス→ハンドルマップ
    gx::HashMap<gx::WString, gx::String> m_materialMap; ///< マテリアルパス→名前マップ
    gx::HashMap<gx::WString, gx::String> m_scriptMap;   ///< スクリプトパス→名前マップ

    ReloadEventCallback m_onReload;   ///< リロードイベントコールバック
    uint32_t m_successCount = 0;      ///< リロード成功回数
    uint32_t m_failureCount = 0;      ///< リロード失敗回数
    bool m_autoReload = true;         ///< 自動リロード有効フラグ
    bool m_initialized = false;       ///< 初期化済みフラグ
};

} // namespace gx
/// @}
