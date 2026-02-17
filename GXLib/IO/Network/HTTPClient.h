#pragma once
/// @file HTTPClient.h
/// @brief HTTPクライアント — WinHTTP API ベース
///
/// 同期/非同期のGET/POSTリクエストをサポートする。
/// 非同期リクエストは Update() をフレームループ内で呼び出してコールバックを発火する。

namespace GX {

/// @brief HTTPレスポンス
struct HTTPResponse {
    int statusCode = 0;                                         ///< HTTPステータスコード
    std::string body;                                           ///< レスポンスボディ
    std::unordered_map<std::string, std::string> headers;       ///< レスポンスヘッダー

    /// @brief リクエストが成功 (2xx) かどうか判定する
    /// @return ステータスコードが200-299の場合true
    bool IsSuccess() const { return statusCode >= 200 && statusCode < 300; }
};

/// @brief HTTPクライアント
class HTTPClient
{
public:
    HTTPClient();
    ~HTTPClient();

    /// @brief 同期GETリクエストを送信する
    /// @param url リクエストURL
    /// @param headers 追加HTTPヘッダー (省略可)
    /// @return HTTPレスポンス
    HTTPResponse Get(const std::string& url,
                     const std::unordered_map<std::string, std::string>& headers = {});

    /// @brief 同期POSTリクエストを送信する
    /// @param url リクエストURL
    /// @param body リクエストボディ
    /// @param contentType Content-Typeヘッダー (デフォルト: "application/json")
    /// @param headers 追加HTTPヘッダー (省略可)
    /// @return HTTPレスポンス
    HTTPResponse Post(const std::string& url, const std::string& body,
                      const std::string& contentType = "application/json",
                      const std::unordered_map<std::string, std::string>& headers = {});

    /// @brief 非同期GETリクエストを送信する (Update()でコールバック発火)
    /// @param url リクエストURL
    /// @param callback 完了時コールバック (メインスレッドで呼ばれる)
    void GetAsync(const std::string& url,
                  std::function<void(HTTPResponse)> callback);

    /// @brief 非同期POSTリクエストを送信する (Update()でコールバック発火)
    /// @param url リクエストURL
    /// @param body リクエストボディ
    /// @param contentType Content-Typeヘッダー
    /// @param callback 完了時コールバック (メインスレッドで呼ばれる)
    void PostAsync(const std::string& url, const std::string& body,
                   const std::string& contentType,
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
    std::vector<std::pair<HTTPResponse, std::function<void(HTTPResponse)>>> m_completedQueue;

    std::vector<std::thread> m_threads;
    std::atomic<bool> m_running{ true };

    HTTPResponse SendRequest(const std::string& method, const std::string& url,
                              const std::string& body,
                              const std::unordered_map<std::string, std::string>& headers);
};

} // namespace GX
