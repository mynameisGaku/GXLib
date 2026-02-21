#pragma once
/// @file TrailRenderer.h
/// @brief トレイル（軌跡）レンダラー — 剣の軌跡・弾丸の尾等

#include "pch.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"
#include "Math/Color.h"

namespace GX
{

/// @brief トレイルの1ポイント
struct TrailPoint
{
    XMFLOAT3 position;      ///< ワールド位置
    XMFLOAT3 up;            ///< 幅方向（正規化推奨）
    float    width;         ///< 幅（片側）
    Color    color;         ///< 頂点カラー
    float    time;          ///< 追加時の経過時間（寿命管理用）
};

/// @brief トレイルレンダラー
///
/// 毎フレーム AddPoint() で先端位置を追加し、Update() で古いポイントを削除、
/// Draw() で帯状メッシュとして描画する。
class TrailRenderer
{
public:
    TrailRenderer() = default;
    ~TrailRenderer() = default;

    /// @brief 初期化する（動的頂点バッファとPSOの生成）
    /// @param device D3D12デバイス
    /// @param maxPoints リングバッファの最大ポイント数
    /// @return 成功ならtrue
    bool Initialize(ID3D12Device* device, uint32_t maxPoints = 256);

    /// @brief 新しいポイントを追加
    /// @param position ワールド位置
    /// @param up 幅方向ベクトル（正規化推奨）
    /// @param width 幅（片側）
    /// @param color 頂点カラー
    void AddPoint(const XMFLOAT3& position, const XMFLOAT3& up,
                  float width = 1.0f, const Color& color = Color::White());

    /// @brief 古いポイントを寿命で削除し、経過時間を更新
    /// @param deltaTime フレームの経過時間（秒）
    void Update(float deltaTime);

    /// @brief トレイルを描画（HDR RT に直接）
    /// @param cmdList コマンドリスト
    /// @param camera 3Dカメラ
    /// @param frameIndex フレームインデックス（ダブルバッファ用）
    /// @param texManager テクスチャマネージャー（textureHandle >= 0 の場合に使用）
    void Draw(ID3D12GraphicsCommandList* cmdList,
              const Camera3D& camera, uint32_t frameIndex,
              TextureManager* texManager = nullptr);

    /// @brief 全ポイントをクリア
    void Clear();

    /// @brief 現在のポイント数
    /// @return 有効ポイント数
    uint32_t GetPointCount() const { return m_pointCount; }

    // --- 設定 ---
    float lifetime = 1.0f;      ///< ポイントの寿命（秒）
    int   textureHandle = -1;   ///< テクスチャハンドル（-1=白テクスチャ不使用）
    bool  fadeWithAge = true;    ///< 古いほど透明にフェード

private:
    /// @brief トレイル描画用の頂点（位置＋UV＋色）
    struct TrailVertex
    {
        XMFLOAT3 position;
        XMFLOAT2 uv;
        XMFLOAT4 color;
    };

    /// @brief ポイント列から三角形ストリップの頂点を構築する
    /// @param outVerts 出力先の頂点配列
    void BuildVertices(std::vector<TrailVertex>& outVerts) const;

    /// @brief パイプラインステートオブジェクトを生成する
    /// @param device D3D12デバイス
    /// @return 成功ならtrue
    bool CreatePipelineState(ID3D12Device* device);

    // リングバッファ方式
    std::vector<TrailPoint> m_points;       ///< ポイントのリングバッファ
    uint32_t m_head = 0;                    ///< 次の書き込み位置
    uint32_t m_pointCount = 0;              ///< 有効ポイント数
    uint32_t m_maxPoints = 256;             ///< リングバッファのサイズ

    float m_elapsedTime = 0.0f;             ///< 累積時間

    DynamicBuffer m_vertexBuffer;           ///< 動的頂点バッファ（毎フレーム書き換え）
    DynamicBuffer m_constantBuffer;         ///< 定数バッファ（ViewProjection行列）

    Shader                      m_shader;
    ComPtr<ID3D12PipelineState> m_pso;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ID3D12Device* m_device = nullptr;
};

} // namespace GX
