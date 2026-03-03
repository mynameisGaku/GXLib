#pragma once
/// @file HTTPClient.h
/// @brief HTTPクライアント — WinHTTP API ベース
///
/// 同期/非同期のGET/POSTリクエストをサポートする。
/// 非同期リクエストは Update() をフレームループ内で呼び出してコールバックを発火する。

namespace gx {
/// @addtogroup grp_network
/// @{

/// @brief HTTPレスポンス
struct HTTPResponse {
    int statusCode = 0;                                         ///< HTTPステータスコード
    gx::String body;                                           ///< レスポンスボディ
    gx::HashMap<gx::String, gx::String> headers;       ///< レスポンスヘッダー

    /// @brief リクエストが成功 (2xx) かどうか判定する
    /// @return ステータスコードが200-299の場合true
    bool IsSuccess() const { return statusCode >= 200 && statusCode < 300; }
};

/// @brief HTTPクライアント
class HTTPClient
{
public:
    /// @brief WinHTTPセッションを初期化する
    HTTPClient();
    /// @brief 全非同期スレッドを停止しセッションを閉じる
    ~HTTPClient();

    /// @brief 同期GETリクエストを送信する
    /// @param url リクエストURL
    /// @param headers 追加HTTPヘッダー (省略可)
    /// @return HTTPレスポンス
    HTTPResponse Get(const gx::String& url,
                     const gx::HashMap<gx::String, gx::String>& headers = {});

    /// @brief 同期POSTリクエストを送信する
    /// @param url リクエストURL
    /// @param body リクエストボディ
    /// @param contentType Content-Typeヘッダー (デフォルト: "application/json")
    /// @param headers 追加HTTPヘッダー (省略可)
    /// @return HTTPレスポンス
    HTTPResponse Post(const gx::String& url, const gx::String& body,
                      const gx::String& contentType = "application/json",
                      const gx::HashMap<gx::String, gx::String>& headers = {});

    /// @brief 非同期GETリクエストを送信する (Update()でコールバック発火)
    /// @param url リクエストURL
    /// @param callback 完了時コールバック (メインスレッドで呼ばれる)
    void GetAsync(const gx::String& url,
                  std::function<void(HTTPResponse)> callback);

    /// @brief 非同期POSTリクエストを送信する (Update()でコールバック発火)
    /// @param url リクエストURL
    /// @param body リクエストボディ
    /// @param contentType Content-Typeヘッダー
    /// @param callback 完了時コールバック (メインスレッドで呼ばれる)
    void PostAsync(const gx::String& url, const gx::String& body,
                   const gx::String& contentType,
                   std::function<void(HTTPResponse)> callback);

    /// @brief 完了した非同期リクエストのコールバックを発火する (メインスレッドで毎フレーム呼ぶ)
    void Update();

    /// @brief リクエストタイムアウトを設定する
    /// @param timeoutMs タイムアウト (ミリ秒、デフォルト: 30000)
    void SetTimeout(int timeoutMs);

private:
    void* m_hSession = nullptr; // HINTERNET
    int m_timeoutMs = 30000;

    std::mutex m_mutex;
    gx::Vector<std::pair<HTTPResponse, std::function<void(HTTPResponse)>>> m_completedQueue;

    gx::Vector<std::thread> m_threads;
    std::atomic<bool> m_running{ true };

    HTTPResponse SendRequest(const gx::String& method, const gx::String& url,
                              const gx::String& body,
                              const gx::HashMap<gx::String, gx::String>& headers);
};

/// @}
} // namespace gx
