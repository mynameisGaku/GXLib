/// @file compat_particle_test.cpp
/// @brief Compat_Particle regression tests (sprint-003 Task 1)
///
/// Guards the three defects fixed in Compat_Particle.cpp on 2026-04-17:
/// 1. CreateParticle2D: count < 0 now rejected with GX_LOG_ERROR + return -1
/// 2. CreateParticle2D: AddEmitter return guarded with `if (handle < 0)`
/// 3. UpdateParticles: uses GetDeltaTime() not a hardcoded 1/60
///
/// This unit test covers defect 1 specifically — the precondition check
/// runs BEFORE `CompatContext::Instance()` is touched, so no engine
/// initialisation is required. Defects 2 and 3 require a live
/// CompatContext (particle system + application timer) and are covered
/// by integration smoke tests run from reference apps, not unit tests.
#include <gtest/gtest.h>
#include "Compat/GXLib.h"

namespace {

/// Defect 1 regression: negative count must return -1 without dereferencing
/// the singleton context. This is the specific path the 2026-04-17 fix added.
TEST(CompatParticleTest, CreateParticle2DNegativeCountReturnsMinusOne)
{
    EXPECT_EQ(gx::CreateParticle2D(0, 0.0f, 0.0f, -1), -1);
}

TEST(CompatParticleTest, CreateParticle2DLargeNegativeCountReturnsMinusOne)
{
    // Regression: the pre-fix code cast a negative int to uint32_t for
    // maxParticles, producing an enormous allocation (~4 GB). Verify the
    // guard rejects pathological inputs.
    EXPECT_EQ(gx::CreateParticle2D(0, 0.0f, 0.0f, -1'000'000), -1);
}

TEST(CompatParticleTest, CreateParticle2DIntMinReturnsMinusOne)
{
    // Edge: INT_MIN also negative — guard must catch it.
    EXPECT_EQ(gx::CreateParticle2D(0, 0.0f, 0.0f,
                                    std::numeric_limits<int>::min()),
              -1);
}

} // namespace
