#pragma once
/// @file ShaderLibrary.h
/// @brief シェーダーライブラリ — コンパイル結果キャッシュ + バリアント管理 + PSO再構築登録
///
/// 全シェーダーの一元管理を行います。
/// - コンパイル結果のキャッシュ（同じシェーダーの再コンパイル防止）
/// - #define ベースのバリアント管理
/// - ホットリロード時のPSO再構築コールバック登録

#include "pch.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief PSO再構築コールバックID
using PSOCallbackID = uint32_t;

/// @brief PSO再構築コールバック型（成功でtrue、失敗でfalse）
using PSORebuilder = std::function<bool(ID3D12Device*)>;

/// @brief シェーダーキー（ファイル+エントリ+ターゲット+定義）
struct ShaderKey
{
    std::wstring filePath;
    std::wstring entryPoint;
    std::wstring target;
    std::vector<std::pair<std::wstring, std::wstring>> defines;

    bool operator==(const ShaderKey& other) const
    {
        return filePath == other.filePath
            && entryPoint == other.entryPoint
            && target == other.target
            && defines == other.defines;
    }
};

/// @brief ShaderKeyのハッシュ関数
struct ShaderKeyHasher
{
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

/// @brief シェーダーライブラリ（シングルトン）
class ShaderLibrary
{
public:
    static ShaderLibrary& Instance();

    /// 初期化
    bool Initialize(ID3D12Device* device);

    /// シェーダー取得（キャッシュヒットまたはコンパイル）
    ShaderBlob GetShader(const std::wstring& filePath,
                         const std::wstring& entryPoint,
                         const std::wstring& target);

    /// シェーダーバリアント取得（#define付き）
    ShaderBlob GetShaderVariant(const std::wstring& filePath,
                                const std::wstring& entryPoint,
                                const std::wstring& target,
                                const std::vector<std::pair<std::wstring, std::wstring>>& defines);

    /// PSO再構築コールバック登録（戻り値のIDで解除可能）
    PSOCallbackID RegisterPSORebuilder(const std::wstring& shaderPath, PSORebuilder callback);

    /// PSO再構築コールバック解除
    void UnregisterPSORebuilder(PSOCallbackID id);

    /// 指定ファイルのキャッシュを無効化し、登録済みPSORebuilderを呼び出す
    /// @return 全PSO再構築が成功したかどうか
    bool InvalidateFile(const std::wstring& filePath);

    /// エラー状態
    bool HasError() const { return !m_lastError.empty(); }
    const std::string& GetLastError() const { return m_lastError; }
    void ClearError() { m_lastError.clear(); }

    /// 終了処理
    void Shutdown();

private:
    ShaderLibrary() = default;
    ~ShaderLibrary() = default;
    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;

    /// PSO再構築登録エントリ
    struct RebuilderEntry
    {
        PSOCallbackID id;
        std::wstring  shaderPath;
        PSORebuilder  callback;
    };

    /// ファイルパスの正規化（バックスラッシュ→スラッシュ、小文字化）
    static std::wstring NormalizePath(const std::wstring& path);

    Shader m_compiler;
    ID3D12Device* m_device = nullptr;
    std::unordered_map<ShaderKey, ShaderBlob, ShaderKeyHasher> m_cache;
    std::vector<RebuilderEntry> m_rebuilders;
    PSOCallbackID m_nextCallbackID = 1;
    std::string m_lastError;
    std::mutex m_mutex;
};

} // namespace GX
