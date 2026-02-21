#pragma once
/// @file ShaderHotReload.h
/// @brief シェーダーファイルの変更を検知してリアルタイムでPSOを再構築する仕組み
///
/// Shadersディレクトリを FileWatcher で監視し、.hlsl/.hlsliの変更を検知する。
/// 短時間に複数回保存されてもデバウンス（0.3秒待機）で1回にまとめ、
/// ShaderLibraryのキャッシュ無効化 → PSO再構築の順で処理する。
/// コンパイルエラーが起きた場合はエラーメッセージを保持し、
/// 画面上のエラーオーバーレイに表示できるようにしている。

#include "pch.h"
#include "IO/FileWatcher.h"
#include "Graphics/Device/CommandQueue.h"

namespace GX
{

/// @brief シェーダーファイル変更の自動検知とPSO再構築を管理するシングルトン
class ShaderHotReload
{
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return ShaderHotReloadの唯一のインスタンス
    static ShaderHotReload& Instance();

    /// @brief Shadersディレクトリの監視を開始する
    /// @param device D3D12デバイス（PSO再構築時にShaderLibraryが使用）
    /// @param cmdQueue 再構築前にGPU処理完了を待つためのコマンドキュー
    /// @return 初期化成功ならtrue
    bool Initialize(ID3D12Device* device, CommandQueue* cmdQueue);

    /// @brief 毎フレーム呼び出す。デバウンス待機とリロード実行を行う
    /// @param deltaTime 前フレームからの経過時間（秒）
    void Update(float deltaTime);

    /// @brief コンパイルエラーが発生しているかどうか
    /// @return エラーがあればtrue
    bool HasError() const { return !m_errorMessage.empty(); }

    /// @brief 直前のコンパイルエラーメッセージを取得する
    /// @return エラー文字列。エラーがなければ空
    const std::string& GetErrorMessage() const { return m_errorMessage; }

    /// @brief ファイル監視を停止し、リソースを解放する
    void Shutdown();

private:
    ShaderHotReload() = default;
    ~ShaderHotReload() = default;
    ShaderHotReload(const ShaderHotReload&) = delete;
    ShaderHotReload& operator=(const ShaderHotReload&) = delete;

    /// @brief FileWatcherのコールバック。ワーカースレッドから呼ばれるためpendingリストに積むだけ
    /// @param path 変更されたファイルのパス
    void OnShaderFileChanged(const std::string& path);

    /// @brief パスの拡張子が .hlsl または .hlsli かどうか判定する
    /// @param path 判定対象のファイルパス
    /// @return シェーダーファイルならtrue
    static bool IsShaderFile(const std::string& path);

    FileWatcher m_watcher;              ///< Shadersディレクトリの監視
    ID3D12Device* m_device = nullptr;
    CommandQueue* m_cmdQueue = nullptr; ///< GPU完了待ち用

    std::vector<std::wstring> m_pendingChanges; ///< 変更検知されたファイルの待機リスト
    std::mutex m_pendingMutex;                  ///< m_pendingChangesの排他制御

    float m_debounceTimer = 0.0f;   ///< デバウンス残り時間
    bool  m_hasPending = false;     ///< 待機中の変更があるか

    std::string m_errorMessage;     ///< 直前のコンパイルエラー（オーバーレイ表示用）

    /// 連続保存を1回のリロードにまとめるための待機時間（秒）
    static constexpr float k_DebounceDelay = 0.3f;
};

} // namespace GX
