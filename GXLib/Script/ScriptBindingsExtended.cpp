/// @file ScriptBindingsExtended.cpp
/// @brief 拡張Luaバインディング（Physics, Audio, GUI, FileSystem, ECS）
#include "pch_common.h"
#include "Script/ScriptBindings.h"

#ifdef GX_ENABLE_LUA

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "Physics/PhysicsWorld3D.h"
#include "Audio/AudioManager.h"
#include "Audio/MusicPlayer.h"
#include "GUI/UIContext.h"
#include "GUI/Widget.h"
#include "IO/FileSystem.h"
#include "ECS/World.h"
#include "Math/Vector3.h"

namespace gx {
namespace ScriptBindings {

void RegisterPhysics(sol::state& lua, PhysicsWorld3D* world)
{
    if (!world) return;
    auto phys = lua.create_named_table("physics");

    phys["raycast"] = [world](sol::this_state ts,
                              float ox, float oy, float oz,
                              float dx, float dy, float dz,
                              float maxDist) -> sol::object
    {
        auto result = world->Raycast({ox, oy, oz}, {dx, dy, dz}, maxDist);
        if (!result.hit) return sol::make_object(ts, sol::nil);
        sol::state_view sv(ts);
        sol::table t = sv.create_table();
        t["hit"]      = true;
        t["px"]       = result.point.x;
        t["py"]       = result.point.y;
        t["pz"]       = result.point.z;
        t["nx"]       = result.normal.x;
        t["ny"]       = result.normal.y;
        t["nz"]       = result.normal.z;
        t["fraction"] = result.fraction;
        return t;
    };

    phys["raycastHit"] = [world](float ox, float oy, float oz,
                                 float dx, float dy, float dz,
                                 float maxDist) -> bool
    {
        auto result = world->Raycast({ox, oy, oz}, {dx, dy, dz}, maxDist);
        return result.hit;
    };

    phys["setGravity"] = [world](float x, float y, float z) {
        world->SetGravity({x, y, z});
    };

    phys["getGravity"] = [world]() -> std::tuple<float, float, float> {
        auto g = world->GetGravity();
        return {g.x, g.y, g.z};
    };

    phys["step"] = [world](float dt) {
        world->Step(dt);
    };
}

void RegisterAudioExtended(sol::state& lua, AudioManager* audio)
{
    if (!audio) return;
    auto snd = lua.create_named_table("audio");

    snd["loadSound"] = [audio](const std::string& path) -> int {
        // AudioManager API用にナロー文字列からワイド文字列へ変換
        gx::WString wpath(std::wstring(path.begin(), path.end()));
        return audio->LoadSound(wpath);
    };

    snd["playSound"] = [audio](int handle, sol::optional<float> volume, sol::optional<float> pan) {
        audio->PlaySound(handle, volume.value_or(1.0f), pan.value_or(0.0f));
    };

    snd["setSoundVolume"] = [audio](int handle, float vol) {
        audio->SetSoundVolume(handle, vol);
    };

    snd["setMasterVolume"] = [audio](float vol) {
        audio->SetMasterVolume(vol);
    };

    snd["playMusic"] = [audio](int handle, sol::optional<bool> loop, sol::optional<float> volume) {
        audio->PlayMusic(handle, loop.value_or(true), volume.value_or(1.0f));
    };

    snd["stopMusic"] = [audio]() {
        audio->StopMusic();
    };

    snd["pauseMusic"] = [audio]() {
        audio->PauseMusic();
    };

    snd["resumeMusic"] = [audio]() {
        audio->ResumeMusic();
    };

    snd["isMusicPlaying"] = [audio]() -> bool {
        return audio->IsMusicPlaying();
    };

    snd["fadeInMusic"] = [audio](float seconds) {
        audio->FadeInMusic(seconds);
    };

    snd["fadeOutMusic"] = [audio](float seconds) {
        audio->FadeOutMusic(seconds);
    };

    snd["releaseSound"] = [audio](int handle) {
        audio->ReleaseSound(handle);
    };
}

void RegisterGUI(sol::state& lua, GUI::UIContext* ctx)
{
    if (!ctx) return;
    auto gui = lua.create_named_table("gui");

    gui["hasRoot"] = [ctx]() -> bool {
        return ctx->GetRoot() != nullptr;
    };

    gui["getRootChildCount"] = [ctx]() -> int {
        auto* root = ctx->GetRoot();
        if (!root) return 0;
        return static_cast<int>(root->GetChildren().size());
    };

    gui["findById"] = [ctx](const std::string& id) -> bool {
        return ctx->FindById(gx::String(id)) != nullptr;
    };
}

void RegisterFileSystem(sol::state& lua)
{
    auto fs = lua.create_named_table("fs");

    fs["exists"] = [](const std::string& path) -> bool {
        return FileSystem::Instance().Exists(gx::String(path));
    };

    fs["readFile"] = [](const std::string& path) -> std::string {
        FileData data = FileSystem::Instance().ReadFile(gx::String(path));
        if (!data.IsValid()) return {};
        gx::String result = data.AsString();
        return std::string(result.c_str(), result.size());
    };

    fs["writeFile"] = [](const std::string& path, const std::string& content) -> bool {
        return FileSystem::Instance().WriteFile(gx::String(path), content.data(), content.size());
    };
}

void RegisterECS(sol::state& lua, ecs::World* world)
{
    if (!world) return;
    auto ecstbl = lua.create_named_table("ecs");

    ecstbl["createEntity"] = [world]() -> uint32_t {
        return world->CreateEntity();
    };

    ecstbl["destroyEntity"] = [world](uint32_t id) {
        world->DestroyEntity(id);
    };

    ecstbl["isAlive"] = [world](uint32_t id) -> bool {
        return world->IsAlive(id);
    };

    ecstbl["getEntityCount"] = [world]() -> uint32_t {
        return world->GetEntityCount();
    };

    ecstbl["getSystemCount"] = [world]() -> uint32_t {
        return world->GetSystemCount();
    };
}

} // namespace ScriptBindings
} // namespace gx

#endif // GX_ENABLE_LUA
