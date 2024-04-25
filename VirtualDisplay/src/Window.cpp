#include "vd/Window.hpp"

#include <ConPrinter.hpp>

namespace tau::vd {

static constexpr const wchar_t WindowClassName[] = L"SoftGpu Virtual Display Window";

Window::~Window() noexcept
{
    ReleaseDC(m_Window, m_hDc);
    DestroyWindow(m_Window);
    UnregisterClassW(WindowClassName, GetModuleHandleW(nullptr));
}

void Window::PollMessages() const noexcept
{
    MSG msg;
    while(PeekMessageW(&msg, m_Window, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void Window::Close() noexcept
{
    m_ShouldClose = true;
}

ReferenceCountingPointer<Window> Window::CreateWindow() noexcept
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
        WS_OVERLAPPEDWINDOW,
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
    newRect.right = 800;
    newRect.bottom = 600;
    //
    // (void) AdjustWindowRect(&newRect, WS_OVERLAPPEDWINDOW, FALSE);
    (void) SetWindowPos(hWnd, nullptr, newRect.left, newRect.top, newRect.right, newRect.bottom, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER);

    GetClientRect(hWnd, &window->m_FramebufferSize);

    window->m_HorizontalSlop = newRect.right - window->m_FramebufferSize.right;
    window->m_VerticalSlop = newRect.bottom - window->m_FramebufferSize.bottom;

    return window;
}

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

void Window::ShowWindow() noexcept
{
    ::ShowWindow(m_Window, SW_SHOWNA);
}

void Window::HideWindow() noexcept
{
    ::ShowWindow(m_Window, SW_HIDE);
}

void Window::SetSize(const u32 width, const u32 height) noexcept
{
    RECT newRect;
    newRect.left = 0;
    newRect.top = 0;
    // newRect.right = static_cast<LONG>(width) + 8;
    // newRect.bottom = static_cast<LONG>(height) + 31;
    newRect.right = static_cast<LONG>(width + m_HorizontalSlop);
    newRect.bottom = static_cast<LONG>(height + m_VerticalSlop);

    // (void) AdjustWindowRect(&newRect, WS_OVERLAPPEDWINDOW, FALSE);

    ::SetWindowPos(m_Window, nullptr, 0, 0, newRect.right, newRect.bottom, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER);
}

}
