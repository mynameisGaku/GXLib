#pragma once
/// @file Tilemap.h
/// @brief 2Dタイルマップ描画
///
/// TMX (Tiled Map Editor) 形式の XML からタイルマップを読み込み、
/// SpriteBatch を使って効率的に描画する。
/// カメラの可視範囲外のタイルは自動的にカリングされる。

#include "pch_graphics.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include "Graphics/Rendering/Camera2D.h"

namespace gx
{

/// @brief タイルセット情報
struct Tileset
{
    int firstGid = 1;       ///< このタイルセットの最初のタイルGID
    int tileWidth = 32;     ///< タイル1枚の幅（ピクセル）
    int tileHeight = 32;    ///< タイル1枚の高さ（ピクセル）
    int columns = 1;        ///< テクスチャ内のタイル列数
    int tileCount = 0;      ///< タイル総数
    int textureHandle = -1; ///< TextureManager のハンドル
};

/// @brief タイルマップレイヤー
struct TilemapLayer
{
    std::string name;           ///< レイヤー名
    int width = 0;              ///< タイル列数
    int height = 0;             ///< タイル行数
    std::vector<int> tiles;     ///< タイル GID 配列（0 = 空タイル）
    bool visible = true;        ///< 可視フラグ
    float opacity = 1.0f;       ///< 不透明度（0.0〜1.0）
};

/// @brief 2Dタイルマップ
///
/// TMX ファイルからマップを読み込み、SpriteBatch でタイルを描画する。
/// 各レイヤーのタイルデータとタイルセット（テクスチャ）を管理する。
class Tilemap
{
public:
    Tilemap() = default;
    ~Tilemap() = default;

    /// @brief TMX ファイルからタイルマップを読み込む
    /// @param path TMX ファイルパス
    /// @param texManager テクスチャマネージャ
    /// @return 成功なら true
    bool LoadFromTMX(const std::string& path, TextureManager& texManager);

    /// @brief タイルマップを手動で構築する
    /// @param width タイル列数
    /// @param height タイル行数
    /// @param tileWidth タイル幅（ピクセル）
    /// @param tileHeight タイル高さ（ピクセル）
    void Create(int width, int height, int tileWidth, int tileHeight);

    /// @brief タイルセットを追加する
    /// @param tileset タイルセット情報
    void AddTileset(const Tileset& tileset);

    /// @brief レイヤーを追加する
    /// @param layer タイルマップレイヤー
    void AddLayer(const TilemapLayer& layer);

    /// @brief 指定レイヤーの指定位置のタイルGIDを取得する
    /// @param layerIndex レイヤーインデックス
    /// @param x タイルX座標
    /// @param y タイルY座標
    /// @return タイルGID（範囲外は0）
    int GetTile(int layerIndex, int x, int y) const;

    /// @brief 指定レイヤーの指定位置にタイルを設定する
    /// @param layerIndex レイヤーインデックス
    /// @param x タイルX座標
    /// @param y タイルY座標
    /// @param gid タイルGID
    void SetTile(int layerIndex, int x, int y, int gid);

    /// @brief 全レイヤーを描画する（カメラカリング付き）
    /// @param batch SpriteBatch
    /// @param camera カメラ（nullptr時はカリングなし）
    /// @param screenWidth 画面幅（カメラ未使用時のカリング範囲）
    /// @param screenHeight 画面高さ
    void Draw(SpriteBatch& batch, const Camera2D* camera = nullptr,
              float screenWidth = 1280.0f, float screenHeight = 720.0f) const;

    /// @brief マップ幅を取得する（タイル単位）
    int GetWidth() const { return m_mapWidth; }

    /// @brief マップ高さを取得する（タイル単位）
    int GetHeight() const { return m_mapHeight; }

    /// @brief タイル幅を取得する（ピクセル）
    int GetTileWidth() const { return m_tileWidth; }

    /// @brief タイル高さを取得する（ピクセル）
    int GetTileHeight() const { return m_tileHeight; }

    /// @brief マップのピクセル幅を取得する
    int GetPixelWidth() const { return m_mapWidth * m_tileWidth; }

    /// @brief マップのピクセル高さを取得する
    int GetPixelHeight() const { return m_mapHeight * m_tileHeight; }

    /// @brief レイヤー数を取得する
    int GetLayerCount() const { return static_cast<int>(m_layers.size()); }

    /// @brief レイヤーを取得する
    /// @param index レイヤーインデックス
    /// @return レイヤーへの参照
    TilemapLayer& GetLayer(int index) { return m_layers[index]; }
    const TilemapLayer& GetLayer(int index) const { return m_layers[index]; }

    /// @brief ワールド座標からタイル座標を取得する
    /// @param worldX ワールドX座標
    /// @param worldY ワールドY座標
    /// @param outTileX 出力先タイルX
    /// @param outTileY 出力先タイルY
    void WorldToTile(float worldX, float worldY, int& outTileX, int& outTileY) const;

    /// @brief タイル座標からワールド座標（タイルの左上）を取得する
    /// @param tileX タイルX
    /// @param tileY タイルY
    /// @param outWorldX 出力先ワールドX
    /// @param outWorldY 出力先ワールドY
    void TileToWorld(int tileX, int tileY, float& outWorldX, float& outWorldY) const;

private:
    /// @brief タイルGIDに対応するタイルセットを検索する
    const Tileset* FindTileset(int gid) const;

    int m_mapWidth = 0;     ///< マップ幅（タイル数）
    int m_mapHeight = 0;    ///< マップ高さ（タイル数）
    int m_tileWidth = 32;   ///< タイル幅（ピクセル）
    int m_tileHeight = 32;  ///< タイル高さ（ピクセル）

    std::vector<Tileset> m_tilesets;
    std::vector<TilemapLayer> m_layers;
};

} // namespace gx
