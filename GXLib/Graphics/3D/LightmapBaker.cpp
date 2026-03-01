/// @file LightmapBaker.cpp
/// @brief ライトマップベイキングシステムの実装
#include "pch_graphics.h"
#include "Graphics/3D/LightmapBaker.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace gx
{

// ============================================================================
// 設定
// ============================================================================

void LightmapBaker::SetConfig(const LightmapConfig& config)
{
    m_config = config;
    // 解像度クランプ: 256-4096
    m_config.resolution = std::clamp(m_config.resolution, 256u, 4096u);
    // サンプル数クランプ: 16-1024
    m_config.samplesPerTexel = std::clamp(m_config.samplesPerTexel, 16u, 1024u);
    // バウンス数クランプ: 1-8
    m_config.bounceCount = std::clamp(m_config.bounceCount, 1u, 8u);
    // 最大アトラスサイズ: 最低256
    m_config.maxAtlasSize = std::max(m_config.maxAtlasSize, 256u);
}

// ============================================================================
// メッシュ/ライト登録
// ============================================================================

void LightmapBaker::AddMeshInstance(uint32_t objectIndex,
                                     const std::vector<Vector3>& positions,
                                     const std::vector<Vector3>& normals,
                                     const std::vector<Vector3>& uvs,
                                     const std::vector<uint32_t>& indices)
{
    MeshInstance mesh;
    mesh.objectIndex = objectIndex;
    mesh.positions   = positions;
    mesh.normals     = normals;
    mesh.uvs         = uvs;
    mesh.indices     = indices;
    m_meshInstances.push_back(std::move(mesh));
}

void LightmapBaker::AddLight(LightmapLightType type, const Vector3& position,
                               const Vector3& direction, const Color& color,
                               float intensity, float radius)
{
    LightSource light;
    light.type      = type;
    light.position  = position;
    light.direction = direction;
    light.color     = color;
    light.intensity = intensity;
    light.radius    = radius;
    m_lights.push_back(light);
}

// ============================================================================
// クリア
// ============================================================================

void LightmapBaker::Clear()
{
    m_meshInstances.clear();
    m_lights.clear();
    m_atlas.charts.clear();
    m_atlas.totalWidth  = 0;
    m_atlas.totalHeight = 0;
    m_atlas.packed      = false;
    m_progress.store(0.0f, std::memory_order_relaxed);
    m_cancelled.store(false, std::memory_order_relaxed);
}

// ============================================================================
// ベイキング
// ============================================================================

void LightmapBaker::Bake()
{
    m_progress.store(0.0f, std::memory_order_relaxed);
    m_cancelled.store(false, std::memory_order_relaxed);

    // チャート生成
    m_atlas.charts.clear();
    for (const auto& mesh : m_meshInstances)
    {
        m_atlas.charts.push_back(GenerateChart(mesh));
    }

    if (m_atlas.charts.empty())
    {
        m_progress.store(1.0f, std::memory_order_relaxed);
        return;
    }

    // 直接照明パス
    BakeDirectPass();

    if (m_cancelled.load(std::memory_order_relaxed))
        return;

    // 間接照明バウンスパス
    for (uint32_t bounce = 0; bounce < m_config.bounceCount; ++bounce)
    {
        if (m_cancelled.load(std::memory_order_relaxed))
            return;
        BakeBouncePass(bounce);
    }

    m_progress.store(1.0f, std::memory_order_relaxed);
    GX_LOG_INFO("LightmapBaker: Bake complete (%u charts, %u lights, %u bounces)",
                static_cast<uint32_t>(m_atlas.charts.size()),
                static_cast<uint32_t>(m_lights.size()),
                m_config.bounceCount);
}

void LightmapBaker::BakeDirectOnly()
{
    m_progress.store(0.0f, std::memory_order_relaxed);
    m_cancelled.store(false, std::memory_order_relaxed);

    // チャート生成
    m_atlas.charts.clear();
    for (const auto& mesh : m_meshInstances)
    {
        m_atlas.charts.push_back(GenerateChart(mesh));
    }

    if (m_atlas.charts.empty())
    {
        m_progress.store(1.0f, std::memory_order_relaxed);
        return;
    }

    // 直接照明パスのみ
    BakeDirectPass();

    m_progress.store(1.0f, std::memory_order_relaxed);
    GX_LOG_INFO("LightmapBaker: Direct-only bake complete (%u charts, %u lights)",
                static_cast<uint32_t>(m_atlas.charts.size()),
                static_cast<uint32_t>(m_lights.size()));
}

// ============================================================================
// 直接照明パス
// ============================================================================

void LightmapBaker::BakeDirectPass()
{
    uint32_t totalTexels = 0;
    for (const auto& chart : m_atlas.charts)
    {
        for (const auto& texel : chart.texelData)
        {
            if (texel.valid) ++totalTexels;
        }
    }

    uint32_t processedTexels = 0;

    for (auto& chart : m_atlas.charts)
    {
        for (auto& texel : chart.texelData)
        {
            if (m_cancelled.load(std::memory_order_relaxed))
                return;

            if (!texel.valid)
                continue;

            // アンビエントで初期化
            Color accum = m_config.ambient;

            // 各ライトからの寄与を加算
            for (const auto& light : m_lights)
            {
                accum = accum + AccumulateLight(texel.position, texel.normal, light);
            }

            texel.color = accum;
            ++processedTexels;

            // 進捗更新（直接照明は全体の半分）
            if (totalTexels > 0)
            {
                float directProgress = static_cast<float>(processedTexels) / static_cast<float>(totalTexels);
                m_progress.store(directProgress * 0.5f, std::memory_order_relaxed);
            }
        }
    }
}

// ============================================================================
// 間接照明バウンスパス
// ============================================================================

void LightmapBaker::BakeBouncePass(uint32_t bounceIndex)
{
    // 前回のライトマップ結果を保持（バウンスの入力として使う）
    std::vector<std::vector<LightmapTexel>> previousTexels;
    for (const auto& chart : m_atlas.charts)
    {
        previousTexels.push_back(chart.texelData);
    }

    uint32_t totalTexels = 0;
    for (const auto& chart : m_atlas.charts)
    {
        for (const auto& texel : chart.texelData)
        {
            if (texel.valid) ++totalTexels;
        }
    }

    uint32_t processedTexels = 0;
    float bounceWeight = 1.0f / static_cast<float>(bounceIndex + 2);

    for (size_t chartIdx = 0; chartIdx < m_atlas.charts.size(); ++chartIdx)
    {
        auto& chart = m_atlas.charts[chartIdx];
        for (size_t texelIdx = 0; texelIdx < chart.texelData.size(); ++texelIdx)
        {
            if (m_cancelled.load(std::memory_order_relaxed))
                return;

            auto& texel = chart.texelData[texelIdx];
            if (!texel.valid)
                continue;

            // 半球サンプリングによる間接照明
            Color indirect{0.0f, 0.0f, 0.0f, 0.0f};
            uint32_t samples = m_config.samplesPerTexel;

            for (uint32_t s = 0; s < samples; ++s)
            {
                Vector3 sampleDir = SampleHemisphere(texel.normal, s, samples);

                // レイをシーンに飛ばす
                Vector3 rayOrigin = texel.position + texel.normal * 0.001f;
                RaycastResult hitResult = TraceRay(rayOrigin, sampleDir);

                if (hitResult.hit)
                {
                    // ヒット点の色を取得（前回バウンスの結果から）
                    // 簡易的にヒットしたチャートの最近傍テクセルの色を使用
                    Color hitColor{0.0f, 0.0f, 0.0f, 1.0f};
                    float minDist = 1e30f;

                    for (size_t ci = 0; ci < previousTexels.size(); ++ci)
                    {
                        for (const auto& prevTexel : previousTexels[ci])
                        {
                            if (!prevTexel.valid) continue;
                            float dist = prevTexel.position.DistanceSquared(hitResult.position);
                            if (dist < minDist)
                            {
                                minDist = dist;
                                hitColor = prevTexel.color;
                            }
                        }
                    }

                    // コサイン重み付け
                    float cosTheta = std::max(0.0f, texel.normal.Dot(sampleDir));
                    indirect = indirect + hitColor * (cosTheta * bounceWeight);
                }
            }

            // サンプル数で平均化
            if (samples > 0)
            {
                float invSamples = 1.0f / static_cast<float>(samples);
                indirect = indirect * invSamples;
            }

            // 間接照明を加算
            texel.color = texel.color + indirect;

            ++processedTexels;

            // 進捗更新（バウンスは後半50%を均等分割）
            if (totalTexels > 0 && m_config.bounceCount > 0)
            {
                float bounceProgress = static_cast<float>(processedTexels) / static_cast<float>(totalTexels);
                float overallProgress = 0.5f +
                    (static_cast<float>(bounceIndex) + bounceProgress) /
                    static_cast<float>(m_config.bounceCount) * 0.5f;
                m_progress.store(overallProgress, std::memory_order_relaxed);
            }
        }
    }
}

// ============================================================================
// チャート生成
// ============================================================================

LightmapChart LightmapBaker::GenerateChart(const MeshInstance& mesh) const
{
    LightmapChart chart;
    chart.objectIndex = mesh.objectIndex;

    // UV範囲からチャートサイズを決定
    uint32_t res = m_config.resolution;

    // UVが存在しない場合は最小サイズのチャートを生成
    if (mesh.uvs.empty() || mesh.positions.empty())
    {
        chart.width  = 1;
        chart.height = 1;
        chart.texelData.resize(1);
        return chart;
    }

    // UV境界ボックスの計算
    float minU = 1e30f, maxU = -1e30f;
    float minV = 1e30f, maxV = -1e30f;
    for (const auto& uv : mesh.uvs)
    {
        minU = std::min(minU, uv.x);
        maxU = std::max(maxU, uv.x);
        minV = std::min(minV, uv.y);
        maxV = std::max(maxV, uv.y);
    }

    float uvRangeU = maxU - minU;
    float uvRangeV = maxV - minV;

    // 解像度に応じたチャートサイズ
    chart.width  = std::max(1u, static_cast<uint32_t>(uvRangeU * res + 0.5f));
    chart.height = std::max(1u, static_cast<uint32_t>(uvRangeV * res + 0.5f));

    // 最大サイズクランプ
    chart.width  = std::min(chart.width, m_config.maxAtlasSize);
    chart.height = std::min(chart.height, m_config.maxAtlasSize);

    chart.texelData.resize(static_cast<size_t>(chart.width) * chart.height);

    // 三角形ごとにテクセルにラスタライズ
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
    {
        uint32_t i0 = mesh.indices[i];
        uint32_t i1 = mesh.indices[i + 1];
        uint32_t i2 = mesh.indices[i + 2];

        if (i0 >= mesh.positions.size() || i1 >= mesh.positions.size() || i2 >= mesh.positions.size())
            continue;
        if (i0 >= mesh.uvs.size() || i1 >= mesh.uvs.size() || i2 >= mesh.uvs.size())
            continue;

        const Vector3& p0 = mesh.positions[i0];
        const Vector3& p1 = mesh.positions[i1];
        const Vector3& p2 = mesh.positions[i2];

        const Vector3& uv0 = mesh.uvs[i0];
        const Vector3& uv1 = mesh.uvs[i1];
        const Vector3& uv2 = mesh.uvs[i2];

        Vector3 n0 = (i0 < mesh.normals.size()) ? mesh.normals[i0] : Vector3{0, 1, 0};
        Vector3 n1 = (i1 < mesh.normals.size()) ? mesh.normals[i1] : Vector3{0, 1, 0};
        Vector3 n2 = (i2 < mesh.normals.size()) ? mesh.normals[i2] : Vector3{0, 1, 0};

        // UV空間でのバウンディングボックス
        float triMinU = std::min({uv0.x, uv1.x, uv2.x});
        float triMaxU = std::max({uv0.x, uv1.x, uv2.x});
        float triMinV = std::min({uv0.y, uv1.y, uv2.y});
        float triMaxV = std::max({uv0.y, uv1.y, uv2.y});

        // テクセル座標に変換
        int startX = std::max(0, static_cast<int>((triMinU - minU) / uvRangeU * chart.width - 0.5f));
        int endX   = std::min(static_cast<int>(chart.width) - 1,
                              static_cast<int>((triMaxU - minU) / uvRangeU * chart.width + 0.5f));
        int startY = std::max(0, static_cast<int>((triMinV - minV) / uvRangeV * chart.height - 0.5f));
        int endY   = std::min(static_cast<int>(chart.height) - 1,
                              static_cast<int>((triMaxV - minV) / uvRangeV * chart.height + 0.5f));

        for (int ty = startY; ty <= endY; ++ty)
        {
            for (int tx = startX; tx <= endX; ++tx)
            {
                // テクセル中心のUV座標
                float u = minU + (static_cast<float>(tx) + 0.5f) / chart.width * uvRangeU;
                float v = minV + (static_cast<float>(ty) + 0.5f) / chart.height * uvRangeV;

                // バリセントリック座標を計算
                float denom = (uv1.y - uv2.y) * (uv0.x - uv2.x) + (uv2.x - uv1.x) * (uv0.y - uv2.y);
                if (std::abs(denom) < 1e-10f) continue;

                float invDenom = 1.0f / denom;
                float w0 = ((uv1.y - uv2.y) * (u - uv2.x) + (uv2.x - uv1.x) * (v - uv2.y)) * invDenom;
                float w1 = ((uv2.y - uv0.y) * (u - uv2.x) + (uv0.x - uv2.x) * (v - uv2.y)) * invDenom;
                float w2 = 1.0f - w0 - w1;

                // バリセントリック座標が三角形内にあるか
                if (w0 < -0.01f || w1 < -0.01f || w2 < -0.01f) continue;
                if (w0 > 1.01f || w1 > 1.01f || w2 > 1.01f) continue;

                size_t idx = static_cast<size_t>(ty) * chart.width + tx;
                auto& texel = chart.texelData[idx];

                // 補間してワールド位置・法線を求める
                texel.position = p0 * w0 + p1 * w1 + p2 * w2;
                Vector3 interpNormal = n0 * w0 + n1 * w1 + n2 * w2;
                float nLen = interpNormal.Length();
                texel.normal = (nLen > 1e-6f) ? interpNormal * (1.0f / nLen) : Vector3{0, 1, 0};
                texel.color  = Color{0.0f, 0.0f, 0.0f, 1.0f};
                texel.valid  = true;
            }
        }
    }

    return chart;
}

// ============================================================================
// アトラスパッキング（Shelf-First-Fit）
// ============================================================================

PackResult LightmapBaker::PackAtlas()
{
    PackResult result;

    if (m_atlas.charts.empty())
    {
        result.success = true;
        result.packEfficiency = 0.0f;
        result.totalTexels = 0;
        result.usedTexels = 0;
        m_atlas.packed = true;
        m_atlas.totalWidth = 0;
        m_atlas.totalHeight = 0;
        return result;
    }

    uint32_t padding = m_config.padding;

    // チャートを高さの降順にソート（インデックスでソート）
    std::vector<size_t> sortedIndices(m_atlas.charts.size());
    std::iota(sortedIndices.begin(), sortedIndices.end(), 0u);
    std::sort(sortedIndices.begin(), sortedIndices.end(),
              [this](size_t a, size_t b) {
                  return m_atlas.charts[a].height > m_atlas.charts[b].height;
              });

    // Shelf-First-Fit アルゴリズム
    struct Shelf
    {
        uint32_t y;
        uint32_t height;
        uint32_t usedWidth;
    };

    std::vector<Shelf> shelves;
    uint32_t maxWidth = m_config.maxAtlasSize;
    uint32_t totalUsedTexels = 0;

    for (size_t si : sortedIndices)
    {
        auto& chart = m_atlas.charts[si];
        uint32_t cw = chart.width + padding;
        uint32_t ch = chart.height + padding;

        bool placed = false;

        // 既存シェルフに収まるか試す
        for (auto& shelf : shelves)
        {
            if (shelf.usedWidth + cw <= maxWidth && ch <= shelf.height)
            {
                chart.atlasX = shelf.usedWidth;
                chart.atlasY = shelf.y;
                shelf.usedWidth += cw;
                placed = true;
                break;
            }
        }

        // 新しいシェルフを追加
        if (!placed)
        {
            uint32_t newY = 0;
            if (!shelves.empty())
            {
                const auto& lastShelf = shelves.back();
                newY = lastShelf.y + lastShelf.height;
            }

            if (newY + ch > m_config.maxAtlasSize || cw > maxWidth)
            {
                // アトラスに収まらない
                result.success = false;
                result.packEfficiency = 0.0f;
                return result;
            }

            Shelf newShelf;
            newShelf.y = newY;
            newShelf.height = ch;
            newShelf.usedWidth = cw;
            shelves.push_back(newShelf);

            chart.atlasX = 0;
            chart.atlasY = newY;
        }

        totalUsedTexels += chart.width * chart.height;
    }

    // アトラスサイズ計算
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    for (const auto& chart : m_atlas.charts)
    {
        atlasWidth  = std::max(atlasWidth, chart.atlasX + chart.width);
        atlasHeight = std::max(atlasHeight, chart.atlasY + chart.height);
    }

    m_atlas.totalWidth  = atlasWidth;
    m_atlas.totalHeight = atlasHeight;
    m_atlas.packed      = true;

    uint32_t totalAtlasTexels = atlasWidth * atlasHeight;

    result.success = true;
    result.usedTexels = totalUsedTexels;
    result.totalTexels = totalAtlasTexels;
    result.packEfficiency = (totalAtlasTexels > 0)
        ? static_cast<float>(totalUsedTexels) / static_cast<float>(totalAtlasTexels)
        : 0.0f;

    GX_LOG_INFO("LightmapBaker: Atlas packed %ux%u, efficiency=%.1f%%",
                atlasWidth, atlasHeight, result.packEfficiency * 100.0f);

    return result;
}

// ============================================================================
// レイトレーシング
// ============================================================================

bool LightmapBaker::RayTriangleIntersect(const Vector3& origin, const Vector3& dir,
                                           const Vector3& v0, const Vector3& v1, const Vector3& v2,
                                           float& outT) const
{
    // Moller-Trumbore intersection algorithm
    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = v2 - v0;
    Vector3 h = dir.Cross(edge2);
    float a = edge1.Dot(h);

    if (std::abs(a) < 1e-8f)
        return false;

    float f = 1.0f / a;
    Vector3 s = origin - v0;
    float u = f * s.Dot(h);

    if (u < 0.0f || u > 1.0f)
        return false;

    Vector3 q = s.Cross(edge1);
    float v = f * dir.Dot(q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    float t = f * edge2.Dot(q);

    if (t > 1e-6f)
    {
        outT = t;
        return true;
    }

    return false;
}

bool LightmapBaker::TraceShadowRay(const Vector3& origin, const Vector3& direction, float maxDistance) const
{
    // シーン内の全三角形と交差判定
    for (const auto& mesh : m_meshInstances)
    {
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            uint32_t i0 = mesh.indices[i];
            uint32_t i1 = mesh.indices[i + 1];
            uint32_t i2 = mesh.indices[i + 2];

            if (i0 >= mesh.positions.size() || i1 >= mesh.positions.size() || i2 >= mesh.positions.size())
                continue;

            float t = 0.0f;
            if (RayTriangleIntersect(origin, direction, mesh.positions[i0], mesh.positions[i1], mesh.positions[i2], t))
            {
                if (t > 0.001f && t < maxDistance)
                    return false; // 遮蔽あり
            }
        }
    }
    return true; // 遮蔽なし
}

RaycastResult LightmapBaker::TraceRay(const Vector3& origin, const Vector3& direction) const
{
    RaycastResult result;
    float closestT = 1e30f;

    for (const auto& mesh : m_meshInstances)
    {
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            uint32_t i0 = mesh.indices[i];
            uint32_t i1 = mesh.indices[i + 1];
            uint32_t i2 = mesh.indices[i + 2];

            if (i0 >= mesh.positions.size() || i1 >= mesh.positions.size() || i2 >= mesh.positions.size())
                continue;

            float t = 0.0f;
            if (RayTriangleIntersect(origin, direction, mesh.positions[i0], mesh.positions[i1], mesh.positions[i2], t))
            {
                if (t > 0.001f && t < closestT)
                {
                    closestT = t;
                    result.hit = true;
                    result.distance = t;
                    result.position = origin + direction * t;

                    // 三角形法線を計算
                    Vector3 edge1 = mesh.positions[i1] - mesh.positions[i0];
                    Vector3 edge2 = mesh.positions[i2] - mesh.positions[i0];
                    Vector3 normal = edge1.Cross(edge2);
                    float nLen = normal.Length();
                    result.normal = (nLen > 1e-6f) ? normal * (1.0f / nLen) : Vector3{0, 1, 0};
                }
            }
        }
    }

    return result;
}

// ============================================================================
// 半球サンプリング
// ============================================================================

Vector3 LightmapBaker::SampleHemisphere(const Vector3& normal, uint32_t sampleIndex, uint32_t totalSamples) const
{
    // コサイン重み付き半球サンプリング（準ランダム）
    // Hammersley sequence for quasi-random distribution
    float u1 = static_cast<float>(sampleIndex) / static_cast<float>(totalSamples);

    // Radical inverse (Van der Corput sequence)
    uint32_t bits = sampleIndex;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float u2 = static_cast<float>(bits) * 2.3283064365386963e-10f;

    // コサイン重み付き半球サンプリング
    float phi = 2.0f * 3.14159265358979f * u1;
    float cosTheta = std::sqrt(1.0f - u2);
    float sinTheta = std::sqrt(u2);

    // ローカルサンプル方向
    float x = std::cos(phi) * sinTheta;
    float y = cosTheta;
    float z = std::sin(phi) * sinTheta;

    // 法線に沿った座標系を構築
    Vector3 up = (std::abs(normal.y) < 0.999f) ? Vector3{0, 1, 0} : Vector3{1, 0, 0};
    Vector3 tangent = normal.Cross(up);
    float tLen = tangent.Length();
    if (tLen > 1e-6f) tangent = tangent * (1.0f / tLen);
    else tangent = Vector3{1, 0, 0};

    Vector3 bitangent = normal.Cross(tangent);

    // ワールド空間に変換
    Vector3 result = tangent * x + normal * y + bitangent * z;
    float rLen = result.Length();
    if (rLen > 1e-6f) result = result * (1.0f / rLen);
    return result;
}

// ============================================================================
// ライト寄与計算
// ============================================================================

Color LightmapBaker::AccumulateLight(const Vector3& texelPos, const Vector3& texelNormal,
                                      const LightSource& light) const
{
    Color contribution{0.0f, 0.0f, 0.0f, 0.0f};

    switch (light.type)
    {
    case LightmapLightType::Point:
    {
        Vector3 toLight = light.position - texelPos;
        float dist = toLight.Length();

        if (dist < 1e-6f || (light.radius > 0.0f && dist > light.radius))
            break;

        Vector3 lightDir = toLight * (1.0f / dist);
        float NdotL = std::max(0.0f, texelNormal.Dot(lightDir));

        if (NdotL <= 0.0f)
            break;

        // シャドウレイ
        Vector3 rayOrigin = texelPos + texelNormal * 0.001f;
        if (!TraceShadowRay(rayOrigin, lightDir, dist))
            break;

        // 減衰計算
        float attenuation = 1.0f / (1.0f + dist * dist);
        if (light.radius > 0.0f)
        {
            float falloff = 1.0f - std::min(1.0f, dist / light.radius);
            attenuation *= falloff * falloff;
        }

        contribution = light.color * (NdotL * attenuation * light.intensity);
        break;
    }

    case LightmapLightType::Directional:
    {
        Vector3 lightDir = Vector3{0, 0, 0} - light.direction;
        float lLen = lightDir.Length();
        if (lLen > 1e-6f) lightDir = lightDir * (1.0f / lLen);

        float NdotL = std::max(0.0f, texelNormal.Dot(lightDir));

        if (NdotL <= 0.0f)
            break;

        // シャドウレイ（ディレクショナルは無限遠）
        Vector3 rayOrigin = texelPos + texelNormal * 0.001f;
        if (!TraceShadowRay(rayOrigin, lightDir, 10000.0f))
            break;

        contribution = light.color * (NdotL * light.intensity);
        break;
    }

    case LightmapLightType::Spot:
    {
        Vector3 toLight = light.position - texelPos;
        float dist = toLight.Length();

        if (dist < 1e-6f || (light.radius > 0.0f && dist > light.radius))
            break;

        Vector3 lightDir = toLight * (1.0f / dist);
        float NdotL = std::max(0.0f, texelNormal.Dot(lightDir));

        if (NdotL <= 0.0f)
            break;

        // スポットライト角度チェック
        Vector3 spotDir = light.direction;
        float sdLen = spotDir.Length();
        if (sdLen > 1e-6f) spotDir = spotDir * (1.0f / sdLen);

        float spotAngle = std::max(0.0f, (Vector3{0, 0, 0} - lightDir).Dot(spotDir));
        float spotCutoff = 0.5f; // 60度コーンのデフォルト
        if (spotAngle < spotCutoff)
            break;

        // シャドウレイ
        Vector3 rayOrigin = texelPos + texelNormal * 0.001f;
        if (!TraceShadowRay(rayOrigin, lightDir, dist))
            break;

        // 減衰
        float attenuation = 1.0f / (1.0f + dist * dist);
        float spotFactor = (spotAngle - spotCutoff) / (1.0f - spotCutoff);
        spotFactor = std::max(0.0f, std::min(1.0f, spotFactor));

        contribution = light.color * (NdotL * attenuation * spotFactor * light.intensity);
        break;
    }
    }

    return contribution;
}

} // namespace gx
