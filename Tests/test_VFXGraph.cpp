/// @file test_VFXGraph.cpp
/// @brief VFXGraph（ビジュアルエフェクトグラフ）のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/VFXGraph.h"

using namespace gx;

// ============================================================================
// VFXRange
// ============================================================================

TEST(VFXRangeTest, EvaluateAtZeroReturnsMin)
{
    VFXRange range;
    range.min = 2.0f;
    range.max = 8.0f;
    EXPECT_FLOAT_EQ(range.Evaluate(0.0f), 2.0f);
}

TEST(VFXRangeTest, EvaluateAtOneReturnsMax)
{
    VFXRange range;
    range.min = 2.0f;
    range.max = 8.0f;
    EXPECT_FLOAT_EQ(range.Evaluate(1.0f), 8.0f);
}

TEST(VFXRangeTest, EvaluateAtHalfReturnsMidpoint)
{
    VFXRange range;
    range.min = 0.0f;
    range.max = 10.0f;
    EXPECT_FLOAT_EQ(range.Evaluate(0.5f), 5.0f);
}

TEST(VFXRangeTest, GetRandomReturnsValueInRange)
{
    VFXRange range;
    range.min = 5.0f;
    range.max = 15.0f;
    for (int i = 0; i < 100; ++i)
    {
        float val = range.GetRandom();
        EXPECT_GE(val, 5.0f);
        EXPECT_LE(val, 15.0f);
    }
}

// ============================================================================
// VFXCurve
// ============================================================================

TEST(VFXCurveTest, EmptyCurveEvaluatesToZero)
{
    VFXCurve curve;
    EXPECT_FLOAT_EQ(curve.Evaluate(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(curve.Evaluate(0.5f), 0.0f);
    EXPECT_FLOAT_EQ(curve.Evaluate(1.0f), 0.0f);
}

TEST(VFXCurveTest, SingleKeyReturnsItsValue)
{
    VFXCurve curve;
    curve.keys.push_back({ 0.5f, 7.0f });
    EXPECT_FLOAT_EQ(curve.Evaluate(0.0f), 7.0f);
    EXPECT_FLOAT_EQ(curve.Evaluate(0.5f), 7.0f);
    EXPECT_FLOAT_EQ(curve.Evaluate(1.0f), 7.0f);
}

TEST(VFXCurveTest, LinearCurveInterpolates)
{
    VFXCurve curve;
    curve.keys.push_back({ 0.0f, 0.0f });
    curve.keys.push_back({ 1.0f, 10.0f });
    EXPECT_FLOAT_EQ(curve.Evaluate(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(curve.Evaluate(0.25f), 2.5f);
    EXPECT_FLOAT_EQ(curve.Evaluate(0.5f), 5.0f);
    EXPECT_FLOAT_EQ(curve.Evaluate(0.75f), 7.5f);
    EXPECT_FLOAT_EQ(curve.Evaluate(1.0f), 10.0f);
}

TEST(VFXCurveTest, ConstantFactory)
{
    VFXCurve curve = VFXCurve::Constant(3.14f);
    EXPECT_EQ(curve.keys.size(), 2u);
    EXPECT_FLOAT_EQ(curve.Evaluate(0.0f), 3.14f);
    EXPECT_FLOAT_EQ(curve.Evaluate(0.5f), 3.14f);
    EXPECT_FLOAT_EQ(curve.Evaluate(1.0f), 3.14f);
}

TEST(VFXCurveTest, LinearFactory)
{
    VFXCurve curve = VFXCurve::Linear(1.0f, 5.0f);
    EXPECT_EQ(curve.keys.size(), 2u);
    EXPECT_FLOAT_EQ(curve.Evaluate(0.0f), 1.0f);
    EXPECT_FLOAT_EQ(curve.Evaluate(0.5f), 3.0f);
    EXPECT_FLOAT_EQ(curve.Evaluate(1.0f), 5.0f);
}

TEST(VFXCurveTest, ClampsBelowFirstKey)
{
    VFXCurve curve;
    curve.keys.push_back({ 0.2f, 10.0f });
    curve.keys.push_back({ 0.8f, 20.0f });
    // t < 最初のキーの時間: 最初のキーの値を返す
    EXPECT_FLOAT_EQ(curve.Evaluate(0.0f), 10.0f);
}

TEST(VFXCurveTest, ClampsAboveLastKey)
{
    VFXCurve curve;
    curve.keys.push_back({ 0.2f, 10.0f });
    curve.keys.push_back({ 0.8f, 20.0f });
    // t > 最後のキーの時間: 最後のキーの値を返す
    EXPECT_FLOAT_EQ(curve.Evaluate(1.0f), 20.0f);
}

// ============================================================================
// VFXEffect
// ============================================================================

TEST(VFXEffectTest, AddEmitterReturnsIndex)
{
    VFXEffect effect;
    VFXEmitter em;
    em.name = "fire";
    uint32_t idx = effect.AddEmitter(em);
    EXPECT_EQ(idx, 0u);
    uint32_t idx2 = effect.AddEmitter(em);
    EXPECT_EQ(idx2, 1u);
}

TEST(VFXEffectTest, GetEmitter)
{
    VFXEffect effect;
    VFXEmitter em;
    em.name = "spark";
    em.emitRate = 50.0f;
    effect.AddEmitter(em);
    const VFXEmitter& retrieved = effect.GetEmitter(0);
    EXPECT_EQ(retrieved.name, "spark");
    EXPECT_FLOAT_EQ(retrieved.emitRate, 50.0f);
}

TEST(VFXEffectTest, GetEmitterCount)
{
    VFXEffect effect;
    EXPECT_EQ(effect.GetEmitterCount(), 0u);
    effect.AddEmitter(VFXEmitter{});
    EXPECT_EQ(effect.GetEmitterCount(), 1u);
    effect.AddEmitter(VFXEmitter{});
    EXPECT_EQ(effect.GetEmitterCount(), 2u);
}

TEST(VFXEffectTest, AddModuleAddsToEmitter)
{
    VFXEffect effect;
    VFXEmitter em;
    em.name = "test";
    effect.AddEmitter(em);

    VFXModule mod;
    mod.type = VFXModuleType::Gravity;
    mod.gravityStrength = -15.0f;
    effect.AddModule(0, mod);

    const VFXEmitter& retrieved = effect.GetEmitter(0);
    ASSERT_EQ(retrieved.modules.size(), 1u);
    EXPECT_EQ(retrieved.modules[0].type, VFXModuleType::Gravity);
    EXPECT_FLOAT_EQ(retrieved.modules[0].gravityStrength, -15.0f);
}

TEST(VFXEffectTest, SetAndGetName)
{
    VFXEffect effect;
    effect.SetName("explosion_01");
    EXPECT_EQ(effect.GetName(), "explosion_01");
}

// ============================================================================
// VFXEmitterデフォルト値
// ============================================================================

TEST(VFXEmitterTest, DefaultValues)
{
    VFXEmitter em;
    EXPECT_FLOAT_EQ(em.emitRate, 10.0f);
    EXPECT_EQ(em.maxParticles, 1000);
    EXPECT_TRUE(em.looping);
    EXPECT_FLOAT_EQ(em.duration, 5.0f);
    EXPECT_FLOAT_EQ(em.startDelay, 0.0f);
    EXPECT_EQ(em.shape, VFXEmitterShape::Point);
    EXPECT_TRUE(em.worldSpace);
}

// ============================================================================
// VFXModuleデフォルト値
// ============================================================================

TEST(VFXModuleTest, DefaultEnabledState)
{
    VFXModule mod;
    EXPECT_TRUE(mod.enabled);
}

// ============================================================================
// VFXInstance
// ============================================================================

TEST(VFXInstanceTest, PlaySetsState)
{
    VFXInstance inst;
    VFXEffect effect;
    VFXEmitter em;
    em.looping = false;
    em.duration = 2.0f;
    effect.AddEmitter(em);
    inst.SetEffect(&effect);

    inst.Play();
    EXPECT_TRUE(inst.IsPlaying());
    EXPECT_FALSE(inst.IsFinished());
    EXPECT_FLOAT_EQ(inst.GetElapsedTime(), 0.0f);
}

TEST(VFXInstanceTest, StopSetsState)
{
    VFXInstance inst;
    VFXEffect effect;
    effect.AddEmitter(VFXEmitter{});
    inst.SetEffect(&effect);
    inst.Play();
    inst.Stop();
    EXPECT_FALSE(inst.IsPlaying());
    EXPECT_TRUE(inst.IsFinished());
}

TEST(VFXInstanceTest, PauseAndResume)
{
    VFXInstance inst;
    VFXEffect effect;
    VFXEmitter em;
    em.duration = 10.0f;
    effect.AddEmitter(em);
    inst.SetEffect(&effect);
    inst.Play();

    inst.Pause();
    EXPECT_TRUE(inst.IsPlaying()); // まだ「再生」状態、一時停止しただけ
    float elapsed = inst.GetElapsedTime();
    inst.Update(1.0f);
    EXPECT_FLOAT_EQ(inst.GetElapsedTime(), elapsed); // 一時停止中は進行しない

    inst.Resume();
    inst.Update(1.0f);
    EXPECT_GT(inst.GetElapsedTime(), elapsed); // 再開後は進行する
}

TEST(VFXInstanceTest, IsFinishedAfterDurationNonLooping)
{
    VFXInstance inst;
    VFXEffect effect;
    VFXEmitter em;
    em.looping = false;
    em.duration = 1.0f;
    em.startDelay = 0.0f;
    effect.AddEmitter(em);
    inst.SetEffect(&effect);
    inst.Play();

    // 持続時間を超えて進める
    inst.Update(2.0f);
    EXPECT_TRUE(inst.IsFinished());
    EXPECT_FALSE(inst.IsPlaying());
}

TEST(VFXInstanceTest, UpdateAdvancesElapsed)
{
    VFXInstance inst;
    VFXEffect effect;
    VFXEmitter em;
    em.duration = 10.0f;
    effect.AddEmitter(em);
    inst.SetEffect(&effect);
    inst.Play();

    inst.Update(0.5f);
    EXPECT_NEAR(inst.GetElapsedTime(), 0.5f, 0.001f);
    inst.Update(0.3f);
    EXPECT_NEAR(inst.GetElapsedTime(), 0.8f, 0.001f);
}

TEST(VFXInstanceTest, SetAndGetPosition)
{
    VFXInstance inst;
    inst.SetPosition({ 1.0f, 2.0f, 3.0f });
    XMFLOAT3 pos = inst.GetPosition();
    EXPECT_FLOAT_EQ(pos.x, 1.0f);
    EXPECT_FLOAT_EQ(pos.y, 2.0f);
    EXPECT_FLOAT_EQ(pos.z, 3.0f);
}

TEST(VFXInstanceTest, SetAndGetScale)
{
    VFXInstance inst;
    EXPECT_FLOAT_EQ(inst.GetScale(), 1.0f); // デフォルト
    inst.SetScale(2.5f);
    EXPECT_FLOAT_EQ(inst.GetScale(), 2.5f);
}

TEST(VFXInstanceTest, SetEffect)
{
    VFXInstance inst;
    EXPECT_EQ(inst.GetEffect(), nullptr);
    VFXEffect effect;
    inst.SetEffect(&effect);
    EXPECT_EQ(inst.GetEffect(), &effect);
}
