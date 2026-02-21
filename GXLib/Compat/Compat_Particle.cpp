/// @file Compat_Particle.cpp
/// @brief 簡易API 2Dパーティクル関数の実装
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatContext.h"
#include "Graphics/Rendering/ParticleSystem2D.h"
#include "Graphics/Rendering/ParticleEmitter2D.h"

using Ctx = GX_Internal::CompatContext;

int CreateParticle2D(int textureHandle, float x, float y, int count)
{
    auto& ctx = Ctx::Instance();

    GX::EmitterConfig2D config;
    config.textureHandle = textureHandle;
    config.burstCount = count;
    config.emissionRate = 0.0f;
    config.blendMode = GX::BlendMode::Add;
    config.maxParticles = static_cast<uint32_t>((std::max)(count, 100));

    int handle = ctx.particleSystem2D.AddEmitter(config);
    ctx.particleSystem2D.SetPosition(handle, x, y);

    if (count > 0)
    {
        ctx.particleSystem2D.Burst(handle, count);
    }

    return handle;
}

int UpdateParticles()
{
    auto& ctx = Ctx::Instance();
    ctx.particleSystem2D.Update(1.0f / 60.0f);
    return 0;
}

int DrawParticles()
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ctx.particleSystem2D.Draw(ctx.spriteBatch);
    return 0;
}
