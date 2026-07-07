#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <cstdint>

// Undef Windows macros that conflict with method names
#ifdef CreateWindow
#undef CreateWindow
#endif

class Overlay {
public:
    Overlay() = default;
    ~Overlay();

    bool Create(const wchar_t* windowTitle, uint32_t width, uint32_t height);
    void Destroy();

    bool BeginFrame();
    void EndFrame();

    bool IsRunning() const { return m_Running; }
    HWND WindowHandle() const { return m_Window; }
    uint32_t Width() const { return m_Width; }
    uint32_t Height() const { return m_Height; }
    void EnableInput(bool enable);

private:
    bool CreateOverlayWindow(uint32_t width, uint32_t height);
    bool CreateDeviceD3D();
    void DestroyDeviceD3D();
    bool ResizeSwapChain(uint32_t width, uint32_t height);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_Window = nullptr;
    HWND m_GameWindow = nullptr;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    bool m_Running = false;

    // DirectX
    ID3D11Device* m_Device = nullptr;
    ID3D11DeviceContext* m_Context = nullptr;
    IDXGISwapChain* m_SwapChain = nullptr;
    ID3D11RenderTargetView* m_RenderTargetView = nullptr;
};

// Forward declare ImGui WndProc handler
LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
