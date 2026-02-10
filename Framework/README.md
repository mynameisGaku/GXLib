# GXLib Framework Template

This folder provides a small wrapper so users only implement scenes.

## How to use

1. Put your FBX model in:
   - `Framework/Assets/Models/Character.fbx`
2. Build the solution and run `GXFramework`.
3. To create your own scene:
   - Add a new class that inherits `Scene`
   - Implement `Update` and `Render`
   - Register it in `Framework/src/main.cpp`

## Minimal scene example

```cpp
class MyScene : public GXFW::Scene
{
public:
    const char* GetName() const override { return "MyScene"; }
    void Update(GXFW::SceneContext& ctx, float dt) override { (void)ctx; (void)dt; }
    void Render(GXFW::SceneContext& ctx) override { (void)ctx; }
};
```
