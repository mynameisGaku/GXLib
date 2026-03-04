#pragma once
/// @file ShaderVariant.h
/// @brief シェーダーバリアント管理システム
///
/// #define マクロの組み合わせでシェーダーバリアントを自動生成・キャッシュする。
/// マテリアル設定から必要なバリアントを自動選択し、PSO を返す。

#include "pch_graphics.h"
#include "Graphics/Pipeline/Shader.h"
#include "Container/ContainerFwd.h"

namespace gx
{
/// @addtogroup grp_gfx_pipeline
/// @{

/// @brief シェーダーキーワード定義
struct ShaderKeyword
{
    gx::String name;              ///< キーワード名 (例: "ENABLE_NORMAL_MAP")
    gx::Vector<gx::String> values; ///< 取り得る値 (空=on/off、値あり=multi-compile)
    bool isMultiCompile = false;   ///< true=multi_compile (全組み合わせ), false=shader_feature (使用時のみ)
};

/// @brief バリアント選択用キーセット
class ShaderVariantKey
{
public:
    /// @brief キーワードをboolean有効/無効で設定する
    /// @param name キーワード名
    /// @param enabled trueなら有効化、falseなら無効化(削除)
    void SetKeyword(const gx::String& name, bool enabled = true);

    /// @brief キーワードを特定の値で設定する
    /// @param name キーワード名
    /// @param value キーワードの値
    void SetKeyword(const gx::String& name, const gx::String& value);

    /// @brief キーワードを削除する
    /// @param name キーワード名
    void ClearKeyword(const gx::String& name);

    /// @brief キーワードが有効か確認する
    /// @param name キーワード名
    /// @return 有効ならtrue
    bool HasKeyword(const gx::String& name) const;

    /// @brief キーワードの値を取得する
    /// @param name キーワード名
    /// @return キーワードの値（未設定なら空文字列）
    gx::String GetKeywordValue(const gx::String& name) const;

    /// @brief 全有効キーワードをソートしたハッシュキーに変換
    /// @return FNV-1aハッシュ値
    uint64_t ComputeHash() const;

    /// @brief #define 文字列リストを生成
    /// @return "NAME=VALUE" 形式の文字列リスト
    gx::Vector<gx::String> ToDefines() const;

    /// @brief 等価比較
    bool operator==(const ShaderVariantKey& other) const;

    /// @brief 設定されているキーワード数を返す
    /// @return キーワード数
    uint32_t GetKeywordCount() const { return static_cast<uint32_t>(m_keywords.size()); }

private:
    gx::HashMap<gx::String, gx::String> m_keywords; ///< name -> value ("1" for bool)
};

/// @brief シェーダーバリアントコレクション
///
/// 一つのHLSLファイルに対して、キーワードの組み合わせから
/// 複数のPSOバリアントをプリコンパイル or オンデマンドコンパイルする。
class ShaderVariantCollection
{
public:
    ShaderVariantCollection() = default;

    /// @brief 初期化
    /// @param shaderPath HLSLファイルパス
    /// @param vsEntry VS エントリーポイント
    /// @param psEntry PS エントリーポイント
    void Initialize(const gx::String& shaderPath,
                    const gx::String& vsEntry = "VSMain",
                    const gx::String& psEntry = "PSMain");

    /// @brief キーワード定義を追加
    /// @param keyword 追加するキーワード
    void AddKeyword(const ShaderKeyword& keyword);

    /// @brief 全multi_compileバリアントをプリコンパイル
    /// @param device D3D12デバイス
    /// @param rootSignature ルートシグネチャ
    /// @param rtFormat レンダーターゲットフォーマット
    /// @return コンパイル成功数
    int PrecompileAll(ID3D12Device* device, ID3D12RootSignature* rootSignature,
                      DXGI_FORMAT rtFormat = DXGI_FORMAT_R16G16B16A16_FLOAT);

    /// @brief キーで指定したバリアントのPSOを取得（未コンパイルならオンデマンド）
    /// @param key バリアントキー
    /// @param device D3D12デバイス
    /// @param rootSignature ルートシグネチャ
    /// @param rtFormat レンダーターゲットフォーマット
    /// @return PSOポインタ（コンパイル失敗ならnullptr）
    ID3D12PipelineState* GetVariant(const ShaderVariantKey& key,
                                     ID3D12Device* device,
                                     ID3D12RootSignature* rootSignature,
                                     DXGI_FORMAT rtFormat = DXGI_FORMAT_R16G16B16A16_FLOAT);

    /// @brief コンパイル済みバリアント数
    uint32_t GetCompiledCount() const { return static_cast<uint32_t>(m_variants.size()); }

    /// @brief 全バリアントを破棄
    void Clear() { m_variants.clear(); }

    /// @brief バリアントキーに対応するdefines文字列を取得（デバッグ用）
    /// @param key バリアントキー
    /// @return デバッグ用のdefines文字列
    gx::String GetVariantDebugString(const ShaderVariantKey& key) const;

    /// @brief 登録済みキーワード数を取得する
    /// @return キーワード数
    uint32_t GetKeywordCount() const { return static_cast<uint32_t>(m_keywords.size()); }

    /// @brief シェーダーパスを取得する
    /// @return シェーダーファイルパス
    const gx::String& GetShaderPath() const { return m_shaderPath; }

    /// @brief 全multi_compileバリアントの組み合わせ数を計算する
    /// @return 組み合わせ数
    uint32_t GetTotalMultiCompileVariantCount() const;

private:
    /// @brief コンパイル済みバリアント情報
    struct CompiledVariant
    {
        ComPtr<ID3D12PipelineState> pso;
        ShaderVariantKey key;
    };

    /// @brief バリアントをコンパイルしてキャッシュに格納する
    /// @param key バリアントキー
    /// @param device D3D12デバイス
    /// @param rootSignature ルートシグネチャ
    /// @param rtFormat レンダーターゲットフォーマット
    /// @return コンパイル成功ならtrue
    bool CompileVariant(const ShaderVariantKey& key, ID3D12Device* device,
                        ID3D12RootSignature* rootSignature, DXGI_FORMAT rtFormat);

    /// @brief multi_compileキーワードの全組み合わせを列挙する
    /// @param outKeys 出力先のキーリスト
    void EnumerateMultiCompileVariants(gx::Vector<ShaderVariantKey>& outKeys) const;

    gx::String m_shaderPath;                              ///< HLSLファイルパス
    gx::String m_vsEntry;                                  ///< VSエントリーポイント
    gx::String m_psEntry;                                  ///< PSエントリーポイント
    gx::Vector<ShaderKeyword> m_keywords;                  ///< 登録済みキーワード
    gx::HashMap<uint64_t, CompiledVariant> m_variants;     ///< コンパイル済みバリアントキャッシュ
    Shader m_shader;                                        ///< DXCコンパイラ
};

/// @}
} // namespace gx
