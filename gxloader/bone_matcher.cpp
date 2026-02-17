/// @file bone_matcher.cpp
/// @brief Bone name matching implementation

#include "bone_matcher.h"
#include <algorithm>
#include <cctype>

namespace gxloader
{

static std::string ToLower(const std::string& s)
{
    std::string result = s;
    for (auto& c : result)
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return result;
}

static std::string StripPrefix(const std::string& name)
{
    // Strip common prefixes: "mixamorig:", "armature|", "Armature_"
    const char* prefixes[] = {
        "mixamorig:", "armature|", "armature_", "armature:", "root|"
    };

    std::string lower = ToLower(name);
    for (auto* prefix : prefixes)
    {
        size_t len = strlen(prefix);
        if (lower.size() > len && lower.substr(0, len) == prefix)
            return name.substr(len);
    }
    return name;
}

static std::string StripNumericSuffix(const std::string& name)
{
    // Strip ".001", ".002", etc.
    if (name.size() >= 4 && name[name.size() - 4] == '.')
    {
        bool allDigits = true;
        for (int i = 1; i <= 3; ++i)
        {
            char c = name[name.size() - i];
            if (c < '0' || c > '9') { allDigits = false; break; }
        }
        if (allDigits)
            return name.substr(0, name.size() - 4);
    }
    return name;
}

std::string NormalizeBoneName(const std::string& name)
{
    std::string result = StripPrefix(name);
    result = StripNumericSuffix(result);
    return ToLower(result);
}

int MatchBoneName(const std::string& animBoneName,
                  const std::vector<std::string>& skeletonBoneNames)
{
    // Level 1: Exact match
    for (size_t i = 0; i < skeletonBoneNames.size(); ++i)
    {
        if (animBoneName == skeletonBoneNames[i])
            return static_cast<int>(i);
    }

    // Level 2: Case-insensitive
    std::string animLower = ToLower(animBoneName);
    for (size_t i = 0; i < skeletonBoneNames.size(); ++i)
    {
        if (animLower == ToLower(skeletonBoneNames[i]))
            return static_cast<int>(i);
    }

    // Level 3: Strip prefixes + case-insensitive
    std::string animStripped = ToLower(StripPrefix(animBoneName));
    for (size_t i = 0; i < skeletonBoneNames.size(); ++i)
    {
        std::string skelStripped = ToLower(StripPrefix(skeletonBoneNames[i]));
        if (animStripped == skelStripped)
            return static_cast<int>(i);
    }

    // Level 4: Strip numeric suffixes + step 3
    std::string animNorm = NormalizeBoneName(animBoneName);
    for (size_t i = 0; i < skeletonBoneNames.size(); ++i)
    {
        std::string skelNorm = NormalizeBoneName(skeletonBoneNames[i]);
        if (animNorm == skelNorm)
            return static_cast<int>(i);
    }

    return -1;
}

} // namespace gxloader
