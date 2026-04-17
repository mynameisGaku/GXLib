/// @file Compat_Particle.cpp
/// @brief 簡易API 2Dパーティクル関数の実装
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatContext.h"
#include "Graphics/Rendering/ParticleSystem2D.h"
#include "Graphics/Rendering/ParticleEmitter2D.h"
#include "Core/Logger.h"

using Ctx = gx_internal::CompatContext;

namespace gx {

int CreateParticle2D(int textureHandle, float x, float y, int count)
{
    if (count < 0)
    {
        GX_LOG_ERROR("CreateParticle2D: count=%d is negative", count);
        return -1;
    }

    auto& ctx = Ctx::Instance();

    EmitterConfig2D config;
    config.textureHandle = textureHandle;
    config.burstCount = count;
    config.emissionRate = 0.0f;
    config.blendMode = BlendMode::Add;
    config.maxParticles = static_cast<uint32_t>((std::max)(count, 100));

    int handle = ctx.particleSystem2D.AddEmitter(config);
    if (handle < 0)
    {
        GX_LOG_ERROR("CreateParticle2D: AddEmitter failed (pool exhausted?)");
        return -1;
    }

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
    ctx.particleSystem2D.Update(GetDeltaTime());
    return 0;
}

int DrawParticles()
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ctx.particleSystem2D.Draw(ctx.spriteBatch);
    return 0;
}

} // namespace gx
