#include <Windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <imgui.h>

#include "memory.h"
#include "game.h"
#include "overlay.h"
#include "esp.h"

int main() {
    printf("[+] RoN ESP starting...\n");

    Memory mem;
    if (!mem.FindProcess(L"ReadyOrNotSteam-Win64-Shipping.exe")) {
        printf("[-] Could not find Ready Or Not process. Is the game running?\n");
        printf("[*] Waiting for process...\n");

        int attempts = 0;
        while (!mem.FindProcess(L"ReadyOrNotSteam-Win64-Shipping.exe")) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (++attempts > 30) {
                printf("[-] Timed out waiting for game. Exiting.\n");
                return 1;
            }
        }
    }
    printf("[+] Attached to process. PID: %d\n", mem.ProcessId());

    uintptr_t moduleBase = mem.GetModuleBase(L"ReadyOrNotSteam-Win64-Shipping.exe");
    printf("[+] Module base: 0x%llX\n", moduleBase);

    GameReader reader(mem);
    GameState state{};

    Overlay overlay;
    if (!overlay.Create(L"RoN ESP", 1920, 1080)) {
        printf("[-] Failed to create overlay\n");
        return 1;
    }
    printf("[+] Overlay created. Window: 0x%llX (%dx%d)\n",
        (uintptr_t)overlay.WindowHandle(), overlay.Width(), overlay.Height());

    ESP esp;
    EspConfig cfg;

    state.camera.screenWidth = overlay.Width();
    state.camera.screenHeight = overlay.Height();

    printf("[+] Running. Press END key to exit.\n");

    bool showMenu = false;

    while (overlay.IsRunning()) {
        if (GetAsyncKeyState(VK_END) & 1) {
            break;
        }

        if (GetAsyncKeyState(VK_INSERT) & 1) {
            showMenu = !showMenu;
            overlay.EnableInput(showMenu);
        }

            if (GetAsyncKeyState(VK_F5) & 1) {
            reader.DumpEntityDiagnostic(state);
        }

        reader.Update(state);

        state.camera.screenWidth = overlay.Width();
        state.camera.screenHeight = overlay.Height();

        if (!overlay.BeginFrame())
            continue;

        if (state.camera.screenWidth > 0 && state.camera.screenHeight > 0) {
            esp.Draw(state, cfg);
        }

            if (showMenu) {
            ImGui::Begin("RoN ESP", &showMenu, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Checkbox("Boxes", &cfg.showBoxes);
            ImGui::SameLine();
            ImGui::Checkbox("Tracers", &cfg.showTracers);
            ImGui::Checkbox("Names", &cfg.showNames);
            ImGui::SameLine();
            ImGui::Checkbox("Health", &cfg.showHealth);
            ImGui::SameLine();
            ImGui::Checkbox("Distance", &cfg.showDist);
            ImGui::Checkbox("Dead", &cfg.showDead);
            ImGui::SameLine();
            ImGui::Checkbox("Crosshair", &cfg.showCrosshair);
            ImGui::Separator();
            ImGui::Text("Entities: %zu", state.entities.size());
            ImGui::Text("Game world: 0x%llX", state.worldPtr);
            ImGui::Separator();
            ImGui::Text("FOV: %.1f", state.camera.fov);
            ImGui::Text("Cam pos: %.1f, %.1f, %.1f",
                state.camera.location.x, state.camera.location.y, state.camera.location.z);
            ImGui::Separator();
            ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
            ImGui::Text("END to exit | INS to toggle menu");
            ImGui::End();
        }

        overlay.EndFrame();
    }

    printf("[+] Shutting down...\n");
    overlay.Destroy();
    return 0;
}
