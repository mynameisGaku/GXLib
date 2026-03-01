#include "pch_graphics.h"
/// @file FrameGraph.cpp
/// @brief フレームグラフ実装 -- トポロジカルソート、バリア生成、パス実行

#include "Graphics/FrameGraph/FrameGraph.h"
#include "Core/Logger.h"
#include <queue>
#include <unordered_set>

namespace gx
{

const std::vector<BarrierEntry> FrameGraph::s_emptyBarriers;

// ============================================================================
// リソース宣言
// ============================================================================

uint32_t FrameGraph::DeclareResource(const std::string& name, DXGI_FORMAT format,
                                      uint32_t width, uint32_t height,
                                      D3D12_RESOURCE_STATES initialState)
{
    ResourceNode node;
    node.name = name;
    node.type = FGResourceType::Texture2D;
    node.format = format;
    node.width = width;
    node.height = height;
    node.initialState = initialState;
    node.currentState = initialState;
    node.id = static_cast<uint32_t>(m_resources.size());
    node.refCount = 0;
    node.resource = nullptr;

    m_resources.push_back(std::move(node));
    m_compiled = false;
    return node.id;
}

uint32_t FrameGraph::DeclareBuffer(const std::string& name,
                                    D3D12_RESOURCE_STATES initialState)
{
    ResourceNode node;
    node.name = name;
    node.type = FGResourceType::Buffer;
    node.format = DXGI_FORMAT_UNKNOWN;
    node.width = 0;
    node.height = 0;
    node.initialState = initialState;
    node.currentState = initialState;
    node.id = static_cast<uint32_t>(m_resources.size());
    node.refCount = 0;
    node.resource = nullptr;

    m_resources.push_back(std::move(node));
    m_compiled = false;
    return node.id;
}

// ============================================================================
// リソースバインディング
// ============================================================================

void FrameGraph::BindResource(uint32_t resourceId, ID3D12Resource* resource)
{
    if (resourceId < static_cast<uint32_t>(m_resources.size()))
    {
        m_resources[resourceId].resource = resource;
    }
}

// ============================================================================
// パス管理
// ============================================================================

uint32_t FrameGraph::AddPass(const std::string& name,
                              std::function<void(const PassContext&)> executeFn)
{
    RenderPass pass;
    pass.name = name;
    pass.id = static_cast<uint32_t>(m_passes.size());
    pass.enabled = true;
    pass.execute = std::move(executeFn);
    pass.sortOrder = 0;

    m_passes.push_back(std::move(pass));
    m_compiled = false;
    return pass.id;
}

void FrameGraph::PassReads(uint32_t passId, uint32_t resourceId,
                            D3D12_RESOURCE_STATES state)
{
    if (passId >= static_cast<uint32_t>(m_passes.size()))
        return;
    if (resourceId >= static_cast<uint32_t>(m_resources.size()))
        return;

    ResourceAccess access;
    access.resourceId = resourceId;
    access.requiredState = state;
    m_passes[passId].reads.push_back(access);
    m_resources[resourceId].refCount++;
    m_compiled = false;
}

void FrameGraph::PassWrites(uint32_t passId, uint32_t resourceId,
                             D3D12_RESOURCE_STATES state)
{
    if (passId >= static_cast<uint32_t>(m_passes.size()))
        return;
    if (resourceId >= static_cast<uint32_t>(m_resources.size()))
        return;

    ResourceAccess access;
    access.resourceId = resourceId;
    access.requiredState = state;
    m_passes[passId].writes.push_back(access);
    m_resources[resourceId].refCount++;
    m_compiled = false;
}

void FrameGraph::SetPassEnabled(uint32_t passId, bool enabled)
{
    if (passId < static_cast<uint32_t>(m_passes.size()))
    {
        m_passes[passId].enabled = enabled;
        m_compiled = false;
    }
}

// ============================================================================
// コンパイル
// ============================================================================

bool FrameGraph::Compile()
{
    m_executionOrder.clear();
    m_passBarriers.clear();
    m_compiled = false;

    if (!TopologicalSort())
    {
        GX_LOG_ERROR("FrameGraph::Compile: cycle detected in render pass graph");
        return false;
    }

    GenerateBarriers();
    m_compiled = true;
    return true;
}

// ============================================================================
// トポロジカルソート（カーンのアルゴリズム）
// ============================================================================

bool FrameGraph::TopologicalSort()
{
    const uint32_t passCount = static_cast<uint32_t>(m_passes.size());

    // リソース依存関係から隣接リストを構築する。
    // パスAがリソースRに書き込み、パスBがリソースRを読み取る場合、
    // AはBの前に実行する必要がある（辺 A -> B）。
    // 書き込み後読み取り（WAR）も処理: AがRを読み、BがRに書く場合、A -> B。
    // 書き込み後書き込み（WAW）も処理: AがRに書き、BがRに書く場合（A宣言が先）、A -> B。

    // 各リソースについて、どのパスが書き込み、どのパスが読み取るかを追跡する。
    // 先に宣言されたライターは、リーダーおよび後続のライターより先に実行される必要がある。
    struct ResourcePassInfo
    {
        std::vector<uint32_t> writers; // このリソースに書き込むパスID（宣言順）
        std::vector<uint32_t> readers; // このリソースを読み取るパスID
    };

    std::vector<ResourcePassInfo> resourcePasses(m_resources.size());

    for (uint32_t pi = 0; pi < passCount; ++pi)
    {
        if (!m_passes[pi].enabled)
            continue;

        for (const auto& w : m_passes[pi].writes)
        {
            resourcePasses[w.resourceId].writers.push_back(pi);
        }
        for (const auto& r : m_passes[pi].reads)
        {
            resourcePasses[r.resourceId].readers.push_back(pi);
        }
    }

    // 隣接リストと入次数カウントを構築する
    std::vector<std::vector<uint32_t>> adjacency(passCount);
    std::vector<uint32_t> inDegree(passCount, 0);

    // 重複辺を回避するためのセット
    std::vector<std::unordered_set<uint32_t>> edgeSet(passCount);

    auto addEdge = [&](uint32_t from, uint32_t to)
    {
        if (from == to) return;
        if (!m_passes[from].enabled || !m_passes[to].enabled) return;
        if (edgeSet[from].count(to)) return;
        edgeSet[from].insert(to);
        adjacency[from].push_back(to);
        inDegree[to]++;
    };

    for (uint32_t ri = 0; ri < static_cast<uint32_t>(m_resources.size()); ++ri)
    {
        const auto& info = resourcePasses[ri];

        // ライター -> リーダー辺（書き込み後読み取り = RAW依存関係）
        for (uint32_t w : info.writers)
        {
            for (uint32_t r : info.readers)
            {
                addEdge(w, r);
            }
        }

        // リーダー -> 後続ライター辺（読み取り後書き込み = WAR依存関係）
        for (uint32_t r : info.readers)
        {
            for (uint32_t w : info.writers)
            {
                // パス順序でライターがリーダーの後に宣言されている場合のみ
                if (w > r)
                {
                    addEdge(r, w);
                }
            }
        }

        // ライター -> ライター辺（宣言順、WAW依存関係）
        for (size_t i = 1; i < info.writers.size(); ++i)
        {
            addEdge(info.writers[i - 1], info.writers[i]);
        }
    }

    // カーンのアルゴリズム
    std::queue<uint32_t> queue;
    for (uint32_t pi = 0; pi < passCount; ++pi)
    {
        if (m_passes[pi].enabled && inDegree[pi] == 0)
        {
            queue.push(pi);
        }
    }

    m_executionOrder.clear();
    m_executionOrder.reserve(passCount);

    while (!queue.empty())
    {
        uint32_t current = queue.front();
        queue.pop();

        m_executionOrder.push_back(current);

        for (uint32_t neighbor : adjacency[current])
        {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0)
            {
                queue.push(neighbor);
            }
        }
    }

    // 有効なパスの数をカウントする
    uint32_t enabledCount = 0;
    for (uint32_t pi = 0; pi < passCount; ++pi)
    {
        if (m_passes[pi].enabled)
            enabledCount++;
    }

    // 全有効パスを訪問しなかった場合、循環が存在する
    if (m_executionOrder.size() != enabledCount)
    {
        m_executionOrder.clear();
        return false;
    }

    // ソート順序を割り当てる
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_executionOrder.size()); ++i)
    {
        m_passes[m_executionOrder[i]].sortOrder = i;
    }

    return true;
}

// ============================================================================
// バリア生成
// ============================================================================

void FrameGraph::GenerateBarriers()
{
    // バリア生成前に全リソースステートを初期値にリセットする
    for (auto& res : m_resources)
    {
        res.currentState = res.initialState;
    }

    m_passBarriers.resize(m_executionOrder.size());

    for (uint32_t execIdx = 0; execIdx < static_cast<uint32_t>(m_executionOrder.size()); ++execIdx)
    {
        m_passBarriers[execIdx].clear();

        const uint32_t passId = m_executionOrder[execIdx];
        const RenderPass& pass = m_passes[passId];

        // 書き込みを先にチェックする（レンダーターゲット、UAVなど）
        for (const auto& w : pass.writes)
        {
            auto& res = m_resources[w.resourceId];
            if (res.currentState != w.requiredState)
            {
                BarrierEntry entry;
                entry.resourceId = w.resourceId;
                entry.before = res.currentState;
                entry.after = w.requiredState;
                m_passBarriers[execIdx].push_back(entry);
                res.currentState = w.requiredState;
            }
        }

        // 読み取りをチェックする（SRV、コピーソースなど）
        for (const auto& r : pass.reads)
        {
            auto& res = m_resources[r.resourceId];
            if (res.currentState != r.requiredState)
            {
                BarrierEntry entry;
                entry.resourceId = r.resourceId;
                entry.before = res.currentState;
                entry.after = r.requiredState;
                m_passBarriers[execIdx].push_back(entry);
                res.currentState = r.requiredState;
            }
        }
    }
}

// ============================================================================
// 実行
// ============================================================================

void FrameGraph::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex)
{
    if (!m_compiled || !cmdList)
        return;

    PassContext ctx;
    ctx.cmdList = cmdList;
    ctx.frameIndex = frameIndex;

    for (uint32_t execIdx = 0; execIdx < static_cast<uint32_t>(m_executionOrder.size()); ++execIdx)
    {
        const uint32_t passId = m_executionOrder[execIdx];
        const RenderPass& pass = m_passes[passId];

        // このパスのリソースバリアを発行する
        const auto& barriers = m_passBarriers[execIdx];
        if (!barriers.empty())
        {
            std::vector<D3D12_RESOURCE_BARRIER> d3dBarriers;
            d3dBarriers.reserve(barriers.size());

            for (const auto& be : barriers)
            {
                const auto& res = m_resources[be.resourceId];
                if (!res.resource)
                    continue;

                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrier.Transition.pResource = res.resource;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier.Transition.StateBefore = be.before;
                barrier.Transition.StateAfter = be.after;
                d3dBarriers.push_back(barrier);
            }

            if (!d3dBarriers.empty())
            {
                cmdList->ResourceBarrier(
                    static_cast<UINT>(d3dBarriers.size()),
                    d3dBarriers.data());
            }
        }

        // パスコールバックを実行する
        if (pass.execute)
        {
            pass.execute(ctx);
        }
    }
}

// ============================================================================
// リセット / クリア
// ============================================================================

void FrameGraph::Reset()
{
    for (auto& res : m_resources)
    {
        res.currentState = res.initialState;
    }
    m_compiled = false;
}

void FrameGraph::Clear()
{
    m_resources.clear();
    m_passes.clear();
    m_executionOrder.clear();
    m_passBarriers.clear();
    m_compiled = false;
}

// ============================================================================
// アクセサ
// ============================================================================

const std::vector<BarrierEntry>& FrameGraph::GetBarriersForPass(uint32_t executionIndex) const
{
    if (executionIndex < static_cast<uint32_t>(m_passBarriers.size()))
    {
        return m_passBarriers[executionIndex];
    }
    return s_emptyBarriers;
}

} // namespace gx
