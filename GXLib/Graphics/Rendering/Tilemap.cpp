/// @file Tilemap.cpp
/// @brief 2Dタイルマップの実装
#include "pch.h"
#include "Graphics/Rendering/Tilemap.h"
#include "GUI/XMLParser.h"
#include "IO/FileSystem.h"
#include "Core/Logger.h"

namespace GX
{

bool Tilemap::LoadFromTMX(const std::string& path, TextureManager& texManager)
{
    GUI::XMLDocument doc;
    if (!doc.LoadFromFile(path))
    {
        GX_LOG_ERROR("Tilemap: Failed to load TMX file: {}", path);
        return false;
    }

    auto* root = doc.GetRoot();
    if (!root || root->tag != "map")
    {
        GX_LOG_ERROR("Tilemap: Invalid TMX root element");
        return false;
    }

    // マップ属性を読み取る
    m_mapWidth  = std::stoi(root->GetAttribute("width", "0"));
    m_mapHeight = std::stoi(root->GetAttribute("height", "0"));
    m_tileWidth = std::stoi(root->GetAttribute("tilewidth", "32"));
    m_tileHeight = std::stoi(root->GetAttribute("tileheight", "32"));

    // TMXファイルのディレクトリを取得（テクスチャパス解決用）
    std::string dir;
    auto slashPos = path.find_last_of("/\\");
    if (slashPos != std::string::npos)
        dir = path.substr(0, slashPos + 1);

    // タイルセットを読み込む
    for (const auto& child : root->children)
    {
        if (child->tag == "tileset")
        {
            Tileset ts;
            ts.firstGid   = std::stoi(child->GetAttribute("firstgid", "1"));
            ts.tileWidth   = std::stoi(child->GetAttribute("tilewidth", std::to_string(m_tileWidth)));
            ts.tileHeight  = std::stoi(child->GetAttribute("tileheight", std::to_string(m_tileHeight)));
            ts.columns     = std::stoi(child->GetAttribute("columns", "1"));
            ts.tileCount   = std::stoi(child->GetAttribute("tilecount", "0"));

            // image 子要素からテクスチャパスを取得
            for (const auto& tsChild : child->children)
            {
                if (tsChild->tag == "image")
                {
                    std::string source = tsChild->GetAttribute("source");
                    if (!source.empty())
                    {
                        std::string texPath = dir + source;
                        std::wstring wTexPath(texPath.begin(), texPath.end());
                        ts.textureHandle = texManager.LoadTexture(wTexPath);
                    }
                }
            }

            m_tilesets.push_back(ts);
        }
        else if (child->tag == "layer")
        {
            TilemapLayer layer;
            layer.name   = child->GetAttribute("name", "");
            layer.width  = std::stoi(child->GetAttribute("width", std::to_string(m_mapWidth)));
            layer.height = std::stoi(child->GetAttribute("height", std::to_string(m_mapHeight)));

            if (child->HasAttribute("visible"))
                layer.visible = child->GetAttribute("visible") != "0";
            if (child->HasAttribute("opacity"))
                layer.opacity = std::stof(child->GetAttribute("opacity", "1.0"));

            // data 子要素からタイルデータを読み取る（CSV形式）
            for (const auto& dataChild : child->children)
            {
                if (dataChild->tag == "data")
                {
                    std::string encoding = dataChild->GetAttribute("encoding", "csv");
                    if (encoding == "csv")
                    {
                        // CSV パース
                        const std::string& text = dataChild->text;
                        layer.tiles.reserve(layer.width * layer.height);
                        std::string num;
                        for (char c : text)
                        {
                            if (c >= '0' && c <= '9')
                            {
                                num += c;
                            }
                            else if (!num.empty())
                            {
                                layer.tiles.push_back(std::stoi(num));
                                num.clear();
                            }
                        }
                        if (!num.empty())
                            layer.tiles.push_back(std::stoi(num));
                    }
                }
            }

            m_layers.push_back(std::move(layer));
        }
    }

    return true;
}

void Tilemap::Create(int width, int height, int tileWidth, int tileHeight)
{
    m_mapWidth = width;
    m_mapHeight = height;
    m_tileWidth = tileWidth;
    m_tileHeight = tileHeight;
    m_layers.clear();
    m_tilesets.clear();
}

void Tilemap::AddTileset(const Tileset& tileset)
{
    m_tilesets.push_back(tileset);
}

void Tilemap::AddLayer(const TilemapLayer& layer)
{
    m_layers.push_back(layer);
}

int Tilemap::GetTile(int layerIndex, int x, int y) const
{
    if (layerIndex < 0 || layerIndex >= static_cast<int>(m_layers.size()))
        return 0;

    const auto& layer = m_layers[layerIndex];
    if (x < 0 || x >= layer.width || y < 0 || y >= layer.height)
        return 0;

    int idx = y * layer.width + x;
    if (idx >= static_cast<int>(layer.tiles.size()))
        return 0;

    return layer.tiles[idx];
}

void Tilemap::SetTile(int layerIndex, int x, int y, int gid)
{
    if (layerIndex < 0 || layerIndex >= static_cast<int>(m_layers.size()))
        return;

    auto& layer = m_layers[layerIndex];
    if (x < 0 || x >= layer.width || y < 0 || y >= layer.height)
        return;

    int idx = y * layer.width + x;
    if (idx >= static_cast<int>(layer.tiles.size()))
        return;

    layer.tiles[idx] = gid;
}

void Tilemap::Draw(SpriteBatch& batch, const Camera2D* camera,
                   float screenWidth, float screenHeight) const
{
    // カメラの可視範囲を計算
    float viewLeft   = 0.0f;
    float viewTop    = 0.0f;
    float viewRight  = screenWidth;
    float viewBottom = screenHeight;

    if (camera)
    {
        float cx = camera->GetPositionX();
        float cy = camera->GetPositionY();
        float zoom = camera->GetZoom();
        float hw = screenWidth  / (2.0f * zoom);
        float hh = screenHeight / (2.0f * zoom);
        viewLeft   = cx - hw;
        viewTop    = cy - hh;
        viewRight  = cx + hw;
        viewBottom = cy + hh;
    }

    // 可視タイル範囲を計算（1タイル分マージンを追加）
    int startX = (std::max)(0, static_cast<int>(floorf(viewLeft / m_tileWidth)) - 1);
    int startY = (std::max)(0, static_cast<int>(floorf(viewTop / m_tileHeight)) - 1);
    int endX   = (std::min)(m_mapWidth,  static_cast<int>(ceilf(viewRight / m_tileWidth)) + 1);
    int endY   = (std::min)(m_mapHeight, static_cast<int>(ceilf(viewBottom / m_tileHeight)) + 1);

    for (const auto& layer : m_layers)
    {
        if (!layer.visible)
            continue;

        // レイヤーの不透明度を SpriteBatch のカラーに反映
        batch.SetDrawColor(1.0f, 1.0f, 1.0f, layer.opacity);

        for (int y = startY; y < endY; ++y)
        {
            for (int x = startX; x < endX; ++x)
            {
                int idx = y * layer.width + x;
                if (idx < 0 || idx >= static_cast<int>(layer.tiles.size()))
                    continue;

                int gid = layer.tiles[idx];
                if (gid <= 0)
                    continue;

                const Tileset* ts = FindTileset(gid);
                if (!ts || ts->textureHandle < 0)
                    continue;

                // タイルセット内のローカルID
                int localId = gid - ts->firstGid;
                int srcX = (localId % ts->columns) * ts->tileWidth;
                int srcY = (localId / ts->columns) * ts->tileHeight;

                float drawX = static_cast<float>(x * m_tileWidth);
                float drawY = static_cast<float>(y * m_tileHeight);

                batch.DrawRectGraph(drawX, drawY,
                    srcX, srcY, ts->tileWidth, ts->tileHeight,
                    ts->textureHandle, true);
            }
        }
    }

    // カラーをリセット
    batch.SetDrawColor(1.0f, 1.0f, 1.0f, 1.0f);
}

const Tileset* Tilemap::FindTileset(int gid) const
{
    const Tileset* result = nullptr;
    for (const auto& ts : m_tilesets)
    {
        if (gid >= ts.firstGid)
        {
            if (!result || ts.firstGid > result->firstGid)
                result = &ts;
        }
    }
    return result;
}

void Tilemap::WorldToTile(float worldX, float worldY, int& outTileX, int& outTileY) const
{
    outTileX = static_cast<int>(floorf(worldX / m_tileWidth));
    outTileY = static_cast<int>(floorf(worldY / m_tileHeight));
}

void Tilemap::TileToWorld(int tileX, int tileY, float& outWorldX, float& outWorldY) const
{
    outWorldX = static_cast<float>(tileX * m_tileWidth);
    outWorldY = static_cast<float>(tileY * m_tileHeight);
}

} // namespace GX
