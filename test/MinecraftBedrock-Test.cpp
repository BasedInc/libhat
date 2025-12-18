#include <Windows.h>
#include <iostream>
#include <chrono>
#include <libhat/scanner.hpp>
#include <string>
#include <vector>

// Helper to find the main window of the current process
struct EnumData {
    DWORD dwProcessId;
    HWND hWnd;
};

BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam) {
    EnumData& data = *reinterpret_cast<EnumData*>(lParam);
    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hWnd, &dwProcessId);
    if (data.dwProcessId == dwProcessId && GetWindow(hWnd, GW_OWNER) == 0 && IsWindowVisible(hWnd)) {
        data.hWnd = hWnd;
        return FALSE; // Stop enumerating
    }
    return TRUE;
}

HWND GetProcessWindow() {
    EnumData data = { GetCurrentProcessId(), 0 };
    EnumWindows(EnumProc, reinterpret_cast<LPARAM>(&data));
    return data.hWnd;
}

// Main thread function
DWORD WINAPI MainThread(LPVOID lpParam) {
    // 1. Pop open the console so we can see what's cookin'
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);
    
    std::cout << "=== Minecraft Bedrock Test DLL Injected ===" << std::endl;

    // 2. Who are we? Where are we?
    HMODULE hModule = GetModuleHandle(nullptr); // Get base address of the executable
    char modulePath[MAX_PATH];
    GetModuleFileNameA(hModule, modulePath, MAX_PATH);
    std::string moduleName = std::string(modulePath);
    size_t lastSlash = moduleName.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        moduleName = moduleName.substr(lastSlash + 1);
    }

    std::cout << "Module Name:    " << moduleName << std::endl;
    std::cout << "Module Address: 0x" << std::hex << reinterpret_cast<uintptr_t>(hModule) << std::dec << std::endl;

    // 3. Stalk the window
    HWND hWnd = GetProcessWindow();
    char windowTitle[256] = "Unknown";
    if (hWnd) {
        GetWindowTextA(hWnd, windowTitle, sizeof(windowTitle));
    }
    std::cout << "Window Name:    " << windowTitle << std::endl;

    // 4. Hunting for the goods
    // Signature: "45 32 F6 EB 0A"
    std::cout << "\nScanning for signature: 45 32 F6 EB 0A" << std::endl;

    auto sig_opt = hat::parse_signature("45 32 F6 EB 0A");
    if (!sig_opt.has_value()) {
        std::cerr << "Failed to parse signature! Rip." << std::endl;
    } else {
        auto signature = sig_opt.value();
        
        // Get module bounds for scanning
        // libhat has a helper for module scanning.
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Using the shiny new scanner class we just made
        hat::scanner scanner(signature);
        const auto module_data = hat::process::get_process_module().get_module_data();
        auto result = scanner.find(module_data.begin(), module_data.end());
        
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end_time - start_time;

        if (result.has_result()) {
            std::cout << "Found Signature Address: 0x" << std::hex << reinterpret_cast<uintptr_t>(result.get()) << std::dec << std::endl;
        } else {
            std::cout << "Signature NOT found. Sadge." << std::endl;
        }
        std::cout << "Time Taken: " << elapsed.count() << " ms" << std::endl;
    }

    std::cout << "\nPress ENTER to yeet this DLL..." << std::endl;
    std::cin.get();

    // Cleanup time
    if (f) fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), 0);
    return 0;
}

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
