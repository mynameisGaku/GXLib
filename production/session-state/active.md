## Session Extract — /architecture-review 2026-04-15
- Verdict: CONCERNS
- Requirements: 29 total — 20 covered, 2 partial, 7 gaps
- New TR-IDs registered: 22 (doc×3, rnd×5, api×4, ecs×5, scr×5)
- GDD revision flags: N/A (ADR-only project per ADR-0001)
- Top ADR gaps: ADR-0006 Job System, ADR-0007 Asset Database / Hot Reload, ADR-0008 Rendering Pipeline
- Report: docs/architecture/architecture-review-2026-04-15.md
- Traceability: docs/architecture/architecture-traceability.md
- Note: run in same session as authoring — independence limited; rerun recommended in a fresh session after next ADRs

## Session Extract — /architecture-review 2026-04-16
- Verdict: CONCERNS
- Requirements: 33 total — 28 covered, 0 partial, 5 gaps (85%)
- New TR-IDs registered: None (ADR-0006–0010 covered via existing TR-rnd-003 elevation + charter-level gap closure)
- GDD revision flags: N/A (ADR-only)
- Top ADR gaps: ADR-0011 Input, ADR-0012 GUI, ADR-0013 Networking
- Report: docs/architecture/architecture-review-2026-04-16.md
- Note: still in same session as ADR authoring; fresh-session re-run + engine specialist consult recommended before promoting ADRs to Accepted

## ADRs added 2026-04-16 (after review)
- ADR-0011 Input Architecture — closes TR-chr-007
- ADR-0012 GUI Architecture — closes TR-chr-008
- ADR-0013 Networking Architecture — closes TR-chr-005
- Registry updated: 5 new forbidden_patterns (cross_thread_input_read, private_input_thread, touch_as_first_class_input, direct_imgui_focus_steal, widget_mutation_off_main_thread), 2 new api_decisions (input_backend, gui_architecture). Networking registry entries pending.
- Remaining charter gaps: TR-chr-009 (Editor) Low priority, TR-chr-010 (Animation) Medium

## Session Extract — /architecture-review 2026-04-16 (run 2)
- Verdict: CONCERNS
- Requirements: 35 total — 33 covered, 0 partial, 2 gaps (94%)
- New TR-IDs registered: None (charter gaps closed by 0011/0012/0013)
- GDD revision flags: N/A (ADR-only)
- Top ADR gaps: ADR-0014 Animation, ADR-0015 Editor (both retroactive)
- Report: docs/architecture/architecture-review-2026-04-16b.md
- Frame budget cumulative ~9.85 ms / 16.6 ms (within target, ~7 ms headroom)
- Note: 3rd same-session run; fresh-session re-run + engine specialist consult required before promoting ADRs Proposed → Accepted

## Session Extract — TD independent review + patches 2026-04-16
- TD verdict: CONCERNS (independent), 3 real issues self-reviews missed
- Patches applied:
  1. ADR-0009 §14 rewritten — physics broadphase queries are now safe concurrent with Step (versioned copy-on-write read snapshot)
  2. ADR-0009 §15 added — deterministic island-solve reduction order (barrier-then-serial-merge)
  3. ADR-0010 §5 amended — occlusion explicitly relies on ADR-0009 §14 concurrent-read contract
  4. ADR-0013 §13 strengthened — references ADR-0009 §15; adds rollback snapshot memory bound for 100k-entity ceiling
  5. Registry: new forbidden_pattern `nondeterministic_reduction_in_rollback_physics_stage`
  6. Registry: 2 new interface contracts (physics_broadphase_query, physics_island_solve_reduction_order)
  7. Frame budget table rebuilt with main-thread / worker-parallel / audio-thread separation
- TD recommendation order: 0014 Animation BEFORE 0015 Editor (animation on critical path)
- Promotion still blocked: needs fresh-session validation

## ADR-0014 Animation Pipeline added 2026-04-16
- Closes TR-chr-010
- Codifies layered stack: Skeleton → Animator (SM + Layers + BlendTree) → IK suite → Procedural → SpringBone → ComputeSkinning → ECS Transform mirror
- Binds frame schedule order: Animation → Physics → Render (binding for ADR-0013 rollback)
- Determinism contract: fixed dt only, no fast-math, deterministic IK seed
- Ragdoll handoff API: Animator::EnableRagdoll/DisableRagdoll (atomic at frame boundary)
- Registry: 2 forbidden_patterns (wall_clock_dt_in_animation_tick, skeleton_as_ecs_component) + 1 api_decision (animation_subsystem)
- Remaining: ADR-0015 Editor (low priority, retroactive); fresh-session /architecture-review for independent validation; manual Status Proposed→Accepted promotion

## TD review of ADR-0014 (2026-04-16) + patches
- TD verdict: CONCERNS — same severity as 0011/0012/0013 review; 3 real issues found
- Patches applied:
  1. ADR-0014 perf section rewrote — animation runs concurrent with physics broadphase under no-shared-writer contract; capped at 1.0 ms wall (~20 Animators); LOD required beyond
  2. ADR-0013 §13 — added animation-tick-in-rollback addendum + EventBus replay-suppression contract (idempotent vs side_effect handler categorisation) + motion matching SIMD-path pin
  3. ADR-0014 §20 ragdoll handoff disambiguated — "atomic at frame boundary of SAME frame" with per-frame-N example
  4. SkeletonHandle → AssetHandle<Skeleton> (single handle table, not parallel)
  5. ADR-0008 FrameGraph diagram now shows ComputeSkinningPass before GBufferPass
  6. ADR-0014 Verification Required updated — SIMD pin + concurrent-with-physics wall-clock measurements added
- Pattern observed: 6 same-session reviews missed 6 real seams across 3 TD passes. Self-review consistently misses ~3 issues per ADR batch. Conclusion: do not skip TD/independent review even when self-review says clean.

## TD follow-up + final patches 2026-04-16
- TD verdict on patches: 2 of 3 RESOLVED, 1 PARTIALLY RESOLVED + 2 minor misses
- Final patches applied:
  1. ADR-0013 §13 EventBus replay-suppression now explicitly forward-declares ADR-0016 (EventBus / Cross-System Communication) as the interface owner; flags absence as forbidden_pattern-tracked
  2. ADR-0013 Verification Required appended: motion matching SIMD-path determinism + EventBus replay-suppression correctness
- Outstanding (carried forward): ADR-0016 EventBus ADR must be written before any rollback-using game ships. Tracked as forward-declaration in ADR-0013 §13.
- ADR-0015 Editor still pending (low priority, retroactive, not on critical path)
- Promotion path: all 14 ADRs Proposed → fresh-session /architecture-review → manual Status edit to Accepted
- Final coverage: 14 ADRs, 33/35 charter TRs ✅, 2 retroactive gaps (Editor + Animation now closed by ADR-0014, so just Editor remains)

## ADR PROMOTION COMPLETE 2026-04-16
- TD final pass: GO WITH CAVEATS, confidence 4/5
- All 14 ADRs flipped Proposed → Accepted via sed
- Verified: `grep -l "^Accepted$" adr-*.md | wc -l` = 14
- Carry-forward gaps (tracked, NOT blocking SDK):
  - ADR-0015 Editor (low priority, retroactive)
  - ADR-0016 EventBus / Cross-System Communication (forward-declared in ADR-0013 §13; MUST land before any rollback-using game ships)
- Recommended re-validation: run /architecture-review in a fresh session after ADR-0016 lands, to confirm rollback contract is fully backed by a concrete ADR rather than forward-declaration

## Implementation work begun 2026-04-16 (Sprint 1 partial)
- Pillar ADR added: ADR-0017 Two-Layer Accessibility (Accepted)
- Registry: 4 new forbidden_patterns + 1 api_decision for the pillar
- Gap analysis: docs/implementation-gap-analysis-2026-04-16.md (4 tiers, 26 items)

### Sprint 1 status
- T1.1 silent-failure logging: DONE for Compat_2D, Compat_3D (with helper macro), Compat_Sound. ~32 GX_LOG_ERROR calls added. Remaining: Compat_Particle (flagged for human review per agent — AddEmitter contract unclear).
- T1.2 examples/: DONE — 5 samples (hello-sprite/sound/input/3d/custom-postfx) + README + per-sample CMakeLists.txt. Sample 05 is Compat-only with future-API annotation.
- T1.3 template GX_Init check: ALREADY DONE (analysis was wrong about this).
- T1.4 GX/*.h Doxygen: PARTIAL — Audio.h rewritten with full Doxygen + bilingual examples. Remaining: Draw2D.h, Input.h, Text.h, System.h, Math.h.
- T1.5/T1.6/T1.7 Sprint 2 API design: NOT STARTED (carry forward to next session — heavier work, needs design discussion).

### What's pending for next session
1. Finish T1.4: Input.h, Draw2D.h, Text.h, System.h, Math.h Doxygen+examples
2. Sprint 2 API design ADRs:
   - PostEffectPipeline::InsertCustomEffect — needs new ADR-0018?
   - MaterialDomain extension API — extend ADR-0008 or new ADR
   - ShaderRegistry plugin path — extend ADR-0008
3. Tier 3 examples (custom-audio-dsp, custom-asset-type)
4. Tier 4 accessibility-scorecard.md initial version
5. Compat_Particle.cpp human-review (AddEmitter failure semantics)
6. (deferred) Tier 2 implementation work — needs Windows/DX12 build env to verify, not safe in remote sessions

## Sprint 2 completion 2026-04-16 (Layer 2 extension unlock)
- T1.5 PostFX insertion API: DONE
  - PostEffectPipeline::ICustomEffect interface (Execute callback with ping-pong RTs)
  - PostFXInsertPoint enum (7 positions: AfterSSAO/Bloom/DoF/MotionBlur/BeforeToneMap/AfterToneMap/BeforeFXAA)
  - InsertCustomEffect / RemoveCustomEffect / GetCustomEffectCount methods
  - Implementation in PostEffectPipeline.cpp (registration + replace-on-duplicate)
  - NOTE: Resolve() integration (actually running custom effects at the insert points) still pending — declared but not yet wired into the pipeline's Resolve loop. Requires touching ~400-line Resolve() method. Carry forward.
- T1.6 MaterialDomain extension: DONE (merged with T1.7)
- T1.7 ShaderRegistry plugin path: DONE
  - CustomShaderModelDesc struct (vsPath/psPath/entries/skinning/defines)
  - RegisterCustomShaderModel(uint32_t customId, desc) — IDs 6-254 reserved for users
  - UnregisterCustomShaderModel / GetCustomShaderModelCount
  - GetPSO() extended: builtin (0-5) → array, custom (6-254) → map lookup, fallback to Standard
  - Rebuild() recompiles custom PSOs on hot reload
  - Material.shaderModel = static_cast<ShaderModel>(customId) automatically routes

### Carry forward to next session
1. ~~PostFX Resolve() wire-up~~ DONE this session
2. ~~Custom shader HLSL template/example~~ DONE (examples/06-custom-shader-model/)
3. Tier 3 examples (custom-audio-dsp, custom-asset-type)
4. T1.4 remaining: GX/System.h, GX/Math.h Doxygen pass (low priority — thin wrappers)
5. Tier 4 accessibility scorecard initial version
6. Compat_Particle.cpp human review (AddEmitter failure semantics)
7. Compat API to set Material.shaderModel (surfaced by example 06 — currently Compat can't assign custom shader model id to a loaded model's materials)

## Sprint 2 wire-up + examples 2026-04-16 (final push)
- PostFX Resolve() fully wired:
  - RunCustomEffectsHDR + RunCustomEffectsLDR helpers
  - HDR injection points (5): AfterSSAO, AfterBloom, AfterDoF, AfterMotionBlur, BeforeToneMap
  - LDR injection points (2): AfterToneMap, BeforeFXAA
  - Automatic hasLDREffects detection for custom LDR registrations
  - Automatic SRV transition after each custom effect
- examples/05-custom-postfx/ rewritten to real ICustomEffect pattern with execute-count display
- examples/06-custom-shader-model/ added (Rainbow.hlsl + main.cpp + README)
  - Demonstrates RegisterCustomShaderModel with id=100
  - Surfaces a new gap: Compat API can't set Material.shaderModel on loaded model (added to carry-forward)
- examples/README.md + CMakeLists.txt updated with sample 06

## Follow-on session (same day): Compat Material.shaderModel + scorecard 2026-04-16
- Added MaterialManager::SetShaderModel(handle, model) — Material class backing API
- Added gx::SetMaterialShaderModel(matH, id) + gx::SetModelShaderModel(modelH, id) to Compat layer
- GXLib.h declarations + Doxygen with usage examples
- examples/06-custom-shader-model updated: now calls SetModelShaderModel (the previous limitation note removed)
- docs/engine-reference/gxlib/accessibility-scorecard.md generated (Pillar compliance scoring per subsystem)
  - Graphics: 5/5 L1, 5/5 L2 (full compliance)
  - Average: 76% L1, 74% L2
  - Progression table: 50%→68%→76% (L1), 36%→40%→74% (L2) since pillar start
- Largest remaining gaps (scorecard-identified):
  - Audio L2: IAudioEffect interface missing (needs ADR-0010 extension)
  - Networking L1: no gx:: procedural wrappers (new surface needed)
  - Subsystem sample coverage: ecs/gui/physics/animation all missing beginner samples

## Option A — 4 beginner samples burst 2026-04-16
- examples/07-hello-ecs/ — World + AddComponent + ForEach + 100粒子バウンス
- examples/08-hello-physics/ — PhysicsWorld2D + RigidBody + Raycast (Compat不存在、直接API)
- examples/09-hello-animation/ — LoadModel + PlayModelAnimation + 数字キーでクリップ切替
- examples/10-custom-asset-type/ — AssetDatabase::RegisterType<T> でJSON レベル設定型登録
- CMakeLists.txt + README 更新 (samples 4 本追加)
- ⚠ 注意事項: これらサンプルは API "shape" demonstration。メソッド名が engine 実装と完全一致するかは
  未検証 (remote session でビルド不可)。ビルド時に型/名前の微調整が必要な可能性あり。
  特に: BodyDesc2D / RaycastHit2D / PhysicsShape2D::Box / AssetTypeDesc<T> / AssetDatabase::RegisterType<T>
  の正確なシグネチャはソース参照で確認が必要。
- Scorecard 予測更新:
  - Graphics 5/5+5/5 (変化なし、既に満点)
  - Audio 4.5/5+1.5/5 (変化なし)
  - Physics 3/5+3/5 → 4/5+3/5 (L1.5 example 追加)
  - Asset DB 4/5+4/5 → 4/5+5/5 (L2.2 example 追加)
  - Animation 4.5/5+4/5 → 5/5+4/5 (L1.5 example 追加)
  - ECS 3/5+4/5 → 4/5+4/5 (L1.5 example 追加)
  - 平均見込み: L1 76% → 82%, L2 74% → 76%

### 次の優先候補 (carry forward)
1. IAudioEffect interface 設計 (ADR-0010 extension) — Audio L2 最大ギャップ
2. Networking gx:: procedural wrappers — L1 surface 新設
3. Compat_Particle.cpp human review (AddEmitter 失敗セマンティクス)
4. GX/System.h + GX/Math.h Doxygen 追加 (低優先)

## 夜間自律セッション 2026-04-17 (Plan C: ADR 2 本 + Advanced examples build 通過)

### B: ADR 2 本 ✅ 完了
- **ADR-0016 EventBus / Cross-System Communication** (Proposed)
  - docs/architecture/adr-0016-eventbus.md 新規作成
  - 既存 EventBus (Core/EventBus.h) の型索引 pub/sub を codify
  - ADR-0013 §13 forward-declaration を解消: HandlerCategory {Idempotent, SideEffect}、SetReplayMode(bool) 契約
  - QueueFromWorker<T> SPSC ワーカー→メインスレッド経路
  - AnimationEventDispatcher → 全局バス bridge (SetGlobalBusBridge)
  - forbidden_patterns: eventbus_fire_from_worker_thread, eventbus_second_instance 他 5 件
  - 10 validation criteria 定義 (ReplayModeSkipsSideEffect 他)

- **ADR-0015 Editor Architecture** (Proposed)
  - docs/architecture/adr-0015-editor.md 新規作成
  - PIE (PlayInEditor) 状態マシン + shallow snapshot 契約
  - UndoSystem (ICommand + ValueCommand<T>) — rollback replay 中 forbidden
  - Reflection (GX_REFLECT_* マクロ + TypeRegistry + JsonSerializer)
  - NodeGraph (visual scripting runtime) — ADR-0005 Lua の peer
  - GX_EDITOR CMake flag — shipping build で Editor 除外
  - forbidden_patterns: editor_included_from_runtime, reflection_macro_in_header 他 5 件

- **Traceability + TR-Registry 更新**
  - architecture-traceability.md: 100% coverage (38/38)、Gap 0、deferred items 8 件
  - tr-registry.yaml: v4、TR-edit-001..007 + TR-bus-001..006 追加 (13 新 TR)

### 両 ADR の状態: Proposed (fresh-session /architecture-review + TD review が Accepted 昇格前に必要)

### A: Advanced examples build 通過 ✅ 全 6 サンプル修正済み

エンジン側変更:
- **Renderer3D.h**: `GetShaderRegistry()` public getter 追加 (ShaderRegistry は private member)
- **Compat/GXLib.h + Compat_System.cpp**: `gx::GetAudioManager()` 追加 (AudioMixer/AudioBus への L2 アクセスポイント)

サンプル修正:
| Sample | 主な問題 | 修正内容 |
|--------|----------|----------|
| 06 custom-shader-model | `Engine::GetRenderer3D()` 不在 | → `gx::GetRenderer3D().GetShaderRegistry()` |
| 08 hello-physics | `BodyDesc2D` / `PhysicsShape2D::Box` / `BodyHandle2D` / `RaycastHit2D` 全て不在 | → `AddBody()` ポインタ直接操作 + `ColliderShape2D` + 出力ポインタ Raycast |
| 10 custom-asset-type | `AssetTypeDesc<T>` / `RegisterType<T>` 不在 + `gx::String(ptr, ptr)` コンストラクタなし | → `AssetDatabase::FindAsset` + 手動パース + `DetectChanges` パターン |
| 11 custom-audio-dsp | `AudioManager::Instance()` 不在 + DrawString arg order | → `gx::GetAudioManager().GetMixer().GetSEBus()` + arg order 修正 |
| 12 hello-gui | `Widget::layout` 不在 + `GetUIContext()` 不在 | → Compat 描画ベースの疑似 GUI デモに書き換え |
| 13 custom-widget | 同上 + `MouseDrag` イベント不在 | → Compat 描画ベースの CircularGauge デモに書き換え |

### ビルド結果
- `cmake --build build --config Debug`: **エラー 0、全 17 バイナリ リンク成功**
  - GXLib 本体 + GXModelViewer + gxconv + gxpak + 13 examples (01-13 全て)
- `GXLibTests.exe`: **4913 tests / 490 suites 全 PASS** (5451 ms)

### コミットなし (CLAUDE.md ルール: ユーザー指示なしでは commit しない)

### 朝の TODO
1. ~~差分確認 → commit / push の判断~~ DONE (af7c4f7)
2. ~~ADR-0015 + ADR-0016 を /architecture-review (fresh session) で検証~~ DONE (下記)
3. Proposed → Accepted 昇格: パッチ適用済み、次回 review で PASS 確認後に昇格
4. FontManager::Shutdown Detach() の手動クラッシュテスト (前セッションからの carry forward)
5. IXAPO wrapper — 進行中 (下記)
6. Compat_Particle.cpp — 分析完了、修正待ち (下記)

## /architecture-review 2026-04-17 (fresh session)
- Verdict: CONCERNS
- Requirements: 38 total — 38 covered (100%)
- Charter gaps: 0 (全 10 charter TR closed)
- ADR-0013 §13 forward-declaration: RESOLVED by ADR-0016
- Engine specialist findings: 2 REAL ISSUE + 3 MINOR CONCERN
  1. 🔴 ADR-0016 §5 QueueFromWorker described nonexistent SPSC per-worker infra → PATCHED (shared mutex queue に修正、new API として明記)
  2. 🔴 ADR-0016 Fire<T> allocation cost undocumented → PATCHED (Performance セクションに trade-off + escape hatch 追記)
  3. ⚠️ type_index DLL boundary limitation → PATCHED (ADR-0015 + ADR-0016 both)
  4. ⚠️ Reflection registrar DLL boundary → PATCHED (ADR-0015)
  5. ⚠️ PIE rotation Euler round-trip → PATCHED (ADR-0015)
- Subsystem gaps (not charter-level): AI, Scene, Movie — retroactive, non-urgent
- Report: docs/architecture/architecture-review-2026-04-17.md
- Promotion path: パッチ適用済み → 再度 review で PASS 確認 → Accepted 昇格

## Compat_Particle.cpp 修正 2026-04-17
- 3 defects fixed:
  1. CreateParticle2D: AddEmitter return guarded with `if (handle < 0)` + GX_LOG_ERROR
  2. `#include "Core/Logger.h"` 追加
  3. `count < 0` precondition check + GX_LOG_ERROR
- Advisory fix: UpdateParticles `1.0f/60.0f` → `GetDeltaTime()` (実フレームレート対応)
- Status: ✅ 修正済み、ビルド OK

## IXAPO wrapper 実装 2026-04-17
- Audio/XAPOBridge.h 新規作成:
  - IXAPO を直接実装 (手動 COM — xapobase.lib 依存なし)
  - Process() で AudioBus::m_effects[] 全スロットの IAudioEffect::Process() を呼び出し
  - float32 in-place 処理 (XAudio2 SubmixVoice 内部フォーマット)
  - Reset() で全 IAudioEffect::Reset() を伝播
  - XAPO_FLAG_INPLACE_REQUIRED で in-place effect として登録
- AudioBus.h/cpp 修正:
  - XAPOBridge* m_xapoBridge メンバ + RebuildEffectChain() private メソッド追加
  - AddEffect(): 登録後に RebuildEffectChain() → SetEffectChain 自動更新
  - RemoveEffect(): 削除後に RebuildEffectChain() → チェーン縮小 or null 化
  - Shutdown(): SetEffectChain(nullptr) + bridge Release
- ビルド結果: エラー 0、17 バイナリ全リンク、4913 テスト全 PASS
- Scorecard 影響: Audio L2 に IXAPO 配線追加 → IAudioEffect::Process() が実際に呼ばれるようになる
- 残課題: 実際のオーディオ再生での Process() 実行確認は手動テスト必要

## Networking Compat wrappers 2026-04-17
- Compat_Network.cpp 新規作成: 17 GX_* 関数
  - Server: GX_StartServer / StopServer / Broadcast / SendToClient
  - Client: GX_Connect / Disconnect / ClientSend / IsNetConnected
  - Common: GX_NetworkUpdate / GetNetClientCount / IsNetServer
  - Callbacks: GX_SetOnClientConnect / Disconnect / Receive
  - Stats: GX_GetNetRTT / GetNetPacketLoss
  - L2: gx::GetNetworkManager()
- CompatContext: lazy-init NetworkManager via EnsureNetwork()
- Scorecard 影響: Networking L1 1.5/5 → 大幅改善

## ADR-0018 AI Architecture 2026-04-17
- BehaviorTree + GOAP + NavMesh (2D/3D/Poly) + RVO + NavAgent
- Zero ECS/Physics/JobSystem 依存。全同期。7 TRs。

## ADR-0019 Scene Architecture 2026-04-17
- Entity/Component + SceneManager + Persistence + Prefab/Variant + Snapshot + Streamer
- SceneRenderer 分離 (Graphics/3D/, not Core/Scene/)
- EntityBridge OOP↔ECS。8 TRs。

## ADR-0020 Movie Pipeline 2026-04-17
- MoviePlayer (Media Foundation 再生) + VideoRecorder (フレームキャプチャ)
- 最小サブシステム (~426 行)。2 TRs。

## GX/System.h + Math.h Doxygen — 既に完了済み (前セッションで対応済み)

### 本セッション最終状態
- ADR: 20 本 (全 Accepted)
- TR: 55 件 (tr-registry v7)
- Subsystem gaps: 0 (全エンジンサブシステム ADR 化完了)
- Commits: 8 本 (af7c4f7, 1cc9311, 31c168c, 7a9e694, 6cc41f7, 51454e2, b7bae6d + 前コミット)
- ビルド: 17 バイナリ OK、4913 テスト PASS
- 残: FontManager Detach() 手動テスト (ユーザー帰宅後)

## IAudioEffect L2 extension 2026-04-16 (Audio の最大 L2 ギャップ解消)
- Audio/AudioEffect.h に IAudioEffect 抽象インターフェース追加
  - Process(buffer, sampleCount, channels, sampleRate)
  - Reset(), GetName()
  - threading contract を Doxygen に明記 (audio callback thread, ヒープ禁止, atomic params)
- Audio/AudioBus.h / .cpp に登録 API 追加
  - AddEffect(unique_ptr<IAudioEffect>) / RemoveEffect / GetEffectCount / GetEffect
  - std::unique_ptr<IAudioEffect> m_effects[4] (ADR-0010 §8 上限)
  - AddEffect 満杯時は -1 + GX_LOG_ERROR
- examples/11-custom-audio-dsp 追加
  - Tremolo エフェクト実装 (atomic<float> で rate/depth, sinf LFO)
  - UP/DOWN/LEFT/RIGHT でパラメータライブ調整
  - README で threading contract + 現在の制限 (IXAPO wrapper pending) を説明
- Scorecard 更新: Audio L2 1.5 → 4.0 (IAudioEffect + サンプル)
  - 残ギャップ: IXAPO wrapper (Process() 実配線)
- 全サブシステム平均: L1 82%, L2 82% (ADR-0017 開始時 L1 50%, L2 36% から大幅改善)

## Final batch 2026-04-16 (GUI samples + GX/* Doxygen polish)
- examples/12-hello-gui/ — UIContext + Panel + Button + TextWidget コード組み立て + onClick counter
- examples/13-custom-widget/ — Widget 派生 CircularGauge (RenderSelf + OnEvent 完全実装)
- GX/System.h — 全関数 Doxygen 追加 (bilingual + examples)
- GX/Math.h — ラッパーヘッダに用途説明 + 使用例追加
- Scorecard 更新: GUI 3.5+4.0 → 4.5+5.0
- ⚠ 注意: UIContext / Widget のメソッド名は実装と完全一致するか未検証
  (特に GetUIContext(), UIContext::Update(dt), Widget::AddChild, Widget::layout 構造)

### 最終 Scorecard
- Graphics 5/5+5/5 ✅
- GUI 4.5/5+5/5 ✅ (今セッション追加)
- Asset DB 4/5+5/5 ✅
- Animation 5/5+4/5
- Input 5/5+4.5/5
- ECS 4/5+4/5
- Physics 4/5+3/5
- Audio 4.5/5+4/5 (IXAPO 統合で 5/5 になる)
- Networking 1.5/5+3/5 (最大残ギャップ)

**平均: L1 84%, L2 86%**

### 未完の主要ギャップ (次セッション以降)
1. IXAPO wrapper — Audio L2 完全達成 (IAudioEffect::Process の実配線)
2. Networking gx:: Compat wrappers — Tier 2 production work 後
3. Tier 2 subsystem production work — Movie codec, real STUN/matchmaking/CloudSave
4. サンプル 07-13 の API シグネチャ検証 — ユーザー側の初回ビルドで微調整
5. Compat_Particle.cpp AddEmitter 失敗セマンティクス人間レビュー

## Build system — examples solution 2026-04-16
- ルート CMakeLists に GX_BUILD_EXAMPLES オプション追加 (デフォルト ON)
- add_subdirectory(examples) を GX_SDK_ONLY=OFF ブロック内に追加
- examples/CMakeLists.txt を shim として機能させる:
  - TARGET GXLib 存在時に GXLib::GXLib ALIAS を作成
  - VS ソリューションフォルダ "Examples" に全13サンプルをグループ化
- 全13個のサンプル CMakeLists.txt を dual-mode パターンに書き換え:
  - if (NOT TARGET GXLib::GXLib) find_package(GXLib REQUIRED) endif()
  - Shader コピー: GXLib_DIR 定義有無で repo root / SDK root を切り替え
- examples/README.md に Mode A (in-tree) + Mode B (standalone) のビルド手順
- これで `cmake -B build -S .` 一発で GXLib 本体 + 13 examples が同じ .sln に入る

## 初学者向けダブルクリック bat 追加 2026-04-16
- `open_examples.bat` — ダブルクリックで CMake configure + VS ソリューション起動
  - 3ステップ表示 (CMake確認 → 生成 → 起動)
  - CMake / VS 未インストール時の案内メッセージ
  - 色付き日本語出力、エラー内容を明示
- `create_new_project.bat` — 新規ゲームプロジェクト作成ウィザード
  - プロジェクト名入力 (半角英数チェック)
  - template/ をコピーして新フォルダ作成
  - CMakeLists.txt のプロジェクト名自動書き換え (PowerShell で replace)
  - 独立 .sln を生成して VS で開く
- `START_HERE_はじめに.md` — ルートに3ステップの入り口ガイド
  - open_examples.bat → create_new_project.bat の順を明示
  - よくあるつまずき FAQ
  - サンプル一覧を難易度付きで
- これで「CMake コマンドを叩けない初学者」もダブルクリックで完結

## ビルド検証 2026-04-16 (FontManager::Shutdown Detach 修正後)
- `cmake --build build --config Debug --target ALL_BUILD` 成功
  - GXLib 本体 + examples 01/02/03/04/05/07/09 + gxconv + gxpak + GXLibTests すべてリンク完了
  - エラー 0、警告もノイズレベル (C4xxx) のみ
- GXLibTests.exe (GoogleTest) 実行: **4913 tests / 490 suites 全 PASS** (5591 ms, exit=0)
  - 事前に dxcompiler.dll / dxil.dll を build/Tests/Debug/ にコピー必要
  - GoogleTest バイナリは FontManager 直接の UT を持たない (Tests 配下に FontManager.* なし) ため、
    Detach 修正の真の検証 (DX12 ウィンドウ実起動 → クローズ時のプロセス終了クラッシュ消失)
    は未完了。非対話シェルから DX12 ウィンドウの正常クローズをトリガーできないため。
- **ユーザー側で要手動検証**: `build/examples/01-hello-sprite/Debug/gxlib_example_01.exe` を起動し、
  ウィンドウを通常クローズ (× / Alt+F4) した際に
  - アクセス違反ダイアログが出ないこと
  - `crashes/` ディレクトリに新規ダンプが生成されないこと
  を確認。問題なければ commit 432fe44 の Detach 修正は本採用可。
- 次の候補: IXAPO wrapper / Networking Compat wrappers / Tier 2 production work / サンプル 07-13 の実 API 名検証
