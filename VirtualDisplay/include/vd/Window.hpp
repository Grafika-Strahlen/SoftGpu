#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Objects.hpp>
#include <ReferenceCountingPointer.hpp>

#undef CreateWindow

namespace tau::vd {

class Window final
{
    DELETE_CM(Window);
public:
    static ReferenceCountingPointer<Window> CreateWindow() noexcept;
public:
    void ShowWindow() noexcept;
    void HideWindow() noexcept;
public:
    Window(const WNDCLASSEXW& windowClass) noexcept
        : m_WindowClass(windowClass)
        , m_Window(nullptr)
    { }

    ~Window() noexcept;
private:
    LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
    static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
    WNDCLASSEXW m_WindowClass;
    HWND m_Window;
    HDC m_hDc;
};

}
