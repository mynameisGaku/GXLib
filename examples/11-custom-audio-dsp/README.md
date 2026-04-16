# 11-custom-audio-dsp — Layer 2 Custom Audio DSP

**Layer**: 2 (extension point per ADR-0017, ADR-0010 §8)

## What This Demonstrates

The `IAudioEffect` interface lets Layer 2 users add custom DSP effects
(distortion, tremolo, custom filters, ring modulator, etc.) to any `AudioBus`.
Registration is immediate; up to 4 effects per bus (ADR-0010 §8).

## API Used

```cpp
// 1. Derive from IAudioEffect
class MyEffect : public gx::IAudioEffect {
    void Process(float* buffer, uint32_t sampleCount,
                 uint32_t channels, uint32_t sampleRate) override {
        // Process in-place. NO heap allocation. NO locks.
        // Use std::atomic for main-thread param changes.
    }
    void Reset() override { /* clear internal state */ }
    const char* GetName() const override { return "MyEffect"; }
};

// 2. Register on a bus
auto& mgr = gx::AudioManager::Instance();
gx::AudioBus* bus = mgr.GetBus("SFX");
int idx = bus->AddEffect(std::make_unique<MyEffect>());

// 3. Remove when done (or let bus destructor handle it)
bus->RemoveEffect(idx);
```

## Threading Contract (ADR-0010 §12)

- `Process` is called from the **XAudio2 audio callback thread** (NOT the main thread).
- **NO heap allocation** inside `Process` (`heap_allocation_on_audio_callback` is a forbidden pattern).
- **NO mutexes, NO I/O, NO std::string growth, NO vector push_back without reserve.**
- Parameter changes from main thread use `std::atomic<T>` loaded at the top of `Process`.

## Current Status

**⚠ Partial integration**: `AudioBus::AddEffect` stores the effect, but the `Process` callback
is not yet dispatched from the audio thread. This requires wrapping `IAudioEffect` as an
XAudio2 `IXAPO` object and inserting into the SubmixVoice's effect chain.

**What works today**:
- `IAudioEffect` interface — final design, Doxygen + contract documented
- `AudioBus::AddEffect / RemoveEffect / GetEffectCount / GetEffect` — registration storage works
- Authoring pattern (derive + atomic params + Process) — final

**What's pending** (next sprint):
- IXAPO wrapper: `IAudioEffect` → `IXAPO` adapter that XAudio2 can insert into the SubmixVoice
- Effect parameter change queuing (write from main, read in callback)
- Tests against the `heap_allocation_on_audio_callback` forbidden pattern
- 48 kHz / 44.1 kHz device rate verification

## Reference

- ADR-0017 L2 pillar
- ADR-0010 §8 (DSP effects per bus, ≤4 limit) + §12 (thread model)
- `docs/implementation-gap-analysis-2026-04-16.md` T3
- `docs/engine-reference/gxlib/accessibility-scorecard.md` — Audio L2 gap this closes
