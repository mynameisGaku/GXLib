#pragma once
/// @file TextureManager.h
/// @brief ハンドルベースのテクスチャ管理
///
/// DxLibで LoadGraph() を呼ぶとint型の「ハンドル」が返るのと同じ仕組み。
/// ファイルパスとハンドルの対応をキャッシュし、同じ画像の二重読み込みを防ぐ。
/// 不要になったテクスチャは ReleaseTexture() でハンドルを解放でき、
/// そのハンドル番号はフリーリストを通じて再利用される。
///
/// DxLibの LoadDivGraph() に相当する分割テクスチャ（リージョン）機能もある。

#include "pch.h"
#include "Graphics/Resource/Texture.h"
#include "Graphics/Device/DescriptorHeap.h"
#include <unordered_map>

namespace GX
{

/// @brief テクスチャのUV矩形情報（スプライトシート用）
///
/// 1枚のテクスチャを格子状に分割し、各区画をUV座標で表現する。
/// DxLibの LoadDivGraph() で得られるハンドル群と同等の機能。
struct TextureRegion
{
    float u0 = 0.0f, v0 = 0.0f;  ///< 左上のUV座標
    float u1 = 1.0f, v1 = 1.0f;  ///< 右下のUV座標
    int textureHandle = -1;       ///< 参照先テクスチャのハンドル
};

/// @brief ハンドルベースのテクスチャ管理クラス
///
/// テクスチャの読み込み・キャッシュ・解放をハンドル(int)経由で管理する。
/// SRV用のshader-visibleディスクリプタヒープも内部に持つ。
class TextureManager
{
public:
    /// 同時に管理可能なテクスチャの最大数
    static constexpr uint32_t k_MaxTextures = 256;

    TextureManager() = default;
    ~TextureManager() = default;

    /// @brief マネージャーを初期化する
    /// @param device D3D12デバイス
    /// @param cmdQueue テクスチャアップロード用のコマンドキュー
    /// @return 成功時true
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);

    /// @brief 画像ファイルからテクスチャを読み込む
    ///
    /// 同じパスで2回以上呼んでもキャッシュから既存ハンドルを返す。
    /// @param filePath 画像ファイルのパス
    /// @return テクスチャハンドル。失敗時は-1
    int LoadTexture(const std::wstring& filePath);

    /// @brief CPUメモリのピクセルデータからテクスチャを作成する
    /// @param pixels RGBAピクセルデータ
    /// @param width テクスチャの幅
    /// @param height テクスチャの高さ
    /// @return テクスチャハンドル。失敗時は-1
    int CreateTextureFromMemory(const void* pixels, uint32_t width, uint32_t height);

    /// @brief 既存テクスチャのピクセルデータを差し替える
    /// @param handle 更新対象のテクスチャハンドル
    /// @param pixels 新しいRGBAピクセルデータ
    /// @param width テクスチャの幅（元と同じ値）
    /// @param height テクスチャの高さ（元と同じ値）
    /// @return 成功時true
    bool UpdateTextureFromMemory(int handle, const void* pixels, uint32_t width, uint32_t height);

    /// @brief ハンドルからTextureオブジェクトを取得する
    /// @param handle テクスチャハンドル
    /// @return Textureへのポインタ。無効なハンドルではnullptr
    Texture* GetTexture(int handle);

    /// @brief ハンドルからUV矩形情報を取得する
    /// @param handle テクスチャまたはリージョンのハンドル
    /// @return UV矩形情報への参照
    const TextureRegion& GetRegion(int handle) const;

    /// @brief ハンドルに対応するファイルパスを取得する
    /// @param handle テクスチャハンドル
    /// @return ファイルパスへの参照。メモリから作成した場合は空文字列
    const std::wstring& GetFilePath(int handle) const;

    /// @brief テクスチャを解放してハンドルを再利用可能にする
    /// @param handle 解放するテクスチャハンドル
    void ReleaseTexture(int handle);

    /// @brief 1枚のテクスチャを格子状に分割し、各区画のリージョンハンドルを作成する
    ///
    /// DxLibの LoadDivGraph() に相当する機能。テクスチャ自体は共有され、
    /// UV矩形情報だけが別ハンドルとして割り当てられる。
    /// @param baseHandle 分割元のテクスチャハンドル
    /// @param allNum 分割数の合計
    /// @param xNum 横方向の分割数
    /// @param yNum 縦方向の分割数
    /// @param xSize 1区画の幅（ピクセル）
    /// @param ySize 1区画の高さ（ピクセル）
    /// @return 最初のリージョンハンドル（連番でallNum個割り当て）。失敗時は-1
    int CreateRegionHandles(int baseHandle, int allNum, int xNum, int yNum, int xSize, int ySize);

    /// @brief shader-visibleなSRVディスクリプタヒープを取得する
    /// @return SRVヒープへの参照
    DescriptorHeap& GetSRVHeap() { return m_srvHeap; }

private:
    /// ハンドルを1つ割り当てる（フリーリスト優先）
    int AllocateHandle();

    ID3D12Device*       m_device   = nullptr;
    ID3D12CommandQueue* m_cmdQueue = nullptr;

    DescriptorHeap m_srvHeap; ///< テクスチャSRV用のshader-visibleヒープ

    /// テクスチャエントリ（テクスチャ実体 or UV矩形情報）
    struct TextureEntry
    {
        std::unique_ptr<Texture> texture;   ///< テクスチャ実体（リージョン専用時はnull）
        TextureRegion region;               ///< UV矩形情報
        std::wstring filePath;              ///< 読み込み元のファイルパス
        bool isRegionOnly = false;          ///< trueならUV矩形のみ（テクスチャは別ハンドルが所有）
    };

    std::vector<TextureEntry>                  m_entries;      ///< ハンドル→エントリのマッピング
    std::unordered_map<std::wstring, int>      m_pathCache;    ///< パス→ハンドルのキャッシュ
    std::vector<int>                           m_freeHandles;  ///< 解放済みハンドルの再利用リスト
    int                                        m_nextHandle = 0; ///< 次に割り当てるハンドル番号
};

} // namespace GX
