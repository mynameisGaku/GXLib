/// @file main.cpp
/// @brief 14-hello-network — GX_* ネットワーク API でチャット (ADR-0017 L1)
///
/// 学習ポイント / Learning points:
///   - GX_StartServer / GX_Connect でサーバー/クライアント切替
///   - GX_SetOnReceive でデータ受信コールバック
///   - GX_Broadcast / GX_ClientSend でデータ送信
///   - GX_NetworkUpdate を毎フレーム呼ぶ
///
/// 使い方:
///   1. まず起動 → S キーでサーバーモード開始 (port 12345)
///   2. 別インスタンスを起動 → C キーで localhost:12345 に接続
///   3. 1-9 キーで数字メッセージを送信、受信側に表示される

#include "GXLib.h"
#include <cstdio>
#include <cstring>

static constexpr int PORT = 12345;
static constexpr int MAX_LOG = 16;

static char g_log[MAX_LOG][128];
static int  g_logCount = 0;

static void AddLog(const char* fmt, ...)
{
    if (g_logCount >= MAX_LOG)
    {
        for (int i = 0; i < MAX_LOG - 1; ++i)
            std::memcpy(g_log[i], g_log[i + 1], 128);
        g_logCount = MAX_LOG - 1;
    }
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(g_log[g_logCount], 128, fmt, args);
    va_end(args);
    ++g_logCount;
}

static void OnReceive(int fromClient, const void* data, int size)
{
    if (size > 0 && size < 120)
    {
        char buf[128];
        std::memcpy(buf, data, static_cast<size_t>(size));
        buf[size] = '\0';
        AddLog("[Client %d] %s", fromClient, buf);
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(640, 480, 32);
    SetMainWindowText("14 - Hello Network");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    gx::GX_SetOnReceive(OnReceive);

    bool isServer = false;
    bool isClient = false;

    unsigned int white  = GetColor(255, 255, 255);
    unsigned int green  = GetColor(80, 220, 80);
    unsigned int cyan   = GetColor(100, 220, 255);
    unsigned int yellow = GetColor(255, 220, 50);
    unsigned int gray   = GetColor(120, 120, 120);

    AddLog("Press S to start server, C to connect as client.");

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        ClearDrawScreen();

        // S: サーバー開始
        if (!isServer && !isClient && CheckHitKey(KEY_INPUT_S))
        {
            if (gx::GX_StartServer(PORT, 8) == 0)
            {
                isServer = true;
                AddLog("Server started on port %d.", PORT);
            }
            else
            {
                AddLog("Failed to start server.");
            }
        }

        // C: クライアント接続
        if (!isServer && !isClient && CheckHitKey(KEY_INPUT_C))
        {
            if (gx::GX_Connect("127.0.0.1", PORT) == 0)
            {
                isClient = true;
                AddLog("Connected to 127.0.0.1:%d.", PORT);
            }
            else
            {
                AddLog("Failed to connect.");
            }
        }

        // 1-9: メッセージ送信
        for (int k = KEY_INPUT_1; k <= KEY_INPUT_9; ++k)
        {
            if (CheckHitKey(k))
            {
                char msg[32];
                std::snprintf(msg, sizeof(msg), "Hello #%d", k - KEY_INPUT_1 + 1);
                int len = static_cast<int>(std::strlen(msg));
                if (isServer)
                {
                    gx::GX_Broadcast(msg, len);
                    AddLog("[Server] Broadcast: %s", msg);
                }
                else if (isClient)
                {
                    gx::GX_ClientSend(msg, len);
                    AddLog("[Me] Sent: %s", msg);
                }
            }
        }

        gx::GX_NetworkUpdate();

        // 描画
        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());

        if (isServer)
            DrawFormatString(10, 30, green, "MODE: Server (port %d)  Clients: %d",
                             PORT, gx::GX_GetNetClientCount());
        else if (isClient)
            DrawFormatString(10, 30, cyan, "MODE: Client  Connected: %s",
                             gx::GX_IsNetConnected() ? "YES" : "NO");
        else
            DrawString(10, 30, "MODE: None  (S=server, C=client)", yellow);

        DrawString(10, 60, "--- Log ---", gray);
        for (int i = 0; i < g_logCount; ++i)
            DrawString(10, 80 + i * 18, g_log[i], white);

        DrawString(10, 460, "1-9: send message  |  ESC: quit", gray);

        ScreenFlip();
    }

    if (isServer) gx::GX_StopServer();
    if (isClient) gx::GX_Disconnect();
    GX_End();
    return 0;
}
