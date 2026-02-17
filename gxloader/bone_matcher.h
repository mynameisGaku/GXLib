#pragma once
/// @file bone_matcher.h
/// @brief Bone name matching with fallback strategies for retargeting

#include <string>
#include <vector>
#include <cstdint>

namespace gxloader
{

/// Match bone name from animation to skeleton joint index.
/// Uses 4-level fallback:
///   1. Exact match
///   2. Case-insensitive
///   3. Strip prefixes (mixamorig:, armature|) + case-insensitive
///   4. Strip numeric suffixes (.001, .002) + step 3
/// Returns -1 if no match found.
int MatchBoneName(const std::string& animBoneName,
                  const std::vector<std::string>& skeletonBoneNames);

/// Normalize a bone name (strip prefixes and suffixes)
std::string NormalizeBoneName(const std::string& name);

} // namespace gxloader
