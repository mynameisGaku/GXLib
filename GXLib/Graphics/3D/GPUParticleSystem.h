#pragma once
/// @file GPUParticleSystem.h
/// @brief Compute Shader駆動のGPUパーティクルシステム
///
/// パーティクルの生成・物理更新・描画を全てGPU上で行う高性能パーティクルシステム。
/// CPUパーティクルシステム（ParticleSystem3D）と異なり、10万粒子規模を
/// フレーム落ちなく処理できる。
///
/// アーキテクチャ:
///   - 単一のRWStructuredBuffer<GPUParticle>パーティクルプール（リングバッファ方式）
///   - Init CS: 全スロットをlife=0（死亡）に初期化
///   - Emit CS: リングバッファインデックスからN個のパーティクルを書き込み
///   - Update CS: 全スロットを走査し、life>0の粒子に物理演算（重力・抗力・寿命減衰）
///   - Draw VS/PS: SV_VertexIDでビルボードクワッドを生成、life<=0の粒子は退化三角形で不可視化
///
/// 使い方:
///   GPUParticleSystem particles;
///   particles.Initialize(device, cmdQueue);
///   particles.SetEmitPosition({0, 0, 0});
///   particles.Emit(1000);  // 1000粒子バースト
///   particles.Update(cmdList, dt, frameIndex);
///   particles.Draw(cmdList, camera, frameIndex);

#include "pch_graphics.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Math/Vector3.h"
#include "Math/Color.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

class Camera3D;

/// @brief エミッター形状
enum class EmitterShape
{
    Point,      ///< 点
    Sphere,     ///< 球体
    Box,        ///< ボックス
    Cone,       ///< 円錐
    Ring,       ///< リング
};

/// @brief パーティクルブレンドモード
enum class ParticleBlendMode
{
    Alpha,      ///< アルファブレンド
    Additive,   ///< 加算ブレンド
    Multiply,   ///< 乗算ブレンド
};

/// @brief GPUパーティクルエミッター構成（VFXGraphからの適用用）
struct GPUParticleEmitterConfig
{
    uint32_t maxParticles = 1000;       ///< 最大パーティクル数
    float emissionRate = 100.0f;        ///< 毎秒放出数
    int textureHandle = -1;             ///< テクスチャハンドル

    EmitterShape shape = EmitterShape::Point;                  ///< エミッター形状
    ParticleBlendMode blendMode = ParticleBlendMode::Additive; ///< ブレンドモード

    Vector3 initialVelocityMin = { -1.0f, 1.0f, -1.0f };   ///< 初期速度最小
    Vector3 initialVelocityMax = { 1.0f, 3.0f, 1.0f };     ///< 初期速度最大
    float startSizeMin = 0.1f;          ///< 開始サイズ最小
    float startSizeMax = 0.3f;          ///< 開始サイズ最大
    float endSize = 0.0f;               ///< 終了サイズ
    float lifetimeMin = 1.0f;           ///< 寿命最小（秒）
    float lifetimeMax = 3.0f;           ///< 寿命最大（秒）
    float drag = 0.1f;                  ///< 抗力係数

    Color startColor = { 1.0f, 1.0f, 1.0f, 1.0f };  ///< 開始カラー
    Color endColor = { 1.0f, 1.0f, 1.0f, 0.0f };    ///< 終了カラー
    Vector3 gravity = { 0.0f, -9.81f, 0.0f };        ///< 重力ベクトル
};

/// @brief GPU上でパーティクルの発生・更新・描画を行うシステム
///
/// Compute Shaderでパーティクルの物理シミュレーションを実行し、
/// ビルボードクワッドで描画する。最大10万パーティクルに対応。
class GPUParticleSystem
{
public:
    GPUParticleSystem() = default;
    ~GPUParticleSystem() = default;

    /// @brief 初期化（バッファ、PSO、ルートシグネチャ作成）
    /// @param device D3D12デバイス
    /// @param cmdQueue コマンドキュー（初期化ディスパッチのFlush用）
    /// @param maxParticles 最大パーティクル数（デフォルト100000）
    /// @return 成功時true
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                    uint32_t maxParticles = 100000);

    /// @brief 簡易初期化（nullptrデバイス対応、CPUモード用）
    /// @param device D3D12デバイス（nullptrでCPUモード）
    /// @param maxTotalParticles 最大パーティクル総数
    /// @return 成功時true
    bool Initialize(std::nullptr_t, uint32_t maxTotalParticles);

    /// @brief エミッターを破棄する
    /// @param emitterId エミッターID
    void DestroyEmitter(int emitterId);

    /// @brief エミッターの位置を設定する
    /// @param emitterId エミッターID
    /// @param position ワールド座標
    void SetEmitterPosition(int emitterId, const XMFLOAT3& position);

    /// @brief エミッターの有効/無効を設定する
    /// @param emitterId エミッターID
    /// @param enabled 有効フラグ
    void SetEmitterEnabled(int emitterId, bool enabled);

    /// @brief 指定エミッターからバースト放出する
    /// @param emitterId エミッターID
    /// @param count 放出数
    void Burst(int emitterId, uint32_t count);

    /// @brief 最大パーティクル総数を取得する
    uint32_t GetMaxTotalParticles() const { return m_maxTotalParticles; }

    /// @brief 現在のエミッター数を取得する
    uint32_t GetEmitterCount() const { return static_cast<uint32_t>(m_emitters.size()); }

    /// @brief アクティブなパーティクル数を取得する（CPUモードでは0）
    uint32_t GetActiveParticleCount() const { return m_activeParticleCount; }

    /// @brief エミッターの位置を設定
    /// @param pos ワールド座標
    void SetEmitPosition(const XMFLOAT3& pos) { m_emitPosition = pos; }

    /// @brief 重力を設定
    /// @param gravity 重力ベクトル（デフォルト: 0, -9.8, 0）
    void SetGravity(const XMFLOAT3& gravity) { m_gravity = gravity; }

    /// @brief 抗力係数を設定
    /// @param drag 速度減衰率（0=なし、1=即停止）
    void SetDrag(float drag) { m_drag = drag; }

    /// @brief 速度範囲を設定
    /// @param vmin 最小速度
    /// @param vmax 最大速度
    void SetVelocityRange(const XMFLOAT3& vmin, const XMFLOAT3& vmax)
    {
        m_velocityMin = vmin;
        m_velocityMax = vmax;
    }

    /// @brief 寿命範囲を設定（秒）
    /// @param lifeMin 最小寿命
    /// @param lifeMax 最大寿命
    void SetLifeRange(float lifeMin, float lifeMax)
    {
        m_lifeMin = lifeMin;
        m_lifeMax = lifeMax;
    }

    /// @brief サイズ範囲を設定（開始→終了で補間）
    /// @param startSize 発生時サイズ
    /// @param endSize 消滅時サイズ
    void SetSizeRange(float startSize, float endSize)
    {
        m_sizeStart = startSize;
        m_sizeEnd = endSize;
    }

    /// @brief カラー範囲を設定（開始→終了で補間）
    /// @param startColor 発生時カラー（RGBA）
    /// @param endColor 消滅時カラー（RGBA）
    void SetColorRange(const XMFLOAT4& startColor, const XMFLOAT4& endColor)
    {
        m_colorStart = startColor;
        m_colorEnd = endColor;
    }

    /// @brief パーティクルをバースト放出
    /// @param count 放出するパーティクル数
    void Emit(uint32_t count);

    /// @brief Compute Shaderでパーティクルを更新
    /// @param cmdList コマンドリスト
    /// @param deltaTime フレーム経過時間（秒）
    /// @param frameIndex フレームインデックス（0 or 1）
    void Update(ID3D12GraphicsCommandList* cmdList, float deltaTime, uint32_t frameIndex);

    /// @brief ビルボード描画
    /// @param cmdList コマンドリスト
    /// @param camera 描画カメラ（ビルボード展開用）
    /// @param frameIndex フレームインデックス
    void Draw(ID3D12GraphicsCommandList* cmdList,
              const Camera3D& camera, uint32_t frameIndex);

    /// @brief 最大パーティクル数
    /// @return 最大パーティクル数
    uint32_t GetMaxParticles() const { return m_maxParticles; }

    /// @brief デプスバッファ衝突を有効化する
    /// @param enabled 衝突有効フラグ
    void SetCollisionEnabled(bool enabled) { m_collisionEnabled = enabled; }
    bool IsCollisionEnabled() const { return m_collisionEnabled; }

    /// @brief バウンス係数（反発係数）
    void SetBounceFactor(float factor) { m_bounceFactor = factor; }
    float GetBounceFactor() const { return m_bounceFactor; }

    /// @brief 摩擦係数
    void SetFriction(float friction) { m_friction = friction; }
    float GetFriction() const { return m_friction; }

    /// @brief エミッター構成からパラメータを一括適用し、エミッターIDを返す
    /// @param config エミッター構成
    /// @return エミッターID（1以上）
    int CreateEmitter(const GPUParticleEmitterConfig& config);

    /// @brief リソース解放
    void Shutdown();

private:
    /// @brief GPU上のパーティクルデータ（96バイト）
    ///
    /// Compute ShaderとDraw Shaderの両方から参照される。
    /// life <= 0 のスロットは「死亡」として扱われる。
    struct GPUParticle
    {
        XMFLOAT3 position;    ///< ワールド座標
        float    life;         ///< 残り寿命（秒）。0以下で死亡
        XMFLOAT3 velocity;    ///< 速度ベクトル（m/s）
        float    maxLife;      ///< 初期寿命（補間計算用）
        float    size;         ///< 現在のサイズ
        float    startSize;    ///< 発生時サイズ
        float    endSize;      ///< 消滅時サイズ
        float    rotation;     ///< Z軸回転（ラジアン）
        XMFLOAT4 color;       ///< 現在のカラー（RGBA）
        XMFLOAT4 startColor;  ///< 発生時カラー
        XMFLOAT4 endColor;    ///< 消滅時カラー
    }; // 96 bytes

    /// @brief Update CS用定数バッファ（96バイト）
    struct alignas(16) UpdateCB
    {
        float    deltaTime;      ///< フレーム経過時間
        XMFLOAT3 gravity;        ///< 重力ベクトル
        float    drag;           ///< 抗力係数
        uint32_t maxParticles;   ///< 最大パーティクル数
        uint32_t collisionEnabled; ///< デプスバッファ衝突有効
        float    bounceFactor;   ///< 反発係数
        float    friction;       ///< 摩擦係数
        XMFLOAT2 screenSize;    ///< 画面サイズ
        float    _pad;
        XMFLOAT4X4 viewProj;    ///< ビュー射影行列
        XMFLOAT4X4 invViewProj; ///< 逆ビュー射影行列
    }; // 160 bytes

    /// @brief Emit CS用定数バッファ（112バイト）
    struct alignas(16) EmitCB
    {
        uint32_t emitCount;       ///< 今回放出する数
        XMFLOAT3 emitPosition;   ///< エミッター位置
        XMFLOAT3 velocityMin;    ///< 速度最小値
        float    _pad0;
        XMFLOAT3 velocityMax;    ///< 速度最大値
        float    _pad1;
        float    lifeMin;         ///< 寿命最小値
        float    lifeMax;         ///< 寿命最大値
        float    sizeStart;       ///< 開始サイズ
        float    sizeEnd;         ///< 終了サイズ
        XMFLOAT4 colorStart;     ///< 開始カラー
        XMFLOAT4 colorEnd;       ///< 終了カラー
        uint32_t randomSeed;      ///< ランダムシード
        uint32_t emitOffset;      ///< リングバッファ書き込み開始位置
        float    _pad2[2];
    }; // 112 bytes

    /// @brief Draw VS/PS用定数バッファ（96バイト）
    struct alignas(16) DrawCB
    {
        XMFLOAT4X4 viewProj;     ///< ビュー射影行列（転置済み）
        XMFLOAT3   cameraRight;  ///< カメラ右方向ベクトル
        float      _pad0;
        XMFLOAT3   cameraUp;     ///< カメラ上方向ベクトル
        float      _pad1;
    }; // 96 bytes

    // --- GPU Buffers ---
    Buffer m_particleBuffer;    ///< DEFAULT heap, UAV — パーティクルプール
    Buffer m_counterBuffer;     ///< DEFAULT heap, UAV — リングバッファカウンター
    Buffer m_counterReadback;   ///< READBACK heap — CPU読み取り用カウンター
    Buffer m_counterUpload;     ///< UPLOAD heap — カウンターリセット用

    // --- Compute PSOs ---
    ComPtr<ID3D12PipelineState> m_initPSO;     ///< 初期化Compute PSO
    ComPtr<ID3D12PipelineState> m_emitPSO;     ///< 放出Compute PSO
    ComPtr<ID3D12PipelineState> m_updatePSO;   ///< 更新Compute PSO
    ComPtr<ID3D12RootSignature> m_computeRS;   ///< Compute用ルートシグネチャ

    // --- Draw PSO ---
    ComPtr<ID3D12PipelineState> m_drawPSO;     ///< 描画Graphics PSO
    ComPtr<ID3D12RootSignature> m_drawRS;      ///< 描画用ルートシグネチャ

    // --- Descriptor heap (SRV/UAV for compute + draw) ---
    DescriptorHeap m_srvUavHeap;

    // --- DynamicBuffers for per-frame CBs ---
    DynamicBuffer m_updateCBBuffer;
    DynamicBuffer m_emitCBBuffer;
    DynamicBuffer m_drawCBBuffer;

    // --- パラメータ ---
    uint32_t m_maxParticles = 100000;
    uint32_t m_emitRingIndex = 0;        ///< リングバッファ書き込み位置
    uint32_t m_pendingEmitCount = 0;     ///< 次のUpdateで放出する数
    uint32_t m_frameCounter = 0;         ///< ランダムシード用フレームカウンター

    XMFLOAT3 m_emitPosition = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 m_gravity = { 0.0f, -9.8f, 0.0f };
    float    m_drag = 0.02f;

    // --- Emit parameters ---
    XMFLOAT3 m_velocityMin = { -2.0f, 5.0f, -2.0f };
    XMFLOAT3 m_velocityMax = { 2.0f, 15.0f, 2.0f };
    float    m_lifeMin = 1.0f;
    float    m_lifeMax = 3.0f;
    float    m_sizeStart = 0.2f;
    float    m_sizeEnd = 0.0f;
    XMFLOAT4 m_colorStart = { 1.0f, 0.8f, 0.2f, 1.0f };
    XMFLOAT4 m_colorEnd = { 1.0f, 0.2f, 0.0f, 0.0f };

    // --- SRV/UAV slot indices in m_srvUavHeap ---
    uint32_t m_particleUAVSlot = 0;    ///< パーティクルバッファUAV
    uint32_t m_counterUAVSlot = 0;     ///< カウンターバッファUAV
    uint32_t m_particleSRVSlot = 0;    ///< パーティクルバッファSRV（描画用）

    bool m_collisionEnabled = false;
    float m_bounceFactor = 0.5f;
    float m_friction = 0.1f;

    bool m_initialized = false;
    bool m_poolInitialized = false;     ///< Init CSが実行済みか

    // --- Multi-emitter CPU management ---
    struct EmitterInstance
    {
        GPUParticleEmitterConfig config;
        XMFLOAT3 position = { 0, 0, 0 };
        bool enabled = true;
        int id = 0;
    };
    std::vector<EmitterInstance> m_emitters;
    int m_nextEmitterId = 1;
    uint32_t m_maxTotalParticles = 0;
    uint32_t m_activeParticleCount = 0;

    Shader m_shader;

    /// @brief GPU DEFAULT heapバッファを作成する
    bool CreateBuffers(ID3D12Device* device);

    /// @brief Compute/Graphics PSOとルートシグネチャを作成する
    bool CreatePSOs(ID3D12Device* device);

    /// @brief SRV/UAVディスクリプタを作成する
    void CreateDescriptors(ID3D12Device* device);

    /// @brief Init CSでパーティクルプールを初期化する
    void InitializeParticlePool(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
};

/// @}
} // namespace gx
