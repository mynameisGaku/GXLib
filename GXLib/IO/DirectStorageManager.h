#pragma once
/// @file DirectStorageManager.h
/// @brief DirectStorage APIラッパー（動的DLLロード + フォールバック非同期IO）
///
/// dstorage.dll が存在すれば DirectStorage API を使用し、
/// なければ std::ifstream ベースのフォールバックで動作する。
/// @addtogroup grp_io/// @{

#include "pch_common.h"
#include <functional>

namespace gx
{

/// @brief DirectStorage読み取りリクエスト記述子
struct DirectStorageRequest
{
    std::wstring filePath;            ///< 読み取り対象ファイルパス
    uint64_t fileOffset = 0;          ///< ファイル内オフセット（バイト）
    uint32_t size = 0;                ///< 読み取りサイズ（バイト）
    void* destination = nullptr;      ///< 読み取り先バッファ
    uint32_t requestId = 0;           ///< リクエストID（内部使用）
    bool gpuDecompress = false;       ///< GPU展開を使用するか
};

/// @brief 個々のDirectStorageリクエストの状態
enum class DirectStorageStatus : uint32_t
{
    Pending,      ///< 保留中
    InProgress,   ///< 実行中
    Completed,    ///< 完了
    Failed,       ///< 失敗
};

/// @brief DirectStorageマネージャー（動的DLLロード + フォールバックIO）
///
/// dstorage.dll をランタイムでロードし、利用可能であれば DirectStorage API を使用する。
/// DLL が見つからない環境では std::ifstream による同期フォールバックを提供する。
class DirectStorageManager
{
public:
    DirectStorageManager();
    ~DirectStorageManager();

    /// @brief DirectStorageを初期化する（dstorage.dllのロードを試行）
    /// @param d3d12Device GPU展開用のオプションD3D12デバイスポインタ
    /// @return dstorage.dllのロードに成功した場合true
    bool Initialize(void* d3d12Device = nullptr);

    /// @brief DirectStorageが利用可能かどうか
    bool IsAvailable() const { return m_available; }

    /// @brief 後続の読み取りリクエスト用にファイルを開く
    /// @param path 開くファイルシステムパス
    /// @return 成功した場合true
    bool OpenFile(const std::wstring& path);

    /// @brief 以前に開いたファイルを閉じる
    void CloseFile(const std::wstring& path);

    /// @brief CPU読み取りリクエストをキューに追加する
    /// @return 状態追跡用のリクエストID
    uint32_t EnqueueRead(const DirectStorageRequest& request);

    /// @brief GPU展開リクエストをキューに追加する
    /// @return 状態追跡用のリクエストID
    uint32_t EnqueueGPUDecompress(const DirectStorageRequest& request);

    /// @brief 保留中の全リクエストを実行に送信する
    void Submit();

    /// @brief 特定リクエストの状態を照会する
    DirectStorageStatus GetRequestStatus(uint32_t requestId) const;

    /// @brief 完了を処理しコールバックを発火する
    void Update();

    /// @brief Submit + Updateを一度に実行する
    void Flush();

    /// @brief 未完了のリクエスト数
    uint32_t GetPendingRequestCount() const { return m_pendingCount; }

    /// @brief 直近の操作で計測された帯域幅 (MB/s)
    float GetBandwidthMBps() const { return m_bandwidthMBps; }

    /// @brief リクエスト完了時に呼ばれるコールバックを設定する
    void SetOnComplete(std::function<void(uint32_t requestId, bool success)> callback)
    {
        m_onComplete = std::move(callback);
    }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;                      ///< DirectStorage API実装詳細
    bool m_available = false;                           ///< DirectStorageが利用可能か
    uint32_t m_nextRequestId = 1;                       ///< 次に割り当てるリクエストID
    uint32_t m_pendingCount = 0;                        ///< 未完了リクエスト数
    float m_bandwidthMBps = 0.0f;                       ///< 直近の帯域幅計測値（MB/s）
    std::function<void(uint32_t, bool)> m_onComplete;   ///< リクエスト完了コールバック

    struct RequestInfo
    {
        uint32_t id;                  ///< リクエストID
        DirectStorageStatus status;   ///< 現在の状態
        std::wstring filePath;        ///< 対象ファイルパス
        uint64_t offset;              ///< ファイル内オフセット
        uint32_t size;                ///< 読み取りサイズ
        void* destination;            ///< 読み取り先バッファ
    };
    std::vector<RequestInfo> m_requests;               ///< アクティブなリクエスト一覧
    std::unordered_map<std::wstring, bool> m_openFiles; ///< 開いているファイルの管理
};

} // namespace gx
/// @}
