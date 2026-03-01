/// @file AtlasPacker.cpp
/// @brief テクスチャアトラスパッキング実装（MaxRects BSSF）
#include "pch_graphics.h"
#include "Graphics/AtlasPacker.h"
#include <algorithm>
#include <numeric>
#include <climits>

namespace gx
{

void AtlasPacker::SetMaxSize(uint32_t maxWidth, uint32_t maxHeight)
{
    m_maxWidth  = maxWidth;
    m_maxHeight = maxHeight;
}

void AtlasPacker::SetPadding(uint32_t padding)
{
    m_padding = padding;
}

uint32_t AtlasPacker::NextPowerOf2(uint32_t v)
{
    if (v == 0) return 1;
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}

AtlasResult AtlasPacker::Pack(const std::vector<AtlasInput>& inputs)
{
    AtlasResult result;
    if (inputs.empty())
    {
        result.success = true;
        return result;
    }

    // 面積の大きい順にソート
    std::vector<size_t> order(inputs.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        uint32_t areaA = inputs[a].width * inputs[a].height;
        uint32_t areaB = inputs[b].width * inputs[b].height;
        return areaA > areaB;
    });

    std::vector<AtlasInput> sorted;
    sorted.reserve(inputs.size());
    for (size_t i : order) sorted.push_back(inputs[i]);

    // 総面積からPower-of-2のスタートサイズを推定
    uint32_t totalArea = 0;
    uint32_t maxSingleW = 0, maxSingleH = 0;
    for (const auto& inp : sorted)
    {
        uint32_t pw = inp.width + m_padding * 2;
        uint32_t ph = inp.height + m_padding * 2;
        totalArea += pw * ph;
        maxSingleW = (std::max)(maxSingleW, pw);
        maxSingleH = (std::max)(maxSingleH, ph);
    }

    uint32_t startSize = NextPowerOf2(static_cast<uint32_t>(std::sqrt(static_cast<float>(totalArea))));
    startSize = (std::max)(startSize, NextPowerOf2((std::max)(maxSingleW, maxSingleH)));

    // 徐々にサイズを拡大してパッキングを試行
    for (uint32_t size = startSize; size <= m_maxWidth && size <= m_maxHeight; size *= 2)
    {
        std::vector<AtlasEntry> entries;
        if (TryPack(sorted, size, size, entries))
        {
            result.atlasWidth  = size;
            result.atlasHeight = size;
            result.entries = std::move(entries);
            result.success = true;
            return result;
        }
    }

    // 正方形で失敗した場合、非正方形を試す
    for (uint32_t w = startSize; w <= m_maxWidth; w *= 2)
    {
        for (uint32_t h = startSize; h <= m_maxHeight; h *= 2)
        {
            if (w == h) continue; // 正方形は試行済み
            std::vector<AtlasEntry> entries;
            if (TryPack(sorted, w, h, entries))
            {
                result.atlasWidth  = w;
                result.atlasHeight = h;
                result.entries = std::move(entries);
                result.success = true;
                return result;
            }
        }
    }

    result.success = false;
    return result;
}

bool AtlasPacker::TryPack(const std::vector<AtlasInput>& sorted, uint32_t atlasW, uint32_t atlasH,
                           std::vector<AtlasEntry>& outEntries)
{
    std::vector<Rect> freeRects;
    freeRects.push_back({ 0, 0, atlasW, atlasH });
    outEntries.clear();
    outEntries.reserve(sorted.size());

    for (const auto& input : sorted)
    {
        uint32_t pw = input.width  + m_padding * 2;
        uint32_t ph = input.height + m_padding * 2;

        int bestScore = INT_MAX;
        Rect best = FindBestBSSF(freeRects, pw, ph, bestScore);

        if (bestScore == INT_MAX)
            return false; // 配置不可

        Rect placed = { best.x, best.y, pw, ph };
        SplitFreeRects(freeRects, placed);
        PruneFreeRects(freeRects);

        AtlasEntry entry;
        entry.name   = input.name;
        entry.x      = placed.x + m_padding;
        entry.y      = placed.y + m_padding;
        entry.width  = input.width;
        entry.height = input.height;
        outEntries.push_back(entry);
    }

    return true;
}

AtlasPacker::Rect AtlasPacker::FindBestBSSF(const std::vector<Rect>& freeRects,
                                             uint32_t w, uint32_t h, int& bestScore)
{
    Rect bestRect = { 0, 0, 0, 0 };
    bestScore = INT_MAX;
    int bestLong = INT_MAX;

    for (const auto& fr : freeRects)
    {
        if (w <= fr.width && h <= fr.height)
        {
            int shortSide = (std::min)(static_cast<int>(fr.width - w),
                                       static_cast<int>(fr.height - h));
            int longSide  = (std::max)(static_cast<int>(fr.width - w),
                                       static_cast<int>(fr.height - h));
            if (shortSide < bestScore || (shortSide == bestScore && longSide < bestLong))
            {
                bestRect = { fr.x, fr.y, w, h };
                bestScore = shortSide;
                bestLong  = longSide;
            }
        }
    }

    return bestRect;
}

void AtlasPacker::SplitFreeRects(std::vector<Rect>& freeRects, const Rect& placed)
{
    std::vector<Rect> newRects;

    for (size_t i = 0; i < freeRects.size(); )
    {
        auto& fr = freeRects[i];

        // 交差しない場合はスキップ
        if (placed.x >= fr.x + fr.width || placed.x + placed.width <= fr.x ||
            placed.y >= fr.y + fr.height || placed.y + placed.height <= fr.y)
        {
            ++i;
            continue;
        }

        // 4方向に分割
        // 左側
        if (placed.x > fr.x)
            newRects.push_back({ fr.x, fr.y, placed.x - fr.x, fr.height });
        // 右側
        if (placed.x + placed.width < fr.x + fr.width)
            newRects.push_back({ placed.x + placed.width, fr.y,
                                 fr.x + fr.width - placed.x - placed.width, fr.height });
        // 上側
        if (placed.y > fr.y)
            newRects.push_back({ fr.x, fr.y, fr.width, placed.y - fr.y });
        // 下側
        if (placed.y + placed.height < fr.y + fr.height)
            newRects.push_back({ fr.x, placed.y + placed.height, fr.width,
                                 fr.y + fr.height - placed.y - placed.height });

        freeRects.erase(freeRects.begin() + static_cast<ptrdiff_t>(i));
    }

    freeRects.insert(freeRects.end(), newRects.begin(), newRects.end());
}

void AtlasPacker::PruneFreeRects(std::vector<Rect>& freeRects)
{
    for (size_t i = 0; i < freeRects.size(); )
    {
        bool contained = false;
        for (size_t j = 0; j < freeRects.size(); ++j)
        {
            if (i == j) continue;
            const auto& a = freeRects[i];
            const auto& b = freeRects[j];
            if (a.x >= b.x && a.y >= b.y &&
                a.x + a.width <= b.x + b.width &&
                a.y + a.height <= b.y + b.height)
            {
                contained = true;
                break;
            }
        }
        if (contained)
            freeRects.erase(freeRects.begin() + static_cast<ptrdiff_t>(i));
        else
            ++i;
    }
}

} // namespace gx
