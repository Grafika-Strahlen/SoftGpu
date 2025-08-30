/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include "vd/Window.hpp"

#include <ConPrinter.hpp>

namespace tau::vd {

#ifdef _WIN32
static constexpr const wchar_t WindowClassName[] = L"SoftGpu Virtual Display Window";
#endif

Window::~Window() noexcept
{
#ifdef _WIN32
    ReleaseDC(m_Window, m_hDc);
    DestroyWindow(m_Window);
    UnregisterClassW(WindowClassName, GetModuleHandleW(nullptr));
#endif
    SDL_DestroyWindow(m_SdlWindow);
}

void Window::PollMessages() const noexcept
{
#ifdef _WIN32
    MSG msg;
    while(PeekMessageW(&msg, m_Window, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
#endif
}

void Window::Close() noexcept
{
    m_ShouldClose = true;
}

#ifdef _WIN32
static void CreateWindowWin32()
{
    WNDCLASSEXW windowClass { };

    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_DBLCLKS;
    windowClass.lpfnWndProc = StaticWindowProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = WindowClassName;

    (void) RegisterClassExW(&windowClass);

    ReferenceCountingPointer<Window> window(windowClass);

    if(!window)
    {
        return nullptr;
    }

    HWND hWnd = CreateWindowExW(
            0,
            WindowClassName,
            L"SoftGpu Display",
            WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            nullptr,
            nullptr,
            windowClass.hInstance,
            window.Get()
    );

    if(!hWnd)
    {
        return nullptr;
    }

    window->m_Window = hWnd;
    window->m_hDc = GetDC(hWnd);

    ::ShowWindow(hWnd, SW_SHOWNORMAL);

    GetClientRect(hWnd, &window->m_FramebufferSize);

    RECT newRect;
    newRect.left = 0;
    newRect.top = 0;
    // newRect.right = 1032;
    newRect.right = 1024;
    newRect.bottom = 768;
    //
    // (void) AdjustWindowRect(&newRect, WS_OVERLAPPEDWINDOW, FALSE);
    (void) SetWindowPos(hWnd, nullptr, newRect.left, newRect.top, newRect.right, newRect.bottom, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER);

    GetClientRect(hWnd, &window->m_FramebufferSize);

    window->m_HorizontalSlop = newRect.right - window->m_FramebufferSize.right;
    window->m_VerticalSlop = newRect.bottom - window->m_FramebufferSize.bottom;

    return window;
}
#endif

ReferenceCountingPointer<Window> Window::CreateWindow()
{
#ifdef _WIN32
    return CreateWindowWin32();
#else
    SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    SDL_Window* window = SDL_CreateWindow(
        "SoftGPU Display",
        1024,
        768,
        windowFlags
    );

    if(!window)
    {
        ConPrinter::PrintLn("Error creating window: {}", SDL_GetError());
        return nullptr;
    }

    int frameWidth;
    int frameHeight;
    if(!SDL_GetWindowSizeInPixels(window, &frameWidth, &frameHeight))
    {
        ConPrinter::PrintLn("Error getting window pixel size: {}", SDL_GetError());

        SDL_DestroyWindow(window);
        return nullptr;
    }

    SDL_ShowWindow(window);

    return ReferenceCountingPointer<Window>(
        window,
        frameWidth,
        frameHeight
    );
#endif
}

#ifdef _WIN32
LRESULT Window::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_SIZE:
            (void) GetClientRect(m_Window, &m_FramebufferSize);
            if(m_ResizeCallback)
            {
                m_ResizeCallback(FramebufferWidth(), FramebufferHeight());
            }
            break;
        case WM_CLOSE:
            m_ShouldClose = true;
            return 0;
        default: break;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT Window::StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(uMsg == WM_NCCREATE)
    {
        const CREATESTRUCTW* const createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);

        if(!createStruct->lpCreateParams)
        {
            return FALSE;
        }

        (void) SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
    }

    Window* const windowPtr = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    if(!windowPtr)
    {
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    return windowPtr->WindowProc(hWnd, uMsg, wParam, lParam);
}
#endif

void Window::ShowWindow() noexcept
{
#ifdef _WIN32
    ::ShowWindow(m_Window, SW_SHOWNA);
#endif
    SDL_ShowWindow(m_SdlWindow);
}

void Window::HideWindow() noexcept
{
#ifdef _WIN32
    ::ShowWindow(m_Window, SW_HIDE);
#endif
    SDL_HideWindow(m_SdlWindow);
}

void Window::SetSize(const u32 width, const u32 height) noexcept
{
#ifdef _WIN32
    RECT newRect;
    newRect.left = 0;
    newRect.top = 0;
    // newRect.right = static_cast<LONG>(width) + 8;
    // newRect.bottom = static_cast<LONG>(height) + 31;
    newRect.right = static_cast<LONG>(width + m_HorizontalSlop);
    newRect.bottom = static_cast<LONG>(height + m_VerticalSlop);

    // (void) AdjustWindowRect(&newRect, WS_OVERLAPPEDWINDOW, FALSE);

    ::SetWindowPos(m_Window, nullptr, 0, 0, newRect.right, newRect.bottom, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER);
#endif
    (void) SDL_SetWindowSize(
        m_SdlWindow,
        static_cast<int>(width),
        static_cast<int>(height)
    );
}

}
