#pragma once
/// @file TimelinePanel.h
/// @brief アニメーションタイムラインパネル
///
/// 再生/一時停止/停止、タイムスクラブ、速度プリセット、クリップ切り替え、
/// ルートモーションロックの操作UIを提供する。

namespace GX { class Animator; class Model; }

/// @brief アニメーション再生制御のタイムラインパネル
class TimelinePanel
{
public:
    /// @brief タイムラインパネルを描画する
    /// @param animator 操作対象のAnimator（nullptrの場合は無効表示）
    /// @param model アニメーションクリップ一覧の取得元
    /// @param deltaTime フレーム経過時間（秒）
    /// @param selectedClipIndex 選択中クリップインデックスの読み書き先（nullptrの場合は無視）
    void Draw(GX::Animator* animator, const GX::Model* model,
              float deltaTime, int* selectedClipIndex);

private:
    bool  m_playing       = false;   ///< 再生中フラグ（UIの表示切り替え用）
    float m_playbackSpeed = 1.0f;    ///< 再生速度倍率
};
