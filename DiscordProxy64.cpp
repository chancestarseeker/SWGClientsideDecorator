// ================================================================
// SWG Decoration Plugin v3.5 - Crash-safe World-Anchored Gizmo
// x64 Discord-RPC Proxy + Keyboard & Mouse Furniture Decorator
//
// Installation: Copy as discord-rpc.dll to SWG Restoration\x64\
//               (rename original to discord-rpc-real.dll)
//
// Controls:
//   F12            - Toggle decoration mode (beep confirms)
//                    Locks gizmo onto furniture closest to screen center
//   Numpad 8/2     - Move furniture forward/back
//   Numpad 4/6     - Move furniture left/right
//   Numpad +/-     - Move furniture up/down
//   Numpad 7/9     - Rotate furniture left/right
//   Numpad 1/3     - Rotate furniture pitch down/up
//   Numpad *       - Increase move step (1->2->5->10->25->50->100)
//   Numpad /       - Decrease move step
//   Hold Shift     - 5x movement/rotation multiplier
//   Numpad 0       - Toggle translate/rotate gizmo tool
//
// Gizmo Mouse Controls (deferred commit):
//   Alt + Mouse    - Free cursor mode: hover gizmo axes to highlight
//   Middle-click   - Lock/unlock hovered axis (drag to accumulate movement)
//   Right-click    - Cancel current axis drag
//   Numpad 5       - COMMIT: select furniture first, then press to apply
//   Numpad .       - Clear all pending movement
//   Escape         - Cancel current axis drag
//
// Workflow: Look at furniture > F12 > gizmo appears on object >
//           hover axis > middle-click to lock > drag >
//           middle-click to unlock > repeat > select furniture >
//           Numpad 5 to commit all movement
//
// Requires: Target a piece of furniture before using numpad keys
// ================================================================
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

// MinHook for D3D9 EndScene hooking
#include "MinHook.h"

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
// D3D9 Type Definitions (no DirectX SDK needed)
// ============================================================
struct IDirect3D9;
struct IDirect3DDevice9;

#define D3D_SDK_VERSION_CONST    32
#define D3DADAPTER_DEFAULT       0
#define D3DDEVTYPE_HAL           1
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0x00000020L
#define D3DSWAPEFFECT_DISCARD    1
#define D3DFMT_UNKNOWN           0

// D3D9 vtable indices
#define D3D9_VTABLE_ENDSCENE       42
#define D3D9_VTABLE_RESET          16
#define D3D9_VTABLE_SETTRANSFORM   44
#define D3D9_VTABLE_GETTRANSFORM   45
#define D3D9_VTABLE_SETVIEWPORT    47
#define D3D9_VTABLE_GETVIEWPORT    48
#define D3D9_VTABLE_SETRENDERSTATE 57
#define D3D9_VTABLE_SETTEXTURE     65
#define D3D9_VTABLE_SETFVF         89
#define D3D9_VTABLE_DRAWPRIMITIVEUP 83
#define D3D9_VTABLE_GETRENDERSTATE 58
#define D3D9_VTABLE_GETTEXTURE     64
#define D3D9_VTABLE_GETFVF         90

// D3D render state / transform enums
#define D3DRS_ZENABLE            7
#define D3DRS_LIGHTING           137
#define D3DRS_CULLMODE           22
#define D3DRS_ALPHABLENDENABLE   27
#define D3DRS_SRCBLEND           19
#define D3DRS_DESTBLEND          20
#define D3DRS_ZWRITEENABLE       14
#define D3DRS_FILLMODE           8
#define D3DRS_ANTIALIASEDLINEENABLE 176
#define D3DCULL_NONE             1
#define D3DFILL_SOLID            3
#define D3DBLEND_SRCALPHA        5
#define D3DBLEND_INVSRCALPHA     6
#define D3DTS_VIEW               2
#define D3DTS_PROJECTION         3
#define D3DTS_WORLD              256
#define D3DPT_LINELIST           2
#define D3DPT_TRIANGLELIST       4
#define D3DPT_TRIANGLEFAN        6
#define D3DFVF_XYZ               0x002
#define D3DFVF_DIFFUSE           0x040
#define D3DFVF_XYZRHW            0x004

struct D3DMATRIX_MINIMAL { float m[4][4]; };
struct D3DVIEWPORT9_MINIMAL { DWORD X, Y, Width, Height; float MinZ, MaxZ; };

struct D3DPRESENT_PARAMETERS_MINIMAL {
    UINT BackBufferWidth, BackBufferHeight, BackBufferFormat, BackBufferCount;
    UINT MultiSampleType; DWORD MultiSampleQuality;
    UINT SwapEffect; HWND hDeviceWindow; BOOL Windowed;
    BOOL EnableAutoDepthStencil; UINT AutoDepthStencilFormat;
    DWORD Flags; UINT FullScreen_RefreshRateInHz; UINT PresentationInterval;
};

// COM function pointer types (stdcall is ignored on x64, that's fine)
typedef long(__stdcall* tEndScene)(IDirect3DDevice9*);
typedef long(__stdcall* tReset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS_MINIMAL*);
typedef long(__stdcall* tSetTransform)(IDirect3DDevice9*, int, const D3DMATRIX_MINIMAL*);
typedef long(__stdcall* tGetTransform)(IDirect3DDevice9*, int, D3DMATRIX_MINIMAL*);
typedef long(__stdcall* tSetViewport)(IDirect3DDevice9*, const D3DVIEWPORT9_MINIMAL*);
typedef long(__stdcall* tGetViewport)(IDirect3DDevice9*, D3DVIEWPORT9_MINIMAL*);
typedef long(__stdcall* tSetRenderState)(IDirect3DDevice9*, int, DWORD);
typedef long(__stdcall* tGetRenderState)(IDirect3DDevice9*, int, DWORD*);
typedef long(__stdcall* tSetTexture)(IDirect3DDevice9*, DWORD, void*);
typedef long(__stdcall* tGetTexture)(IDirect3DDevice9*, DWORD, void**);
typedef long(__stdcall* tSetFVF)(IDirect3DDevice9*, DWORD);
typedef long(__stdcall* tGetFVF)(IDirect3DDevice9*, DWORD*);
typedef long(__stdcall* tDrawPrimitiveUP)(IDirect3DDevice9*, int, UINT, const void*, UINT);

typedef IDirect3D9* (__stdcall* tDirect3DCreate9)(UINT);
typedef long(__stdcall* tCreateDevice)(IDirect3D9*, UINT, int, HWND, DWORD,
    D3DPRESENT_PARAMETERS_MINIMAL*, IDirect3DDevice9**);
typedef ULONG(__stdcall* tRelease)(void*);

// D3D9 vtable helpers
inline void** getD3D9Vtable(IDirect3DDevice9* device) {
    return *reinterpret_cast<void***>(device);
}
template<typename FuncType>
inline FuncType getD3D9Method(IDirect3DDevice9* device, int vtableIndex) {
    return reinterpret_cast<FuncType>(getD3D9Vtable(device)[vtableIndex]);
}

// ============================================================
// D3D9 Device Helper Wrappers
// ============================================================
static void d3dSetRenderState(IDirect3DDevice9* dev, int state, DWORD value) {
    getD3D9Method<tSetRenderState>(dev, D3D9_VTABLE_SETRENDERSTATE)(dev, state, value);
}
static DWORD d3dGetRenderState(IDirect3DDevice9* dev, int state) {
    DWORD value = 0;
    getD3D9Method<tGetRenderState>(dev, D3D9_VTABLE_GETRENDERSTATE)(dev, state, &value);
    return value;
}
static void d3dSetTexture(IDirect3DDevice9* dev, DWORD stage, void* tex) {
    getD3D9Method<tSetTexture>(dev, D3D9_VTABLE_SETTEXTURE)(dev, stage, tex);
}
static void* d3dGetTexture(IDirect3DDevice9* dev, DWORD stage) {
    void* tex = nullptr;
    getD3D9Method<tGetTexture>(dev, D3D9_VTABLE_GETTEXTURE)(dev, stage, &tex);
    return tex;
}
static void d3dSetFVF(IDirect3DDevice9* dev, DWORD fvf) {
    getD3D9Method<tSetFVF>(dev, D3D9_VTABLE_SETFVF)(dev, fvf);
}
static void d3dSetTransform(IDirect3DDevice9* dev, int state, const D3DMATRIX_MINIMAL* mat) {
    getD3D9Method<tSetTransform>(dev, D3D9_VTABLE_SETTRANSFORM)(dev, state, mat);
}
static void d3dGetTransform(IDirect3DDevice9* dev, int state, D3DMATRIX_MINIMAL* mat) {
    getD3D9Method<tGetTransform>(dev, D3D9_VTABLE_GETTRANSFORM)(dev, state, mat);
}
static void d3dGetViewport(IDirect3DDevice9* dev, D3DVIEWPORT9_MINIMAL* vp) {
    getD3D9Method<tGetViewport>(dev, D3D9_VTABLE_GETVIEWPORT)(dev, vp);
}
static void d3dDrawPrimitiveUP(IDirect3DDevice9* dev, int type, UINT count, const void* data, UINT stride) {
    getD3D9Method<tDrawPrimitiveUP>(dev, D3D9_VTABLE_DRAWPRIMITIVEUP)(dev, type, count, data, stride);
}

// ============================================================
// Vertex Types
// ============================================================
struct HudVertex { float x, y, z, rhw; DWORD color; };

#define HUD_FVF   (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

// Colors (ARGB)
#define COLOR_AXIS_X          0xFFFF2222
#define COLOR_AXIS_Y          0xFF22FF22
#define COLOR_AXIS_Z          0xFF4444FF
#define COLOR_AXIS_X_HOVER    0xFFFF8888
#define COLOR_AXIS_Y_HOVER    0xFF88FF88
#define COLOR_AXIS_Z_HOVER    0xFF8888FF
#define COLOR_AXIS_X_ACTIVE   0xFFFFFF44
#define COLOR_AXIS_Y_ACTIVE   0xFFFFFF44
#define COLOR_AXIS_Z_ACTIVE   0xFFFFFF44
#define COLOR_CENTER          0xFFFFFFFF
#define COLOR_WIREFRAME       0x88FFFFFF
#define COLOR_HUD_BG          0xAA000000
#define COLOR_HUD_BORDER      0xFF22FF22
#define COLOR_ROTATION_RING   0x88FFAA00

// ============================================================
// Matrix Math Helpers
// ============================================================
static void matrixIdentity(D3DMATRIX_MINIMAL& m) {
    memset(&m, 0, sizeof(m));
    m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
}

static void matrixMultiply(D3DMATRIX_MINIMAL& out,
    const D3DMATRIX_MINIMAL& a, const D3DMATRIX_MINIMAL& b) {
    D3DMATRIX_MINIMAL tmp;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            tmp.m[i][j] = a.m[i][0] * b.m[0][j]
                + a.m[i][1] * b.m[1][j]
                + a.m[i][2] * b.m[2][j]
                + a.m[i][3] * b.m[3][j];
        }
    }
    memcpy(&out, &tmp, sizeof(D3DMATRIX_MINIMAL));
}

// Project a world-space point through the combined ViewProjection matrix
// to screen coordinates. Returns false if point is behind camera.
static bool projectWorldToScreen(float wx, float wy, float wz,
    const D3DMATRIX_MINIMAL& viewProj, const D3DVIEWPORT9_MINIMAL& vp,
    float& sx, float& sy) {
    float clipX = wx * viewProj.m[0][0] + wy * viewProj.m[1][0] + wz * viewProj.m[2][0] + viewProj.m[3][0];
    float clipY = wx * viewProj.m[0][1] + wy * viewProj.m[1][1] + wz * viewProj.m[2][1] + viewProj.m[3][1];
    float clipW = wx * viewProj.m[0][3] + wy * viewProj.m[1][3] + wz * viewProj.m[2][3] + viewProj.m[3][3];

    if (fabsf(clipW) < 0.001f) return false;
    if (clipW < 0.0f) return false; // behind camera

    float ndcX = clipX / clipW;
    float ndcY = clipY / clipW;

    // Reject extreme NDC values (point is way off screen)
    if (fabsf(ndcX) > 10.0f || fabsf(ndcY) > 10.0f) return false;

    sx = (ndcX * 0.5f + 0.5f) * vp.Width + vp.X;
    sy = (-ndcY * 0.5f + 0.5f) * vp.Height + vp.Y;
    return true;
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// Plugin State
// ============================================================
static volatile bool g_decoMode = false;
static int g_moveStep = 1;
static int g_rotateStep = 5;
static volatile bool g_running = true;

// Gizmo state
enum GizmoAxis { AXIS_NONE = 0, AXIS_X, AXIS_Y, AXIS_Z };
enum GizmoTool { TOOL_TRANSLATE = 0, TOOL_ROTATE };
enum GizmoState { GIZMO_IDLE = 0, GIZMO_ACTIVE, GIZMO_DRAGGING };

static GizmoState g_gizmoState = GIZMO_IDLE;
static GizmoAxis g_hoveredAxis = AXIS_NONE;
static GizmoAxis g_selectedAxis = AXIS_NONE;
static GizmoTool g_gizmoTool = TOOL_TRANSLATE;
static int g_dragStartX = 0, g_dragStartY = 0;
static float g_dragAccumX = 0.0f, g_dragAccumY = 0.0f;
static DWORD g_lastCommandTime = 0;
static int g_mouseX = 0, g_mouseY = 0;

// Pending movement accumulator (deferred commit model)
static float g_pendingMoveX = 0.0f;   // left(-)/right(+)
static float g_pendingMoveY = 0.0f;   // down(-)/up(+)
static float g_pendingMoveZ = 0.0f;   // forward(-)/back(+)
static float g_pendingRotYaw = 0.0f;
static float g_pendingRotPitch = 0.0f;
static float g_pendingRotRoll = 0.0f;
static volatile bool g_commitPending = false;

// D3D9 hook state
static IDirect3DDevice9* g_device = nullptr;
static bool g_deviceLost = false;
static HWND g_gameHwnd = nullptr;
static tEndScene g_oEndScene = nullptr;
static tReset g_oReset = nullptr;
static bool g_hooksInstalled = false;
static int g_endSceneCallCount = 0;

// Gizmo position: anchored to a world-space point captured at F12 press time.
// Each frame, this world point is re-projected to screen using current VIEW/PROJ,
// so the gizmo stays on the furniture as camera orbits.
static bool g_gizmoAnchored = false;        // True once we have a world anchor
static float g_anchorWorldX = 0, g_anchorWorldY = 0, g_anchorWorldZ = 0; // World-space anchor
static bool g_anchorOnScreen = false;        // Whether anchor projects to visible screen area

// Diagnostic
static char g_diagStatus[512] = "Not started";
static int g_rawInputCount = 0;
static int g_clickCount = 0;
static int g_dragStartCount = 0;

// Gizmo screen-space layout
static float g_gizmoCenterX = 0.0f;
static float g_gizmoCenterY = 0.0f;
static float g_gizmoScale = 1.0f;

// Camera-projected axis screen directions (for hit testing and drag mapping)
// Updated each frame from VIEW/PROJ matrices — axes track camera orientation
static float g_axisScreenDirX[3] = { 1.0f, 0.0f, -0.707f };
static float g_axisScreenDirY[3] = { 0.0f, -1.0f, -0.707f };

// Screen-space gizmo arm length (in pixels, scales with resolution)
static float g_gizmoArmLen = 100.0f;

// Screen-space sizes (scaled for resolution)
static const float BASE_HIT_THRESHOLD = 14.0f;
static const float BASE_GIZMO_ARM = 100.0f;

#define GIZMO_HIT_THRESHOLD (BASE_HIT_THRESHOLD * g_gizmoScale)

// Drag sensitivity
static const float DRAG_SENSITIVITY_MOVE = 40.0f;
static const float DRAG_SENSITIVITY_ROT = 3.0f;
static const DWORD COMMAND_THROTTLE_MS = 250;

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
// Screen-space distance from point to line segment
// ============================================================
static float distToLine(float px, float py, float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    float lenSq = dx * dx + dy * dy;
    if (lenSq < 0.0001f)
        return sqrtf((px - x1) * (px - x1) + (py - y1) * (py - y1));
    float t = ((px - x1) * dx + (py - y1) * dy) / lenSq;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    float cx = x1 + t * dx, cy = y1 + t * dy;
    return sqrtf((px - cx) * (px - cx) + (py - cy) * (py - cy));
}

// ============================================================
// Get color for an axis considering hover/selection state
// ============================================================
static DWORD getAxisColor(GizmoAxis axis) {
    if (g_selectedAxis == axis) {
        switch (axis) {
        case AXIS_X: return COLOR_AXIS_X_ACTIVE;
        case AXIS_Y: return COLOR_AXIS_Y_ACTIVE;
        case AXIS_Z: return COLOR_AXIS_Z_ACTIVE;
        default: break;
        }
    }
    if (g_hoveredAxis == axis) {
        switch (axis) {
        case AXIS_X: return COLOR_AXIS_X_HOVER;
        case AXIS_Y: return COLOR_AXIS_Y_HOVER;
        case AXIS_Z: return COLOR_AXIS_Z_HOVER;
        default: break;
        }
    }
    switch (axis) {
    case AXIS_X: return COLOR_AXIS_X;
    case AXIS_Y: return COLOR_AXIS_Y;
    case AXIS_Z: return COLOR_AXIS_Z;
    default: return COLOR_CENTER;
    }
}

// ============================================================
// Update hover test using projected screen-space positions
// The gizmo center and axis endpoints are projected from world
// space each frame, stored in g_gizmoCenterX/Y and g_axisScreenDirX/Y
// ============================================================
static void updateGizmoHover(int mouseX, int mouseY) {
    if (g_gizmoState == GIZMO_DRAGGING) return;
    // Skip hover if we're already dragging

    float mx = (float)mouseX, my = (float)mouseY;
    float cx = g_gizmoCenterX, cy = g_gizmoCenterY;

    float bestDist = 9999.0f;
    GizmoAxis bestAxis = AXIS_NONE;

    for (int a = AXIS_X; a <= AXIS_Z; a++) {
        int idx = a - 1;
        float ex = cx + g_axisScreenDirX[idx];
        float ey = cy + g_axisScreenDirY[idx];

        float dist;
        if (g_gizmoTool == TOOL_TRANSLATE) {
            dist = distToLine(mx, my, cx, cy, ex, ey);
        } else {
            // For rotation: check distance to a ring arc centered on this axis direction
            float ringRadius = sqrtf(g_axisScreenDirX[idx] * g_axisScreenDirX[idx] +
                                     g_axisScreenDirY[idx] * g_axisScreenDirY[idx]) * 0.8f;
            if (ringRadius < 20.0f) ringRadius = 20.0f;
            float dFromCenter = sqrtf((mx - cx) * (mx - cx) + (my - cy) * (my - cy));
            dist = fabsf(dFromCenter - ringRadius);
            // Only match if mouse angle is near this axis's direction
            float axisAngle = atan2f(-g_axisScreenDirY[idx], g_axisScreenDirX[idx]);
            float mouseAngle = atan2f(-(my - cy), mx - cx);
            float angleDiff = mouseAngle - axisAngle;
            while (angleDiff > (float)M_PI) angleDiff -= 2.0f * (float)M_PI;
            while (angleDiff < -(float)M_PI) angleDiff += 2.0f * (float)M_PI;
            if (fabsf(angleDiff) > 0.785f) dist = 9999.0f;
        }

        if (dist < bestDist && dist < GIZMO_HIT_THRESHOLD) {
            bestDist = dist;
            bestAxis = (GizmoAxis)a;
        }
    }

    g_hoveredAxis = bestAxis;
}

// ============================================================
// Render screen-space translate gizmo at screen center
// Axes follow camera orientation (projected from VIEW matrix)
// ============================================================
static void renderTranslateGizmo(IDirect3DDevice9* dev) {
    float cx = g_gizmoCenterX, cy = g_gizmoCenterY;

    // Draw 3 axis lines from center, using camera-projected directions
    HudVertex lines[6];
    int idx = 0;
    for (int a = 0; a < 3; a++) {
        GizmoAxis axis = (GizmoAxis)(a + 1);
        DWORD col = getAxisColor(axis);
        float ex = cx + g_axisScreenDirX[a];
        float ey = cy + g_axisScreenDirY[a];
        lines[idx++] = { cx, cy, 0.0f, 1.0f, col };
        lines[idx++] = { ex, ey, 0.0f, 1.0f, col };
    }
    d3dSetFVF(dev, HUD_FVF);
    d3dDrawPrimitiveUP(dev, D3DPT_LINELIST, 3, lines, sizeof(HudVertex));

    // Arrowheads (screen-space triangles at the tip of each axis)
    for (int a = 0; a < 3; a++) {
        GizmoAxis axis = (GizmoAxis)(a + 1);
        DWORD col = getAxisColor(axis);
        float ex = cx + g_axisScreenDirX[a];
        float ey = cy + g_axisScreenDirY[a];

        // Direction of the axis on screen
        float dx = g_axisScreenDirX[a], dy = g_axisScreenDirY[a];
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 1.0f) continue;
        float ndx = dx / len, ndy = dy / len;
        // Perpendicular
        float px = -ndy, py = ndx;
        float arrowSz = 6.0f * g_gizmoScale;
        float arrowLen = 12.0f * g_gizmoScale;

        HudVertex arrow[3] = {
            { ex + ndx * arrowLen, ey + ndy * arrowLen, 0.0f, 1.0f, col },
            { ex + px * arrowSz,  ey + py * arrowSz,   0.0f, 1.0f, col },
            { ex - px * arrowSz,  ey - py * arrowSz,   0.0f, 1.0f, col },
        };
        d3dDrawPrimitiveUP(dev, D3DPT_TRIANGLELIST, 1, arrow, sizeof(HudVertex));
    }

    // Center dot
    float ds = 4.0f * g_gizmoScale;
    HudVertex centerDot[6] = {
        { cx - ds, cy - ds, 0.0f, 1.0f, COLOR_CENTER },
        { cx + ds, cy - ds, 0.0f, 1.0f, COLOR_CENTER },
        { cx - ds, cy + ds, 0.0f, 1.0f, COLOR_CENTER },
        { cx + ds, cy - ds, 0.0f, 1.0f, COLOR_CENTER },
        { cx + ds, cy + ds, 0.0f, 1.0f, COLOR_CENTER },
        { cx - ds, cy + ds, 0.0f, 1.0f, COLOR_CENTER },
    };
    d3dDrawPrimitiveUP(dev, D3DPT_TRIANGLELIST, 2, centerDot, sizeof(HudVertex));
}

// ============================================================
// Render screen-space rotation gizmo (3 arcs)
// ============================================================
static void renderRotateGizmo(IDirect3DDevice9* dev) {
    float cx = g_gizmoCenterX, cy = g_gizmoCenterY;
    int segs = 24;

    d3dSetFVF(dev, HUD_FVF);

    for (int a = 0; a < 3; a++) {
        GizmoAxis axis = (GizmoAxis)(a + 1);
        DWORD col = getAxisColor(axis);

        // Ring radius = ~80% of the axis arm length
        float ringRadius = sqrtf(g_axisScreenDirX[a] * g_axisScreenDirX[a] +
                                  g_axisScreenDirY[a] * g_axisScreenDirY[a]) * 0.8f;
        if (ringRadius < 20.0f) ringRadius = 20.0f;

        // Draw as screen-space arc centered on gizmo center
        // Arc direction follows the axis projection angle
        float axisAngle = atan2f(-g_axisScreenDirY[a], g_axisScreenDirX[a]);
        float arcStart = axisAngle - (float)M_PI * 0.45f;
        float arcEnd = axisAngle + (float)M_PI * 0.45f;

        HudVertex ring[48 * 2]; // segs line segments
        int vi = 0;
        for (int s = 0; s < segs; s++) {
            float t1 = arcStart + (arcEnd - arcStart) * s / segs;
            float t2 = arcStart + (arcEnd - arcStart) * (s + 1) / segs;
            ring[vi++] = { cx + cosf(t1) * ringRadius, cy - sinf(t1) * ringRadius, 0.0f, 1.0f, col };
            ring[vi++] = { cx + cosf(t2) * ringRadius, cy - sinf(t2) * ringRadius, 0.0f, 1.0f, col };
        }
        d3dDrawPrimitiveUP(dev, D3DPT_LINELIST, vi / 2, ring, sizeof(HudVertex));
    }
}

// Forward declarations
static bool hasPendingMovement();

// ============================================================
// Render HUD info box (always screen-space)
// ============================================================
static void renderHUD(IDirect3DDevice9* dev) {
    D3DVIEWPORT9_MINIMAL vp;
    d3dGetViewport(dev, &vp);

    float hudX = 10.0f * g_gizmoScale, hudY = 10.0f * g_gizmoScale;
    bool showPending = hasPendingMovement() || g_gizmoState == GIZMO_DRAGGING;
    float hudW = 260.0f * g_gizmoScale, hudH = (showPending ? 90.0f : 50.0f) * g_gizmoScale;

    HudVertex bg[6] = {
        { hudX,        hudY,        0.0f, 1.0f, COLOR_HUD_BG },
        { hudX + hudW, hudY,        0.0f, 1.0f, COLOR_HUD_BG },
        { hudX,        hudY + hudH, 0.0f, 1.0f, COLOR_HUD_BG },
        { hudX + hudW, hudY,        0.0f, 1.0f, COLOR_HUD_BG },
        { hudX + hudW, hudY + hudH, 0.0f, 1.0f, COLOR_HUD_BG },
        { hudX,        hudY + hudH, 0.0f, 1.0f, COLOR_HUD_BG },
    };
    d3dSetFVF(dev, HUD_FVF);
    d3dDrawPrimitiveUP(dev, D3DPT_TRIANGLELIST, 2, bg, sizeof(HudVertex));

    HudVertex border[8] = {
        { hudX,        hudY,        0.0f, 1.0f, COLOR_HUD_BORDER },
        { hudX + hudW, hudY,        0.0f, 1.0f, COLOR_HUD_BORDER },
        { hudX + hudW, hudY,        0.0f, 1.0f, COLOR_HUD_BORDER },
        { hudX + hudW, hudY + hudH, 0.0f, 1.0f, COLOR_HUD_BORDER },
        { hudX + hudW, hudY + hudH, 0.0f, 1.0f, COLOR_HUD_BORDER },
        { hudX,        hudY + hudH, 0.0f, 1.0f, COLOR_HUD_BORDER },
        { hudX,        hudY + hudH, 0.0f, 1.0f, COLOR_HUD_BORDER },
        { hudX,        hudY,        0.0f, 1.0f, COLOR_HUD_BORDER },
    };
    d3dDrawPrimitiveUP(dev, D3DPT_LINELIST, 4, border, sizeof(HudVertex));

    // Mode indicator + lock indicator
    DWORD modeColor = (g_gizmoTool == TOOL_TRANSLATE) ? 0xFF44FF44 : 0xFFFF8844;
    float ix = hudX + 8 * g_gizmoScale, iy = hudY + 8 * g_gizmoScale, isz = 12.0f * g_gizmoScale;
    HudVertex indicator[6] = {
        { ix,       iy,       0.0f, 1.0f, modeColor },
        { ix + isz, iy,       0.0f, 1.0f, modeColor },
        { ix,       iy + isz, 0.0f, 1.0f, modeColor },
        { ix + isz, iy,       0.0f, 1.0f, modeColor },
        { ix + isz, iy + isz, 0.0f, 1.0f, modeColor },
        { ix,       iy + isz, 0.0f, 1.0f, modeColor },
    };
    d3dDrawPrimitiveUP(dev, D3DPT_TRIANGLELIST, 2, indicator, sizeof(HudVertex));

    // Deco mode status indicator (always green since we only render when deco is on)
    DWORD lockCol = 0xFF44FF44;
    float lx = ix + isz + 4 * g_gizmoScale;
    HudVertex lockInd[6] = {
        { lx,       iy,       0.0f, 1.0f, lockCol },
        { lx + isz, iy,       0.0f, 1.0f, lockCol },
        { lx,       iy + isz, 0.0f, 1.0f, lockCol },
        { lx + isz, iy,       0.0f, 1.0f, lockCol },
        { lx + isz, iy + isz, 0.0f, 1.0f, lockCol },
        { lx,       iy + isz, 0.0f, 1.0f, lockCol },
    };
    d3dDrawPrimitiveUP(dev, D3DPT_TRIANGLELIST, 2, lockInd, sizeof(HudVertex));

    // Step size indicator
    float sx = hudX + 60 * g_gizmoScale, sy = hudY + 8 * g_gizmoScale;
    for (int i = 0; i < 7; i++) {
        int thresholds[] = { 1, 2, 5, 10, 25, 50, 100 };
        DWORD sc = (g_moveStep >= thresholds[i]) ? 0xFF88CCFF : 0xFF333333;
        float ssz = 6.0f * g_gizmoScale;
        HudVertex dot[6] = {
            { sx,       sy,       0.0f, 1.0f, sc },
            { sx + ssz, sy,       0.0f, 1.0f, sc },
            { sx,       sy + ssz, 0.0f, 1.0f, sc },
            { sx + ssz, sy,       0.0f, 1.0f, sc },
            { sx + ssz, sy + ssz, 0.0f, 1.0f, sc },
            { sx,       sy + ssz, 0.0f, 1.0f, sc },
        };
        d3dDrawPrimitiveUP(dev, D3DPT_TRIANGLELIST, 2, dot, sizeof(HudVertex));
        sx += 10.0f * g_gizmoScale;
    }

    // Pending movement bars
    if (showPending) {
        float barY = hudY + 30 * g_gizmoScale;
        float barH = 8.0f * g_gizmoScale;
        float barMaxW = hudW - 20 * g_gizmoScale;
        float barX = hudX + 10 * g_gizmoScale;
        float barSpacing = 12.0f * g_gizmoScale;

        struct PendingBar { float value; DWORD color; };
        PendingBar bars[3];
        if (g_gizmoTool == TOOL_TRANSLATE) {
            bars[0] = { g_pendingMoveX, COLOR_AXIS_X };
            bars[1] = { g_pendingMoveY, COLOR_AXIS_Y };
            bars[2] = { g_pendingMoveZ, COLOR_AXIS_Z };
        } else {
            bars[0] = { g_pendingRotPitch, COLOR_AXIS_X };
            bars[1] = { g_pendingRotYaw, COLOR_AXIS_Y };
            bars[2] = { g_pendingRotRoll, COLOR_AXIS_Z };
        }

        for (int i = 0; i < 3; i++) {
            float y = barY + i * barSpacing;
            float val = bars[i].value;
            float norm = val / 100.0f;
            if (norm > 1.0f) norm = 1.0f;
            if (norm < -1.0f) norm = -1.0f;
            float barW = fabsf(norm) * barMaxW * 0.5f;
            float startX = barX + barMaxW * 0.5f;

            if (barW > 1.0f) {
                float x1 = (val > 0) ? startX : startX - barW;
                HudVertex bar[6] = {
                    { x1,        y,        0.0f, 1.0f, bars[i].color },
                    { x1 + barW, y,        0.0f, 1.0f, bars[i].color },
                    { x1,        y + barH, 0.0f, 1.0f, bars[i].color },
                    { x1 + barW, y,        0.0f, 1.0f, bars[i].color },
                    { x1 + barW, y + barH, 0.0f, 1.0f, bars[i].color },
                    { x1,        y + barH, 0.0f, 1.0f, bars[i].color },
                };
                d3dDrawPrimitiveUP(dev, D3DPT_TRIANGLELIST, 2, bar, sizeof(HudVertex));
            }
            HudVertex centerLine[2] = {
                { startX, y - 1, 0.0f, 1.0f, 0x88FFFFFF },
                { startX, y + barH + 1, 0.0f, 1.0f, 0x88FFFFFF },
            };
            d3dDrawPrimitiveUP(dev, D3DPT_LINELIST, 1, centerLine, sizeof(HudVertex));
        }

        if (hasPendingMovement() && g_gizmoState != GIZMO_DRAGGING) {
            DWORD blinkCol = (GetTickCount() / 500 % 2 == 0) ? 0xFF44FF44 : 0xFF226622;
            float bx = hudX + hudW - 40 * g_gizmoScale, by = hudY + 6 * g_gizmoScale;
            float csz = 14.0f * g_gizmoScale;
            HudVertex commitBox[6] = {
                { bx,       by,       0.0f, 1.0f, blinkCol },
                { bx + csz, by,       0.0f, 1.0f, blinkCol },
                { bx,       by + csz, 0.0f, 1.0f, blinkCol },
                { bx + csz, by,       0.0f, 1.0f, blinkCol },
                { bx + csz, by + csz, 0.0f, 1.0f, blinkCol },
                { bx,       by + csz, 0.0f, 1.0f, blinkCol },
            };
            d3dDrawPrimitiveUP(dev, D3DPT_TRIANGLELIST, 2, commitBox, sizeof(HudVertex));
        }
    }

    if (g_gizmoState == GIZMO_DRAGGING) {
        DWORD dragCol = getAxisColor(g_selectedAxis);
        float bx = hudX + 8 * g_gizmoScale, by = hudY + hudH - 14 * g_gizmoScale;
        float bw = hudW - 16 * g_gizmoScale, bh = 8.0f * g_gizmoScale;
        HudVertex dragBar[6] = {
            { bx,      by,      0.0f, 1.0f, dragCol },
            { bx + bw, by,      0.0f, 1.0f, dragCol },
            { bx,      by + bh, 0.0f, 1.0f, dragCol },
            { bx + bw, by,      0.0f, 1.0f, dragCol },
            { bx + bw, by + bh, 0.0f, 1.0f, dragCol },
            { bx,      by + bh, 0.0f, 1.0f, dragCol },
        };
        d3dDrawPrimitiveUP(dev, D3DPT_TRIANGLELIST, 2, dragBar, sizeof(HudVertex));
    }
}

// ============================================================
// Drag handling: accumulate mouse delta into pending movement
// ============================================================
static void beginDrag(GizmoAxis axis, int mx, int my) {
    g_dragStartCount++;
    g_gizmoState = GIZMO_DRAGGING;
    g_selectedAxis = axis;
    g_dragStartX = mx;
    g_dragStartY = my;
    g_dragAccumX = 0.0f;
    g_dragAccumY = 0.0f;
}

static void updateDrag(int mx, int my) {
    if (g_gizmoState != GIZMO_DRAGGING) return;
    if (g_selectedAxis < AXIS_X || g_selectedAxis > AXIS_Z) return;

    float dx = (float)(mx - g_dragStartX);
    float dy = (float)(my - g_dragStartY);
    g_dragStartX = mx;
    g_dragStartY = my;

    float sensitivity = (g_gizmoTool == TOOL_TRANSLATE) ? DRAG_SENSITIVITY_MOVE : DRAG_SENSITIVITY_ROT;

    // Project mouse delta onto the axis's screen direction using dot product
    int idx = (int)g_selectedAxis - 1;
    float axisDx = g_axisScreenDirX[idx];
    float axisDy = g_axisScreenDirY[idx];

    // Normalize the screen direction for dot product (it may not be unit length)
    float screenLen = sqrtf(axisDx * axisDx + axisDy * axisDy);
    if (screenLen < 1.0f) return; // axis projects to a point, skip
    float ndx = axisDx / screenLen;
    float ndy = axisDy / screenLen;

    float dotVal = dx * ndx + dy * ndy;

    if (g_gizmoTool == TOOL_TRANSLATE) {
        // Axes are world-aligned: X = right/left, Y = up/down, Z = forward/back
        float moveAmt = dotVal / sensitivity;
        switch (g_selectedAxis) {
        case AXIS_X: g_pendingMoveX += moveAmt; break;
        case AXIS_Y: g_pendingMoveY += moveAmt; break;
        case AXIS_Z: g_pendingMoveZ += moveAmt; break;
        default: break;
        }
    } else {
        float rotAmt = dotVal / sensitivity;
        switch (g_selectedAxis) {
        case AXIS_X: g_pendingRotPitch += rotAmt; break;
        case AXIS_Y: g_pendingRotYaw += rotAmt; break;
        case AXIS_Z: g_pendingRotRoll += rotAmt; break;
        default: break;
        }
    }
}

static void endDrag() {
    g_gizmoState = GIZMO_ACTIVE;
    g_selectedAxis = AXIS_NONE;
    g_dragAccumX = 0.0f;
    g_dragAccumY = 0.0f;
}

static void cancelDrag() {
    g_gizmoState = GIZMO_ACTIVE;
    g_selectedAxis = AXIS_NONE;
    g_dragAccumX = 0.0f;
    g_dragAccumY = 0.0f;
}

static bool hasPendingMovement() {
    return fabsf(g_pendingMoveX) >= 0.5f || fabsf(g_pendingMoveY) >= 0.5f ||
           fabsf(g_pendingMoveZ) >= 0.5f || fabsf(g_pendingRotYaw) >= 0.5f ||
           fabsf(g_pendingRotPitch) >= 0.5f || fabsf(g_pendingRotRoll) >= 0.5f;
}

static void clearPendingMovement() {
    g_pendingMoveX = g_pendingMoveY = g_pendingMoveZ = 0.0f;
    g_pendingRotYaw = g_pendingRotPitch = g_pendingRotRoll = 0.0f;
}

static DWORD WINAPI CommitThread(LPVOID) {
    int moveX = (int)roundf(g_pendingMoveX);
    int moveY = (int)roundf(g_pendingMoveY);
    int moveZ = (int)roundf(g_pendingMoveZ);
    int rotYaw = (int)roundf(g_pendingRotYaw);
    int rotPitch = (int)roundf(g_pendingRotPitch);
    int rotRoll = (int)roundf(g_pendingRotRoll);
    clearPendingMovement();

    auto sendMoveChunked = [](const char* dir, int total) {
        while (total > 0) {
            int chunk = (total > 500) ? 500 : total;
            sendMove(dir, chunk);
            total -= chunk;
            if (total > 0) Sleep(300);
        }
    };

    if (moveX > 0) { sendMoveChunked("right", moveX); Sleep(300); }
    else if (moveX < 0) { sendMoveChunked("left", -moveX); Sleep(300); }
    if (moveY > 0) { sendMoveChunked("up", moveY); Sleep(300); }
    else if (moveY < 0) { sendMoveChunked("down", -moveY); Sleep(300); }
    if (moveZ > 0) { sendMoveChunked("back", moveZ); Sleep(300); }
    else if (moveZ < 0) { sendMoveChunked("forward", -moveZ); Sleep(300); }
    if (rotYaw != 0) { sendRotate("yaw", rotYaw); Sleep(300); }
    if (rotPitch != 0) { sendRotate("pitch", rotPitch); Sleep(300); }
    if (rotRoll != 0) { sendRotate("roll", rotRoll); Sleep(300); }

    g_commitPending = false;
    MessageBeep(MB_OK);
    return 0;
}

// ============================================================
// Project world-space axis directions to screen-space directions
// Uses the current VIEW and PROJECTION matrices to figure out
// how world X/Y/Z axes appear on screen from camera perspective.
// ============================================================
static void projectAxesToScreen(const D3DMATRIX_MINIMAL& view,
    const D3DMATRIX_MINIMAL& proj, const D3DVIEWPORT9_MINIMAL& vp) {

    D3DMATRIX_MINIMAL viewProj;
    matrixMultiply(viewProj, view, proj);

    // Use a reference point far away (camera look direction * 100 units)
    // This avoids issues near the camera where projection distorts
    // Camera forward = -Z in view space = row2 of view matrix (negated)
    float camFwdX = -view.m[0][2];
    float camFwdY = -view.m[1][2];
    float camFwdZ = -view.m[2][2];

    // Camera position from view matrix
    float camX = -(view.m[3][0] * view.m[0][0] + view.m[3][1] * view.m[0][1] + view.m[3][2] * view.m[0][2]);
    float camY = -(view.m[3][0] * view.m[1][0] + view.m[3][1] * view.m[1][1] + view.m[3][2] * view.m[1][2]);
    float camZ = -(view.m[3][0] * view.m[2][0] + view.m[3][1] * view.m[2][1] + view.m[3][2] * view.m[2][2]);

    // Reference point: 50 units in front of camera
    float refX = camX + camFwdX * 50.0f;
    float refY = camY + camFwdY * 50.0f;
    float refZ = camZ + camFwdZ * 50.0f;

    // Project reference point to screen
    float refSx, refSy;
    if (!projectWorldToScreen(refX, refY, refZ, viewProj, vp, refSx, refSy)) {
        // Fallback: use viewport center
        refSx = vp.Width * 0.5f;
        refSy = vp.Height * 0.5f;
    }

    // For each world axis, project refPoint + axis * offset, then compute screen delta
    float worldAxes[3][3] = {
        { 1.0f, 0.0f, 0.0f },  // World X (right/left)
        { 0.0f, 1.0f, 0.0f },  // World Y (up/down)
        { 0.0f, 0.0f, 1.0f },  // World Z (forward/back)
    };

    float armLen = BASE_GIZMO_ARM * g_gizmoScale;

    for (int a = 0; a < 3; a++) {
        float tipX = refX + worldAxes[a][0] * 2.0f;
        float tipY = refY + worldAxes[a][1] * 2.0f;
        float tipZ = refZ + worldAxes[a][2] * 2.0f;

        float tipSx, tipSy;
        if (projectWorldToScreen(tipX, tipY, tipZ, viewProj, vp, tipSx, tipSy)) {
            float dx = tipSx - refSx;
            float dy = tipSy - refSy;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 0.001f) {
                // Normalize and scale to arm length
                g_axisScreenDirX[a] = (dx / len) * armLen;
                g_axisScreenDirY[a] = (dy / len) * armLen;
            } else {
                g_axisScreenDirX[a] = 0;
                g_axisScreenDirY[a] = 0;
            }
        } else {
            g_axisScreenDirX[a] = 0;
            g_axisScreenDirY[a] = 0;
        }
    }
}

// ============================================================
// Capture world-space anchor point from screen center
// Shoots a ray from camera through screen center at a set distance
// ============================================================
static void captureAnchorFromScreenCenter(const D3DMATRIX_MINIMAL& view,
    const D3DVIEWPORT9_MINIMAL& vp, float distance) {
    // Camera position from view matrix
    float camX = -(view.m[3][0] * view.m[0][0] + view.m[3][1] * view.m[0][1] + view.m[3][2] * view.m[0][2]);
    float camY = -(view.m[3][0] * view.m[1][0] + view.m[3][1] * view.m[1][1] + view.m[3][2] * view.m[1][2]);
    float camZ = -(view.m[3][0] * view.m[2][0] + view.m[3][1] * view.m[2][1] + view.m[3][2] * view.m[2][2]);

    // Camera forward direction (negative Z axis in view space)
    float fwdX = -view.m[0][2];
    float fwdY = -view.m[1][2];
    float fwdZ = -view.m[2][2];

    // Anchor point: camera position + forward * distance
    g_anchorWorldX = camX + fwdX * distance;
    g_anchorWorldY = camY + fwdY * distance;
    g_anchorWorldZ = camZ + fwdZ * distance;
    g_gizmoAnchored = true;
}

// ============================================================
// Safe COM Release helper - releases a texture obtained via GetTexture
// GetTexture increments the refcount, so we must Release to balance it.
// We use the device's own SetTexture(NULL) to detach, and only Release
// via the known-safe function pointer to avoid vtable corruption crashes.
// ============================================================
typedef ULONG(__stdcall* tIUnknownRelease)(void*);

static void safeReleaseTexture(void* tex) {
    if (!tex) return;
    __try {
        // Validate the pointer looks like a COM object (vtable pointer is readable)
        void** vtable = *reinterpret_cast<void***>(tex);
        if (vtable && vtable[2]) {
            tIUnknownRelease pRelease = reinterpret_cast<tIUnknownRelease>(vtable[2]);
            pRelease(tex);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        // Silently ignore — texture was already freed or corrupted
    }
}

// Helper: check if float is valid (not NaN or Inf)
static bool isFinite(float f) {
    // NaN and Inf have all exponent bits set
    unsigned int* pi = reinterpret_cast<unsigned int*>(&f);
    return ((*pi) & 0x7F800000) != 0x7F800000;
}

// Clamp screen coordinate to safe viewport range
static float clampScreen(float v, float maxVal) {
    if (!isFinite(v)) return maxVal * 0.5f;
    if (v < -2000.0f) v = -2000.0f;
    if (v > maxVal + 2000.0f) v = maxVal + 2000.0f;
    return v;
}

// ============================================================
// Hooked EndScene - renders gizmo overlay
// ============================================================
static long __stdcall hkEndScene(IDirect3DDevice9* pDevice) {
    g_device = pDevice;
    g_endSceneCallCount++;

    if (g_deviceLost) {
        return g_oEndScene(pDevice);
    }

    // Check viewport — only do ALL our work on the main viewport
    D3DVIEWPORT9_MINIMAL vp;
    d3dGetViewport(pDevice, &vp);
    if (vp.Width < 400 || vp.Height < 400) {
        return g_oEndScene(pDevice);
    }

    // === Always-on debug indicator: small colored square at top-right ===
    // Minimal state changes — no texture save/restore (just set NULL and restore NULL)
    {
        d3dSetTexture(pDevice, 0, nullptr);
        d3dSetRenderState(pDevice, D3DRS_LIGHTING, 0);
        d3dSetRenderState(pDevice, D3DRS_ZENABLE, 0);

        DWORD dbgCol = g_decoMode ? 0xFF00FF00 : 0xFF4444FF;
        float dbgX = (float)vp.Width - 30.0f;
        float dbgY = 5.0f;
        float dbgSz = 20.0f;
        HudVertex dbgQuad[6] = {
            { dbgX,        dbgY,        0.0f, 1.0f, dbgCol },
            { dbgX + dbgSz, dbgY,       0.0f, 1.0f, dbgCol },
            { dbgX,        dbgY + dbgSz, 0.0f, 1.0f, dbgCol },
            { dbgX + dbgSz, dbgY,       0.0f, 1.0f, dbgCol },
            { dbgX + dbgSz, dbgY + dbgSz, 0.0f, 1.0f, dbgCol },
            { dbgX,        dbgY + dbgSz, 0.0f, 1.0f, dbgCol },
        };
        d3dSetFVF(pDevice, HUD_FVF);
        d3dDrawPrimitiveUP(pDevice, D3DPT_TRIANGLELIST, 2, dbgQuad, sizeof(HudVertex));
        // Note: we don't restore debug indicator state — the main deco block below
        // will set its own state, and if deco is off, the game resets state next frame anyway.
    }

    if (g_decoMode) {
        g_gizmoScale = (float)vp.Height / 1080.0f;
        if (g_gizmoScale < 1.0f) g_gizmoScale = 1.0f;
        g_gizmoArmLen = BASE_GIZMO_ARM * g_gizmoScale;

        if (g_gizmoState == GIZMO_IDLE) {
            g_gizmoState = GIZMO_ACTIVE;
        }

        // === Get VIEW/PROJ matrices to compute camera-aligned axis directions ===
        D3DMATRIX_MINIMAL savedView, savedProj;
        d3dGetTransform(pDevice, D3DTS_VIEW, &savedView);
        d3dGetTransform(pDevice, D3DTS_PROJECTION, &savedProj);

        // Sanity check: verify matrices contain valid floats
        bool matricesValid = isFinite(savedView.m[0][0]) && isFinite(savedView.m[3][3])
                          && isFinite(savedProj.m[0][0]) && isFinite(savedProj.m[3][3]);
        if (!matricesValid) {
            // Matrices are garbage — skip all rendering this frame
            return g_oEndScene(pDevice);
        }

        // Project world axes to screen directions for this camera angle
        projectAxesToScreen(savedView, savedProj, vp);

        // === Capture anchor on first frame of deco mode ===
        if (!g_gizmoAnchored) {
            captureAnchorFromScreenCenter(savedView, vp, 5.0f);
        }

        // === Project world anchor to current screen position ===
        D3DMATRIX_MINIMAL viewProj;
        matrixMultiply(viewProj, savedView, savedProj);

        float anchorSx, anchorSy;
        g_anchorOnScreen = projectWorldToScreen(
            g_anchorWorldX, g_anchorWorldY, g_anchorWorldZ,
            viewProj, vp, anchorSx, anchorSy);

        if (g_anchorOnScreen && isFinite(anchorSx) && isFinite(anchorSy)) {
            g_gizmoCenterX = clampScreen(anchorSx, (float)vp.Width);
            g_gizmoCenterY = clampScreen(anchorSy, (float)vp.Height);
        } else {
            g_gizmoCenterX = vp.Width * 0.5f;
            g_gizmoCenterY = vp.Height * 0.5f;
            g_anchorOnScreen = false;
        }

        // === Save and set render states ===
        // Save only render states (cheap DWORD values), NOT textures
        DWORD savedLighting   = d3dGetRenderState(pDevice, D3DRS_LIGHTING);
        DWORD savedZEnable    = d3dGetRenderState(pDevice, D3DRS_ZENABLE);
        DWORD savedZWrite     = d3dGetRenderState(pDevice, D3DRS_ZWRITEENABLE);
        DWORD savedAlphaBlend = d3dGetRenderState(pDevice, D3DRS_ALPHABLENDENABLE);
        DWORD savedSrcBlend   = d3dGetRenderState(pDevice, D3DRS_SRCBLEND);
        DWORD savedDestBlend  = d3dGetRenderState(pDevice, D3DRS_DESTBLEND);
        DWORD savedCullMode   = d3dGetRenderState(pDevice, D3DRS_CULLMODE);
        DWORD savedFillMode   = d3dGetRenderState(pDevice, D3DRS_FILLMODE);
        DWORD savedAALine     = d3dGetRenderState(pDevice, D3DRS_ANTIALIASEDLINEENABLE);

        // Set render states for overlay (texture already NULL from debug indicator)
        d3dSetTexture(pDevice, 0, nullptr);
        d3dSetRenderState(pDevice, D3DRS_LIGHTING, 0);
        d3dSetRenderState(pDevice, D3DRS_ZENABLE, 0);
        d3dSetRenderState(pDevice, D3DRS_ZWRITEENABLE, 0);
        d3dSetRenderState(pDevice, D3DRS_ALPHABLENDENABLE, 1);
        d3dSetRenderState(pDevice, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        d3dSetRenderState(pDevice, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        d3dSetRenderState(pDevice, D3DRS_CULLMODE, D3DCULL_NONE);
        d3dSetRenderState(pDevice, D3DRS_FILLMODE, D3DFILL_SOLID);
        d3dSetRenderState(pDevice, D3DRS_ANTIALIASEDLINEENABLE, 1);

        d3dSetFVF(pDevice, HUD_FVF);

        // Render gizmo
        if (g_gizmoTool == TOOL_TRANSLATE)
            renderTranslateGizmo(pDevice);
        else
            renderRotateGizmo(pDevice);

        // Render HUD info panel
        renderHUD(pDevice);

        // Crosshair cursor (when Alt is held for free cursor)
        {
            float cx = (float)g_mouseX, cy = (float)g_mouseY;
            float cs = 10.0f * g_gizmoScale;
            DWORD cursorCol = (g_hoveredAxis != AXIS_NONE) ? 0xFFFFFF00 : 0xFFFFFFFF;
            HudVertex crosshair[4] = {
                { cx - cs, cy, 0.0f, 1.0f, cursorCol },
                { cx + cs, cy, 0.0f, 1.0f, cursorCol },
                { cx, cy - cs, 0.0f, 1.0f, cursorCol },
                { cx, cy + cs, 0.0f, 1.0f, cursorCol },
            };
            d3dDrawPrimitiveUP(pDevice, D3DPT_LINELIST, 2, crosshair, sizeof(HudVertex));
        }

        // === Restore render states (DWORD values only — safe, no COM pointers) ===
        d3dSetRenderState(pDevice, D3DRS_LIGHTING, savedLighting);
        d3dSetRenderState(pDevice, D3DRS_ZENABLE, savedZEnable);
        d3dSetRenderState(pDevice, D3DRS_ZWRITEENABLE, savedZWrite);
        d3dSetRenderState(pDevice, D3DRS_ALPHABLENDENABLE, savedAlphaBlend);
        d3dSetRenderState(pDevice, D3DRS_SRCBLEND, savedSrcBlend);
        d3dSetRenderState(pDevice, D3DRS_DESTBLEND, savedDestBlend);
        d3dSetRenderState(pDevice, D3DRS_CULLMODE, savedCullMode);
        d3dSetRenderState(pDevice, D3DRS_FILLMODE, savedFillMode);
        d3dSetRenderState(pDevice, D3DRS_ANTIALIASEDLINEENABLE, savedAALine);
        // Note: We don't save/restore texture or FVF — the game sets these itself
        // before each draw call. Saving/restoring COM texture pointers was crash-prone.
    } else if (!g_decoMode && g_gizmoState != GIZMO_IDLE) {
        if (g_gizmoState == GIZMO_DRAGGING) cancelDrag();
        g_gizmoState = GIZMO_IDLE;
        g_hoveredAxis = AXIS_NONE;
    }

    return g_oEndScene(pDevice);
}

// ============================================================
// Hooked Reset - handle device lost/restored
// ============================================================
static long __stdcall hkReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS_MINIMAL* pp) {
    g_deviceLost = true;
    long result = g_oReset(pDevice, pp);
    if (result == 0) g_deviceLost = false;
    return result;
}

// ============================================================
// Mouse polling
// ============================================================
static bool g_prevLButton = false;
static bool g_prevRButton = false;

static void pollMouseForGizmo() {
    if (!g_decoMode || g_gizmoState < GIZMO_ACTIVE || !g_gameHwnd) return;

    bool lButton = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool rButton = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    CURSORINFO ci = {};
    ci.cbSize = sizeof(ci);
    GetCursorInfo(&ci);
    bool cursorVisible = (ci.flags & CURSOR_SHOWING) != 0;

    if (cursorVisible || g_gizmoState == GIZMO_DRAGGING) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(g_gameHwnd, &pt);
        g_mouseX = pt.x;
        g_mouseY = pt.y;
        g_rawInputCount++;

        if (g_gizmoState != GIZMO_DRAGGING) {
            updateGizmoHover(g_mouseX, g_mouseY);
        } else {
            updateDrag(g_mouseX, g_mouseY);
        }
    }

    bool mButton = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
    bool escKey  = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    static bool prevMButton = false;
    static bool prevEscKey  = false;

    if (mButton && !prevMButton) {
        g_clickCount++;
        if (g_gizmoState == GIZMO_DRAGGING) {
            endDrag();
        } else if (g_hoveredAxis != AXIS_NONE) {
            beginDrag(g_hoveredAxis, g_mouseX, g_mouseY);
        }
    }
    if (rButton && !g_prevRButton) {
        if (g_gizmoState == GIZMO_DRAGGING) cancelDrag();
    }
    if (escKey && !prevEscKey) {
        if (g_gizmoState == GIZMO_DRAGGING) cancelDrag();
    }

    g_prevLButton = lButton;
    g_prevRButton = rButton;
    prevMButton = mButton;
    prevEscKey = escKey;
}

// ============================================================
// Find the game window
// ============================================================
struct FindWindowData { DWORD processId; HWND result; };

static BOOL CALLBACK enumWindowsCallback(HWND hwnd, LPARAM lParam) {
    FindWindowData* data = reinterpret_cast<FindWindowData*>(lParam);
    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);
    if (windowPid == data->processId && IsWindowVisible(hwnd)) {
        char title[256];
        GetWindowTextA(hwnd, title, sizeof(title));
        if (strlen(title) > 0) {
            data->result = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}

static HWND findGameWindow() {
    FindWindowData data;
    data.processId = GetCurrentProcessId();
    data.result = nullptr;
    EnumWindows(enumWindowsCallback, reinterpret_cast<LPARAM>(&data));
    return data.result;
}

// ============================================================
// Find D3D9 device via temporary device creation
// ============================================================
static bool findD3D9Device() {
    HMODULE d3d9Module = GetModuleHandleA("d3d9.dll");
    if (!d3d9Module) {
        wsprintfA(g_diagStatus, "FAIL: d3d9.dll not loaded");
        return false;
    }

    tDirect3DCreate9 pDirect3DCreate9 = reinterpret_cast<tDirect3DCreate9>(
        GetProcAddress(d3d9Module, "Direct3DCreate9"));
    if (!pDirect3DCreate9) {
        wsprintfA(g_diagStatus, "FAIL: Direct3DCreate9 not found");
        return false;
    }

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "DecoD3D9TempWnd";
    RegisterClassExA(&wc);

    HWND tempHwnd = CreateWindowExA(0, "DecoD3D9TempWnd", "", WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
    if (!tempHwnd) {
        wsprintfA(g_diagStatus, "FAIL: temp window creation");
        return false;
    }

    IDirect3D9* d3d = pDirect3DCreate9(D3D_SDK_VERSION_CONST);
    if (!d3d) {
        DestroyWindow(tempHwnd);
        UnregisterClassA("DecoD3D9TempWnd", wc.hInstance);
        wsprintfA(g_diagStatus, "FAIL: Direct3DCreate9 returned null");
        return false;
    }

    IDirect3DDevice9* tempDevice = nullptr;
    void** d3dVtable = *reinterpret_cast<void***>(d3d);
    tCreateDevice pCreateDevice = reinterpret_cast<tCreateDevice>(d3dVtable[16]);

    struct DeviceAttempt { int devType; DWORD flags; const char* name; };
    DeviceAttempt attempts[] = {
        { 1, D3DCREATE_SOFTWARE_VERTEXPROCESSING, "HAL+SW" },
        { 1, 0x00000040L, "HAL+HW" },
        { 4, D3DCREATE_SOFTWARE_VERTEXPROCESSING, "NULLREF" },
        { 2, D3DCREATE_SOFTWARE_VERTEXPROCESSING, "REF" },
    };

    long hr = -1;
    const char* successMethod = "none";
    for (int attempt = 0; attempt < 4; attempt++) {
        D3DPRESENT_PARAMETERS_MINIMAL pp = {};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow = tempHwnd;
        pp.BackBufferFormat = D3DFMT_UNKNOWN;
        pp.BackBufferWidth = 1;
        pp.BackBufferHeight = 1;

        hr = pCreateDevice(d3d, D3DADAPTER_DEFAULT, attempts[attempt].devType,
            tempHwnd, attempts[attempt].flags, &pp, &tempDevice);
        if (hr == 0 && tempDevice) {
            successMethod = attempts[attempt].name;
            break;
        }
        tempDevice = nullptr;
    }

    if (!tempDevice) {
        tRelease pRelease = reinterpret_cast<tRelease>(d3dVtable[2]);
        pRelease(d3d);
        DestroyWindow(tempHwnd);
        UnregisterClassA("DecoD3D9TempWnd", wc.hInstance);
        wsprintfA(g_diagStatus, "FAIL: All CreateDevice attempts failed, last hr=0x%08X", (unsigned)hr);
        return false;
    }

    void** deviceVtable = *reinterpret_cast<void***>(tempDevice);
    g_oEndScene = reinterpret_cast<tEndScene>(deviceVtable[D3D9_VTABLE_ENDSCENE]);
    g_oReset = reinterpret_cast<tReset>(deviceVtable[D3D9_VTABLE_RESET]);

    tRelease pReleaseDevice = reinterpret_cast<tRelease>(deviceVtable[2]);
    pReleaseDevice(tempDevice);
    tRelease pReleaseD3D = reinterpret_cast<tRelease>(d3dVtable[2]);
    pReleaseD3D(d3d);
    DestroyWindow(tempHwnd);
    UnregisterClassA("DecoD3D9TempWnd", wc.hInstance);

    wsprintfA(g_diagStatus, "findD3D9 OK(%s): ES=%p R=%p", successMethod, g_oEndScene, g_oReset);
    return (g_oEndScene != nullptr);
}

// ============================================================
// Install D3D9 hooks
// ============================================================
static bool installD3D9Hooks() {
    if (g_hooksInstalled) return true;

    wsprintfA(g_diagStatus, "Starting hook install...");

    MH_STATUS mhStatus = MH_Initialize();
    if (mhStatus != MH_OK) {
        wsprintfA(g_diagStatus, "FAIL: MH_Initialize returned %d", (int)mhStatus);
        OutputDebugStringA(g_diagStatus);
        return false;
    }

    if (!findD3D9Device()) {
        OutputDebugStringA(g_diagStatus);
        MH_Uninitialize();
        return false;
    }

    wsprintfA(g_diagStatus, "D3D9 found, creating hooks...");

    mhStatus = MH_CreateHook(reinterpret_cast<void*>(g_oEndScene), reinterpret_cast<void*>(&hkEndScene),
        reinterpret_cast<void**>(&g_oEndScene));
    if (mhStatus != MH_OK) {
        wsprintfA(g_diagStatus, "FAIL: MH_CreateHook EndScene returned %d", (int)mhStatus);
        MH_Uninitialize();
        return false;
    }

    mhStatus = MH_CreateHook(reinterpret_cast<void*>(g_oReset), reinterpret_cast<void*>(&hkReset),
        reinterpret_cast<void**>(&g_oReset));
    if (mhStatus != MH_OK) {
        wsprintfA(g_diagStatus, "FAIL: MH_CreateHook Reset returned %d", (int)mhStatus);
        MH_Uninitialize();
        return false;
    }

    mhStatus = MH_EnableHook(MH_ALL_HOOKS);
    if (mhStatus != MH_OK) {
        wsprintfA(g_diagStatus, "FAIL: MH_EnableHook returned %d", (int)mhStatus);
        MH_Uninitialize();
        return false;
    }

    g_gameHwnd = findGameWindow();
    if (g_gameHwnd) {
        wsprintfA(g_diagStatus, "OK: 2 hooks installed. HWND=%p.", g_gameHwnd);
    } else {
        wsprintfA(g_diagStatus, "PARTIAL: hooks OK but no HWND.");
    }

    g_hooksInstalled = true;
    OutputDebugStringA(g_diagStatus);
    return true;
}

static void removeD3D9Hooks() {
    if (!g_hooksInstalled) return;
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    g_hooksInstalled = false;
    g_device = nullptr;
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

        pollMouseForGizmo();

        // F11: diagnostics
        if (keys[VK_F11] && !prevKeys[VK_F11]) {
            char diagMsg[256];
            wsprintfA(diagMsg, "/echo [Deco] %s | ES: %d", g_diagStatus, g_endSceneCallCount);
            executeChatCommand(diagMsg);
            Sleep(350);
            wsprintfA(diagMsg, "/echo [Deco] Deco: %s | GizState: %d | Tool: %s | Center: %.0f,%.0f",
                g_decoMode ? "ON" : "OFF", (int)g_gizmoState,
                g_gizmoTool == TOOL_TRANSLATE ? "MOVE" : "ROT",
                g_gizmoCenterX, g_gizmoCenterY);
            executeChatCommand(diagMsg);
            Sleep(350);
            wsprintfA(diagMsg, "/echo [Deco] Hover: %d | Clicks: %d | Drags: %d | Pending: X=%d Y=%d Z=%d",
                (int)g_hoveredAxis, g_clickCount, g_dragStartCount,
                (int)roundf(g_pendingMoveX), (int)roundf(g_pendingMoveY), (int)roundf(g_pendingMoveZ));
            executeChatCommand(diagMsg);
            Sleep(350);
            wsprintfA(diagMsg, "/echo [Deco] Anchor: %s (%.1f,%.1f,%.1f) OnScr: %s GizPos: %.0f,%.0f",
                g_gizmoAnchored ? "YES" : "NO",
                g_anchorWorldX, g_anchorWorldY, g_anchorWorldZ,
                g_anchorOnScreen ? "YES" : "NO",
                g_gizmoCenterX, g_gizmoCenterY);
            executeChatCommand(diagMsg);
        }

        // F12: Toggle decoration mode
        if (keys[VK_F12] && !prevKeys[VK_F12]) {
            g_decoMode = !g_decoMode;
            if (g_decoMode) {
                // Reset anchor — it will be captured on the next EndScene frame
                // at the point the camera is looking at
                g_gizmoAnchored = false;
                g_anchorOnScreen = false;
            }
            MessageBeep(g_decoMode ? MB_OK : MB_ICONEXCLAMATION);
        }

        if (g_decoMode) {
            bool shift = keys[VK_SHIFT];
            int step = shift ? g_moveStep * 5 : g_moveStep;
            if (step > 500) step = 500;
            int rotStep = shift ? g_rotateStep * 5 : g_rotateStep;

            if (keys[VK_NUMPAD0] && !prevKeys[VK_NUMPAD0]) {
                g_gizmoTool = (g_gizmoTool == TOOL_TRANSLATE) ? TOOL_ROTATE : TOOL_TRANSLATE;
                MessageBeep(MB_OK);
            }

            if (keys[VK_NUMPAD5] && !prevKeys[VK_NUMPAD5]) {
                if (hasPendingMovement() && !g_commitPending) {
                    g_commitPending = true;
                    CreateThread(NULL, 0, CommitThread, NULL, 0, NULL);
                } else if (!hasPendingMovement()) {
                    MessageBeep(MB_ICONEXCLAMATION);
                }
            }

            if (keys[VK_DECIMAL] && !prevKeys[VK_DECIMAL]) {
                if (g_gizmoState == GIZMO_DRAGGING) cancelDrag();
                clearPendingMovement();
                MessageBeep(MB_ICONEXCLAMATION);
            }

            if (g_gizmoState != GIZMO_DRAGGING) {
                if (keys[VK_NUMPAD8] && !prevKeys[VK_NUMPAD8]) sendMove("forward", step);
                if (keys[VK_NUMPAD2] && !prevKeys[VK_NUMPAD2]) sendMove("back", step);
                if (keys[VK_NUMPAD4] && !prevKeys[VK_NUMPAD4]) sendMove("left", step);
                if (keys[VK_NUMPAD6] && !prevKeys[VK_NUMPAD6]) sendMove("right", step);
                if (keys[VK_ADD]     && !prevKeys[VK_ADD])      sendMove("up", step);
                if (keys[VK_SUBTRACT]&& !prevKeys[VK_SUBTRACT]) sendMove("down", step);
                if (keys[VK_NUMPAD7] && !prevKeys[VK_NUMPAD7]) sendRotate("yaw", -rotStep);
                if (keys[VK_NUMPAD9] && !prevKeys[VK_NUMPAD9]) sendRotate("yaw", rotStep);
                if (keys[VK_NUMPAD1] && !prevKeys[VK_NUMPAD1]) sendRotate("pitch", -rotStep);
                if (keys[VK_NUMPAD3] && !prevKeys[VK_NUMPAD3]) sendRotate("pitch", rotStep);
            }

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
    wsprintfA(g_diagStatus, "Waiting for game window...");

    bool foundWindow = false;
    for (int i = 0; i < 120; i++) {
        Sleep(500);
        HWND fgWnd = GetForegroundWindow();
        if (fgWnd) {
            DWORD fgPid; GetWindowThreadProcessId(fgWnd, &fgPid);
            if (fgPid == GetCurrentProcessId()) {
                foundWindow = true;
                break;
            }
        }
    }

    if (!foundWindow) {
        wsprintfA(g_diagStatus, "WARN: Timed out waiting for game window");
    }

    wsprintfA(g_diagStatus, "Window found, waiting 3s for D3D9...");
    Sleep(3000);

    HMODULE d3dCheck = GetModuleHandleA("d3d9.dll");
    wsprintfA(g_diagStatus, "d3d9.dll=%p, installing hooks...", d3dCheck);

    if (!installD3D9Hooks()) {
        OutputDebugStringA("[Deco] D3D9 hooks failed");
    }

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
        removeD3D9Hooks();
        if (hReal) FreeLibrary(hReal);
    }
    return TRUE;
}
