#include "overlay.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

Overlay::~Overlay() {
    Destroy();
}

bool Overlay::Create(const wchar_t* windowTitle, uint32_t width, uint32_t height) {
    m_Width = width;
    m_Height = height;

    if (!CreateOverlayWindow(width, height))
        return false;

    if (!CreateDeviceD3D())
        return false;

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(m_Window);
    ImGui_ImplDX11_Init(m_Device, m_Context);

    m_Running = true;
    return true;
}

void Overlay::Destroy() {
    m_Running = false;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    DestroyDeviceD3D();

    if (m_Window) {
        DestroyWindow(m_Window);
        m_Window = nullptr;
    }
}

bool Overlay::BeginFrame() {
    if (!m_Running) return false;

    // Process window messages
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
            m_Running = false;
    }

    if (!m_Running)
        return false;

    // Check if window was resized (e.g. after alt-tab)
    RECT cr;
    if (GetClientRect(m_Window, &cr)) {
        uint32_t newW = cr.right - cr.left;
        uint32_t newH = cr.bottom - cr.top;
        if (newW != m_Width || newH != m_Height) {
            m_Width = newW;
            m_Height = newH;
            if (!ResizeSwapChain(m_Width, m_Height))
                return false;
        }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    return true;
}

void Overlay::EndFrame() {
    ImGui::Render();

    FLOAT clearColor[4] = { 0.f, 0.f, 0.f, 0.f };
    m_Context->OMSetRenderTargets(1, &m_RenderTargetView, nullptr);
    m_Context->ClearRenderTargetView(m_RenderTargetView, clearColor);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HRESULT hr = m_SwapChain->Present(1, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        // Device lost - recreate
        m_Running = false;
    }
}

bool Overlay::CreateOverlayWindow(uint32_t width, uint32_t height) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"RoNESPPOverlayWindow";

    RegisterClassExW(&wc);

    // Find game window
    m_GameWindow = FindWindowW(nullptr, L"Ready Or Not");
    if (!m_GameWindow) {
        m_GameWindow = FindWindowW(L"UnrealWindow", L"Ready Or Not  ");
    }

    RECT gameRect = { 0, 0, (LONG)width, (LONG)height };
    if (m_GameWindow) {
        GetClientRect(m_GameWindow, &gameRect);
        m_Width = gameRect.right - gameRect.left;
        m_Height = gameRect.bottom - gameRect.top;
    }

    m_Window = CreateWindowExW(
        WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        wc.lpszClassName,
        L"RoN ESP",
        WS_POPUP,
        gameRect.left, gameRect.top,
        m_Width, m_Height,
        nullptr, nullptr,
        wc.hInstance, nullptr
    );

    if (!m_Window)
        return false;

    // Attach as child of game window
    if (m_GameWindow) {
        SetParent(m_Window, m_GameWindow);
    }

    // Transparent via colorkey
    SetLayeredWindowAttributes(m_Window, RGB(0, 0, 0), 0, LWA_COLORKEY);

    SetWindowPos(m_Window, HWND_TOP, 0, 0, m_Width, m_Height, SWP_NOACTIVATE);

    ShowWindow(m_Window, SW_SHOW);
    UpdateWindow(m_Window);

    return true;
}

bool Overlay::CreateDeviceD3D() {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_Window;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, nullptr, 0, D3D11_SDK_VERSION,
        &sd, &m_SwapChain, &m_Device, &featureLevel, &m_Context
    );

    if (FAILED(hr))
        return false;

    ID3D11Texture2D* backBuffer = nullptr;
    hr = m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr))
        return false;

    hr = m_Device->CreateRenderTargetView(backBuffer, nullptr, &m_RenderTargetView);
    backBuffer->Release();

    return SUCCEEDED(hr);
}

void Overlay::DestroyDeviceD3D() {
    if (m_RenderTargetView) { m_RenderTargetView->Release(); m_RenderTargetView = nullptr; }
    if (m_SwapChain) { m_SwapChain->Release(); m_SwapChain = nullptr; }
    if (m_Context) { m_Context->Release(); m_Context = nullptr; }
    if (m_Device) { m_Device->Release(); m_Device = nullptr; }
}

void Overlay::EnableInput(bool enable) {
    if (enable) {
        SetWindowLongPtrW(m_Window, GWL_EXSTYLE,
            GetWindowLongPtrW(m_Window, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
        // Release cursor clip so the user can reach the menu
        ClipCursor(nullptr);
    } else {
        SetWindowLongPtrW(m_Window, GWL_EXSTYLE,
            GetWindowLongPtrW(m_Window, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
    }
}

bool Overlay::ResizeSwapChain(uint32_t width, uint32_t height) {
    if (!m_SwapChain || !m_Device)
        return false;

    // Release old RTV
    if (m_RenderTargetView) {
        m_RenderTargetView->Release();
        m_RenderTargetView = nullptr;
    }

    // Resize swap chain buffers
    HRESULT hr = m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
        return false;

    // Recreate RTV
    ID3D11Texture2D* backBuffer = nullptr;
    hr = m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr))
        return false;

    hr = m_Device->CreateRenderTargetView(backBuffer, nullptr, &m_RenderTargetView);
    backBuffer->Release();

    return SUCCEEDED(hr);
}

LRESULT CALLBACK Overlay::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
