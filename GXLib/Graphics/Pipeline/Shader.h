#pragma once
/// @file Shader.h
/// @brief シェーダーコンパイルクラス
///
/// 【初学者向け解説】
/// シェーダーとは、GPU上で動作する小さなプログラムです。
/// グラフィックス描画には最低でも2種類のシェーダーが必要です：
///
/// - 頂点シェーダー（VS）: 3Dの頂点座標を画面上の2D座標に変換する
/// - ピクセルシェーダー（PS）: 各ピクセルの色を決定する
///
/// シェーダーはHLSL（High Level Shading Language）という言語で書き、
/// DXC（DirectX Shader Compiler）でコンパイルしてGPUが理解できる
/// バイトコードに変換します。
///
/// このクラスはHLSLファイルを読み込み、DXCでコンパイルする機能を提供します。

#include "pch.h"

namespace GX
{

/// @brief シェーダーコンパイル結果
struct ShaderBlob
{
    ComPtr<IDxcBlob> blob;
    bool             valid = false;

    /// D3D12_SHADER_BYTECODE構造体として取得（PSO設定用）
    D3D12_SHADER_BYTECODE GetBytecode() const
    {
        if (!valid || !blob) return { nullptr, 0 };
        return { blob->GetBufferPointer(), blob->GetBufferSize() };
    }
};

/// @brief DXCを使ったシェーダーコンパイラ
class Shader
{
public:
    Shader() = default;
    ~Shader() = default;

    /// DXCコンパイラを初期化
    bool Initialize();

    /// HLSLファイルをコンパイル
    ShaderBlob CompileFromFile(const std::wstring& filePath,
                               const std::wstring& entryPoint,
                               const std::wstring& target);

    /// HLSLファイルをコンパイル（#define付きバリアント）
    ShaderBlob CompileFromFile(const std::wstring& filePath,
                               const std::wstring& entryPoint,
                               const std::wstring& target,
                               const std::vector<std::pair<std::wstring, std::wstring>>& defines);

    /// HLSLファイルをシェーダーライブラリとしてコンパイル（DXR用、lib_6_3ターゲット）
    ShaderBlob CompileLibrary(const std::wstring& filePath);

    /// コンパイルエラーメッセージを取得（直前のCompileFromFileが失敗した場合）
    const std::string& GetLastError() const { return m_lastError; }

private:
    std::string m_lastError;
    ComPtr<IDxcCompiler3> m_compiler;
    ComPtr<IDxcUtils>     m_utils;
};

} // namespace GX
