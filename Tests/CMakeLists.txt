# =============================================================================
# GXLib Tests — Google Test
# =============================================================================

# Google Test (FetchContent)
include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# テストソース
set(TEST_SOURCES
    test_main.cpp
    test_Vector.cpp
    test_Matrix.cpp
    test_Quaternion.cpp
    test_Color.cpp
    test_MathUtil.cpp
    test_Collision2D.cpp
    test_Collision3D.cpp
    test_Spatial.cpp
    test_Crypto.cpp
    test_Allocator.cpp
    # Phase 37: テスト強化
    test_Spline.cpp
    test_Transform3D.cpp
    test_Entity.cpp
    test_SceneSerializer.cpp
    test_ActionMapping.cpp
    test_NavMesh.cpp
    test_LODGroup.cpp
    test_Animation.cpp
    test_StyleSheet.cpp
    test_FileSystem.cpp
    # Phase 41: テスト拡充
    test_Transform2D.cpp
    test_Camera2D.cpp
    test_Camera3D.cpp
    test_Light.cpp
    test_Material.cpp
    test_XMLParser.cpp
    test_Tilemap.cpp
    test_IK.cpp
    test_AnimatorStateMachine.cpp
    test_BlendSystem.cpp
    # Phase 5: DI + テスタビリティ
    test_ServiceLocator.cpp
    # Math拡張
    test_MathExtended.cpp
    # Phase 100%: 全サブシステム拡張テスト
    test_CollisionExtended.cpp
    test_PhysicsExtended.cpp
    test_AIExtended.cpp
    test_Prefab.cpp
    # Phase 2: 全機能100%化
    test_CoreSystems.cpp
    test_SceneManager.cpp
    test_AIBehaviorTree.cpp
    test_AdvancedFeatures.cpp
    # Phase 3: 高度機能14項目
    test_Coroutine.cpp
    test_UndoNodeGraph.cpp
    test_PhysicsMaterial.cpp
    test_ClothNetworkCapture.cpp
    test_AssetDebugScene.cpp
    # Wave E: Reflection + ECS
    test_Reflection.cpp
    test_ECS.cpp
    # Wave F: DX12 Advanced Features
    test_VRS.cpp
    test_MeshShader.cpp
    test_DX12Advanced.cpp
    # Wave G: ReliableUDP, EditorWorkflow, UITween
    test_ReliableUDP.cpp
    test_EditorWorkflow.cpp
    test_UITween.cpp
    # Phase 5: 新規テスト10ファイル
    test_AudioSystem.cpp
    test_GUIWidget.cpp
    test_ScriptEngine.cpp
    test_VideoRecording.cpp
    test_PhysicsSimulation.cpp
    test_CharacterController.cpp
    test_RagdollBuilder.cpp
    test_ScenePersistence.cpp
    test_GPUParticle.cpp
    test_JobSystem.cpp
    # Phase 6: new test files
    test_Terrain.cpp
    test_Gizmo.cpp
    # Phase 7 Wave C: AssetDatabase拡張テスト (EditorPanels分割)
    test_AssetDatabaseExtended.cpp
    # Phase 7 Wave D: CutsceneDialogue, TilemapTMJ, SoundBank, BenchmarkRunner
    test_CutsceneDialogue.cpp
    test_TilemapTMJ.cpp
    test_SoundBank.cpp
    test_BenchmarkRunner.cpp
    # Phase 7 Wave A/B: Graphics Infrastructure + Rendering
    test_FrameGraph.cpp
    test_GPUMemoryAllocator.cpp
    test_GPUDebugLayer.cpp
    test_SkyWater.cpp
    test_LightProbe.cpp
    # Phase 7+: SpriteAnimation, TransitionEffect, AnimationEventDispatcher, FontSystem, CameraController
    test_SpriteAnimation.cpp
    test_TransitionEffect.cpp
    test_AnimationEventDispatcher.cpp
    test_FontSystem.cpp
    test_CameraController.cpp
    # Phase 8: ResourceCache, VFXGraph, NavMesh3D, DragDrop, AssetReloader
    test_ResourceCache.cpp
    test_VFXGraph.cpp
    test_NavMesh3D.cpp
    test_DragDrop.cpp
    test_AssetReloader.cpp
    # Noise, Localization, Random, PostEffects
    test_Noise.cpp
    test_Localization.cpp
    test_Random.cpp
    test_PostEffects.cpp
    # Phase 8 Wave A: GPU-Driven, VirtualTexture, DynamicResolution, POM
    test_GPUDrivenRenderer.cpp
    test_VirtualTexture.cpp
    test_DynamicResolution.cpp
    test_ParallaxOcclusion.cpp
    # Phase 8 Wave B: (Editor UI tests removed — UI wrappers deleted)
    # Phase 8 Wave C: FullBodyIK, AnimationLayer, CheckpointSystem, DialogueGraph
    test_FullBodyIK.cpp
    test_AnimationLayer.cpp
    test_CheckpointSystem.cpp
    test_DialogueGraph.cpp
    # Phase 8 Wave D: InterestManagement, NATTraversal, AsyncComputeScheduler, TLSF/Buddy Allocators
    test_InterestManagement.cpp
    test_NATTraversal.cpp
    test_AsyncComputeScheduler.cpp
    test_TLSFAllocator.cpp
    test_BuddyAllocator.cpp
    # Phase 9: Editor features (PIE, ShaderGraph, SceneViewport, TerrainSculptor)
    test_PlayInEditor.cpp
    test_ShaderGraph.cpp
    test_TerrainSculptor.cpp
    # Phase 10: MatchmakingLobby, VisualScript, CloudSave
    test_MatchmakingLobby.cpp
    test_VisualScript.cpp
    test_CloudSave.cpp
    # Phase 11: PrefabVariant, DataBinding, CompatAudio, Accessibility, TouchInput
    test_PrefabVariant.cpp
    test_DataBinding.cpp
    test_CompatAudio.cpp
    test_Accessibility.cpp
    test_TouchInput.cpp
    # Phase 9+: Additional feature tests
    test_BuoyancySystem.cpp
    test_CrashReporter.cpp
    test_GOAPPlanner.cpp
    test_GPUIntegration.cpp
    test_LightmapBaker.cpp
    test_ProceduralAnimation.cpp
    test_ProfilerGUI.cpp
    test_SaveVersion.cpp
    test_VehiclePhysics.cpp
    # Phase 9 Wave A: SpringBone, RootMotion, MotionMatching
    test_SpringBone.cpp
    test_RootMotion.cpp
    test_MotionMatching.cpp
    # InventorySystem, QuestSystem
    test_InventorySystem.cpp
    test_QuestSystem.cpp
    # Audio features: ReverbZone, AudioSnapshot
    test_ReverbZone.cpp
    test_AudioSnapshot.cpp
    # Phase 9 E17-E19: Graphics Device, Core Module, Integration tests
    test_GraphicsDevice.cpp
    test_CoreModule.cpp
    test_Integration.cpp
    # B6-B10: SSGI, Hair, MeshletStreaming, DecalEnhanced, Eye/Skin
    test_SSGI.cpp
    test_HairShader.cpp
    test_MeshletStreaming.cpp
    test_DecalEnhanced.cpp
    test_EyeSkinShader.cpp
    # D13-D16: DevTools (GameConsole, MemoryProfiler, ReplaySystem, PluginSystem)
    test_GameConsole.cpp
    test_MemoryProfiler.cpp
    test_ReplaySystem.cpp
    test_PluginSystem.cpp
    # Phase 10 Wave 1: Test quality enhancement
    test_PostEffectIntegration.cpp
    test_SceneRendererIntegration.cpp
    test_PhysicsIntegration.cpp
    test_AudioMixerRouting.cpp
    test_GUIEventPropagation.cpp
    # Phase 10 Wave 2: Shadow Enhancement
    test_CascadeBlend.cpp
    test_PCSS.cpp
    # Phase 10 Wave 3: Existing System Extensions
    test_SpriteAnimator.cpp
    test_ProceduralMesh.cpp
    test_GPUParticleCollision.cpp
    test_DecalDeferred.cpp
    # Phase 10 Wave 4: New Rendering Features
    test_VolumetricFog.cpp
    test_LensFlare.cpp
    test_RTSoftShadows.cpp
    # Phase 10 Wave 5: Core Gameplay Features
    test_SceneStreamer.cpp
    test_PolyNavMesh.cpp
    test_MaterialGraph.cpp
    # Phase 10 Wave 6: GPU + Advanced Features
    test_ClothSimulation.cpp
    test_ClusteredLighting.cpp
    # Phase 10 Wave 7: Rollback Netcode
    test_RollbackNetcode.cpp
    # Container Library: EASTL-like高速コンテナ
    test_ContainerInfra.cpp
    test_ContainerVector.cpp
    test_ContainerString.cpp
    test_ContainerHashMap.cpp
    test_ContainerTreeMap.cpp
    test_ContainerList.cpp
    test_ContainerAdapters.cpp
    test_ContainerFixed.cpp
    test_ContainerFlat.cpp
    test_ContainerIntrusiveHash.cpp
    test_ContainerAlgorithm.cpp
    test_ContainerArray.cpp
    test_ContainerBitset.cpp
    test_ContainerDeque.cpp
    test_ContainerRingBuffer.cpp
    # RenderProfile: Unity Volume Profile 風レンダリング設定
    test_RenderProfile.cpp
    # Asset Pipeline: GUID, PPtr, BundleManager, AsyncLoadPipeline, AssetRemapper
    test_AssetGUID.cpp
    test_BundleManager.cpp
    test_AsyncLoadPipeline.cpp
    test_AssetRemapper.cpp
    # Result<T> type and ConcurrentObjectPool
    test_Result.cpp
    test_ConcurrentObjectPool.cpp
    # SSS: Screen-Space Subsurface Scattering
    test_SSS.cpp
    # Editor Panels: SceneHierarchy, PropertyInspector, AssetBrowser, Console, Timeline
    test_EditorPanels.cpp
    # ShaderVariant: シェーダーバリアント管理システム
    test_ShaderVariant.cpp
    # ECS QueryCache: クエリ結果キャッシュ
    test_ECSQueryCache.cpp
    # Hair Renderer + Spline Camera Rail
    test_HairRenderer.cpp
    test_SplineCameraRail.cpp
    # TLS/DTLS, DecalProjector, ConvolutionReverb
    test_TLSSocket.cpp
    test_DecalProjector.cpp
    test_ConvolutionReverb.cpp
)

add_executable(GXLibTests ${TEST_SOURCES})
target_link_libraries(GXLibTests PRIVATE GXLib GTest::gtest GTest::gtest_main)
target_include_directories(GXLibTests PRIVATE
    ${CMAKE_SOURCE_DIR}/GXLib
    ${CMAKE_SOURCE_DIR}
)

# CTest 連携
include(GoogleTest)
gtest_discover_tests(GXLibTests)
