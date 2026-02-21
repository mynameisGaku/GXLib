#pragma once
/// @file RTPipeline.h
/// @brief DXR レイトレーシング パイプライン (State Object + Shader Table)
///
/// DxLibには無い機能。DXRのState Object (レイトレ用PSO) とシェーダーテーブルを
/// 管理する。シェーダーパスやエクスポート名をパラメータ化しており、
/// RTReflections用とRTGI用の両方で共通して使える。

#include "pch.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

/// @brief DXR State Object/シェーダーテーブルを管理するレイトレパイプラインクラス
///
/// シェーダーパスやエクスポート名をInitializeの引数で切り替え可能。
/// デフォルトではRTReflections用のシェーダーが設定される。
class RTPipeline
{
public:
    RTPipeline() = default;
    ~RTPipeline() = default;

    /// @brief State Objectとシェーダーテーブルを作成する
    /// @param device DXR対応デバイス (ID3D12Device5)
    /// @param shaderPath HLSLファイルのパス
    /// @param rayGenExport RayGeneration シェーダーのエクスポート名
    /// @param closestHitExport ClosestHit シェーダーのエクスポート名
    /// @param missExport Miss シェーダーのエクスポート名
    /// @param shadowMissExport シャドウレイ用 Miss シェーダーのエクスポート名
    /// @param hitGroupName ヒットグループ名
    /// @return 成功でtrue
    bool Initialize(ID3D12Device5* device,
                    const wchar_t* shaderPath = L"Shaders/RTReflections.hlsl",
                    const wchar_t* rayGenExport = L"RayGen",
                    const wchar_t* closestHitExport = L"ClosestHit",
                    const wchar_t* missExport = L"Miss",
                    const wchar_t* shadowMissExport = L"ShadowMiss",
                    const wchar_t* hitGroupName = L"ReflectionHitGroup");

    /// @brief レイをディスパッチする
    /// @param cmdList DXR対応コマンドリスト
    /// @param width ディスパッチ幅 (ピクセル)
    /// @param height ディスパッチ高さ (ピクセル)
    void DispatchRays(ID3D12GraphicsCommandList4* cmdList, uint32_t width, uint32_t height);

    /// @brief グローバルルートシグネチャを取得
    ID3D12RootSignature* GetGlobalRootSignature() const { return m_globalRS.Get(); }

private:
    bool CreateGlobalRootSignature(ID3D12Device5* device);
    bool CreateLocalRootSignatures(ID3D12Device5* device);
    bool CreateStateObject(ID3D12Device5* device);
    bool CreateShaderTable(ID3D12Device5* device);

    ID3D12Device5* m_device = nullptr;

    // シェーダーパス / エクスポート名 (Initialize で設定)
    std::wstring m_shaderPath;
    std::wstring m_rayGenExport;
    std::wstring m_closestHitExport;
    std::wstring m_missExport;
    std::wstring m_shadowMissExport;
    std::wstring m_hitGroupName;

    // シェーダー
    Shader m_shader;

    // Global Root Signature
    ComPtr<ID3D12RootSignature> m_globalRS;

    // Local Root Signatures (空 — DXR spec推奨の明示的association用)
    ComPtr<ID3D12RootSignature> m_rayGenLocalRS;
    ComPtr<ID3D12RootSignature> m_hitMissLocalRS;

    // State Object
    ComPtr<ID3D12StateObject> m_stateObject;
    ComPtr<ID3D12StateObjectProperties> m_stateObjectProperties;

    // Shader Table
    Buffer m_rayGenShaderTable;
    Buffer m_missShaderTable;
    Buffer m_hitGroupShaderTable;

    uint32_t m_rayGenRecordSize   = 0;
    uint32_t m_missRecordSize     = 0;
    uint32_t m_hitGroupRecordSize = 0;
};

} // namespace GX
