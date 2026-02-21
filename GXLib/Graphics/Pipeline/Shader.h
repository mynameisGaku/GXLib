#pragma once
/// @file Shader.h
/// @brief DXCコンパイラによるHLSLシェーダーのコンパイル
///
/// DxLibでは内蔵シェーダーが自動適用されるが、DX12では自分でHLSLを
/// コンパイルしてバイトコードを取得する必要がある。
///
/// このクラスはDXC（DirectX Shader Compiler）のラッパーで、
/// HLSLファイルを読み込み、頂点シェーダー(vs_6_0)やピクセルシェーダー(ps_6_0)、
/// DXRライブラリ(lib_6_3)にコンパイルする。
/// コンパイル結果のShaderBlobをPipelineStateBuilderに渡してPSOを構築する。

#include "pch.h"

namespace GX
{

/// @brief シェーダーコンパイル結果を保持する構造体
struct ShaderBlob
{
    ComPtr<IDxcBlob> blob;      ///< コンパイル済みバイトコード
    bool             valid = false;  ///< コンパイル成功ならtrue

    /// @brief PSOに設定するためのD3D12_SHADER_BYTECODE形式で取得する
    /// @return バイトコード構造体。無効な場合は空のバイトコードを返す
    D3D12_SHADER_BYTECODE GetBytecode() const
    {
        if (!valid || !blob) return { nullptr, 0 };
        return { blob->GetBufferPointer(), blob->GetBufferSize() };
    }
};

/// @brief HLSLファイルをDXCでコンパイルするシェーダーコンパイラ
class Shader
{
public:
    Shader() = default;
    ~Shader() = default;

    /// @brief DXCコンパイラとユーティリティを初期化する
    /// @return 成功時true。dxcompiler.dllが見つからない場合等はfalse
    bool Initialize();

    /// @brief HLSLファイルを指定エントリポイント・ターゲットでコンパイルする
    /// @param filePath HLSLファイルのパス
    /// @param entryPoint エントリポイント関数名（例: L"VSMain", L"PSMain"）
    /// @param target ターゲットプロファイル（例: L"vs_6_0", L"ps_6_0"）
    /// @return コンパイル結果。失敗時はvalid==false
    ShaderBlob CompileFromFile(const std::wstring& filePath,
                               const std::wstring& entryPoint,
                               const std::wstring& target);

    /// @brief HLSLファイルを#defineマクロ付きでコンパイルする（バリアント生成用）
    /// @param filePath HLSLファイルのパス
    /// @param entryPoint エントリポイント関数名
    /// @param target ターゲットプロファイル
    /// @param defines マクロ定義のペア（名前, 値）。値が空なら値なし定義
    /// @return コンパイル結果
    ShaderBlob CompileFromFile(const std::wstring& filePath,
                               const std::wstring& entryPoint,
                               const std::wstring& target,
                               const std::vector<std::pair<std::wstring, std::wstring>>& defines);

    /// @brief HLSLファイルをDXRシェーダーライブラリとしてコンパイルする（lib_6_3ターゲット）
    /// @param filePath HLSLファイルのパス
    /// @return コンパイル結果。エントリポイント指定なし（ライブラリ内の全関数がエクスポート対象）
    ShaderBlob CompileLibrary(const std::wstring& filePath);

    /// @brief 直前のコンパイルで発生したエラーメッセージを取得する
    /// @return エラー文字列。エラーがなければ空文字列
    const std::string& GetLastError() const { return m_lastError; }

private:
    std::string m_lastError;            ///< 直前のコンパイルエラー
    ComPtr<IDxcCompiler3> m_compiler;   ///< DXCコンパイラ本体
    ComPtr<IDxcUtils>     m_utils;      ///< ファイル読み込み等のユーティリティ
};

} // namespace GX
