#pragma once
/// @file ShaderLibrary.h
/// @brief コンパイル済みシェーダーの一元管理とPSO再構築コールバック
///
/// DxLibでは内蔵シェーダーが自動管理されるが、DX12では自前でキャッシュする。
/// このクラスはシングルトンで以下を担当する:
/// - コンパイル済みシェーダーのキャッシュ（同じ組み合わせの再コンパイル防止）
/// - #defineバリアントの管理（SKINNED有無等、同一HLSLから異なるPSO用にコンパイル）
/// - ホットリロード連携: ファイル変更時にキャッシュを無効化し、登録済みPSO再構築を実行

#include "pch.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief PSO再構築コールバックを識別するID。UnregisterPSORebuilderで解除に使う
using PSOCallbackID = uint32_t;

/// @brief PSO再構築コールバックの型。デバイスを受け取り、成功でtrue/失敗でfalseを返す
using PSORebuilder = std::function<bool(ID3D12Device*)>;

/// @brief キャッシュ検索用のシェーダー識別キー（ファイル+エントリポイント+ターゲット+マクロ定義）
struct ShaderKey
{
    std::wstring filePath;      ///< HLSLファイルパス
    std::wstring entryPoint;    ///< エントリポイント関数名
    std::wstring target;        ///< ターゲットプロファイル（vs_6_0等）
    std::vector<std::pair<std::wstring, std::wstring>> defines;  ///< #defineマクロ定義

    bool operator==(const ShaderKey& other) const
    {
        return filePath == other.filePath
            && entryPoint == other.entryPoint
            && target == other.target
            && defines == other.defines;
    }
};

/// @brief ShaderKeyをunordered_mapで使うためのハッシュ関数オブジェクト
struct ShaderKeyHasher
{
    /// @brief ShaderKeyのハッシュ値を計算する（boost::hash_combine方式）
    /// @param key ハッシュ対象のキー
    /// @return ハッシュ値
    size_t operator()(const ShaderKey& key) const
    {
        size_t h = std::hash<std::wstring>{}(key.filePath);
        h ^= std::hash<std::wstring>{}(key.entryPoint) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<std::wstring>{}(key.target) + 0x9e3779b9 + (h << 6) + (h >> 2);
        for (auto& [name, value] : key.defines)
        {
            h ^= std::hash<std::wstring>{}(name) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<std::wstring>{}(value) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};

/// @brief コンパイル済みシェーダーのキャッシュとPSO再構築コールバックを管理するシングルトン
class ShaderLibrary
{
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return ShaderLibraryの唯一のインスタンス
    static ShaderLibrary& Instance();

    /// @brief DXCコンパイラを初期化し、デバイスを記憶する
    /// @param device D3D12デバイス（PSO再構築時に使用）
    /// @return 初期化成功ならtrue
    bool Initialize(ID3D12Device* device);

    /// @brief シェーダーを取得する（キャッシュにあればそれを返し、なければコンパイルする）
    /// @param filePath HLSLファイルパス
    /// @param entryPoint エントリポイント関数名
    /// @param target ターゲットプロファイル
    /// @return コンパイル済みシェーダー
    ShaderBlob GetShader(const std::wstring& filePath,
                         const std::wstring& entryPoint,
                         const std::wstring& target);

    /// @brief #define付きシェーダーバリアントを取得する
    /// @param filePath HLSLファイルパス
    /// @param entryPoint エントリポイント関数名
    /// @param target ターゲットプロファイル
    /// @param defines マクロ定義のペア
    /// @return コンパイル済みシェーダー
    ShaderBlob GetShaderVariant(const std::wstring& filePath,
                                const std::wstring& entryPoint,
                                const std::wstring& target,
                                const std::vector<std::pair<std::wstring, std::wstring>>& defines);

    /// @brief シェーダーファイルに対するPSO再構築コールバックを登録する
    /// @param shaderPath 監視対象のHLSLファイルパス
    /// @param callback ファイル変更時に呼ばれる再構築関数
    /// @return 登録ID（解除時に使用）
    PSOCallbackID RegisterPSORebuilder(const std::wstring& shaderPath, PSORebuilder callback);

    /// @brief 登録済みのPSO再構築コールバックを解除する
    /// @param id RegisterPSORebuilderの戻り値
    void UnregisterPSORebuilder(PSOCallbackID id);

    /// @brief 指定ファイルのキャッシュを無効化し、登録済みPSO再構築コールバックを実行する
    /// @param filePath 変更されたHLSLファイルパス
    /// @return 全PSO再構築が成功すればtrue
    bool InvalidateFile(const std::wstring& filePath);

    /// @brief コンパイルエラーが発生しているかどうか
    /// @return エラーがあればtrue
    bool HasError() const { return !m_lastError.empty(); }

    /// @brief 直前のエラーメッセージを取得する
    /// @return エラー文字列
    const std::string& GetLastError() const { return m_lastError; }

    /// @brief エラー状態をクリアする
    void ClearError() { m_lastError.clear(); }

    /// @brief キャッシュとコールバックを全て解放する
    void Shutdown();

private:
    ShaderLibrary() = default;
    ~ShaderLibrary() = default;
    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;

    /// PSO再構築コールバックの登録情報
    struct RebuilderEntry
    {
        PSOCallbackID id;           ///< 登録ID
        std::wstring  shaderPath;   ///< 正規化済みファイルパス
        PSORebuilder  callback;     ///< 再構築関数
    };

    /// @brief ファイルパスを正規化する（バックスラッシュ→スラッシュ、小文字化）
    /// @param path 元のパス
    /// @return 正規化済みパス
    static std::wstring NormalizePath(const std::wstring& path);

    Shader m_compiler;                  ///< DXCコンパイラ
    ID3D12Device* m_device = nullptr;   ///< PSO再構築用のデバイス
    std::unordered_map<ShaderKey, ShaderBlob, ShaderKeyHasher> m_cache;  ///< コンパイル結果キャッシュ
    std::vector<RebuilderEntry> m_rebuilders;   ///< PSO再構築コールバック一覧
    PSOCallbackID m_nextCallbackID = 1;         ///< 次に発行するコールバックID
    std::string m_lastError;                    ///< 直前のエラーメッセージ
    std::mutex m_mutex;                         ///< マルチスレッドアクセス保護
};

} // namespace GX
