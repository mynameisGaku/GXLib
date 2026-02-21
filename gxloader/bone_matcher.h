#pragma once
/// @file bone_matcher.h
/// @brief ボーン名マッチング (リターゲティング用のフォールバック付き)
///
/// GXANのボーン名をモデルのスケルトンにマッピングする。
/// Mixamo/Blender等のツール間でボーン名の表記が異なる場合に
/// 段階的なフォールバック戦略で対応する。

#include <string>
#include <vector>
#include <cstdint>

namespace gxloader
{

/// @brief アニメーションのボーン名をスケルトンのジョイントインデックスにマッチする
/// @details 4段階フォールバック:
///   1. 完全一致
///   2. 大文字小文字無視
///   3. プレフィックス除去 (mixamorig:, armature|等) + 大文字小文字無視
///   4. 数値サフィックス除去 (.001, .002等) + ステップ3
/// @param animBoneName アニメーション側のボーン名
/// @param skeletonBoneNames スケルトンのボーン名一覧
/// @return マッチしたインデックス。見つからなければ-1
int MatchBoneName(const std::string& animBoneName,
                  const std::vector<std::string>& skeletonBoneNames);

/// @brief ボーン名を正規化する (プレフィックス・数値サフィックスを除去して小文字化)
/// @param name 正規化対象のボーン名
/// @return 正規化後のボーン名
std::string NormalizeBoneName(const std::string& name);

} // namespace gxloader
