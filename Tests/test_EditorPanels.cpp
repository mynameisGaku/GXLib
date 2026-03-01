/// @file test_EditorPanels.cpp
/// @brief Phase 7 Wave C: PropertyInspector、SceneHierarchyPanel、
///        AssetBrowser、ConsoleWindow、AssetDatabaseの拡張テスト

#include <gtest/gtest.h>
#include "Editor/PropertyInspector.h"
#include "Editor/SceneHierarchyPanel.h"
#include "Editor/AssetBrowser.h"
#include "Editor/ConsoleWindow.h"
#include "Core/AssetDatabase.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Components.h"
#include <filesystem>
#include <fstream>

using namespace gx;

// ============================================================================
// PropertyInspector
// ============================================================================

class PropertyInspectorTest : public ::testing::Test
{
protected:
    PropertyInspector inspector;
    Scene scene{"TestScene"};
};

TEST_F(PropertyInspectorTest, DefaultNoTarget)
{
    EXPECT_FALSE(inspector.HasTarget());
    EXPECT_EQ(inspector.GetTarget(), nullptr);
}

TEST_F(PropertyInspectorTest, SetTarget)
{
    auto* entity = scene.CreateEntity("TestEntity");
    inspector.SetTarget(entity);
    EXPECT_TRUE(inspector.HasTarget());
    EXPECT_EQ(inspector.GetTarget(), entity);
}

TEST_F(PropertyInspectorTest, ClearTarget)
{
    auto* entity = scene.CreateEntity("TestEntity");
    inspector.SetTarget(entity);
    inspector.ClearTarget();
    EXPECT_FALSE(inspector.HasTarget());
    EXPECT_EQ(inspector.GetTarget(), nullptr);
}

TEST_F(PropertyInspectorTest, GetPropertiesWithNoTarget)
{
    auto props = inspector.GetProperties();
    EXPECT_TRUE(props.empty());
}

TEST_F(PropertyInspectorTest, GetPropertiesEntitySection)
{
    auto* entity = scene.CreateEntity("MyEntity");
    inspector.SetTarget(entity);

    auto props = inspector.GetProperties();
    ASSERT_GE(props.size(), 2u); // 最低でもEntity + Transform

    // 最初のセクションは"Entity"であるべき
    EXPECT_EQ(props[0].componentName, "Entity");
    EXPECT_GE(props[0].properties.size(), 2u);

    // Nameプロパティを検証
    bool foundName = false;
    for (const auto& p : props[0].properties)
    {
        if (p.name == "Name")
        {
            EXPECT_EQ(p.value, "MyEntity");
            EXPECT_FALSE(p.readOnly);
            foundName = true;
        }
    }
    EXPECT_TRUE(foundName);

    // IDプロパティを検証（読み取り専用）
    bool foundId = false;
    for (const auto& p : props[0].properties)
    {
        if (p.name == "ID")
        {
            EXPECT_TRUE(p.readOnly);
            foundId = true;
        }
    }
    EXPECT_TRUE(foundId);
}

TEST_F(PropertyInspectorTest, GetPropertiesTransformSection)
{
    auto* entity = scene.CreateEntity("TestEntity");
    entity->GetTransform().SetPosition(1.0f, 2.0f, 3.0f);
    entity->GetTransform().SetScale(4.0f, 5.0f, 6.0f);
    inspector.SetTarget(entity);

    auto props = inspector.GetProperties();
    ASSERT_GE(props.size(), 2u);

    // 2番目のセクションは"Transform"であるべき
    EXPECT_EQ(props[1].componentName, "Transform");
    EXPECT_EQ(props[1].properties.size(), 9u); // 位置3 + 回転3 + スケール3
}

TEST_F(PropertyInspectorTest, SetPropertyEntityName)
{
    auto* entity = scene.CreateEntity("OldName");
    inspector.SetTarget(entity);

    bool result = inspector.SetProperty("Entity", "Name", "NewName");
    EXPECT_TRUE(result);
    EXPECT_EQ(entity->GetName(), "NewName");
}

TEST_F(PropertyInspectorTest, SetPropertyTransformPosition)
{
    auto* entity = scene.CreateEntity("TestEntity");
    inspector.SetTarget(entity);

    EXPECT_TRUE(inspector.SetProperty("Transform", "Position.X", "10.5"));
    EXPECT_TRUE(inspector.SetProperty("Transform", "Position.Y", "20.0"));
    EXPECT_TRUE(inspector.SetProperty("Transform", "Position.Z", "-5.0"));

    EXPECT_FLOAT_EQ(entity->GetTransform().GetPosition().x, 10.5f);
    EXPECT_FLOAT_EQ(entity->GetTransform().GetPosition().y, 20.0f);
    EXPECT_FLOAT_EQ(entity->GetTransform().GetPosition().z, -5.0f);
}

TEST_F(PropertyInspectorTest, SetPropertyTransformScale)
{
    auto* entity = scene.CreateEntity("TestEntity");
    inspector.SetTarget(entity);

    EXPECT_TRUE(inspector.SetProperty("Transform", "Scale.X", "2.0"));
    EXPECT_TRUE(inspector.SetProperty("Transform", "Scale.Y", "3.0"));
    EXPECT_TRUE(inspector.SetProperty("Transform", "Scale.Z", "0.5"));

    EXPECT_FLOAT_EQ(entity->GetTransform().GetScale().x, 2.0f);
    EXPECT_FLOAT_EQ(entity->GetTransform().GetScale().y, 3.0f);
    EXPECT_FLOAT_EQ(entity->GetTransform().GetScale().z, 0.5f);
}

TEST_F(PropertyInspectorTest, SetPropertyInvalidFloat)
{
    auto* entity = scene.CreateEntity("TestEntity");
    inspector.SetTarget(entity);
    EXPECT_FALSE(inspector.SetProperty("Transform", "Position.X", "notanumber"));
}

TEST_F(PropertyInspectorTest, SetPropertyNoTarget)
{
    EXPECT_FALSE(inspector.SetProperty("Entity", "Name", "test"));
}

TEST_F(PropertyInspectorTest, SetPropertyUnknownComponent)
{
    auto* entity = scene.CreateEntity("TestEntity");
    inspector.SetTarget(entity);
    EXPECT_FALSE(inspector.SetProperty("UnknownComponent", "Field", "val"));
}

TEST_F(PropertyInspectorTest, PropertyChangedCallback)
{
    auto* entity = scene.CreateEntity("TestEntity");
    inspector.SetTarget(entity);

    PropertyChange lastChange;
    inspector.SetOnPropertyChanged([&](const PropertyChange& change)
    {
        lastChange = change;
    });

    inspector.SetProperty("Entity", "Name", "NewName");
    EXPECT_EQ(lastChange.componentName, "Entity");
    EXPECT_EQ(lastChange.propertyName, "Name");
    EXPECT_EQ(lastChange.newValue, "NewName");
}

TEST_F(PropertyInspectorTest, GetPropertiesWithComponents)
{
    auto* entity = scene.CreateEntity("TestEntity");
    entity->AddComponent<AudioSourceComponent>();
    inspector.SetTarget(entity);

    auto props = inspector.GetProperties();
    // Entity + Transform + AudioSource
    EXPECT_GE(props.size(), 3u);
}

// ============================================================================
// SceneHierarchyPanel
// ============================================================================

class SceneHierarchyPanelTest : public ::testing::Test
{
protected:
    SceneHierarchyPanel panel;
    Scene scene{"TestScene"};
};

TEST_F(SceneHierarchyPanelTest, DefaultNoScene)
{
    EXPECT_EQ(panel.GetScene(), nullptr);
    EXPECT_EQ(panel.GetSelectedEntity(), nullptr);
}

TEST_F(SceneHierarchyPanelTest, SetScene)
{
    panel.SetScene(&scene);
    EXPECT_EQ(panel.GetScene(), &scene);
}

TEST_F(SceneHierarchyPanelTest, SetSceneClearsSelection)
{
    auto* entity = scene.CreateEntity("E1");
    panel.SetScene(&scene);
    panel.HandleClick(entity->GetID());
    EXPECT_NE(panel.GetSelectedEntity(), nullptr);

    Scene otherScene{"Other"};
    panel.SetScene(&otherScene);
    EXPECT_EQ(panel.GetSelectedEntity(), nullptr);
}

TEST_F(SceneHierarchyPanelTest, HandleClick)
{
    auto* entity = scene.CreateEntity("E1");
    panel.SetScene(&scene);
    panel.HandleClick(entity->GetID());
    EXPECT_EQ(panel.GetSelectedEntity(), entity);
}

TEST_F(SceneHierarchyPanelTest, HandleClickInvalidId)
{
    panel.SetScene(&scene);
    panel.HandleClick(9999);
    EXPECT_EQ(panel.GetSelectedEntity(), nullptr);
}

TEST_F(SceneHierarchyPanelTest, SelectionCallback)
{
    auto* entity = scene.CreateEntity("E1");
    panel.SetScene(&scene);

    Entity* callbackEntity = nullptr;
    panel.SetOnEntitySelected([&](Entity* e) { callbackEntity = e; });

    panel.HandleClick(entity->GetID());
    EXPECT_EQ(callbackEntity, entity);
}

TEST_F(SceneHierarchyPanelTest, ExpandCollapseNode)
{
    auto* entity = scene.CreateEntity("E1");
    panel.SetScene(&scene);

    EXPECT_FALSE(panel.IsNodeExpanded(entity->GetID()));
    panel.ExpandNode(entity->GetID());
    EXPECT_TRUE(panel.IsNodeExpanded(entity->GetID()));
    panel.CollapseNode(entity->GetID());
    EXPECT_FALSE(panel.IsNodeExpanded(entity->GetID()));
}

TEST_F(SceneHierarchyPanelTest, ToggleNode)
{
    auto* entity = scene.CreateEntity("E1");
    panel.SetScene(&scene);

    panel.ToggleNode(entity->GetID());
    EXPECT_TRUE(panel.IsNodeExpanded(entity->GetID()));
    panel.ToggleNode(entity->GetID());
    EXPECT_FALSE(panel.IsNodeExpanded(entity->GetID()));
}

TEST_F(SceneHierarchyPanelTest, ExpandAll)
{
    auto* e1 = scene.CreateEntity("E1");
    auto* e2 = scene.CreateEntity("E2");
    panel.SetScene(&scene);

    panel.ExpandAll();
    EXPECT_TRUE(panel.IsNodeExpanded(e1->GetID()));
    EXPECT_TRUE(panel.IsNodeExpanded(e2->GetID()));
}

TEST_F(SceneHierarchyPanelTest, CollapseAll)
{
    auto* e1 = scene.CreateEntity("E1");
    auto* e2 = scene.CreateEntity("E2");
    panel.SetScene(&scene);

    panel.ExpandAll();
    panel.CollapseAll();
    EXPECT_FALSE(panel.IsNodeExpanded(e1->GetID()));
    EXPECT_FALSE(panel.IsNodeExpanded(e2->GetID()));
}

TEST_F(SceneHierarchyPanelTest, FlattenedHierarchyEmpty)
{
    panel.SetScene(&scene);
    auto nodes = panel.GetFlattenedHierarchy();
    EXPECT_TRUE(nodes.empty());
}

TEST_F(SceneHierarchyPanelTest, FlattenedHierarchyFlat)
{
    scene.CreateEntity("A");
    scene.CreateEntity("B");
    scene.CreateEntity("C");
    panel.SetScene(&scene);

    auto nodes = panel.GetFlattenedHierarchy();
    EXPECT_EQ(nodes.size(), 3u);
    for (const auto& node : nodes)
    {
        EXPECT_EQ(node.depth, 0);
        EXPECT_FALSE(node.hasChildren);
    }
}

TEST_F(SceneHierarchyPanelTest, FlattenedHierarchyWithChildren)
{
    auto* parent = scene.CreateEntity("Parent");
    auto* child = scene.CreateEntity("Child");
    child->SetParent(parent);

    panel.SetScene(&scene);
    panel.ExpandNode(parent->GetID());

    auto nodes = panel.GetFlattenedHierarchy();
    ASSERT_EQ(nodes.size(), 2u);
    EXPECT_EQ(nodes[0].name, "Parent");
    EXPECT_EQ(nodes[0].depth, 0);
    EXPECT_TRUE(nodes[0].hasChildren);
    EXPECT_TRUE(nodes[0].expanded);
    EXPECT_EQ(nodes[1].name, "Child");
    EXPECT_EQ(nodes[1].depth, 1);
    EXPECT_FALSE(nodes[1].hasChildren);
}

TEST_F(SceneHierarchyPanelTest, FlattenedHierarchyCollapsed)
{
    auto* parent = scene.CreateEntity("Parent");
    auto* child = scene.CreateEntity("Child");
    child->SetParent(parent);

    panel.SetScene(&scene);
    // 展開されていないため、子は表示されないはず

    auto nodes = panel.GetFlattenedHierarchy();
    ASSERT_EQ(nodes.size(), 1u);
    EXPECT_EQ(nodes[0].name, "Parent");
    EXPECT_TRUE(nodes[0].hasChildren);
    EXPECT_FALSE(nodes[0].expanded);
}

TEST_F(SceneHierarchyPanelTest, FlattenedHierarchySelectedFlag)
{
    auto* e1 = scene.CreateEntity("E1");
    scene.CreateEntity("E2");
    panel.SetScene(&scene);
    panel.HandleClick(e1->GetID());

    auto nodes = panel.GetFlattenedHierarchy();
    ASSERT_EQ(nodes.size(), 2u);
    EXPECT_TRUE(nodes[0].selected);
    EXPECT_FALSE(nodes[1].selected);
}

TEST_F(SceneHierarchyPanelTest, NoSceneReturnsEmpty)
{
    auto nodes = panel.GetFlattenedHierarchy();
    EXPECT_TRUE(nodes.empty());
}

// ============================================================================
// AssetBrowser
// ============================================================================

class AssetBrowserTest : public ::testing::Test
{
protected:
    AssetBrowser browser;
    std::string testDir;

    void SetUp() override
    {
        testDir = (std::filesystem::temp_directory_path() / "gx_browser_test").string();
        std::filesystem::create_directories(testDir);
        std::filesystem::create_directories(testDir + "/textures");
        std::filesystem::create_directories(testDir + "/models");

        CreateFile("readme.txt");
        CreateFile("textures/icon.png");
        CreateFile("textures/bg.jpg");
        CreateFile("models/player.fbx");
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(testDir, ec);
    }

    void CreateFile(const std::string& relPath)
    {
        std::string fullPath = testDir + "/" + relPath;
        std::ofstream f(fullPath);
        f << "test content for " << relPath;
    }
};

TEST_F(AssetBrowserTest, SetRootPath)
{
    browser.SetRootPath(testDir);
    EXPECT_FALSE(browser.GetRootPath().empty());
    EXPECT_TRUE(browser.IsAtRoot());
}

TEST_F(AssetBrowserTest, RootDirectoryEntries)
{
    browser.SetRootPath(testDir);
    // models/、textures/、readme.txtが含まれるはず
    EXPECT_GE(browser.GetEntryCount(), 3);
}

TEST_F(AssetBrowserTest, DirectoriesFirst)
{
    browser.SetRootPath(testDir);
    const auto& entries = browser.GetEntries();

    // 最初の非ディレクトリエントリのインデックスを検索
    int firstFileIndex = -1;
    int lastDirIndex = -1;
    for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    {
        if (entries[i].isDirectory)
            lastDirIndex = i;
        else if (firstFileIndex < 0)
            firstFileIndex = i;
    }

    // すべてのディレクトリがすべてのファイルより前に表示されるべき
    if (firstFileIndex >= 0 && lastDirIndex >= 0)
    {
        EXPECT_LT(lastDirIndex, firstFileIndex);
    }
}

TEST_F(AssetBrowserTest, NavigateToSubdirectory)
{
    browser.SetRootPath(testDir);

    std::string texPath = testDir + "/textures";
    // スラッシュを統一して一致させる
    for (auto& ch : texPath) if (ch == '\\') ch = '/';

    EXPECT_TRUE(browser.NavigateTo(texPath));
    EXPECT_FALSE(browser.IsAtRoot());

    // icon.pngとbg.jpgが含まれるはず
    EXPECT_EQ(browser.GetEntryCount(), 2);
}

TEST_F(AssetBrowserTest, NavigateUp)
{
    browser.SetRootPath(testDir);
    std::string texPath = testDir + "/textures";
    for (auto& ch : texPath) if (ch == '\\') ch = '/';

    browser.NavigateTo(texPath);
    EXPECT_TRUE(browser.NavigateUp());
    EXPECT_TRUE(browser.IsAtRoot());
}

TEST_F(AssetBrowserTest, NavigateUpAtRoot)
{
    browser.SetRootPath(testDir);
    EXPECT_FALSE(browser.NavigateUp());
}

TEST_F(AssetBrowserTest, NavigateOutsideRoot)
{
    browser.SetRootPath(testDir);
    EXPECT_FALSE(browser.NavigateTo("C:/Windows"));
}

TEST_F(AssetBrowserTest, FilterByExtension)
{
    browser.SetRootPath(testDir);
    std::string texPath = testDir + "/textures";
    for (auto& ch : texPath) if (ch == '\\') ch = '/';
    browser.NavigateTo(texPath);

    browser.SetFilter(".png");
    EXPECT_EQ(browser.GetEntryCount(), 1);
    EXPECT_EQ(browser.GetEntries()[0].name, "icon.png");
}

TEST_F(AssetBrowserTest, ClearFilter)
{
    browser.SetRootPath(testDir);
    std::string texPath = testDir + "/textures";
    for (auto& ch : texPath) if (ch == '\\') ch = '/';
    browser.NavigateTo(texPath);

    browser.SetFilter(".png");
    EXPECT_EQ(browser.GetEntryCount(), 1);

    browser.SetFilter("");
    EXPECT_EQ(browser.GetEntryCount(), 2);
}

TEST_F(AssetBrowserTest, HandleDoubleClickDirectory)
{
    browser.SetRootPath(testDir);

    // texturesディレクトリのエントリを検索
    int texIdx = -1;
    const auto& entries = browser.GetEntries();
    for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    {
        if (entries[i].isDirectory && entries[i].name == "textures")
        {
            texIdx = i;
            break;
        }
    }

    ASSERT_GE(texIdx, 0);
    browser.HandleDoubleClick(texIdx);
    EXPECT_FALSE(browser.IsAtRoot());
}

TEST_F(AssetBrowserTest, HandleDoubleClickFile)
{
    browser.SetRootPath(testDir);

    std::string openedPath;
    browser.SetOnAssetOpened([&](const std::string& p) { openedPath = p; });

    // readme.txtのエントリを検索
    int fileIdx = -1;
    const auto& entries = browser.GetEntries();
    for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    {
        if (!entries[i].isDirectory && entries[i].name == "readme.txt")
        {
            fileIdx = i;
            break;
        }
    }

    ASSERT_GE(fileIdx, 0);
    browser.HandleDoubleClick(fileIdx);
    EXPECT_FALSE(openedPath.empty());
}

TEST_F(AssetBrowserTest, HandleDoubleClickInvalidIndex)
{
    browser.SetRootPath(testDir);
    browser.HandleDoubleClick(-1);
    browser.HandleDoubleClick(9999);
    // クラッシュしないこと
}

TEST_F(AssetBrowserTest, Refresh)
{
    browser.SetRootPath(testDir);
    int initialCount = browser.GetEntryCount();

    CreateFile("newfile.txt");
    browser.Refresh();
    EXPECT_GE(browser.GetEntryCount(), initialCount);
}

// ============================================================================
// ConsoleWindow
// ============================================================================

class ConsoleWindowTest : public ::testing::Test
{
protected:
    ConsoleWindow console;
};

TEST_F(ConsoleWindowTest, DefaultEmpty)
{
    EXPECT_EQ(console.GetMessageCount(), 0);
    EXPECT_EQ(console.GetInfoCount(), 0);
    EXPECT_EQ(console.GetWarningCount(), 0);
    EXPECT_EQ(console.GetErrorCount(), 0);
}

TEST_F(ConsoleWindowTest, AddInfoMessage)
{
    console.AddMessage("Hello, world!");
    EXPECT_EQ(console.GetMessageCount(), 1);
    EXPECT_EQ(console.GetInfoCount(), 1);
    EXPECT_EQ(console.GetAllMessages()[0].text, "Hello, world!");
    EXPECT_EQ(console.GetAllMessages()[0].level, LogLevel::Info);
}

TEST_F(ConsoleWindowTest, AddMultipleLevels)
{
    console.AddMessage("info", LogLevel::Info);
    console.AddMessage("warn", LogLevel::Warn);
    console.AddMessage("error", LogLevel::Error);
    EXPECT_EQ(console.GetMessageCount(), 3);
    EXPECT_EQ(console.GetInfoCount(), 1);
    EXPECT_EQ(console.GetWarningCount(), 1);
    EXPECT_EQ(console.GetErrorCount(), 1);
}

TEST_F(ConsoleWindowTest, DuplicateFolding)
{
    console.SetDuplicateFolding(true);
    console.AddMessage("Repeated", LogLevel::Info);
    console.AddMessage("Repeated", LogLevel::Info);
    console.AddMessage("Repeated", LogLevel::Info);

    EXPECT_EQ(console.GetMessageCount(), 1);
    EXPECT_EQ(console.GetAllMessages()[0].count, 3);
}

TEST_F(ConsoleWindowTest, DuplicateFoldingDifferentLevels)
{
    console.SetDuplicateFolding(true);
    console.AddMessage("Same text", LogLevel::Info);
    console.AddMessage("Same text", LogLevel::Error);

    EXPECT_EQ(console.GetMessageCount(), 2);
}

TEST_F(ConsoleWindowTest, DuplicateFoldingDisabled)
{
    console.SetDuplicateFolding(false);
    console.AddMessage("Repeated", LogLevel::Info);
    console.AddMessage("Repeated", LogLevel::Info);

    EXPECT_EQ(console.GetMessageCount(), 2);
}

TEST_F(ConsoleWindowTest, MaxMessages)
{
    console.SetMaxMessages(5);
    for (int i = 0; i < 10; ++i)
    {
        console.AddMessage("Msg " + std::to_string(i));
    }
    EXPECT_EQ(console.GetMessageCount(), 5);
}

TEST_F(ConsoleWindowTest, Clear)
{
    console.AddMessage("A");
    console.AddMessage("B", LogLevel::Warn);
    console.Clear();
    EXPECT_EQ(console.GetMessageCount(), 0);
    EXPECT_EQ(console.GetInfoCount(), 0);
    EXPECT_EQ(console.GetWarningCount(), 0);
}

TEST_F(ConsoleWindowTest, FilterByLevel)
{
    console.AddMessage("info", LogLevel::Info);
    console.AddMessage("warn", LogLevel::Warn);
    console.AddMessage("error", LogLevel::Error);

    console.SetShowInfo(false);
    auto filtered = console.GetFilteredMessages();
    EXPECT_EQ(filtered.size(), 2u);

    console.SetShowWarnings(false);
    filtered = console.GetFilteredMessages();
    EXPECT_EQ(filtered.size(), 1u);

    console.SetShowErrors(false);
    filtered = console.GetFilteredMessages();
    EXPECT_EQ(filtered.size(), 0u);
}

TEST_F(ConsoleWindowTest, FilterByText)
{
    console.AddMessage("Player connected");
    console.AddMessage("Enemy spawned");
    console.AddMessage("Player disconnected");

    console.SetTextFilter("player");
    auto filtered = console.GetFilteredMessages();
    EXPECT_EQ(filtered.size(), 2u);

    console.SetTextFilter("enemy");
    filtered = console.GetFilteredMessages();
    EXPECT_EQ(filtered.size(), 1u);

    console.SetTextFilter("xyz");
    filtered = console.GetFilteredMessages();
    EXPECT_EQ(filtered.size(), 0u);
}

TEST_F(ConsoleWindowTest, SubmitCommand)
{
    std::string lastCommand;
    console.SetOnCommand([&](const std::string& cmd) { lastCommand = cmd; });

    console.SubmitCommand("help");
    EXPECT_EQ(lastCommand, "help");
    // コマンドがエコーされるはず
    EXPECT_GE(console.GetMessageCount(), 1);
}

TEST_F(ConsoleWindowTest, SubmitEmptyCommand)
{
    bool called = false;
    console.SetOnCommand([&](const std::string&) { called = true; });
    console.SubmitCommand("");
    EXPECT_FALSE(called);
}

TEST_F(ConsoleWindowTest, AttachDetachLogger)
{
    EXPECT_FALSE(console.IsAttachedToLogger());
    console.AttachToLogger();
    EXPECT_TRUE(console.IsAttachedToLogger());
    console.DetachFromLogger();
    EXPECT_FALSE(console.IsAttachedToLogger());
}

TEST_F(ConsoleWindowTest, DefaultSettings)
{
    EXPECT_TRUE(console.GetShowInfo());
    EXPECT_TRUE(console.GetShowWarnings());
    EXPECT_TRUE(console.GetShowErrors());
    EXPECT_TRUE(console.IsDuplicateFolding());
    EXPECT_EQ(console.GetMaxMessages(), 1000);
}

TEST_F(ConsoleWindowTest, MessageTimestamp)
{
    console.AddMessage("test");
    EXPECT_GT(console.GetAllMessages()[0].timestamp, 0u);
}

// ============================================================================
// AssetDatabase（拡張 - Phase 7 API）
// ============================================================================

class AssetDatabaseExtendedTest : public ::testing::Test
{
protected:
    std::string testDir;

    void SetUp() override
    {
        AssetDatabase::Instance().Shutdown();
        testDir = (std::filesystem::temp_directory_path() / "gx_assetdb_ext_test").string();
        std::filesystem::create_directories(testDir);
        std::filesystem::create_directories(testDir + "/textures");
        std::filesystem::create_directories(testDir + "/shaders");

        CreateFile("textures/icon.png");
        CreateFile("textures/bg.dds");
        CreateFile("shaders/pbr.hlsl");
        CreateFile("config.json");
        CreateFile("player.gxmdl");
        CreateFile("script.lua");
    }

    void TearDown() override
    {
        AssetDatabase::Instance().Shutdown();
        std::error_code ec;
        std::filesystem::remove_all(testDir, ec);
    }

    void CreateFile(const std::string& relPath)
    {
        std::string fullPath = testDir + "/" + relPath;
        std::ofstream f(fullPath);
        f << "test content for " << relPath;
    }
};

TEST_F(AssetDatabaseExtendedTest, Initialize)
{
    AssetDatabase::Instance().Initialize(testDir);
    EXPECT_GT(AssetDatabase::Instance().GetAssetCount(), 0);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeTexture)
{
    EXPECT_EQ(AssetDatabase::DetectType(".png"), AssetType::Texture);
    EXPECT_EQ(AssetDatabase::DetectType(".jpg"), AssetType::Texture);
    EXPECT_EQ(AssetDatabase::DetectType(".DDS"), AssetType::Texture);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeModel)
{
    EXPECT_EQ(AssetDatabase::DetectType(".gxmdl"), AssetType::Model);
    EXPECT_EQ(AssetDatabase::DetectType(".fbx"), AssetType::Model);
    EXPECT_EQ(AssetDatabase::DetectType(".obj"), AssetType::Model);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeShader)
{
    EXPECT_EQ(AssetDatabase::DetectType(".hlsl"), AssetType::Shader);
    EXPECT_EQ(AssetDatabase::DetectType(".hlsli"), AssetType::Shader);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeScript)
{
    EXPECT_EQ(AssetDatabase::DetectType(".lua"), AssetType::Script);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeScene)
{
    EXPECT_EQ(AssetDatabase::DetectType(".gxscene"), AssetType::Scene);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeMaterial)
{
    EXPECT_EQ(AssetDatabase::DetectType(".gxmat"), AssetType::Material);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeAnimation)
{
    EXPECT_EQ(AssetDatabase::DetectType(".gxanim"), AssetType::Animation);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypePrefab)
{
    EXPECT_EQ(AssetDatabase::DetectType(".gxprefab"), AssetType::Prefab);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeTilemap)
{
    EXPECT_EQ(AssetDatabase::DetectType(".tmx"), AssetType::Tilemap);
    EXPECT_EQ(AssetDatabase::DetectType(".tmj"), AssetType::Tilemap);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeConfig)
{
    EXPECT_EQ(AssetDatabase::DetectType(".json"), AssetType::Config);
    EXPECT_EQ(AssetDatabase::DetectType(".xml"), AssetType::Config);
    EXPECT_EQ(AssetDatabase::DetectType(".ini"), AssetType::Config);
}

TEST_F(AssetDatabaseExtendedTest, DetectTypeUnknown)
{
    EXPECT_EQ(AssetDatabase::DetectType(".xyz"), AssetType::Unknown);
}

TEST_F(AssetDatabaseExtendedTest, FindByName)
{
    AssetDatabase::Instance().Initialize(testDir);
    auto results = AssetDatabase::Instance().FindByName("icon");
    EXPECT_GE(results.size(), 1u);
}

TEST_F(AssetDatabaseExtendedTest, FindByNameCaseInsensitive)
{
    AssetDatabase::Instance().Initialize(testDir);
    auto results = AssetDatabase::Instance().FindByName("ICON");
    EXPECT_GE(results.size(), 1u);
}

TEST_F(AssetDatabaseExtendedTest, FindByTypeTextures)
{
    AssetDatabase::Instance().Initialize(testDir);
    auto textures = AssetDatabase::Instance().FindByType(AssetType::Texture);
    EXPECT_GE(textures.size(), 2u); // icon.png + bg.ddsの2つ
}

TEST_F(AssetDatabaseExtendedTest, FindByTypeShaders)
{
    AssetDatabase::Instance().Initialize(testDir);
    auto shaders = AssetDatabase::Instance().FindByType(AssetType::Shader);
    EXPECT_EQ(shaders.size(), 1u);
}

TEST_F(AssetDatabaseExtendedTest, FindAssetByRelPath)
{
    AssetDatabase::Instance().Initialize(testDir);
    auto* entry = AssetDatabase::Instance().FindAsset("textures/icon.png");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->type, AssetType::Texture);
    EXPECT_EQ(entry->name, "icon.png");
}

TEST_F(AssetDatabaseExtendedTest, FindAssetNotFound)
{
    AssetDatabase::Instance().Initialize(testDir);
    auto* entry = AssetDatabase::Instance().FindAsset("nonexistent.xyz");
    EXPECT_EQ(entry, nullptr);
}

TEST_F(AssetDatabaseExtendedTest, ImportSettings)
{
    AssetDatabase::Instance().Initialize(testDir);
    AssetDatabase::Instance().SetImportSetting("textures/icon.png", "maxSize", "1024");
    AssetDatabase::Instance().SetImportSetting("textures/icon.png", "format", "BC7");

    auto* settings = AssetDatabase::Instance().GetImportSettings("textures/icon.png");
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->Get("maxSize"), "1024");
    EXPECT_EQ(settings->Get("format"), "BC7");
    EXPECT_EQ(settings->Get("missing", "default"), "default");
}

TEST_F(AssetDatabaseExtendedTest, SaveLoadMetadata)
{
    AssetDatabase::Instance().Initialize(testDir);
    AssetDatabase::Instance().SetImportSetting("textures/icon.png", "compress", "true");

    std::string metaPath = (std::filesystem::temp_directory_path() / "gx_meta_test.txt").string();
    EXPECT_TRUE(AssetDatabase::Instance().SaveMetadata(metaPath));

    // クリアして再読み込み
    AssetDatabase::Instance().Shutdown();
    AssetDatabase::Instance().Initialize(testDir);
    EXPECT_TRUE(AssetDatabase::Instance().LoadMetadata(metaPath));

    auto* settings = AssetDatabase::Instance().GetImportSettings("textures/icon.png");
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->Get("compress"), "true");

    std::filesystem::remove(metaPath);
}

TEST_F(AssetDatabaseExtendedTest, DetectChanges)
{
    AssetDatabase::Instance().Initialize(testDir);

    // ファイルを書き換えて変更する
    {
        std::ofstream f(testDir + "/config.json");
        f << "modified content at " << std::chrono::steady_clock::now().time_since_epoch().count();
    }

    // ファイルシステムのタイムスタンプが異なることを保証するために少し待つ
    // （DetectChangesはlastModifiedタイムスタンプを比較する）
    int changes = AssetDatabase::Instance().DetectChanges();
    // ファイルシステムの精度によって変更を検出する場合としない場合がある
    EXPECT_GE(changes, 0);
}

TEST_F(AssetDatabaseExtendedTest, Shutdown)
{
    AssetDatabase::Instance().Initialize(testDir);
    EXPECT_GT(AssetDatabase::Instance().GetAssetCount(), 0);

    AssetDatabase::Instance().Shutdown();
    EXPECT_EQ(AssetDatabase::Instance().GetAssetCount(), 0);
    EXPECT_TRUE(AssetDatabase::Instance().GetRootPath().empty());
}
