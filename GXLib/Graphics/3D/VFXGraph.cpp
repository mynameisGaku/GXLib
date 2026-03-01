/// @file VFXGraph.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Graphics/3D/VFXGraph.h"
#include "Graphics/3D/GPUParticleSystem.h"
#include "Core/Logger.h"
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace gx
{

// ============================================================================
// VFXRange
// ============================================================================

float VFXRange::GetRandom() const
{
    thread_local std::mt19937 s_rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(min, max);
    return dist(s_rng);
}

// ============================================================================
// VFXCurve
// ============================================================================

float VFXCurve::Evaluate(float t) const
{
    if (keys.empty()) return 0.0f;
    if (keys.size() == 1) return keys[0].value;

    // tをクランプ
    if (t <= keys.front().time) return keys.front().value;
    if (t >= keys.back().time) return keys.back().value;

    // 前後のキーを探索
    for (size_t i = 0; i + 1 < keys.size(); ++i)
    {
        if (t >= keys[i].time && t <= keys[i + 1].time)
        {
            float segLen = keys[i + 1].time - keys[i].time;
            if (segLen <= 0.0f) return keys[i].value;
            float localT = (t - keys[i].time) / segLen;
            return keys[i].value + localT * (keys[i + 1].value - keys[i].value);
        }
    }
    return keys.back().value;
}

VFXCurve VFXCurve::Constant(float value)
{
    VFXCurve c;
    c.keys.push_back({ 0.0f, value });
    c.keys.push_back({ 1.0f, value });
    return c;
}

VFXCurve VFXCurve::Linear(float start, float end)
{
    VFXCurve c;
    c.keys.push_back({ 0.0f, start });
    c.keys.push_back({ 1.0f, end });
    return c;
}

// ============================================================================
// VFXEffect
// ============================================================================

uint32_t VFXEffect::AddEmitter(const VFXEmitter& emitter)
{
    uint32_t index = static_cast<uint32_t>(m_emitters.size());
    m_emitters.push_back(emitter);
    return index;
}

VFXEmitter& VFXEffect::GetEmitter(uint32_t index)
{
    return m_emitters[index];
}

const VFXEmitter& VFXEffect::GetEmitter(uint32_t index) const
{
    return m_emitters[index];
}

void VFXEffect::AddModule(uint32_t emitterIndex, const VFXModule& module)
{
    if (emitterIndex < m_emitters.size())
    {
        m_emitters[emitterIndex].modules.push_back(module);
    }
}

// ============================================================================
// 手書きJSONパーサヘルパー（Tilemap TMJと同じパターン）
// ============================================================================

namespace
{

void SkipWS(const std::string& s, size_t& p)
{
    while (p < s.size() && (s[p] == ' ' || s[p] == '\t' || s[p] == '\n' || s[p] == '\r'))
        ++p;
}

std::string ParseStr(const std::string& s, size_t& p)
{
    SkipWS(s, p);
    if (p >= s.size() || s[p] != '"') return "";
    ++p;
    std::string r;
    while (p < s.size() && s[p] != '"')
    {
        if (s[p] == '\\' && p + 1 < s.size())
        {
            ++p;
            switch (s[p])
            {
            case '"':  r += '"';  break;
            case '\\': r += '\\'; break;
            case 'n':  r += '\n'; break;
            case 't':  r += '\t'; break;
            case '/':  r += '/';  break;
            default:   r += s[p]; break;
            }
        }
        else
        {
            r += s[p];
        }
        ++p;
    }
    if (p < s.size()) ++p;
    return r;
}

double ParseNum(const std::string& s, size_t& p)
{
    SkipWS(s, p);
    size_t start = p;
    if (p < s.size() && (s[p] == '-' || s[p] == '+')) ++p;
    while (p < s.size() && ((s[p] >= '0' && s[p] <= '9') || s[p] == '.' ||
           s[p] == 'e' || s[p] == 'E' || s[p] == '+' || s[p] == '-'))
    {
        if ((s[p] == '+' || s[p] == '-') && p > start &&
            s[p - 1] != 'e' && s[p - 1] != 'E')
            break;
        ++p;
    }
    std::string ns = s.substr(start, p - start);
    if (ns.empty()) return 0.0;
    return std::stod(ns);
}

bool ParseBool(const std::string& s, size_t& p)
{
    SkipWS(s, p);
    if (p + 4 <= s.size() && s.substr(p, 4) == "true")  { p += 4; return true; }
    if (p + 5 <= s.size() && s.substr(p, 5) == "false") { p += 5; return false; }
    return false;
}

void SkipVal(const std::string& s, size_t& p)
{
    SkipWS(s, p);
    if (p >= s.size()) return;
    if (s[p] == '"') { ParseStr(s, p); }
    else if (s[p] == '{')
    {
        int d = 1; ++p;
        while (p < s.size() && d > 0)
        {
            if (s[p] == '{') ++d;
            else if (s[p] == '}') --d;
            else if (s[p] == '"') { ParseStr(s, p); continue; }
            ++p;
        }
    }
    else if (s[p] == '[')
    {
        int d = 1; ++p;
        while (p < s.size() && d > 0)
        {
            if (s[p] == '[') ++d;
            else if (s[p] == ']') --d;
            else if (s[p] == '"') { ParseStr(s, p); continue; }
            ++p;
        }
    }
    else if (s[p] == 'n' && p + 4 <= s.size() && s.substr(p, 4) == "null") { p += 4; }
    else if (s[p] == 't' || s[p] == 'f') { ParseBool(s, p); }
    else { ParseNum(s, p); }
}

size_t FindBracket(const std::string& s, size_t p)
{
    char open = s[p];
    char close = (open == '[') ? ']' : '}';
    int d = 1; ++p;
    while (p < s.size() && d > 0)
    {
        if (s[p] == open) ++d;
        else if (s[p] == close) --d;
        else if (s[p] == '"')
        {
            ++p;
            while (p < s.size() && s[p] != '"')
            {
                if (s[p] == '\\') ++p;
                ++p;
            }
        }
        ++p;
    }
    return p;
}

VFXModuleType ParseModuleType(const std::string& typeStr)
{
    if (typeStr == "InitialVelocity")   return VFXModuleType::InitialVelocity;
    if (typeStr == "InitialSize")       return VFXModuleType::InitialSize;
    if (typeStr == "InitialColor")      return VFXModuleType::InitialColor;
    if (typeStr == "InitialRotation")   return VFXModuleType::InitialRotation;
    if (typeStr == "InitialLifetime")   return VFXModuleType::InitialLifetime;
    if (typeStr == "VelocityOverLife")  return VFXModuleType::VelocityOverLife;
    if (typeStr == "SizeOverLife")      return VFXModuleType::SizeOverLife;
    if (typeStr == "ColorOverLife")     return VFXModuleType::ColorOverLife;
    if (typeStr == "RotationOverLife")  return VFXModuleType::RotationOverLife;
    if (typeStr == "Gravity")           return VFXModuleType::Gravity;
    if (typeStr == "Noise")             return VFXModuleType::Noise;
    if (typeStr == "Drag")              return VFXModuleType::Drag;
    if (typeStr == "Orbit")             return VFXModuleType::Orbit;
    if (typeStr == "Attraction")        return VFXModuleType::Attraction;
    if (typeStr == "TextureSheet")      return VFXModuleType::TextureSheet;
    if (typeStr == "SubEmitter")        return VFXModuleType::SubEmitter;
    return VFXModuleType::InitialVelocity;
}

const char* ModuleTypeToString(VFXModuleType type)
{
    switch (type)
    {
    case VFXModuleType::InitialVelocity:   return "InitialVelocity";
    case VFXModuleType::InitialSize:       return "InitialSize";
    case VFXModuleType::InitialColor:      return "InitialColor";
    case VFXModuleType::InitialRotation:   return "InitialRotation";
    case VFXModuleType::InitialLifetime:   return "InitialLifetime";
    case VFXModuleType::VelocityOverLife:  return "VelocityOverLife";
    case VFXModuleType::SizeOverLife:      return "SizeOverLife";
    case VFXModuleType::ColorOverLife:     return "ColorOverLife";
    case VFXModuleType::RotationOverLife:  return "RotationOverLife";
    case VFXModuleType::Gravity:           return "Gravity";
    case VFXModuleType::Noise:             return "Noise";
    case VFXModuleType::Drag:              return "Drag";
    case VFXModuleType::Orbit:             return "Orbit";
    case VFXModuleType::Attraction:        return "Attraction";
    case VFXModuleType::TextureSheet:      return "TextureSheet";
    case VFXModuleType::SubEmitter:        return "SubEmitter";
    }
    return "InitialVelocity";
}

const char* EmitterShapeToString(VFXEmitterShape shape)
{
    switch (shape)
    {
    case VFXEmitterShape::Point:      return "Point";
    case VFXEmitterShape::Sphere:     return "Sphere";
    case VFXEmitterShape::Hemisphere: return "Hemisphere";
    case VFXEmitterShape::Cone:       return "Cone";
    case VFXEmitterShape::Box:        return "Box";
    case VFXEmitterShape::Circle:     return "Circle";
    case VFXEmitterShape::Edge:       return "Edge";
    }
    return "Point";
}

VFXEmitterShape ParseEmitterShape(const std::string& str)
{
    if (str == "Point")      return VFXEmitterShape::Point;
    if (str == "Sphere")     return VFXEmitterShape::Sphere;
    if (str == "Hemisphere") return VFXEmitterShape::Hemisphere;
    if (str == "Cone")       return VFXEmitterShape::Cone;
    if (str == "Box")        return VFXEmitterShape::Box;
    if (str == "Circle")     return VFXEmitterShape::Circle;
    if (str == "Edge")       return VFXEmitterShape::Edge;
    return VFXEmitterShape::Point;
}

VFXRange ParseRange(const std::string& s, size_t& p)
{
    VFXRange r;
    SkipWS(s, p);
    if (p >= s.size() || s[p] != '{') return r;
    ++p;
    while (p < s.size())
    {
        SkipWS(s, p);
        if (p >= s.size() || s[p] == '}') { ++p; break; }
        if (s[p] == ',') { ++p; continue; }
        std::string k = ParseStr(s, p);
        SkipWS(s, p);
        if (p < s.size() && s[p] == ':') ++p;
        if (k == "min")      r.min = static_cast<float>(ParseNum(s, p));
        else if (k == "max") r.max = static_cast<float>(ParseNum(s, p));
        else SkipVal(s, p);
    }
    return r;
}

VFXColorRange ParseColorRange(const std::string& s, size_t& p)
{
    VFXColorRange cr;
    SkipWS(s, p);
    if (p >= s.size() || s[p] != '{') return cr;
    ++p;
    while (p < s.size())
    {
        SkipWS(s, p);
        if (p >= s.size() || s[p] == '}') { ++p; break; }
        if (s[p] == ',') { ++p; continue; }
        std::string k = ParseStr(s, p);
        SkipWS(s, p);
        if (p < s.size() && s[p] == ':') ++p;
        if (k == "r0")      cr.r0 = static_cast<float>(ParseNum(s, p));
        else if (k == "g0") cr.g0 = static_cast<float>(ParseNum(s, p));
        else if (k == "b0") cr.b0 = static_cast<float>(ParseNum(s, p));
        else if (k == "a0") cr.a0 = static_cast<float>(ParseNum(s, p));
        else if (k == "r1") cr.r1 = static_cast<float>(ParseNum(s, p));
        else if (k == "g1") cr.g1 = static_cast<float>(ParseNum(s, p));
        else if (k == "b1") cr.b1 = static_cast<float>(ParseNum(s, p));
        else if (k == "a1") cr.a1 = static_cast<float>(ParseNum(s, p));
        else SkipVal(s, p);
    }
    return cr;
}

VFXCurve ParseCurve(const std::string& s, size_t& p)
{
    VFXCurve curve;
    SkipWS(s, p);
    if (p >= s.size() || s[p] != '[') return curve;
    ++p;
    while (p < s.size())
    {
        SkipWS(s, p);
        if (p >= s.size() || s[p] == ']') { ++p; break; }
        if (s[p] == ',') { ++p; continue; }
        if (s[p] == '{')
        {
            ++p;
            VFXCurveKey key;
            while (p < s.size())
            {
                SkipWS(s, p);
                if (p >= s.size() || s[p] == '}') { ++p; break; }
                if (s[p] == ',') { ++p; continue; }
                std::string k = ParseStr(s, p);
                SkipWS(s, p);
                if (p < s.size() && s[p] == ':') ++p;
                if (k == "time")       key.time = static_cast<float>(ParseNum(s, p));
                else if (k == "value") key.value = static_cast<float>(ParseNum(s, p));
                else SkipVal(s, p);
            }
            curve.keys.push_back(key);
        }
        else
        {
            SkipVal(s, p);
        }
    }
    return curve;
}

VFXModule ParseModule(const std::string& s, size_t& p)
{
    VFXModule mod;
    mod.type = VFXModuleType::InitialVelocity;
    SkipWS(s, p);
    if (p >= s.size() || s[p] != '{') return mod;
    ++p;
    while (p < s.size())
    {
        SkipWS(s, p);
        if (p >= s.size() || s[p] == '}') { ++p; break; }
        if (s[p] == ',') { ++p; continue; }
        std::string k = ParseStr(s, p);
        SkipWS(s, p);
        if (p < s.size() && s[p] == ':') ++p;
        SkipWS(s, p);

        if (k == "type")                    mod.type = ParseModuleType(ParseStr(s, p));
        else if (k == "enabled")            mod.enabled = ParseBool(s, p);
        else if (k == "velocityX")          mod.velocityX = ParseRange(s, p);
        else if (k == "velocityY")          mod.velocityY = ParseRange(s, p);
        else if (k == "velocityZ")          mod.velocityZ = ParseRange(s, p);
        else if (k == "velocityRadial")     mod.velocityRadial = ParseBool(s, p);
        else if (k == "radialSpeed")        mod.radialSpeed = ParseRange(s, p);
        else if (k == "sizeMin")            mod.sizeMin = ParseRange(s, p);
        else if (k == "sizeMax")            mod.sizeMax = ParseRange(s, p);
        else if (k == "colorRange")         mod.colorRange = ParseColorRange(s, p);
        else if (k == "rotationRange")      mod.rotationRange = ParseRange(s, p);
        else if (k == "lifetimeRange")      mod.lifetimeRange = ParseRange(s, p);
        else if (k == "sizeCurve")          mod.sizeCurve = ParseCurve(s, p);
        else if (k == "alphaCurve")         mod.alphaCurve = ParseCurve(s, p);
        else if (k == "velocityCurve")      mod.velocityCurve = ParseCurve(s, p);
        else if (k == "rotationSpeedCurve") mod.rotationSpeedCurve = ParseCurve(s, p);
        else if (k == "colorStart")         mod.colorStart = ParseColorRange(s, p);
        else if (k == "colorEnd")           mod.colorEnd = ParseColorRange(s, p);
        else if (k == "gravityStrength")    mod.gravityStrength = static_cast<float>(ParseNum(s, p));
        else if (k == "noiseFrequency")     mod.noiseFrequency = static_cast<float>(ParseNum(s, p));
        else if (k == "noiseAmplitude")     mod.noiseAmplitude = static_cast<float>(ParseNum(s, p));
        else if (k == "dragCoefficient")    mod.dragCoefficient = static_cast<float>(ParseNum(s, p));
        else if (k == "orbitSpeed")         mod.orbitSpeed = static_cast<float>(ParseNum(s, p));
        else if (k == "orbitRadius")        mod.orbitRadius = static_cast<float>(ParseNum(s, p));
        else if (k == "attractStrength")    mod.attractStrength = static_cast<float>(ParseNum(s, p));
        else if (k == "sheetRows")          mod.sheetRows = static_cast<int>(ParseNum(s, p));
        else if (k == "sheetCols")          mod.sheetCols = static_cast<int>(ParseNum(s, p));
        else if (k == "sheetFPS")           mod.sheetFPS = static_cast<float>(ParseNum(s, p));
        else if (k == "subEmitterIndex")    mod.subEmitterIndex = static_cast<int>(ParseNum(s, p));
        else if (k == "attractPoint")
        {
            // x,y,zフィールドを持つオブジェクトとしてパース
            SkipWS(s, p);
            if (p < s.size() && s[p] == '{')
            {
                ++p;
                while (p < s.size())
                {
                    SkipWS(s, p);
                    if (p >= s.size() || s[p] == '}') { ++p; break; }
                    if (s[p] == ',') { ++p; continue; }
                    std::string ak = ParseStr(s, p);
                    SkipWS(s, p);
                    if (p < s.size() && s[p] == ':') ++p;
                    if (ak == "x")      mod.attractPoint.x = static_cast<float>(ParseNum(s, p));
                    else if (ak == "y") mod.attractPoint.y = static_cast<float>(ParseNum(s, p));
                    else if (ak == "z") mod.attractPoint.z = static_cast<float>(ParseNum(s, p));
                    else SkipVal(s, p);
                }
            }
            else
            {
                SkipVal(s, p);
            }
        }
        else
        {
            SkipVal(s, p);
        }
    }
    return mod;
}

VFXEmitter ParseEmitter(const std::string& s, size_t& p)
{
    VFXEmitter em;
    SkipWS(s, p);
    if (p >= s.size() || s[p] != '{') return em;
    ++p;
    while (p < s.size())
    {
        SkipWS(s, p);
        if (p >= s.size() || s[p] == '}') { ++p; break; }
        if (s[p] == ',') { ++p; continue; }
        std::string k = ParseStr(s, p);
        SkipWS(s, p);
        if (p < s.size() && s[p] == ':') ++p;
        SkipWS(s, p);

        if (k == "name")              em.name = ParseStr(s, p);
        else if (k == "emitRate")     em.emitRate = static_cast<float>(ParseNum(s, p));
        else if (k == "maxParticles") em.maxParticles = static_cast<int>(ParseNum(s, p));
        else if (k == "duration")     em.duration = static_cast<float>(ParseNum(s, p));
        else if (k == "looping")      em.looping = ParseBool(s, p);
        else if (k == "startDelay")   em.startDelay = static_cast<float>(ParseNum(s, p));
        else if (k == "shape")        em.shape = ParseEmitterShape(ParseStr(s, p));
        else if (k == "shapeRadius")  em.shapeRadius = static_cast<float>(ParseNum(s, p));
        else if (k == "shapeConeAngle") em.shapeConeAngle = static_cast<float>(ParseNum(s, p));
        else if (k == "emitFromEdge") em.emitFromEdge = ParseBool(s, p);
        else if (k == "worldSpace")   em.worldSpace = ParseBool(s, p);
        else if (k == "textureHandle") em.textureHandle = static_cast<int>(ParseNum(s, p));
        else if (k == "blendMode")    em.blendMode = static_cast<int>(ParseNum(s, p));
        else if (k == "shapeSize")
        {
            SkipWS(s, p);
            if (p < s.size() && s[p] == '{')
            {
                ++p;
                while (p < s.size())
                {
                    SkipWS(s, p);
                    if (p >= s.size() || s[p] == '}') { ++p; break; }
                    if (s[p] == ',') { ++p; continue; }
                    std::string sk = ParseStr(s, p);
                    SkipWS(s, p);
                    if (p < s.size() && s[p] == ':') ++p;
                    if (sk == "x")      em.shapeSize.x = static_cast<float>(ParseNum(s, p));
                    else if (sk == "y") em.shapeSize.y = static_cast<float>(ParseNum(s, p));
                    else if (sk == "z") em.shapeSize.z = static_cast<float>(ParseNum(s, p));
                    else SkipVal(s, p);
                }
            }
            else
            {
                SkipVal(s, p);
            }
        }
        else if (k == "modules")
        {
            SkipWS(s, p);
            if (p < s.size() && s[p] == '[')
            {
                ++p;
                while (p < s.size())
                {
                    SkipWS(s, p);
                    if (p >= s.size() || s[p] == ']') { ++p; break; }
                    if (s[p] == ',') { ++p; continue; }
                    if (s[p] == '{')
                    {
                        em.modules.push_back(ParseModule(s, p));
                    }
                    else
                    {
                        SkipVal(s, p);
                    }
                }
            }
            else
            {
                SkipVal(s, p);
            }
        }
        else
        {
            SkipVal(s, p);
        }
    }
    return em;
}

/// @brief JSON形式のレンジをストリームに書き出す
void WriteRange(std::ostream& os, const VFXRange& r, const std::string& indent)
{
    os << indent << "{ \"min\": " << r.min << ", \"max\": " << r.max << " }";
}

void WriteColorRange(std::ostream& os, const VFXColorRange& cr, const std::string& indent)
{
    os << indent << "{\n";
    os << indent << "  \"r0\": " << cr.r0 << ", \"g0\": " << cr.g0
       << ", \"b0\": " << cr.b0 << ", \"a0\": " << cr.a0 << ",\n";
    os << indent << "  \"r1\": " << cr.r1 << ", \"g1\": " << cr.g1
       << ", \"b1\": " << cr.b1 << ", \"a1\": " << cr.a1 << "\n";
    os << indent << "}";
}

void WriteCurve(std::ostream& os, const VFXCurve& c, const std::string& indent)
{
    os << "[";
    for (size_t i = 0; i < c.keys.size(); ++i)
    {
        if (i > 0) os << ", ";
        os << "{ \"time\": " << c.keys[i].time << ", \"value\": " << c.keys[i].value << " }";
    }
    os << "]";
}

void WriteFloat3(std::ostream& os, const XMFLOAT3& v)
{
    os << "{ \"x\": " << v.x << ", \"y\": " << v.y << ", \"z\": " << v.z << " }";
}

} // anonymous namespace

// ============================================================================
// VFXEffect — JSON入出力
// ============================================================================

bool VFXEffect::LoadFromJSON(const std::string& jsonPath)
{
    std::ifstream file(jsonPath);
    if (!file.is_open())
    {
        GX_LOG_ERROR("VFXEffect: Failed to open JSON file: %s", jsonPath.c_str());
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string json = ss.str();

    if (json.empty())
    {
        GX_LOG_ERROR("VFXEffect: JSON file is empty: %s", jsonPath.c_str());
        return false;
    }

    size_t p = 0;
    SkipWS(json, p);
    if (p >= json.size() || json[p] != '{')
    {
        GX_LOG_ERROR("VFXEffect: JSON root is not an object");
        return false;
    }
    ++p;

    m_emitters.clear();
    m_name.clear();

    while (p < json.size())
    {
        SkipWS(json, p);
        if (p >= json.size() || json[p] == '}') break;
        if (json[p] == ',') { ++p; continue; }

        std::string key = ParseStr(json, p);
        SkipWS(json, p);
        if (p < json.size() && json[p] == ':') ++p;
        SkipWS(json, p);

        if (key == "name")
        {
            m_name = ParseStr(json, p);
        }
        else if (key == "emitters")
        {
            if (p < json.size() && json[p] == '[')
            {
                ++p;
                while (p < json.size())
                {
                    SkipWS(json, p);
                    if (p >= json.size() || json[p] == ']') { ++p; break; }
                    if (json[p] == ',') { ++p; continue; }
                    if (json[p] == '{')
                    {
                        m_emitters.push_back(ParseEmitter(json, p));
                    }
                    else
                    {
                        SkipVal(json, p);
                    }
                }
            }
            else
            {
                SkipVal(json, p);
            }
        }
        else
        {
            SkipVal(json, p);
        }
    }

    return true;
}

bool VFXEffect::SaveToJSON(const std::string& jsonPath) const
{
    std::ofstream ofs(jsonPath);
    if (!ofs.is_open())
    {
        GX_LOG_ERROR("VFXEffect: Failed to create JSON file: %s", jsonPath.c_str());
        return false;
    }

    ofs << "{\n";
    ofs << "  \"name\": \"" << m_name << "\",\n";
    ofs << "  \"emitters\": [\n";

    for (size_t ei = 0; ei < m_emitters.size(); ++ei)
    {
        const auto& em = m_emitters[ei];
        ofs << "    {\n";
        ofs << "      \"name\": \"" << em.name << "\",\n";
        ofs << "      \"emitRate\": " << em.emitRate << ",\n";
        ofs << "      \"maxParticles\": " << em.maxParticles << ",\n";
        ofs << "      \"duration\": " << em.duration << ",\n";
        ofs << "      \"looping\": " << (em.looping ? "true" : "false") << ",\n";
        ofs << "      \"startDelay\": " << em.startDelay << ",\n";
        ofs << "      \"shape\": \"" << EmitterShapeToString(em.shape) << "\",\n";
        ofs << "      \"shapeRadius\": " << em.shapeRadius << ",\n";
        ofs << "      \"shapeSize\": ";
        WriteFloat3(ofs, em.shapeSize);
        ofs << ",\n";
        ofs << "      \"shapeConeAngle\": " << em.shapeConeAngle << ",\n";
        ofs << "      \"emitFromEdge\": " << (em.emitFromEdge ? "true" : "false") << ",\n";
        ofs << "      \"worldSpace\": " << (em.worldSpace ? "true" : "false") << ",\n";
        ofs << "      \"textureHandle\": " << em.textureHandle << ",\n";
        ofs << "      \"blendMode\": " << em.blendMode << ",\n";

        ofs << "      \"modules\": [\n";
        for (size_t mi = 0; mi < em.modules.size(); ++mi)
        {
            const auto& mod = em.modules[mi];
            ofs << "        {\n";
            ofs << "          \"type\": \"" << ModuleTypeToString(mod.type) << "\",\n";
            ofs << "          \"enabled\": " << (mod.enabled ? "true" : "false") << ",\n";

            // 全モジュールパラメータを書き出し
            ofs << "          \"velocityX\": ";  WriteRange(ofs, mod.velocityX, "");  ofs << ",\n";
            ofs << "          \"velocityY\": ";  WriteRange(ofs, mod.velocityY, "");  ofs << ",\n";
            ofs << "          \"velocityZ\": ";  WriteRange(ofs, mod.velocityZ, "");  ofs << ",\n";
            ofs << "          \"velocityRadial\": " << (mod.velocityRadial ? "true" : "false") << ",\n";
            ofs << "          \"radialSpeed\": "; WriteRange(ofs, mod.radialSpeed, ""); ofs << ",\n";
            ofs << "          \"sizeMin\": ";     WriteRange(ofs, mod.sizeMin, "");     ofs << ",\n";
            ofs << "          \"sizeMax\": ";     WriteRange(ofs, mod.sizeMax, "");     ofs << ",\n";
            ofs << "          \"colorRange\": ";  WriteColorRange(ofs, mod.colorRange, "          "); ofs << ",\n";
            ofs << "          \"rotationRange\": "; WriteRange(ofs, mod.rotationRange, ""); ofs << ",\n";
            ofs << "          \"lifetimeRange\": "; WriteRange(ofs, mod.lifetimeRange, ""); ofs << ",\n";
            ofs << "          \"sizeCurve\": ";     WriteCurve(ofs, mod.sizeCurve, ""); ofs << ",\n";
            ofs << "          \"alphaCurve\": ";    WriteCurve(ofs, mod.alphaCurve, ""); ofs << ",\n";
            ofs << "          \"velocityCurve\": "; WriteCurve(ofs, mod.velocityCurve, ""); ofs << ",\n";
            ofs << "          \"rotationSpeedCurve\": "; WriteCurve(ofs, mod.rotationSpeedCurve, ""); ofs << ",\n";
            ofs << "          \"colorStart\": ";  WriteColorRange(ofs, mod.colorStart, "          "); ofs << ",\n";
            ofs << "          \"colorEnd\": ";    WriteColorRange(ofs, mod.colorEnd, "          "); ofs << ",\n";
            ofs << "          \"gravityStrength\": " << mod.gravityStrength << ",\n";
            ofs << "          \"noiseFrequency\": "  << mod.noiseFrequency << ",\n";
            ofs << "          \"noiseAmplitude\": "  << mod.noiseAmplitude << ",\n";
            ofs << "          \"dragCoefficient\": " << mod.dragCoefficient << ",\n";
            ofs << "          \"orbitSpeed\": "      << mod.orbitSpeed << ",\n";
            ofs << "          \"orbitRadius\": "     << mod.orbitRadius << ",\n";
            ofs << "          \"attractPoint\": ";   WriteFloat3(ofs, mod.attractPoint); ofs << ",\n";
            ofs << "          \"attractStrength\": " << mod.attractStrength << ",\n";
            ofs << "          \"sheetRows\": "       << mod.sheetRows << ",\n";
            ofs << "          \"sheetCols\": "       << mod.sheetCols << ",\n";
            ofs << "          \"sheetFPS\": "        << mod.sheetFPS << ",\n";
            ofs << "          \"subEmitterIndex\": " << mod.subEmitterIndex << "\n";

            ofs << "        }";
            if (mi + 1 < em.modules.size()) ofs << ",";
            ofs << "\n";
        }
        ofs << "      ]\n";

        ofs << "    }";
        if (ei + 1 < m_emitters.size()) ofs << ",";
        ofs << "\n";
    }

    ofs << "  ]\n";
    ofs << "}\n";

    return true;
}

// ============================================================================
// VFXEffect — バイナリ入出力
// ============================================================================

static constexpr uint32_t k_VFXBinaryMagic   = 0x56465842;  // "VFXB"
static constexpr uint32_t k_VFXBinaryVersion  = 1;

namespace
{

template<typename T>
void WritePOD(std::ofstream& os, const T& val)
{
    os.write(reinterpret_cast<const char*>(&val), sizeof(T));
}

template<typename T>
bool ReadPOD(std::ifstream& is, T& val)
{
    is.read(reinterpret_cast<char*>(&val), sizeof(T));
    return is.good();
}

void WriteString(std::ofstream& os, const std::string& str)
{
    uint32_t len = static_cast<uint32_t>(str.size());
    WritePOD(os, len);
    if (len > 0) os.write(str.data(), len);
}

bool ReadString(std::ifstream& is, std::string& str)
{
    uint32_t len = 0;
    if (!ReadPOD(is, len)) return false;
    str.resize(len);
    if (len > 0) is.read(str.data(), len);
    return is.good() || is.eof();
}

void WriteCurveBin(std::ofstream& os, const VFXCurve& c)
{
    uint32_t count = static_cast<uint32_t>(c.keys.size());
    WritePOD(os, count);
    for (const auto& k : c.keys)
    {
        WritePOD(os, k.time);
        WritePOD(os, k.value);
    }
}

bool ReadCurveBin(std::ifstream& is, VFXCurve& c)
{
    uint32_t count = 0;
    if (!ReadPOD(is, count)) return false;
    c.keys.resize(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        if (!ReadPOD(is, c.keys[i].time)) return false;
        if (!ReadPOD(is, c.keys[i].value)) return false;
    }
    return true;
}

void WriteModuleBin(std::ofstream& os, const VFXModule& m)
{
    WritePOD(os, static_cast<uint32_t>(m.type));
    WritePOD(os, m.enabled);

    WritePOD(os, m.velocityX);
    WritePOD(os, m.velocityY);
    WritePOD(os, m.velocityZ);
    WritePOD(os, m.velocityRadial);
    WritePOD(os, m.radialSpeed);
    WritePOD(os, m.sizeMin);
    WritePOD(os, m.sizeMax);
    WritePOD(os, m.colorRange);
    WritePOD(os, m.rotationRange);
    WritePOD(os, m.lifetimeRange);

    WriteCurveBin(os, m.sizeCurve);
    WriteCurveBin(os, m.alphaCurve);
    WriteCurveBin(os, m.velocityCurve);
    WriteCurveBin(os, m.rotationSpeedCurve);

    WritePOD(os, m.colorStart);
    WritePOD(os, m.colorEnd);
    WritePOD(os, m.gravityStrength);
    WritePOD(os, m.noiseFrequency);
    WritePOD(os, m.noiseAmplitude);
    WritePOD(os, m.dragCoefficient);
    WritePOD(os, m.orbitSpeed);
    WritePOD(os, m.orbitRadius);
    WritePOD(os, m.attractPoint);
    WritePOD(os, m.attractStrength);
    WritePOD(os, m.sheetRows);
    WritePOD(os, m.sheetCols);
    WritePOD(os, m.sheetFPS);
    WritePOD(os, m.subEmitterIndex);
}

bool ReadModuleBin(std::ifstream& is, VFXModule& m)
{
    uint32_t typeVal = 0;
    if (!ReadPOD(is, typeVal)) return false;
    m.type = static_cast<VFXModuleType>(typeVal);
    if (!ReadPOD(is, m.enabled)) return false;

    if (!ReadPOD(is, m.velocityX)) return false;
    if (!ReadPOD(is, m.velocityY)) return false;
    if (!ReadPOD(is, m.velocityZ)) return false;
    if (!ReadPOD(is, m.velocityRadial)) return false;
    if (!ReadPOD(is, m.radialSpeed)) return false;
    if (!ReadPOD(is, m.sizeMin)) return false;
    if (!ReadPOD(is, m.sizeMax)) return false;
    if (!ReadPOD(is, m.colorRange)) return false;
    if (!ReadPOD(is, m.rotationRange)) return false;
    if (!ReadPOD(is, m.lifetimeRange)) return false;

    if (!ReadCurveBin(is, m.sizeCurve)) return false;
    if (!ReadCurveBin(is, m.alphaCurve)) return false;
    if (!ReadCurveBin(is, m.velocityCurve)) return false;
    if (!ReadCurveBin(is, m.rotationSpeedCurve)) return false;

    if (!ReadPOD(is, m.colorStart)) return false;
    if (!ReadPOD(is, m.colorEnd)) return false;
    if (!ReadPOD(is, m.gravityStrength)) return false;
    if (!ReadPOD(is, m.noiseFrequency)) return false;
    if (!ReadPOD(is, m.noiseAmplitude)) return false;
    if (!ReadPOD(is, m.dragCoefficient)) return false;
    if (!ReadPOD(is, m.orbitSpeed)) return false;
    if (!ReadPOD(is, m.orbitRadius)) return false;
    if (!ReadPOD(is, m.attractPoint)) return false;
    if (!ReadPOD(is, m.attractStrength)) return false;
    if (!ReadPOD(is, m.sheetRows)) return false;
    if (!ReadPOD(is, m.sheetCols)) return false;
    if (!ReadPOD(is, m.sheetFPS)) return false;
    if (!ReadPOD(is, m.subEmitterIndex)) return false;

    return true;
}

void WriteEmitterBin(std::ofstream& os, const VFXEmitter& em)
{
    WriteString(os, em.name);
    WritePOD(os, em.emitRate);
    WritePOD(os, em.maxParticles);
    WritePOD(os, em.duration);
    WritePOD(os, em.looping);
    WritePOD(os, em.startDelay);
    WritePOD(os, static_cast<uint32_t>(em.shape));
    WritePOD(os, em.shapeRadius);
    WritePOD(os, em.shapeSize);
    WritePOD(os, em.shapeConeAngle);
    WritePOD(os, em.emitFromEdge);
    WritePOD(os, em.worldSpace);
    WritePOD(os, em.textureHandle);
    WritePOD(os, em.blendMode);

    uint32_t modCount = static_cast<uint32_t>(em.modules.size());
    WritePOD(os, modCount);
    for (const auto& mod : em.modules)
    {
        WriteModuleBin(os, mod);
    }
}

bool ReadEmitterBin(std::ifstream& is, VFXEmitter& em)
{
    if (!ReadString(is, em.name)) return false;
    if (!ReadPOD(is, em.emitRate)) return false;
    if (!ReadPOD(is, em.maxParticles)) return false;
    if (!ReadPOD(is, em.duration)) return false;
    if (!ReadPOD(is, em.looping)) return false;
    if (!ReadPOD(is, em.startDelay)) return false;

    uint32_t shapeVal = 0;
    if (!ReadPOD(is, shapeVal)) return false;
    em.shape = static_cast<VFXEmitterShape>(shapeVal);

    if (!ReadPOD(is, em.shapeRadius)) return false;
    if (!ReadPOD(is, em.shapeSize)) return false;
    if (!ReadPOD(is, em.shapeConeAngle)) return false;
    if (!ReadPOD(is, em.emitFromEdge)) return false;
    if (!ReadPOD(is, em.worldSpace)) return false;
    if (!ReadPOD(is, em.textureHandle)) return false;
    if (!ReadPOD(is, em.blendMode)) return false;

    uint32_t modCount = 0;
    if (!ReadPOD(is, modCount)) return false;
    em.modules.resize(modCount);
    for (uint32_t i = 0; i < modCount; ++i)
    {
        if (!ReadModuleBin(is, em.modules[i])) return false;
    }
    return true;
}

} // anonymous namespace

bool VFXEffect::SaveToBinary(const std::string& path) const
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open())
    {
        GX_LOG_ERROR("VFXEffect: Failed to create binary file: %s", path.c_str());
        return false;
    }

    WritePOD(ofs, k_VFXBinaryMagic);
    WritePOD(ofs, k_VFXBinaryVersion);
    WriteString(ofs, m_name);

    uint32_t emitterCount = static_cast<uint32_t>(m_emitters.size());
    WritePOD(ofs, emitterCount);
    for (const auto& em : m_emitters)
    {
        WriteEmitterBin(ofs, em);
    }

    return ofs.good();
}

bool VFXEffect::LoadFromBinary(const std::string& path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open())
    {
        GX_LOG_ERROR("VFXEffect: Failed to open binary file: %s", path.c_str());
        return false;
    }

    uint32_t magic = 0, version = 0;
    if (!ReadPOD(ifs, magic) || magic != k_VFXBinaryMagic)
    {
        GX_LOG_ERROR("VFXEffect: Invalid binary magic in: %s", path.c_str());
        return false;
    }
    if (!ReadPOD(ifs, version) || version != k_VFXBinaryVersion)
    {
        GX_LOG_ERROR("VFXEffect: Unsupported binary version %u in: %s", version, path.c_str());
        return false;
    }

    if (!ReadString(ifs, m_name)) return false;

    uint32_t emitterCount = 0;
    if (!ReadPOD(ifs, emitterCount)) return false;

    m_emitters.resize(emitterCount);
    for (uint32_t i = 0; i < emitterCount; ++i)
    {
        if (!ReadEmitterBin(ifs, m_emitters[i]))
        {
            GX_LOG_ERROR("VFXEffect: Failed reading emitter %u from: %s", i, path.c_str());
            m_emitters.clear();
            return false;
        }
    }

    return true;
}

// ============================================================================
// VFXEffect — GPUParticleSystemへの適用
// ============================================================================

void VFXEffect::ApplyToParticleSystem(GPUParticleSystem& ps, uint32_t emitterIndex) const
{
    if (emitterIndex >= m_emitters.size()) return;

    const VFXEmitter& em = m_emitters[emitterIndex];

    GPUParticleEmitterConfig cfg;
    cfg.maxParticles = static_cast<uint32_t>(em.maxParticles);
    cfg.emissionRate = em.emitRate;
    cfg.textureHandle = em.textureHandle;

    // VFXEmitterShapeをGPUParticleSystemのEmitterShapeにマッピング
    switch (em.shape)
    {
    case VFXEmitterShape::Point:      cfg.shape = EmitterShape::Point;  break;
    case VFXEmitterShape::Sphere:     cfg.shape = EmitterShape::Sphere; break;
    case VFXEmitterShape::Box:        cfg.shape = EmitterShape::Box;    break;
    case VFXEmitterShape::Cone:       cfg.shape = EmitterShape::Cone;   break;
    case VFXEmitterShape::Circle:     cfg.shape = EmitterShape::Ring;   break;
    case VFXEmitterShape::Hemisphere: cfg.shape = EmitterShape::Sphere; break;
    case VFXEmitterShape::Edge:       cfg.shape = EmitterShape::Point;  break;
    }

    // ブレンドモードのマッピング
    switch (em.blendMode)
    {
    case 0: cfg.blendMode = ParticleBlendMode::Alpha;    break;
    case 1: cfg.blendMode = ParticleBlendMode::Additive; break;
    default: cfg.blendMode = ParticleBlendMode::Alpha;   break;
    }

    // モジュールをコンフィグに適用
    for (const auto& mod : em.modules)
    {
        if (!mod.enabled) continue;

        switch (mod.type)
        {
        case VFXModuleType::InitialVelocity:
            cfg.initialVelocityMin = Vector3(mod.velocityX.min, mod.velocityY.min, mod.velocityZ.min);
            cfg.initialVelocityMax = Vector3(mod.velocityX.max, mod.velocityY.max, mod.velocityZ.max);
            break;

        case VFXModuleType::InitialSize:
            cfg.startSizeMin = mod.sizeMin.min;
            cfg.startSizeMax = mod.sizeMax.max;
            break;

        case VFXModuleType::InitialColor:
            cfg.startColor = Color(mod.colorRange.r0, mod.colorRange.g0,
                                   mod.colorRange.b0, mod.colorRange.a0);
            break;

        case VFXModuleType::InitialLifetime:
            cfg.lifetimeMin = mod.lifetimeRange.min;
            cfg.lifetimeMax = mod.lifetimeRange.max;
            break;

        case VFXModuleType::Gravity:
            cfg.gravity = Vector3(0.0f, mod.gravityStrength, 0.0f);
            break;

        case VFXModuleType::Drag:
            cfg.drag = mod.dragCoefficient;
            break;

        case VFXModuleType::SizeOverLife:
            if (!mod.sizeCurve.keys.empty())
            {
                cfg.endSize = mod.sizeCurve.Evaluate(1.0f);
            }
            break;

        case VFXModuleType::ColorOverLife:
            cfg.endColor = Color(mod.colorEnd.r0, mod.colorEnd.g0,
                                 mod.colorEnd.b0, mod.colorEnd.a0);
            break;

        default:
            break;
        }
    }

    ps.CreateEmitter(cfg);
}

// ============================================================================
// VFXInstance
// ============================================================================

void VFXInstance::Play()
{
    m_playing = true;
    m_paused = false;
    m_finished = false;
    m_elapsed = 0.0f;
    m_emitAccumulator = 0.0f;
}

void VFXInstance::Stop()
{
    m_playing = false;
    m_paused = false;
    m_finished = true;
    m_elapsed = 0.0f;
    m_emitAccumulator = 0.0f;
}

void VFXInstance::Pause()
{
    if (m_playing)
    {
        m_paused = true;
    }
}

void VFXInstance::Resume()
{
    if (m_paused)
    {
        m_paused = false;
    }
}

void VFXInstance::Update(float deltaTime)
{
    if (!m_playing || m_paused || !m_effect) return;

    m_elapsed += deltaTime;

    // 各エミッターのデュレーションをチェック
    bool allFinished = true;
    for (uint32_t i = 0; i < m_effect->GetEmitterCount(); ++i)
    {
        const VFXEmitter& em = m_effect->GetEmitter(i);

        // ディレイをチェック
        float effectiveTime = m_elapsed - em.startDelay;
        if (effectiveTime < 0.0f)
        {
            allFinished = false;
            continue;
        }

        // ループしないエミッターのデュレーションをチェック
        if (!em.looping && effectiveTime >= em.duration)
        {
            continue;
        }

        allFinished = false;

        // エミッション蓄積
        m_emitAccumulator += em.emitRate * deltaTime;

        // パーティクル放出（整数分を消費）
        while (m_emitAccumulator >= 1.0f)
        {
            m_emitAccumulator -= 1.0f;
            // 完全な実装ではGPUParticleSystem経由でパーティクルを生成する。
            // ここではアキュムレータの追跡のみ行う。
        }
    }

    if (allFinished && m_effect->GetEmitterCount() > 0)
    {
        m_playing = false;
        m_finished = true;
    }
}

} // namespace gx
