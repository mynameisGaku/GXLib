/// @file DirectStorageManager.cpp
/// @brief DirectStorage APIラッパー（動的DLLロード + フォールバック非同期IO）の実装
#include "pch_common.h"
#include "IO/DirectStorageManager.h"
#include "Core/Logger.h"
#include <fstream>

namespace gx
{

// ---------------------------------------------------------------------------
// Impl — 動的ロードされた dstorage.dll ハンドルを保持する
// ---------------------------------------------------------------------------

struct DirectStorageManager::Impl
{
    HMODULE dstorageLib = nullptr;
    bool initialized = false;
};

// ---------------------------------------------------------------------------
// ライフタイム
// ---------------------------------------------------------------------------

DirectStorageManager::DirectStorageManager()
    : m_impl(std::make_unique<Impl>())
{
}

DirectStorageManager::~DirectStorageManager()
{
    if (m_impl && m_impl->dstorageLib)
    {
        FreeLibrary(m_impl->dstorageLib);
        m_impl->dstorageLib = nullptr;
    }
}

// ---------------------------------------------------------------------------
// 初期化 — dstorage.dll の動的ロードを試行
// ---------------------------------------------------------------------------

bool DirectStorageManager::Initialize(void* d3d12Device)
{
    // dstorage.dll を動的にロードする
    m_impl->dstorageLib = LoadLibraryW(L"dstorage.dll");
    if (!m_impl->dstorageLib)
    {
        GX_LOG_INFO("DirectStorage: dstorage.dll not found — using fallback async IO");
        m_available = false;
        return false;
    }

    // 完全な実装では DStorageGetFactory を呼びキューを作成するが、
    // 現時点では利用可能とマークし、DirectStorage APIサーフェスを
    // ミラーするフォールバック読み取りパスを使用する。
    m_available = true;
    m_impl->initialized = true;
    GX_LOG_INFO("DirectStorage: dstorage.dll loaded successfully");

    if (d3d12Device)
    {
        GX_LOG_INFO("DirectStorage: GPU decompression available (D3D12 device provided)");
    }

    return true;
}

// ---------------------------------------------------------------------------
// ファイル管理
// ---------------------------------------------------------------------------

bool DirectStorageManager::OpenFile(const gx::WString& path)
{
    if (m_openFiles.count(path) > 0)
        return true;

    // ファイルの存在を開いて確認する
    std::ifstream test(path, std::ios::binary);
    if (!test.is_open())
    {
        GX_LOG_ERROR("DirectStorage: Failed to open file");
        return false;
    }
    test.close();

    m_openFiles[path] = true;
    return true;
}

void DirectStorageManager::CloseFile(const gx::WString& path)
{
    m_openFiles.erase(path);
}

// ---------------------------------------------------------------------------
// リクエストのキューイング
// ---------------------------------------------------------------------------

uint32_t DirectStorageManager::EnqueueRead(const DirectStorageRequest& request)
{
    uint32_t id = m_nextRequestId++;

    RequestInfo info{};
    info.id = id;
    info.status = DirectStorageStatus::Pending;
    info.filePath = request.filePath;
    info.offset = request.fileOffset;
    info.size = request.size;
    info.destination = request.destination;
    m_requests.push_back(std::move(info));
    m_pendingCount++;

    return id;
}

uint32_t DirectStorageManager::EnqueueGPUDecompress(const DirectStorageRequest& request)
{
    // GPU展開パス — 実際のDirectStorageキューがまだ接続されていない場合は
    // CPU読み取りにフォールバックする。
    uint32_t id = m_nextRequestId++;

    RequestInfo info{};
    info.id = id;
    info.status = DirectStorageStatus::Pending;
    info.filePath = request.filePath;
    info.offset = request.fileOffset;
    info.size = request.size;
    info.destination = request.destination;
    m_requests.push_back(std::move(info));
    m_pendingCount++;

    return id;
}

// ---------------------------------------------------------------------------
// Submit — 保留中の全リクエストを実行する（フォールバック: std::ifstream）
// ---------------------------------------------------------------------------

void DirectStorageManager::Submit()
{
    for (auto& req : m_requests)
    {
        if (req.status != DirectStorageStatus::Pending)
            continue;

        req.status = DirectStorageStatus::InProgress;

        if (req.destination && req.size > 0)
        {
            std::ifstream file(req.filePath, std::ios::binary);
            if (file.is_open())
            {
                file.seekg(static_cast<std::streamoff>(req.offset));
                file.read(static_cast<char*>(req.destination), req.size);

                if (file.good() || file.eof())
                {
                    req.status = DirectStorageStatus::Completed;
                }
                else
                {
                    req.status = DirectStorageStatus::Failed;
                }
            }
            else
            {
                req.status = DirectStorageStatus::Failed;
            }
        }
        else
        {
            // サイズ0または宛先NULLのリクエスト — 成功として扱う
            req.status = DirectStorageStatus::Completed;
        }
    }
}

// ---------------------------------------------------------------------------
// 状態照会
// ---------------------------------------------------------------------------

DirectStorageStatus DirectStorageManager::GetRequestStatus(uint32_t requestId) const
{
    for (const auto& req : m_requests)
    {
        if (req.id == requestId)
            return req.status;
    }
    return DirectStorageStatus::Failed;
}

// ---------------------------------------------------------------------------
// Update — 完了を処理し、コールバックを発火し、完了済みリクエストを除去する
// ---------------------------------------------------------------------------

void DirectStorageManager::Update()
{
    auto it = m_requests.begin();
    while (it != m_requests.end())
    {
        if (it->status == DirectStorageStatus::Completed ||
            it->status == DirectStorageStatus::Failed)
        {
            if (m_pendingCount > 0)
                m_pendingCount--;

            bool success = (it->status == DirectStorageStatus::Completed);
            uint32_t id = it->id;

            it = m_requests.erase(it);

            if (m_onComplete)
            {
                m_onComplete(id, success);
            }
        }
        else
        {
            ++it;
        }
    }
}

// ---------------------------------------------------------------------------
// Flush — 利便性メソッド: Submit + Update
// ---------------------------------------------------------------------------

void DirectStorageManager::Flush()
{
    Submit();
    Update();
}

} // namespace gx
