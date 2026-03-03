#pragma once
/// @file GPUDebugLayer.h
/// @brief D3D12デバッグレイヤーの有効化・エラー診断ユーティリティ
///
/// デバイス作成前にEnable()を呼ぶとAPI誤用をリアルタイムで検出できる。
/// GPUがクラッシュした場合はGetDREDReport()でどの描画命令が原因かをログに出力できる。
/// シャットダウン時のReportLiveObjects()でリソースリークも検出可能。
/// @addtogroup grp_gfx_device/// @{

#include "pch_graphics.h"

namespace gx
{

/// @brief DRED (Device Removed Extended Data) レポート
struct DREDReport
{
    gx::String breadcrumbs;        ///< 自動ブレッドクラムの経路
    gx::String pageFaultInfo;      ///< ページフォルトアドレス情報
    bool available = false;         ///< DREDデータが取得できた場合true
};

/// @brief D3D12バリデーションおよび診断用GPUデバッグレイヤーユーティリティ
class GPUDebugLayer
{
public:
    /// @brief D3D12デバッグレイヤーを有効化する。デバイス作成の前に呼ぶ必要がある。
    /// @param gpuValidation GPUベースバリデーションを有効化する（低速だがより多くのエラーを検出）
    /// @return デバッグレイヤーの有効化に成功した場合true
    static bool Enable(bool gpuValidation = false);

    /// @brief エラー/破損時にデバッガブレークするようInfoQueueを設定する
    /// @param device D3D12デバイス
    /// @param breakOnError エラー時にデバッガブレークする
    /// @param breakOnCorruption 破損時にデバッガブレークする
    static void ConfigureInfoQueue(ID3D12Device* device,
                                    bool breakOnError = true,
                                    bool breakOnCorruption = true);

    /// @brief デバイス削除イベント後にDREDレポートを取得する
    /// @param device D3D12デバイス（削除状態の可能性あり）
    /// @return ブレッドクラムとページフォルト情報を含むDREDレポート
    static DREDReport GetDREDReport(ID3D12Device* device);

    /// @brief 生存中のD3D12/DXGIオブジェクトを報告する（シャットダウン時のリーク検出用）
    static void ReportLiveObjects();

    /// @brief デバイス削除理由のHRESULTを人間が読める文字列に変換する
    /// @param hr GetDeviceRemovedReason()からのHRESULT
    /// @return 人間が読める説明文
    static gx::String TranslateDeviceRemovedReason(HRESULT hr);

    /// @brief デバッグレイヤーが現在有効かどうかを確認する
    static bool IsEnabled() { return s_enabled; }

private:
    static bool s_enabled;
};

} // namespace gx
/// @}
