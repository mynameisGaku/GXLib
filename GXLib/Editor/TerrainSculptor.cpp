/// @file TerrainSculptor.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Editor/TerrainSculptor.h"
#include "Core/Logger.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>

namespace gx
{

// ============================================================================
// ターゲットハイトマップ
// ============================================================================

void TerrainSculptor::SetTargetHeightmap(float* data, int width, int height)
{
    m_heightmap = data;
    m_width     = width;
    m_height    = height;
}

void TerrainSculptor::ClearTarget()
{
    m_heightmap = nullptr;
    m_width     = 0;
    m_height    = 0;
}

// ============================================================================
// ブラシ重み評価
// ============================================================================

float TerrainSculptor::EvaluateBrush(float distance, const BrushSettings& settings)
{
    if (settings.radius <= 0.0f)
        return 0.0f;

    float normalizedDist = distance / settings.radius;

    switch (settings.type)
    {
    case BrushType::Circle:
    {
        if (normalizedDist > 1.0f) return 0.0f;
        // 線形フォールオフ
        float edge = 1.0f - settings.falloff;
        if (normalizedDist <= edge) return 1.0f;
        return 1.0f - (normalizedDist - edge) / (1.0f - edge);
    }

    case BrushType::Square:
    {
        // Square は距離ベースだが形状が四角 (ここではmax(|dx|,|dy|)がdistanceとして渡される)
        if (normalizedDist > 1.0f) return 0.0f;
        return 1.0f;
    }

    case BrushType::Diamond:
    {
        // Diamond は |dx| + |dy| がdistanceとして渡される
        if (normalizedDist > 1.0f) return 0.0f;
        return 1.0f - normalizedDist;
    }

    case BrushType::Gaussian:
    {
        if (normalizedDist > 1.0f) return 0.0f;
        // ガウシアン: exp(-k * d^2), k は falloff で制御
        float sigma = (std::max)(0.1f, 1.0f - settings.falloff);
        float exponent = -(normalizedDist * normalizedDist) / (2.0f * sigma * sigma);
        return std::exp(exponent);
    }

    default:
        return 0.0f;
    }
}

// ============================================================================
// ブラシマスク取得
// ============================================================================

gx::Vector<std::pair<std::pair<int, int>, float>>
TerrainSculptor::GetBrushMask(float centerX, float centerY) const
{
    gx::Vector<std::pair<std::pair<int, int>, float>> mask;
    if (!m_heightmap) return mask;

    int cx = static_cast<int>(centerX);
    int cy = static_cast<int>(centerY);
    int r  = static_cast<int>(std::ceil(m_brush.radius));

    for (int y = cy - r; y <= cy + r; ++y)
    {
        for (int x = cx - r; x <= cx + r; ++x)
        {
            if (x < 0 || x >= m_width || y < 0 || y >= m_height)
                continue;

            float dx = static_cast<float>(x) - centerX;
            float dy = static_cast<float>(y) - centerY;
            float dist = std::sqrt(dx * dx + dy * dy);

            float weight = EvaluateBrush(dist, m_brush);
            if (weight > 0.0f)
            {
                mask.push_back({{ x, y }, weight});
            }
        }
    }

    return mask;
}

// ============================================================================
// ブラシ適用
// ============================================================================

SculptAction TerrainSculptor::ApplyBrush(float centerX, float centerY)
{
    SculptAction action;
    action.mode = m_mode;

    if (!m_heightmap) return action;

    auto mask = GetBrushMask(centerX, centerY);

    for (const auto& [pos, weight] : mask)
    {
        int x = pos.first;
        int y = pos.second;
        int idx = y * m_width + x;

        float oldValue = m_heightmap[idx];
        float newValue = oldValue;
        float adjustedStrength = m_brush.strength * weight * m_brush.opacity;

        switch (m_mode)
        {
        case SculptMode::Raise:
            newValue = oldValue + adjustedStrength;
            break;

        case SculptMode::Lower:
            newValue = oldValue - adjustedStrength;
            break;

        case SculptMode::Flatten:
            newValue = oldValue + (m_flattenHeight - oldValue) * adjustedStrength;
            break;

        case SculptMode::Smooth:
        {
            // 3x3 カーネルの平均
            float sum = 0.0f;
            int count = 0;
            for (int dy = -1; dy <= 1; ++dy)
            {
                for (int dx = -1; dx <= 1; ++dx)
                {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height)
                    {
                        sum += m_heightmap[ny * m_width + nx];
                        count++;
                    }
                }
            }
            float avg = sum / static_cast<float>(count);
            newValue = oldValue + (avg - oldValue) * adjustedStrength;
            break;
        }

        case SculptMode::Noise:
        {
            // 疑似ランダムノイズ（位置ベースのハッシュ）
            uint32_t hash = static_cast<uint32_t>(x * 73856093u ^ y * 19349663u);
            float noise = (static_cast<float>(hash % 10000) / 10000.0f) * 2.0f - 1.0f;
            newValue = oldValue + noise * adjustedStrength;
            break;
        }

        case SculptMode::Paint:
            // ペイントモードは高さを変更しない（テクスチャレイヤー用）
            newValue = oldValue;
            break;

        case SculptMode::Erode:
        {
            // 簡易侵食: 周囲の最低値に近づける
            float minH = oldValue;
            for (int dy = -1; dy <= 1; ++dy)
            {
                for (int dx = -1; dx <= 1; ++dx)
                {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height)
                    {
                        float h = m_heightmap[ny * m_width + nx];
                        if (h < minH) minH = h;
                    }
                }
            }
            newValue = oldValue + (minH - oldValue) * adjustedStrength;
            break;
        }
        }

        action.affectedCells.push_back({ x, y });
        action.oldValues.push_back(oldValue);
        action.newValues.push_back(newValue);

        m_heightmap[idx] = newValue;
    }

    // 履歴に追加
    m_history.push_back(action);

    return action;
}

void TerrainSculptor::ApplyStroke(const SculptStroke& stroke)
{
    // 元のブラシ設定を保存
    BrushSettings savedBrush = m_brush;
    SculptMode savedMode = m_mode;

    m_brush = stroke.brush;
    m_mode  = stroke.mode;

    for (const auto& pos : stroke.positions)
    {
        ApplyBrush(pos.first, pos.second);
    }

    // ブラシ設定を復元
    m_brush = savedBrush;
    m_mode  = savedMode;
}

// ============================================================================
// Undo
// ============================================================================

void TerrainSculptor::Undo(const SculptAction& action)
{
    if (!m_heightmap) return;

    for (size_t i = 0; i < action.affectedCells.size(); ++i)
    {
        int x = action.affectedCells[i].first;
        int y = action.affectedCells[i].second;
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        {
            m_heightmap[y * m_width + x] = action.oldValues[i];
        }
    }

    // 履歴から最新を削除
    if (!m_history.empty())
        m_history.pop_back();
}

// ============================================================================
// ユーティリティ
// ============================================================================

float TerrainSculptor::SampleHeight(float x, float y) const
{
    if (!m_heightmap || m_width <= 0 || m_height <= 0)
        return 0.0f;

    // クランプ
    x = (std::max)(0.0f, (std::min)(x, static_cast<float>(m_width - 1)));
    y = (std::max)(0.0f, (std::min)(y, static_cast<float>(m_height - 1)));

    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = (std::min)(x0 + 1, m_width - 1);
    int y1 = (std::min)(y0 + 1, m_height - 1);

    float fx = x - static_cast<float>(x0);
    float fy = y - static_cast<float>(y0);

    float h00 = m_heightmap[y0 * m_width + x0];
    float h10 = m_heightmap[y0 * m_width + x1];
    float h01 = m_heightmap[y1 * m_width + x0];
    float h11 = m_heightmap[y1 * m_width + x1];

    // バイリニア補間
    float h0 = h00 + (h10 - h00) * fx;
    float h1 = h01 + (h11 - h01) * fx;
    return h0 + (h1 - h0) * fy;
}

gx::Vector<Vector3> TerrainSculptor::ComputeNormals(const float* heightmap,
                                                                  int width, int height)
{
    gx::Vector<Vector3> normals(static_cast<size_t>(width) * height);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // 中心差分法
            float hL = (x > 0)          ? heightmap[y * width + (x - 1)] : heightmap[y * width + x];
            float hR = (x < width - 1)  ? heightmap[y * width + (x + 1)] : heightmap[y * width + x];
            float hD = (y > 0)          ? heightmap[(y - 1) * width + x] : heightmap[y * width + x];
            float hU = (y < height - 1) ? heightmap[(y + 1) * width + x] : heightmap[y * width + x];

            float nx = hL - hR;
            float ny = 2.0f;
            float nz = hD - hU;

            // 正規化
            float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (len > 1e-8f)
            {
                nx /= len;
                ny /= len;
                nz /= len;
            }
            else
            {
                nx = 0.0f;
                ny = 1.0f;
                nz = 0.0f;
            }

            normals[static_cast<size_t>(y) * width + x] = { nx, ny, nz };
        }
    }

    return normals;
}

} // namespace gx
