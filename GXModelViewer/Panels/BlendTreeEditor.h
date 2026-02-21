#pragma once
/// @file BlendTreeEditor.h
/// @brief ブレンドツリーパラメータ編集・可視化パネル
///
/// 1Dブレンドツリーは数直線上にノード閾値を表示し、三角形で現在パラメータを示す。
/// 2Dブレンドツリーは散布図上にノード位置を表示し、ドラッグでパラメータを操作できる。

namespace GX { class BlendTree; }

/// @brief BlendTree（1D/2D）のパラメータ編集と視覚化を行うパネル
class BlendTreeEditor
{
public:
    /// @brief ブレンドツリーエディタパネルを描画する
    /// @param blendTree 編集対象のブレンドツリー（nullptrの場合は無効表示）
    void Draw(GX::BlendTree* blendTree);
};
