#pragma once
/// @file IndirectCommandBuffer.h
/// @brief GPU間接描画用コマンドバッファ
///
/// ExecuteIndirect を使用して GPU 駆動の描画を実現する。
/// CPU 側でコマンド引数バッファを構築し、GPU が一括で描画を実行する。
/// フラスタムカリング後のシーン描画等で DrawCall オーバーヘッドを削減できる。

#include "pch.h"

namespace GX
{

/// @brief 間接描画コマンドの種類
enum class IndirectCommandType
{
    Draw,           ///< 非インデックス描画 (DrawInstanced)
    DrawIndexed,    ///< インデックス描画 (DrawIndexedInstanced)
};

/// @brief GPU 間接描画用コマンドバッファ
///
/// D3D12_DRAW_INDEXED_ARGUMENTS のバッファを管理し、
/// ExecuteIndirect で GPU に一括描画させる。
class IndirectCommandBuffer
{
public:
    IndirectCommandBuffer() = default;
    ~IndirectCommandBuffer() = default;

    /// @brief 間接コマンドバッファを初期化する
    /// @param device D3D12デバイス
    /// @param maxCommands バッファに格納可能な最大コマンド数
    /// @param type コマンドの種類（Draw or DrawIndexed）
    /// @return 成功なら true
    bool Initialize(ID3D12Device* device, uint32_t maxCommands,
                    IndirectCommandType type = IndirectCommandType::DrawIndexed);

    /// @brief フレーム開始時にコマンドバッファをリセットする
    void Reset();

    /// @brief インデックス付き描画コマンドを追加する
    /// @param indexCount インデックス数
    /// @param instanceCount インスタンス数
    /// @param startIndex 開始インデックス位置
    /// @param baseVertex 頂点オフセット
    /// @param startInstance インスタンスオフセット
    void AddDrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                        uint32_t startIndex, int32_t baseVertex,
                        uint32_t startInstance);

    /// @brief 非インデックス描画コマンドを追加する
    /// @param vertexCount 頂点数
    /// @param instanceCount インスタンス数
    /// @param startVertex 開始頂点位置
    /// @param startInstance インスタンスオフセット
    void AddDraw(uint32_t vertexCount, uint32_t instanceCount,
                 uint32_t startVertex, uint32_t startInstance);

    /// @brief 蓄積した全コマンドを一括実行する
    /// @param cmdList コマンドリスト
    void Execute(ID3D12GraphicsCommandList* cmdList);

    /// @brief 現在蓄積されているコマンド数を取得する
    /// @return コマンド数
    uint32_t GetCommandCount() const { return m_commandCount; }

    /// @brief 最大コマンド数を取得する
    /// @return 最大コマンド数
    uint32_t GetMaxCommands() const { return m_maxCommands; }

    /// @brief コマンドシグネチャを取得する
    /// @return ID3D12CommandSignatureへのポインタ
    ID3D12CommandSignature* GetCommandSignature() const { return m_commandSignature.Get(); }

private:
    ComPtr<ID3D12CommandSignature> m_commandSignature;
    ComPtr<ID3D12Resource>         m_argumentBuffer;
    void*                          m_mappedData = nullptr;

    IndirectCommandType m_type = IndirectCommandType::DrawIndexed;
    uint32_t m_maxCommands = 0;
    uint32_t m_commandCount = 0;
    uint32_t m_stride = 0;
};

} // namespace GX
