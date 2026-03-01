#pragma once
/// @file FrameGraph.h
/// @brief フレームグラフ（レンダーグラフ） -- パスとリソースを宣言し、バリアをコンパイルして実行する
///
/// シャドウ・Gバッファ・ライティング・ポストエフェクトなどの
/// 各レンダーパスをグラフとして宣言しておくと、
/// Compile() がトポロジカルソートとリソースバリアを自動計算し、
/// Execute() が正しい順序でパスを実行する。手書きバリアが不要になる。
/// @addtogroup grp_gfx_framegraph/// @{

#include "pch_graphics.h"
#include "Graphics/FrameGraph/ResourceNode.h"
#include "Graphics/FrameGraph/RenderPass.h"

namespace gx
{

/// @brief Compile()中に計算されるバリアエントリ
struct BarrierEntry
{
    uint32_t resourceId = 0;                                     ///< 対象リソースID
    D3D12_RESOURCE_STATES before = D3D12_RESOURCE_STATE_COMMON;  ///< 遷移前のステート
    D3D12_RESOURCE_STATES after = D3D12_RESOURCE_STATE_COMMON;   ///< 遷移後のステート
};

/// @brief フレームグラフ -- レンダーパスを整理し、リソースバリアを自動化する
///
/// 使用手順:
///   1. DeclareResource() -- GPUリソースを登録
///   2. AddPass() -- 読み書き宣言付きでレンダーパスを追加
///   3. Compile() -- トポロジカルソート + バリア最適化
///   4. Execute() -- 自動バリア付きで順番にパスを実行
///   5. Reset() -- 次フレームのためにフレーム単位の状態をクリア
class FrameGraph
{
public:
    FrameGraph() = default;
    ~FrameGraph() = default;

    /// @brief フレームグラフにテクスチャリソースを宣言する
    uint32_t DeclareResource(const std::string& name, DXGI_FORMAT format,
                              uint32_t width, uint32_t height,
                              D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON);

    /// @brief フレームグラフにバッファリソースを宣言する
    uint32_t DeclareBuffer(const std::string& name,
                            D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON);

    /// @brief 宣言済みリソースノードに物理ID3D12Resourceをバインドする
    void BindResource(uint32_t resourceId, ID3D12Resource* resource);

    /// @brief フレームグラフにレンダーパスを追加する
    uint32_t AddPass(const std::string& name,
                      std::function<void(const PassContext&)> executeFn);

    /// @brief パスが指定ステートでリソースを読み取ることを宣言する
    void PassReads(uint32_t passId, uint32_t resourceId,
                    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    /// @brief パスが指定ステートでリソースに書き込むことを宣言する
    void PassWrites(uint32_t passId, uint32_t resourceId,
                     D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_RENDER_TARGET);

    /// @brief パスを有効化または無効化する
    void SetPassEnabled(uint32_t passId, bool enabled);

    /// @brief フレームグラフをコンパイルする: トポロジカルソート + バリア生成
    /// @return コンパイル成功時（循環が検出されなかった場合）true
    bool Compile();

    /// @brief コンパイル順序で全有効パスを実行する
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);

    /// @brief フレーム単位の状態をリセットする（リソースステートを初期値に戻す）
    void Reset();

    /// @brief 全てをクリアする（リソース + パス）
    void Clear();

    /// @brief 宣言済みリソースの数を取得する
    uint32_t GetResourceCount() const { return static_cast<uint32_t>(m_resources.size()); }

    /// @brief パスの数を取得する
    uint32_t GetPassCount() const { return static_cast<uint32_t>(m_passes.size()); }

    /// @brief コンパイル済みパスの実行順序を取得する
    const std::vector<uint32_t>& GetExecutionOrder() const { return m_executionOrder; }

    /// @brief 特定パス（実行インデックス指定）のコンパイル済みバリアを取得する
    const std::vector<BarrierEntry>& GetBarriersForPass(uint32_t executionIndex) const;

    /// @brief グラフがコンパイル済みかどうかを確認する
    bool IsCompiled() const { return m_compiled; }

private:
    bool TopologicalSort();
    void GenerateBarriers();

    std::vector<ResourceNode> m_resources;                    ///< 宣言済みリソースノード
    std::vector<RenderPass> m_passes;                         ///< 登録済みレンダーパス
    std::vector<uint32_t> m_executionOrder;                   ///< コンパイル済み実行順序
    std::vector<std::vector<BarrierEntry>> m_passBarriers;    ///< パスごとのバリアエントリ（実行順序でインデックス）
    bool m_compiled = false;                                  ///< コンパイル済みフラグ

    static const std::vector<BarrierEntry> s_emptyBarriers;
};

} // namespace gx
/// @}
