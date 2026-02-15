// ================================================================
// SWG Furniture Mover v5.0 - Numpad Controls
// x64 Discord-RPC Proxy DLL
//
// Installation: Copy as discord-rpc.dll to SWG Restoration\x64\
//               (rename original to discord-rpc-real.dll)
//
// Controls (target furniture first, then press F12 to enable):
//   F12            - Toggle decoration mode on/off (beep confirms)
//   Numpad 8       - Move forward
//   Numpad 2       - Move back
//   Numpad 4       - Move left
//   Numpad 6       - Move right
//   Numpad +       - Move up
//   Numpad -       - Move down
//   Numpad 7       - Rotate left (yaw)
//   Numpad 9       - Rotate right (yaw)
//   Numpad *       - Increase step size (1->2->5->10->25->50->100)
//   Numpad /       - Decrease step size
//   Hold Shift     - 5x multiplier
//   F11            - Show current status in chat
// ================================================================
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// ============================================================
// Discord-RPC Forwarding
// ============================================================
static HMODULE hReal = NULL;
static void* realFuncs[9] = {};
static const char* funcNames[] = {
    "Discord_ClearPresence","Discord_Initialize","Discord_Register",
    "Discord_RegisterSteamGame","Discord_Respond","Discord_RunCallbacks",
    "Discord_Shutdown","Discord_UpdateHandlers","Discord_UpdatePresence"
};
static void LoadReal() {
    if (hReal) return;
    hReal = LoadLibraryA("discord-rpc-real.dll");
    if (hReal) for (int i = 0; i < 9; i++) realFuncs[i] = (void*)GetProcAddress(hReal, funcNames[i]);
}
extern "C" {
__declspec(dllexport) void Discord_ClearPresence() { LoadReal(); if(realFuncs[0]) ((void(*)())realFuncs[0])(); }
__declspec(dllexport) void Discord_Initialize(const char* a, void* b, int c, const char* d) { LoadReal(); if(realFuncs[1]) ((void(*)(const char*,void*,int,const char*))realFuncs[1])(a,b,c,d); }
__declspec(dllexport) void Discord_Register(const char* a, const char* b) { LoadReal(); if(realFuncs[2]) ((void(*)(const char*,const char*))realFuncs[2])(a,b); }
__declspec(dllexport) void Discord_RegisterSteamGame(const char* a, const char* b) { LoadReal(); if(realFuncs[3]) ((void(*)(const char*,const char*))realFuncs[3])(a,b); }
__declspec(dllexport) void Discord_Respond(const char* a, int b) { LoadReal(); if(realFuncs[4]) ((void(*)(const char*,int))realFuncs[4])(a,b); }
__declspec(dllexport) void Discord_RunCallbacks() { LoadReal(); if(realFuncs[5]) ((void(*)())realFuncs[5])(); }
__declspec(dllexport) void Discord_Shutdown() { LoadReal(); if(realFuncs[6]) ((void(*)())realFuncs[6])(); }
__declspec(dllexport) void Discord_UpdateHandlers(void* a) { LoadReal(); if(realFuncs[7]) ((void(*)(void*))realFuncs[7])(a); }
__declspec(dllexport) void Discord_UpdatePresence(const void* a) { LoadReal(); if(realFuncs[8]) ((void(*)(const void*))realFuncs[8])(a); }
}

// ============================================================
// Plugin State
// ============================================================
static volatile bool g_decoMode = false;
static volatile bool g_running = true;
static int g_moveStep = 1;
static int g_rotateStep = 5;

// ============================================================
// SendInput Command Execution
// ============================================================
static void pressKey(WORD vk) {
    INPUT inp[2] = {};
    inp[0].type = INPUT_KEYBOARD;
    inp[0].ki.wVk = vk;
    inp[0].ki.wScan = (WORD)MapVirtualKeyA(vk, 0);
    inp[1].type = INPUT_KEYBOARD;
    inp[1].ki.wVk = vk;
    inp[1].ki.wScan = inp[0].ki.wScan;
    inp[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inp, sizeof(INPUT));
}

static void typeUnicodeChar(wchar_t ch) {
    INPUT inp[2] = {};
    inp[0].type = INPUT_KEYBOARD;
    inp[0].ki.wScan = ch;
    inp[0].ki.dwFlags = KEYEVENTF_UNICODE;
    inp[1].type = INPUT_KEYBOARD;
    inp[1].ki.wScan = ch;
    inp[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    SendInput(2, inp, sizeof(INPUT));
}

static void executeChatCommand(const char* command) {
    pressKey(VK_RETURN);
    Sleep(100);
    for (int i = 0; command[i]; i++)
        typeUnicodeChar((wchar_t)(unsigned char)command[i]);
    Sleep(50);
    pressKey(VK_RETURN);
}

static void sendMove(const char* direction, int amount) {
    if (amount < 1) amount = 1;
    if (amount > 500) amount = 500;
    char cmd[128];
    wsprintfA(cmd, "/moveFurniture %s %d", direction, amount);
    executeChatCommand(cmd);
}

static void sendRotate(const char* axis, int degrees) {
    char cmd[128];
    wsprintfA(cmd, "/rotateFurniture %s %d", axis, degrees);
    executeChatCommand(cmd);
}

// ============================================================
// Keyboard Polling Thread
// ============================================================
static DWORD WINAPI KeyboardPollThread(LPVOID) {
    bool prevKeys[256] = {};
    DWORD ourPid = GetCurrentProcessId();

    while (g_running) {
        Sleep(50);

        HWND fgWnd = GetForegroundWindow();
        if (!fgWnd) continue;
        DWORD fgPid = 0;
        GetWindowThreadProcessId(fgWnd, &fgPid);
        if (fgPid != ourPid) continue;

        bool keys[256] = {};
        for (int i = 0; i < 256; i++)
            keys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;

        // F11: Status
        if (keys[VK_F11] && !prevKeys[VK_F11]) {
            char msg[128];
            wsprintfA(msg, "/echo [Deco] Mode: %s | Step: %d | RotStep: %d",
                g_decoMode ? "ON" : "OFF", g_moveStep, g_rotateStep);
            executeChatCommand(msg);
        }

        // F12: Toggle decoration mode
        if (keys[VK_F12] && !prevKeys[VK_F12]) {
            g_decoMode = !g_decoMode;
            MessageBeep(g_decoMode ? MB_OK : MB_ICONEXCLAMATION);
        }

        if (g_decoMode) {
            bool shift = keys[VK_SHIFT];
            int step = shift ? g_moveStep * 5 : g_moveStep;
            if (step > 500) step = 500;
            int rotStep = shift ? g_rotateStep * 5 : g_rotateStep;

            // Movement
            if (keys[VK_NUMPAD8] && !prevKeys[VK_NUMPAD8]) sendMove("forward", step);
            if (keys[VK_NUMPAD2] && !prevKeys[VK_NUMPAD2]) sendMove("back", step);
            if (keys[VK_NUMPAD4] && !prevKeys[VK_NUMPAD4]) sendMove("left", step);
            if (keys[VK_NUMPAD6] && !prevKeys[VK_NUMPAD6]) sendMove("right", step);
            if (keys[VK_ADD]     && !prevKeys[VK_ADD])      sendMove("up", step);
            if (keys[VK_SUBTRACT]&& !prevKeys[VK_SUBTRACT]) sendMove("down", step);

            // Rotation
            if (keys[VK_NUMPAD7] && !prevKeys[VK_NUMPAD7]) sendRotate("yaw", -rotStep);
            if (keys[VK_NUMPAD9] && !prevKeys[VK_NUMPAD9]) sendRotate("yaw", rotStep);

            // Step size
            if (keys[VK_MULTIPLY] && !prevKeys[VK_MULTIPLY]) {
                if (g_moveStep < 2) g_moveStep = 2;
                else if (g_moveStep < 5) g_moveStep = 5;
                else if (g_moveStep < 10) g_moveStep = 10;
                else if (g_moveStep < 25) g_moveStep = 25;
                else if (g_moveStep < 50) g_moveStep = 50;
                else if (g_moveStep < 100) g_moveStep = 100;
                MessageBeep(MB_OK);
            }
            if (keys[VK_DIVIDE] && !prevKeys[VK_DIVIDE]) {
                if (g_moveStep > 50) g_moveStep = 50;
                else if (g_moveStep > 25) g_moveStep = 25;
                else if (g_moveStep > 10) g_moveStep = 10;
                else if (g_moveStep > 5) g_moveStep = 5;
                else if (g_moveStep > 2) g_moveStep = 2;
                else g_moveStep = 1;
                MessageBeep(MB_OK);
            }
        }

        memcpy(prevKeys, keys, sizeof(prevKeys));
    }
    return 0;
}

// ============================================================
// Plugin Init Thread
// ============================================================
static DWORD WINAPI PluginThread(LPVOID) {
    // Wait for game window
    for (int i = 0; i < 120; i++) {
        Sleep(500);
        HWND fgWnd = GetForegroundWindow();
        if (fgWnd) {
            DWORD fgPid;
            GetWindowThreadProcessId(fgWnd, &fgPid);
            if (fgPid == GetCurrentProcessId()) break;
        }
    }

    Sleep(3000);
    CreateThread(NULL, 0, KeyboardPollThread, NULL, 0, NULL);
    return 0;
}

// ============================================================
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, PluginThread, NULL, 0, NULL);
    }
    if (reason == DLL_PROCESS_DETACH) {
        g_running = false;
        if (hReal) FreeLibrary(hReal);
    }
    return TRUE;
}
