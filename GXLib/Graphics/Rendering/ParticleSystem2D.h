#pragma once
/// @file ParticleSystem2D.h
/// @brief 2Dパーティクルシステム（複数エミッター管理）

#include "pch.h"
#include "Graphics/Rendering/ParticleEmitter2D.h"

namespace GX
{

/// @brief 2Dパーティクルシステム
///
/// 複数のParticleEmitter2Dを管理し、SpriteBatchで一括描画する。
class ParticleSystem2D
{
public:
    ParticleSystem2D() = default;
    ~ParticleSystem2D() = default;

    /// @brief エミッターを追加してハンドルを返す
    /// @param config エミッター設定
    /// @return エミッターハンドル
    int AddEmitter(const EmitterConfig2D& config);

    /// @brief エミッターを削除する
    /// @param handle AddEmitterの戻り値
    void RemoveEmitter(int handle);

    /// @brief エミッター位置を設定する
    void SetPosition(int handle, const Vector2& pos);
    void SetPosition(int handle, float x, float y);

    /// @brief バースト発射
    void Burst(int handle, int count);

    /// @brief エミッターを取得する
    ParticleEmitter2D* GetEmitter(int handle);

    /// @brief 全エミッターを更新する
    void Update(float deltaTime);

    /// @brief SpriteBatchに描画を発行する
    void Draw(SpriteBatch& batch);

    /// @brief 全エミッターをクリアする
    void Clear();

    /// @brief 生存パーティクル総数を取得する
    uint32_t GetAliveCount() const;

    /// @brief エミッター数を取得する
    int GetEmitterCount() const { return static_cast<int>(m_emitters.size()); }

private:
    struct EmitterEntry
    {
        ParticleEmitter2D emitter;
        bool valid = false;
    };

    std::vector<EmitterEntry> m_emitters;
    std::vector<int> m_freeList;
    int m_whiteTexture = -1; ///< テクスチャ無しパーティクル用の16x16白テクスチャ
};

} // namespace GX
