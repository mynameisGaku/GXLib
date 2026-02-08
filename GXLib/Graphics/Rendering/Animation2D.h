#pragma once
/// @file Animation2D.h
/// @brief 2Dアニメーション管理クラス
///
/// 【初学者向け解説】
/// Animation2Dは、スプライトシートのフレームを時間で切り替えて
/// アニメーションさせるためのクラスです。
///
/// 使い方：
/// 1. LoadDivGraphでスプライトシートを読み込んでハンドル配列を取得
/// 2. AddFrames()でアニメーションフレームを登録
/// 3. 毎フレームUpdate(deltaTime)を呼び出し
/// 4. GetCurrentHandle()で現在のフレームのハンドルを取得してDrawGraph

#include "pch.h"

namespace GX
{

/// @brief 2Dスプライトアニメーション管理
class Animation2D
{
public:
    Animation2D() = default;
    ~Animation2D() = default;

    /// アニメーションフレームを追加
    /// @param handles フレームのハンドル配列
    /// @param count フレーム数
    /// @param frameDuration 1フレームあたりの表示時間（秒）
    void AddFrames(const int* handles, int count, float frameDuration);

    /// アニメーションを更新
    /// @param deltaTime 経過時間（秒）
    void Update(float deltaTime);

    /// 現在のフレームのテクスチャハンドルを取得
    int GetCurrentHandle() const;

    /// 現在のフレームインデックスを取得
    int GetCurrentFrameIndex() const { return m_currentFrame; }

    /// ループ設定
    void SetLoop(bool loop) { m_loop = loop; }

    /// アニメーションをリセット（先頭フレームに戻る）
    void Reset();

    /// アニメーションが終了したかどうか（非ループ時のみ有効）
    bool IsFinished() const { return m_finished; }

    /// 再生速度を設定（デフォルト: 1.0）
    void SetSpeed(float speed) { m_speed = speed; }

private:
    struct Frame
    {
        int   handle;
        float duration;
    };

    std::vector<Frame> m_frames;
    int    m_currentFrame = 0;
    float  m_timer        = 0.0f;
    bool   m_loop         = true;
    bool   m_finished     = false;
    float  m_speed        = 1.0f;
};

} // namespace GX
