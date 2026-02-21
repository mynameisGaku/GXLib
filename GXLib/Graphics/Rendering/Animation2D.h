#pragma once
/// @file Animation2D.h
/// @brief 2Dスプライトアニメーション
///
/// SpriteSheet で分割したコマを時間経過で切り替え、アニメーション再生を行う。
/// DxLib には直接の対応はないが、LoadDivGraph + フレームカウンタで実装するパターンを
/// クラスにまとめたもの。
///
/// 使い方:
///   1. SpriteSheet::LoadDivGraph でコマのハンドル配列を取得
///   2. AddFrames() でアニメーションフレームとして登録
///   3. 毎フレーム Update(deltaTime) を呼ぶ
///   4. GetCurrentHandle() で今のコマを取得し DrawGraph で描画

#include "pch.h"

namespace GX
{

/// @brief 2Dスプライトアニメーション再生クラス
class Animation2D
{
public:
    Animation2D() = default;
    ~Animation2D() = default;

    /// @brief アニメーションフレームを追加する
    /// @param handles テクスチャハンドルの配列（SpriteSheet::LoadDivGraph 等で取得したもの）
    /// @param count フレーム数
    /// @param frameDuration 1コマあたりの表示時間（秒）
    void AddFrames(const int* handles, int count, float frameDuration);

    /// @brief アニメーションの時間を進める（毎フレーム呼ぶ）
    /// @param deltaTime 前フレームからの経過時間（秒）
    void Update(float deltaTime);

    /// @brief 現在表示すべきコマのテクスチャハンドルを取得する
    /// @return テクスチャハンドル。フレーム未登録時は -1
    int GetCurrentHandle() const;

    /// @brief 現在のフレームインデックスを取得する
    /// @return 0始まりのフレーム番号
    int GetCurrentFrameIndex() const { return m_currentFrame; }

    /// @brief ループ再生の有無を設定する
    /// @param loop true でループ、false で末尾で停止
    void SetLoop(bool loop) { m_loop = loop; }

    /// @brief アニメーションを先頭フレームに巻き戻す
    void Reset();

    /// @brief 非ループ時にアニメーションが末尾に到達したか
    /// @return 到達していれば true
    bool IsFinished() const { return m_finished; }

    /// @brief 再生速度の倍率を設定する（デフォルト 1.0）
    /// @param speed 2.0 で倍速、0.5 で半速
    void SetSpeed(float speed) { m_speed = speed; }

private:
    /// 1コマ分の情報
    struct Frame
    {
        int   handle;   ///< テクスチャハンドル
        float duration; ///< 表示時間（秒）
    };

    std::vector<Frame> m_frames;          ///< 全フレームの配列
    int    m_currentFrame = 0;            ///< 現在のフレームインデックス
    float  m_timer        = 0.0f;         ///< 現在のコマ内経過時間
    bool   m_loop         = true;         ///< ループ再生するか
    bool   m_finished     = false;        ///< 非ループ時の終了フラグ
    float  m_speed        = 1.0f;         ///< 再生速度倍率
};

} // namespace GX
