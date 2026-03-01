/// @file ScenePersistence.cpp
/// @brief ScenePersistence の実装
#include "pch_common.h"
#include "Core/Scene/ScenePersistence.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Components.h"
#include <fstream>
#include <sstream>

namespace gx
{

// ============================================================================
// テキスト形式ヘルパー
// ============================================================================

/// @brief テキスト形式のヘッダー
static constexpr const char* k_TextHeader = "GXSCENE 1";

/// @brief エンティティ区切り
static constexpr const char* k_TextSeparator = "---";

// ============================================================================
// バイナリ形式ヘルパー
// ============================================================================

/// @brief バイナリ形式マジック（8 bytes: "GXSCBIN\0"）
static constexpr char k_BinaryMagic[8] = { 'G', 'X', 'S', 'C', 'B', 'I', 'N', '\0' };

/// @brief バイナリ形式バージョン
static constexpr uint32_t k_BinaryVersion = 1;

// --- バイナリ書き込みヘルパー ---

static void WriteU32(std::vector<uint8_t>& buf, uint32_t value)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&value);
    buf.insert(buf.end(), p, p + sizeof(uint32_t));
}

static void WriteU8(std::vector<uint8_t>& buf, uint8_t value)
{
    buf.push_back(value);
}

static void WriteFloat(std::vector<uint8_t>& buf, float value)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&value);
    buf.insert(buf.end(), p, p + sizeof(float));
}

static void WriteString(std::vector<uint8_t>& buf, const std::string& str)
{
    WriteU32(buf, static_cast<uint32_t>(str.size()));
    buf.insert(buf.end(), str.begin(), str.end());
}

static void WriteBytes(std::vector<uint8_t>& buf, const void* data, size_t size)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    buf.insert(buf.end(), p, p + size);
}

// --- バイナリ読み取りヘルパー ---

/// @brief バイナリ読み取りカーソル
struct BinaryReader
{
    const uint8_t* data;
    size_t size;
    size_t offset;

    bool CanRead(size_t bytes) const { return offset + bytes <= size; }

    bool ReadU32(uint32_t& out)
    {
        if (!CanRead(sizeof(uint32_t))) return false;
        std::memcpy(&out, data + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        return true;
    }

    bool ReadU8(uint8_t& out)
    {
        if (!CanRead(1)) return false;
        out = data[offset++];
        return true;
    }

    bool ReadFloat(float& out)
    {
        if (!CanRead(sizeof(float))) return false;
        std::memcpy(&out, data + offset, sizeof(float));
        offset += sizeof(float);
        return true;
    }

    bool ReadString(std::string& out)
    {
        uint32_t len = 0;
        if (!ReadU32(len)) return false;
        if (!CanRead(len)) return false;
        out.assign(reinterpret_cast<const char*>(data + offset), len);
        offset += len;
        return true;
    }

    bool ReadBytes(void* dst, size_t bytes)
    {
        if (!CanRead(bytes)) return false;
        std::memcpy(dst, data + offset, bytes);
        offset += bytes;
        return true;
    }
};

// ============================================================================
// テキスト形式の TagComponent 検索ヘルパー
// ============================================================================

/// @brief エンティティから TagComponent を検索する
/// @note TagComponent::k_Type == ComponentType::Custom のため、
///       コンポーネント配列を走査して動的型チェックを行う
static const TagComponent* FindTagComponent(const Entity& entity)
{
    for (const auto& comp : entity.GetComponents())
    {
        if (auto* tag = dynamic_cast<const TagComponent*>(comp.get()))
        {
            return tag;
        }
    }
    return nullptr;
}

// ============================================================================
// テキスト形式のパーサーヘルパー
// ============================================================================

/// @brief "key: value" 形式の行からキーと値を分離する
static bool ParseKeyValue(const std::string& line, std::string& key, std::string& value)
{
    auto pos = line.find(':');
    if (pos == std::string::npos) return false;

    key = line.substr(0, pos);
    // ':' の直後のスペースをスキップ
    size_t valStart = pos + 1;
    if (valStart < line.size() && line[valStart] == ' ')
        ++valStart;
    value = line.substr(valStart);
    return true;
}

/// @brief "key: v1 key2: v2 ..." 形式の行から複数のキー値ペアを抽出する
static std::unordered_map<std::string, std::string> ParseMultiKeyValue(const std::string& line)
{
    std::unordered_map<std::string, std::string> result;
    std::istringstream iss(line);
    std::string token;

    while (iss >> token)
    {
        // "key:" 形式をチェック
        if (token.back() == ':')
        {
            std::string key = token.substr(0, token.size() - 1);
            std::string value;
            if (iss >> value)
            {
                result[key] = value;
            }
        }
    }
    return result;
}

// ============================================================================
// SerializeToString (テキスト形式)
// ============================================================================

std::string ScenePersistence::SerializeToString(const Scene& scene)
{
    std::ostringstream ss;

    // ヘッダー
    ss << k_TextHeader << "\n";
    ss << "scene_name: " << scene.GetName() << "\n";
    ss << "entity_count: " << scene.GetEntityCount() << "\n";

    const auto& entities = scene.GetEntities();
    for (const auto& entity : entities)
    {
        ss << k_TextSeparator << "\n";

        // 基本情報
        ss << "entity_id: " << entity->GetID() << "\n";
        ss << "name: " << entity->GetName() << "\n";
        ss << "active: " << (entity->IsActive() ? 1 : 0) << "\n";

        // 親ID（0 = ルート）
        uint32_t parentId = 0;
        if (entity->GetParent())
        {
            parentId = entity->GetParent()->GetID();
        }
        ss << "parent_id: " << parentId << "\n";

        // トランスフォーム
        const auto& transform = entity->GetTransform();
        const auto& pos = transform.GetPosition();
        const auto& rot = transform.GetRotation();
        const auto& scl = transform.GetScale();

        ss << "px: " << pos.x << " py: " << pos.y << " pz: " << pos.z << "\n";
        ss << "rx: " << rot.x << " ry: " << rot.y << " rz: " << rot.z << "\n";
        ss << "sx: " << scl.x << " sy: " << scl.y << " sz: " << scl.z << "\n";

        // TagComponent（任意）
        const auto* tagComp = FindTagComponent(*entity);
        if (tagComp)
        {
            ss << "tag: " << tagComp->tag << "\n";
        }
    }

    return ss.str();
}

// ============================================================================
// DeserializeFromString (テキスト形式)
// ============================================================================

std::unique_ptr<Scene> ScenePersistence::DeserializeFromString(const std::string& data)
{
    std::istringstream iss(data);
    std::string line;

    // --- ヘッダー検証 ---
    if (!std::getline(iss, line)) return nullptr;
    // CR を除去（Windows 改行対応）
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != k_TextHeader) return nullptr;

    // --- シーン名 ---
    std::string sceneName = "Untitled";
    if (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string key, value;
        if (ParseKeyValue(line, key, value) && key == "scene_name")
        {
            sceneName = value;
        }
    }

    // --- エンティティ数 ---
    uint32_t entityCount = 0;
    if (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string key, value;
        if (ParseKeyValue(line, key, value) && key == "entity_count")
        {
            entityCount = static_cast<uint32_t>(std::stoul(value));
        }
    }

    auto scene = std::make_unique<Scene>(sceneName);

    // 旧ID → 新Entity* のマッピング（階層復元用）
    std::unordered_map<uint32_t, Entity*> idMap;
    // 各エンティティの parent_id を記録
    std::vector<std::pair<Entity*, uint32_t>> parentRelations;

    // --- エンティティのパース ---
    // 各エンティティは "---" 区切りで始まる
    struct EntityData
    {
        uint32_t entityId = 0;
        std::string name = "Entity";
        bool active = true;
        uint32_t parentId = 0;
        float px = 0, py = 0, pz = 0;
        float rx = 0, ry = 0, rz = 0;
        float sx = 1, sy = 1, sz = 1;
        bool hasTag = false;
        std::string tag;
    };

    std::vector<EntityData> entityDataList;
    EntityData current;
    bool inEntity = false;

    auto FlushEntity = [&]()
    {
        if (inEntity)
        {
            entityDataList.push_back(current);
            current = EntityData{};
        }
    };

    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line == k_TextSeparator)
        {
            FlushEntity();
            inEntity = true;
            continue;
        }

        if (!inEntity) continue;

        std::string key, value;
        if (!ParseKeyValue(line, key, value))
        {
            // 複数キーバリュー行の可能性（px: 0 py: 1 pz: 0）
            auto kvMap = ParseMultiKeyValue(line);
            if (kvMap.count("px"))
            {
                current.px = std::stof(kvMap["px"]);
                current.py = std::stof(kvMap["py"]);
                current.pz = std::stof(kvMap["pz"]);
            }
            else if (kvMap.count("rx"))
            {
                current.rx = std::stof(kvMap["rx"]);
                current.ry = std::stof(kvMap["ry"]);
                current.rz = std::stof(kvMap["rz"]);
            }
            else if (kvMap.count("sx"))
            {
                current.sx = std::stof(kvMap["sx"]);
                current.sy = std::stof(kvMap["sy"]);
                current.sz = std::stof(kvMap["sz"]);
            }
            continue;
        }

        if (key == "entity_id") current.entityId = static_cast<uint32_t>(std::stoul(value));
        else if (key == "name") current.name = value;
        else if (key == "active") current.active = (value == "1" || value == "true");
        else if (key == "parent_id") current.parentId = static_cast<uint32_t>(std::stoul(value));
        else if (key == "tag") { current.hasTag = true; current.tag = value; }
        // 単一キーバリュー行の Transform もパース対象
        // "px: 0 py: 1 pz: 0" は ParseKeyValue で key="px" value="0 py: 1 pz: 0" になる
        // この場合は ParseMultiKeyValue で再パースする
        else if (key == "px")
        {
            // "0 py: 1 pz: 0" のような残りを再パース
            auto kvMap = ParseMultiKeyValue(key + ": " + value);
            if (kvMap.count("px"))
            {
                current.px = std::stof(kvMap["px"]);
                current.py = std::stof(kvMap.count("py") ? kvMap["py"] : "0");
                current.pz = std::stof(kvMap.count("pz") ? kvMap["pz"] : "0");
            }
        }
        else if (key == "rx")
        {
            auto kvMap = ParseMultiKeyValue(key + ": " + value);
            if (kvMap.count("rx"))
            {
                current.rx = std::stof(kvMap["rx"]);
                current.ry = std::stof(kvMap.count("ry") ? kvMap["ry"] : "0");
                current.rz = std::stof(kvMap.count("rz") ? kvMap["rz"] : "0");
            }
        }
        else if (key == "sx")
        {
            auto kvMap = ParseMultiKeyValue(key + ": " + value);
            if (kvMap.count("sx"))
            {
                current.sx = std::stof(kvMap["sx"]);
                current.sy = std::stof(kvMap.count("sy") ? kvMap["sy"] : "1");
                current.sz = std::stof(kvMap.count("sz") ? kvMap["sz"] : "1");
            }
        }
    }
    FlushEntity(); // 最後のエンティティ

    // --- パス1: エンティティ作成 ---
    for (const auto& ed : entityDataList)
    {
        Entity* entity = scene->CreateEntity(ed.name);
        entity->SetActive(ed.active);

        // Transform 復元
        entity->GetTransform().SetPosition(ed.px, ed.py, ed.pz);
        entity->GetTransform().SetRotation(ed.rx, ed.ry, ed.rz);
        entity->GetTransform().SetScale(ed.sx, ed.sy, ed.sz);

        // TagComponent 復元
        if (ed.hasTag)
        {
            auto* tag = entity->AddComponent<TagComponent>();
            tag->tag = ed.tag;
        }

        // マッピング登録（旧IDをキーに、新Entity*を値に）
        idMap[ed.entityId] = entity;

        // 親関係を記録
        if (ed.parentId != 0)
        {
            parentRelations.push_back({ entity, ed.parentId });
        }
    }

    // --- パス2: 階層復元 ---
    for (const auto& [entity, parentId] : parentRelations)
    {
        auto it = idMap.find(parentId);
        if (it != idMap.end())
        {
            entity->SetParent(it->second);
        }
    }

    return scene;
}

// ============================================================================
// SerializeToBinary (バイナリ形式)
// ============================================================================

std::vector<uint8_t> ScenePersistence::SerializeToBinary(const Scene& scene)
{
    std::vector<uint8_t> buf;
    buf.reserve(4096);

    // マジック（8 bytes）
    WriteBytes(buf, k_BinaryMagic, 8);

    // バージョン
    WriteU32(buf, k_BinaryVersion);

    // シーン名
    WriteString(buf, scene.GetName());

    // エンティティ数
    const auto& entities = scene.GetEntities();
    WriteU32(buf, static_cast<uint32_t>(entities.size()));

    // 各エンティティ
    for (const auto& entity : entities)
    {
        // entity_id
        WriteU32(buf, entity->GetID());

        // name
        WriteString(buf, entity->GetName());

        // active
        WriteU8(buf, entity->IsActive() ? 1 : 0);

        // parent_id（0 = 親なし）
        uint32_t parentId = 0;
        if (entity->GetParent())
        {
            parentId = entity->GetParent()->GetID();
        }
        WriteU32(buf, parentId);

        // トランスフォーム: 位置（3x float）
        const auto& transform = entity->GetTransform();
        const auto& pos = transform.GetPosition();
        WriteFloat(buf, pos.x);
        WriteFloat(buf, pos.y);
        WriteFloat(buf, pos.z);

        // トランスフォーム: 回転（3x float、オイラー角）
        const auto& rot = transform.GetRotation();
        WriteFloat(buf, rot.x);
        WriteFloat(buf, rot.y);
        WriteFloat(buf, rot.z);

        // トランスフォーム: スケール（3x float）
        const auto& scl = transform.GetScale();
        WriteFloat(buf, scl.x);
        WriteFloat(buf, scl.y);
        WriteFloat(buf, scl.z);

        // タグコンポーネント
        const auto* tagComp = FindTagComponent(*entity);
        if (tagComp)
        {
            WriteU8(buf, 1);
            WriteString(buf, tagComp->tag);
        }
        else
        {
            WriteU8(buf, 0);
        }
    }

    return buf;
}

// ============================================================================
// DeserializeFromBinary (バイナリ形式)
// ============================================================================

std::unique_ptr<Scene> ScenePersistence::DeserializeFromBinary(const std::vector<uint8_t>& data)
{
    BinaryReader reader{ data.data(), data.size(), 0 };

    // --- マジック検証 ---
    char magic[8] = {};
    if (!reader.ReadBytes(magic, 8)) return nullptr;
    if (std::memcmp(magic, k_BinaryMagic, 8) != 0) return nullptr;

    // --- バージョン検証 ---
    uint32_t version = 0;
    if (!reader.ReadU32(version)) return nullptr;
    if (version != k_BinaryVersion) return nullptr;

    // --- シーン名 ---
    std::string sceneName;
    if (!reader.ReadString(sceneName)) return nullptr;

    // --- エンティティ数 ---
    uint32_t entityCount = 0;
    if (!reader.ReadU32(entityCount)) return nullptr;

    auto scene = std::make_unique<Scene>(sceneName);

    // 旧ID → 新Entity* のマッピング
    std::unordered_map<uint32_t, Entity*> idMap;
    std::vector<std::pair<Entity*, uint32_t>> parentRelations;

    // --- パス1: エンティティ読み取り＆作成 ---
    for (uint32_t i = 0; i < entityCount; ++i)
    {
        // entity_id
        uint32_t entityId = 0;
        if (!reader.ReadU32(entityId)) return nullptr;

        // name
        std::string name;
        if (!reader.ReadString(name)) return nullptr;

        // active
        uint8_t active = 1;
        if (!reader.ReadU8(active)) return nullptr;

        // parent_id
        uint32_t parentId = 0;
        if (!reader.ReadU32(parentId)) return nullptr;

        // position
        float px = 0, py = 0, pz = 0;
        if (!reader.ReadFloat(px)) return nullptr;
        if (!reader.ReadFloat(py)) return nullptr;
        if (!reader.ReadFloat(pz)) return nullptr;

        // rotation
        float rx = 0, ry = 0, rz = 0;
        if (!reader.ReadFloat(rx)) return nullptr;
        if (!reader.ReadFloat(ry)) return nullptr;
        if (!reader.ReadFloat(rz)) return nullptr;

        // scale
        float sx = 1, sy = 1, sz = 1;
        if (!reader.ReadFloat(sx)) return nullptr;
        if (!reader.ReadFloat(sy)) return nullptr;
        if (!reader.ReadFloat(sz)) return nullptr;

        // has_tag
        uint8_t hasTag = 0;
        if (!reader.ReadU8(hasTag)) return nullptr;

        std::string tag;
        if (hasTag)
        {
            if (!reader.ReadString(tag)) return nullptr;
        }

        // エンティティ作成
        Entity* entity = scene->CreateEntity(name);
        entity->SetActive(active != 0);
        entity->GetTransform().SetPosition(px, py, pz);
        entity->GetTransform().SetRotation(rx, ry, rz);
        entity->GetTransform().SetScale(sx, sy, sz);

        if (hasTag)
        {
            auto* tagComp = entity->AddComponent<TagComponent>();
            tagComp->tag = tag;
        }

        idMap[entityId] = entity;
        if (parentId != 0)
        {
            parentRelations.push_back({ entity, parentId });
        }
    }

    // --- パス2: 階層復元 ---
    for (const auto& [entity, parentId] : parentRelations)
    {
        auto it = idMap.find(parentId);
        if (it != idMap.end())
        {
            entity->SetParent(it->second);
        }
    }

    return scene;
}

// ============================================================================
// SaveToFile / LoadFromFile
// ============================================================================

bool ScenePersistence::SaveToFile(const Scene& scene, const std::string& path, SceneFileFormat format)
{
    if (format == SceneFileFormat::Text)
    {
        std::string text = SerializeToString(scene);

        std::ofstream file(path, std::ios::out);
        if (!file.is_open()) return false;

        file << text;
        return file.good();
    }
    else // Binary
    {
        auto bin = SerializeToBinary(scene);

        std::ofstream file(path, std::ios::out | std::ios::binary);
        if (!file.is_open()) return false;

        file.write(reinterpret_cast<const char*>(bin.data()), static_cast<std::streamsize>(bin.size()));
        return file.good();
    }
}

std::unique_ptr<Scene> ScenePersistence::LoadFromFile(const std::string& path, SceneFileFormat format)
{
    if (format == SceneFileFormat::Text)
    {
        std::ifstream file(path, std::ios::in);
        if (!file.is_open()) return nullptr;

        std::ostringstream ss;
        ss << file.rdbuf();
        return DeserializeFromString(ss.str());
    }
    else // Binary
    {
        std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
        if (!file.is_open()) return nullptr;

        auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buf(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(buf.data()), size);
        if (!file.good()) return nullptr;

        return DeserializeFromBinary(buf);
    }
}

} // namespace gx
