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
        , m_hDc(nullptr)
        , m_FramebufferSize{}
    { }

    ~Window() noexcept;

    [[nodiscard]] WNDCLASSEXW WindowClass() const noexcept { return m_WindowClass; }
    [[nodiscard]] HINSTANCE ModuleInstance() const noexcept { return m_WindowClass.hInstance; }
    [[nodiscard]] HWND WindowHandle() const noexcept { return m_Window; }
    [[nodiscard]] HDC DC() const noexcept { return m_hDc; }
    [[nodiscard]] const RECT& FramebufferSize() const noexcept { return m_FramebufferSize; }
    [[nodiscard]] u32 FramebufferWidth() const noexcept { return static_cast<u32>(m_FramebufferSize.right); }
    [[nodiscard]] u32 FramebufferHeight() const noexcept { return static_cast<u32>(m_FramebufferSize.bottom); }
private:
    LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
    static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
    WNDCLASSEXW m_WindowClass;
    HWND m_Window;
    HDC m_hDc;
    RECT m_FramebufferSize;
};

}
