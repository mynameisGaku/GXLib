/// @file test_ParticleEditor.cpp
#include <gtest/gtest.h>
#include "Editor/ParticleEditor.h"
using namespace gx;

// ParticleCurve tests
TEST(ParticleCurveTest, EmptyReturnsZero) {
    ParticleCurve c; EXPECT_FLOAT_EQ(c.Evaluate(0.5f), 0.0f);
}
TEST(ParticleCurveTest, SinglePoint) {
    ParticleCurve c; c.AddPoint(0.5f, 3.0f);
    EXPECT_FLOAT_EQ(c.Evaluate(0.0f), 3.0f);
}
TEST(ParticleCurveTest, LinearInterpolation) {
    ParticleCurve c; c.AddPoint(0.0f, 0.0f); c.AddPoint(1.0f, 1.0f);
    EXPECT_NEAR(c.Evaluate(0.5f), 0.5f, 0.01f);
}
TEST(ParticleCurveTest, ClampBefore) {
    ParticleCurve c; c.AddPoint(0.2f, 1.0f); c.AddPoint(0.8f, 2.0f);
    EXPECT_FLOAT_EQ(c.Evaluate(0.0f), 1.0f);
}
TEST(ParticleCurveTest, ClampAfter) {
    ParticleCurve c; c.AddPoint(0.2f, 1.0f); c.AddPoint(0.8f, 2.0f);
    EXPECT_FLOAT_EQ(c.Evaluate(1.0f), 2.0f);
}
TEST(ParticleCurveTest, SortedInsertion) {
    ParticleCurve c; c.AddPoint(0.8f, 2.0f); c.AddPoint(0.2f, 1.0f); c.AddPoint(0.5f, 1.5f);
    EXPECT_EQ(c.GetPointCount(), 3u);
    EXPECT_NEAR(c.Evaluate(0.5f), 1.5f, 0.01f);
}
TEST(ParticleCurveTest, Clear) {
    ParticleCurve c; c.AddPoint(0.0f, 1.0f); c.Clear();
    EXPECT_EQ(c.GetPointCount(), 0u);
}

// ColorCurve tests
TEST(ColorCurveTest, Evaluate) {
    ColorCurve cc;
    cc.r.AddPoint(0.0f, 1.0f); cc.r.AddPoint(1.0f, 0.0f);
    cc.g.AddPoint(0.0f, 0.0f); cc.g.AddPoint(1.0f, 1.0f);
    cc.b.AddPoint(0.0f, 0.5f); cc.b.AddPoint(1.0f, 0.5f);
    cc.a.AddPoint(0.0f, 1.0f); cc.a.AddPoint(1.0f, 0.0f);
    float r, g, b, a;
    cc.Evaluate(0.5f, r, g, b, a);
    EXPECT_NEAR(r, 0.5f, 0.01f);
    EXPECT_NEAR(g, 0.5f, 0.01f);
    EXPECT_NEAR(a, 0.5f, 0.01f);
}

// ParticleEditor tests
TEST(ParticleEditorTest, DefaultState) {
    ParticleEditor pe;
    EXPECT_FALSE(pe.IsPlaying());
    EXPECT_FALSE(pe.IsPaused());
    EXPECT_FALSE(pe.IsDirty());
    EXPECT_FLOAT_EQ(pe.GetPlaybackTime(), 0.0f);
}
TEST(ParticleEditorTest, EmissionRate) {
    ParticleEditor pe;
    pe.SetEmissionRate(25.0f);
    EXPECT_FLOAT_EQ(pe.GetEmissionRate(), 25.0f);
}
TEST(ParticleEditorTest, EmissionRateClamp) {
    ParticleEditor pe;
    pe.SetEmissionRate(-5.0f);
    EXPECT_GE(pe.GetEmissionRate(), 0.0f);
}
TEST(ParticleEditorTest, MaxParticles) {
    ParticleEditor pe;
    pe.SetMaxParticles(5000);
    EXPECT_EQ(pe.GetMaxParticles(), 5000u);
}
TEST(ParticleEditorTest, Lifetime) {
    ParticleEditor pe;
    pe.SetLifetime(3.0f);
    EXPECT_FLOAT_EQ(pe.GetLifetime(), 3.0f);
}
TEST(ParticleEditorTest, LifetimeMinClamp) {
    ParticleEditor pe;
    pe.SetLifetime(-1.0f);
    EXPECT_GT(pe.GetLifetime(), 0.0f);
}
TEST(ParticleEditorTest, StartSpeed) {
    ParticleEditor pe;
    pe.SetStartSpeed(8.0f);
    EXPECT_FLOAT_EQ(pe.GetStartSpeed(), 8.0f);
}
TEST(ParticleEditorTest, StartSize) {
    ParticleEditor pe;
    pe.SetStartSize(0.5f);
    EXPECT_FLOAT_EQ(pe.GetStartSize(), 0.5f);
}
TEST(ParticleEditorTest, StartSizeClamp) {
    ParticleEditor pe;
    pe.SetStartSize(-0.1f);
    EXPECT_GE(pe.GetStartSize(), 0.0f);
}
TEST(ParticleEditorTest, Gravity) {
    ParticleEditor pe;
    pe.SetGravity(-9.81f);
    EXPECT_FLOAT_EQ(pe.GetGravity(), -9.81f);
}
TEST(ParticleEditorTest, EmitterShape) {
    ParticleEditor pe;
    pe.SetEmitterShape(EditorEmitterShape::Cone);
    EXPECT_EQ(pe.GetEmitterShape(), EditorEmitterShape::Cone);
}
TEST(ParticleEditorTest, ShapeRadius) {
    ParticleEditor pe;
    pe.SetShapeRadius(5.0f);
    EXPECT_FLOAT_EQ(pe.GetShapeRadius(), 5.0f);
}
TEST(ParticleEditorTest, ShapeRadiusMinClamp) {
    ParticleEditor pe;
    pe.SetShapeRadius(-1.0f);
    EXPECT_GT(pe.GetShapeRadius(), 0.0f);
}
TEST(ParticleEditorTest, Looping) {
    ParticleEditor pe;
    pe.SetLooping(false);
    EXPECT_FALSE(pe.IsLooping());
}
TEST(ParticleEditorTest, WorldSpace) {
    ParticleEditor pe;
    pe.SetWorldSpace(false);
    EXPECT_FALSE(pe.IsWorldSpace());
}
TEST(ParticleEditorTest, PlayPauseStop) {
    ParticleEditor pe;
    pe.Play();
    EXPECT_TRUE(pe.IsPlaying());
    pe.Pause();
    EXPECT_TRUE(pe.IsPaused());
    pe.Stop();
    EXPECT_FALSE(pe.IsPlaying());
    EXPECT_FALSE(pe.IsPaused());
}
TEST(ParticleEditorTest, Restart) {
    ParticleEditor pe;
    pe.SetPlaybackTime(5.0f);
    pe.Restart();
    EXPECT_FLOAT_EQ(pe.GetPlaybackTime(), 0.0f);
    EXPECT_TRUE(pe.IsPlaying());
}
TEST(ParticleEditorTest, PresetFire) {
    ParticleEditor pe;
    pe.LoadPreset(ParticleEditor::Preset::Fire);
    EXPECT_EQ(pe.GetEmitterSettings().name, "Fire");
    EXPECT_GT(pe.GetEmissionRate(), 0.0f);
}
TEST(ParticleEditorTest, PresetSmoke) {
    ParticleEditor pe;
    pe.LoadPreset(ParticleEditor::Preset::Smoke);
    EXPECT_EQ(pe.GetEmitterSettings().name, "Smoke");
}
TEST(ParticleEditorTest, PresetSparks) {
    ParticleEditor pe;
    pe.LoadPreset(ParticleEditor::Preset::Sparks);
    EXPECT_EQ(pe.GetEmitterSettings().name, "Sparks");
}
TEST(ParticleEditorTest, PresetSnow) {
    ParticleEditor pe;
    pe.LoadPreset(ParticleEditor::Preset::Snow);
    EXPECT_EQ(pe.GetEmitterSettings().name, "Snow");
    EXPECT_EQ(pe.GetEmitterShape(), EditorEmitterShape::Box);
}
TEST(ParticleEditorTest, PresetName) {
    EXPECT_EQ(ParticleEditor::GetPresetName(ParticleEditor::Preset::Fire), "Fire");
}
TEST(ParticleEditorTest, DirtyFlag) {
    ParticleEditor pe;
    pe.SetEmissionRate(50.0f);
    EXPECT_TRUE(pe.IsDirty());
    pe.ClearDirty();
    EXPECT_FALSE(pe.IsDirty());
}
TEST(ParticleEditorTest, SetEmitterSettings) {
    ParticleEditor pe;
    ParticleEmitterSettings s;
    s.name = "Custom";
    s.emissionRate = 42.0f;
    pe.SetEmitterSettings(s);
    EXPECT_EQ(pe.GetEmitterSettings().name, "Custom");
    EXPECT_FLOAT_EQ(pe.GetEmissionRate(), 42.0f);
}
