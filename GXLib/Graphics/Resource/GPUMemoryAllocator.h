#pragma once
/// @file GPUMemoryAllocator.h
/// @brief GPUメモリアロケータ -- ヒープ管理と一時リングバッファ
///
/// 2種類のアロケーションを提供する。ひとつは D3D12 ヒープから長寿命
/// リソース（テクスチャ・バッファ）を確保する Allocate()、
/// もうひとつは 1 フレーム限りのアップロード用に
/// リングバッファから高速に確保する AllocateTransient()。
/// @addtogroup grp_gfx_resource/// @{

#include "pch_graphics.h"

namespace gx
{

/// @brief GPUメモリ使用状況の統計
struct GPUMemoryStats
{
    uint64_t totalAllocated = 0;    ///< 現在確保済みの合計バイト数
    uint64_t peakAllocated = 0;     ///< 確保済みバイト数のピーク値
    uint64_t totalHeapSize = 0;     ///< ヒープ容量の合計（バイト）
    uint32_t allocationCount = 0;   ///< アクティブなアロケーション数
    uint32_t heapCount = 0;         ///< ヒープ数
};

/// @brief GPUメモリアロケーション結果
struct GPUAllocation
{
    ID3D12Resource* resource = nullptr; ///< 配置リソース（またはコミット済み）
    uint64_t offset = 0;                ///< ヒープ内のオフセット
    uint64_t size = 0;                  ///< アロケーションサイズ（バイト）
    uint32_t heapIndex = 0;             ///< 所属するヒープのインデックス
    bool valid = false;                 ///< アロケーション成功時true
};

/// @brief リングバッファからの一時アロケーション（アップロードヒープ、1フレーム寿命）
struct TransientAllocation
{
    void* cpuAddress = nullptr;         ///< 書き込み用CPUマップドポインタ
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0; ///< GPU仮想アドレス
    uint64_t size = 0;
    bool valid = false;
};

/// @brief ヒープベースアロケーションと一時リングバッファを持つGPUメモリアロケータ
class GPUMemoryAllocator
{
public:
    GPUMemoryAllocator() = default;
    ~GPUMemoryAllocator() = default;

    /// @brief アロケータを初期化する
    bool Initialize(ID3D12Device* device, uint64_t defaultHeapSize = 64 * 1024 * 1024,
                     uint64_t ringBufferSize = 16 * 1024 * 1024);

    /// @brief デフォルトヒープからGPUメモリを確保する
    GPUAllocation Allocate(uint64_t size, uint64_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    /// @brief 以前のアロケーションを解放する
    void Free(const GPUAllocation& alloc);

    /// @brief リングバッファから一時メモリを確保する（1フレーム寿命）
    TransientAllocation AllocateTransient(uint64_t size, uint64_t alignment = 256);

    /// @brief 新しいフレームのために一時リングバッファをリセットする
    void ResetTransientForFrame(uint64_t frameIndex);

    /// @brief 現在のメモリ統計を取得する
    GPUMemoryStats GetStats() const;

    /// @brief シャットダウンして全ヒープを解放する
    void Shutdown();

private:
    struct FreeRange
    {
        uint64_t offset = 0;
        uint64_t size = 0;
    };

    struct HeapBlock
    {
        ComPtr<ID3D12Heap> heap;
        uint64_t totalSize = 0;
        std::vector<FreeRange> freeRanges;
    };

    bool CreateHeap(uint64_t size);

    ID3D12Device* m_device = nullptr;                  ///< D3D12デバイス
    uint64_t m_defaultHeapSize = 64 * 1024 * 1024;     ///< デフォルトヒープサイズ（バイト）

    std::vector<HeapBlock> m_heaps;                     ///< ヒープブロックリスト

    // 一時リングバッファ
    ComPtr<ID3D12Resource> m_ringBuffer;                ///< リングバッファリソース（UPLOADヒープ）
    void* m_ringBufferCPU = nullptr;                    ///< CPUマップドポインタ
    D3D12_GPU_VIRTUAL_ADDRESS m_ringBufferGPU = 0;      ///< GPU仮想アドレス
    uint64_t m_ringBufferSize = 0;                      ///< リングバッファサイズ（バイト）
    uint64_t m_ringBufferHead = 0;                      ///< 現在の書き込みヘッド位置
    uint64_t m_ringBufferFrameFences[3] = {};            ///< フレーム毎のウォーターマーク

    GPUMemoryStats m_stats;                              ///< メモリ統計
};

} // namespace gx
/// @}
