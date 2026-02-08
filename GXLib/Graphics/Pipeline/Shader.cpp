/// @file Shader.cpp
/// @brief シェーダーコンパイルの実装
#include "pch.h"
#include "Graphics/Pipeline/Shader.h"
#include "Core/Logger.h"

namespace GX
{

bool Shader::Initialize()
{
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create DXC Utils (HRESULT: 0x%08X)", hr);
        return false;
    }

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create DXC Compiler (HRESULT: 0x%08X)", hr);
        return false;
    }

    GX_LOG_INFO("DXC Shader Compiler initialized");
    return true;
}

ShaderBlob Shader::CompileFromFile(const std::wstring& filePath,
                                    const std::wstring& entryPoint,
                                    const std::wstring& target)
{
    ShaderBlob result;

    // ファイルを読み込む
    ComPtr<IDxcBlobEncoding> sourceBlob;
    HRESULT hr = m_utils->LoadFile(filePath.c_str(), nullptr, &sourceBlob);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to load shader file (HRESULT: 0x%08X)", hr);
        return result;
    }

    // コンパイル引数
    std::vector<LPCWSTR> arguments;
    arguments.push_back(filePath.c_str());
    arguments.push_back(L"-E");
    arguments.push_back(entryPoint.c_str());
    arguments.push_back(L"-T");
    arguments.push_back(target.c_str());

#ifdef _DEBUG
    arguments.push_back(L"-Zi");
    arguments.push_back(L"-Od");
#else
    arguments.push_back(L"-O3");
#endif

    // ファイルのディレクトリをインクルードパスに追加
    std::wstring dirPath = filePath;
    auto lastSlash = dirPath.find_last_of(L"/\\");
    if (lastSlash != std::wstring::npos)
        dirPath = dirPath.substr(0, lastSlash);
    else
        dirPath = L".";
    arguments.push_back(L"-I");
    arguments.push_back(dirPath.c_str());

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr      = sourceBlob->GetBufferPointer();
    sourceBuffer.Size     = sourceBlob->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_ACP;

    // インクルードハンドラを作成
    ComPtr<IDxcIncludeHandler> includeHandler;
    m_utils->CreateDefaultIncludeHandler(&includeHandler);

    ComPtr<IDxcResult> compileResult;
    hr = m_compiler->Compile(
        &sourceBuffer,
        arguments.data(),
        static_cast<UINT32>(arguments.size()),
        includeHandler.Get(),
        IID_PPV_ARGS(&compileResult)
    );

    // エラー確認
    ComPtr<IDxcBlobUtf8> errors;
    compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors && errors->GetStringLength() > 0)
    {
        GX_LOG_ERROR("Shader compilation errors:\n%s", errors->GetStringPointer());
    }

    HRESULT status;
    compileResult->GetStatus(&status);
    if (FAILED(status))
    {
        GX_LOG_ERROR("Shader compilation failed");
        return result;
    }

    compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&result.blob), nullptr);
    result.valid = true;

    GX_LOG_INFO("Shader compiled successfully: %ls [%ls]", entryPoint.c_str(), target.c_str());
    return result;
}

} // namespace GX
