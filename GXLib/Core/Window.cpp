/// @file Window.cpp
/// @brief Win32ウィンドウ管理の実装
#include "pch.h"
#include "Core/Window.h"
#include "Core/Logger.h"

namespace GX
{

Window::~Window()
{
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool Window::Initialize(const WindowDesc& desc)
{
    m_width  = desc.width;
    m_height = desc.height;

    // ウィンドウクラスを登録
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandle(nullptr);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"GXLibWindowClass";

    if (!RegisterClassExW(&wc))
    {
        GX_LOG_ERROR("Failed to register window class");
        return false;
    }

    // クライアント領域のサイズからウィンドウ全体のサイズを計算
    RECT rect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth  = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    // 画面中央に配置
    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth  - windowWidth)  / 2;
    int posY = (screenHeight - windowHeight) / 2;

    // ウィンドウを作成
    m_hwnd = CreateWindowExW(
        0,
        L"GXLibWindowClass",
        desc.title.c_str(),
        WS_OVERLAPPEDWINDOW,
        posX, posY,
        windowWidth, windowHeight,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this    // WndProcにthisポインタを渡す
    );

    if (!m_hwnd)
    {
        GX_LOG_ERROR("Failed to create window");
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    GX_LOG_INFO("Window created: %dx%d", m_width, m_height);
    return true;
}

bool Window::ProcessMessages()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

void Window::SetResizeCallback(std::function<void(uint32_t, uint32_t)> callback)
{
    m_resizeCallback = std::move(callback);
}

void Window::AddMessageCallback(std::function<bool(HWND, UINT, WPARAM, LPARAM)> callback)
{
    m_messageCallbacks.push_back(std::move(callback));
}

void Window::SetTitle(const std::wstring& title)
{
    if (m_hwnd)
    {
        SetWindowTextW(m_hwnd, title.c_str());
    }
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
    {
        auto createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto window = static_cast<Window*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        return 0;
    }

    auto window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    // 登録されたメッセージコールバックを呼び出す（Input等が利用）
    if (window)
    {
        for (auto& callback : window->m_messageCallbacks)
        {
            if (callback(hwnd, msg, wParam, lParam))
                break;  // 処理済み（ただしWindowの処理は続行）
        }
    }

    switch (msg)
    {
    case WM_SIZE:
    {
        if (window && wParam != SIZE_MINIMIZED)
        {
            uint32_t width  = LOWORD(lParam);
            uint32_t height = HIWORD(lParam);
            if (width > 0 && height > 0)
            {
                window->m_width  = width;
                window->m_height = height;
                if (window->m_resizeCallback)
                {
                    window->m_resizeCallback(width, height);
                }
            }
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

} // namespace GX
