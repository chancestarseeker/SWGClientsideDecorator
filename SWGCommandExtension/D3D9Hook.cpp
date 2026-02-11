#include "stdafx.h"
#include "D3D9Hook.h"
#include "GizmoRenderer.h"
#include "DecorationMode.h"
#include "Game.h"

#include <detours.h>

// ======================================================================
// Direct3DCreate9 function type (loaded dynamically from d3d9.dll)
// ======================================================================
// IDirect3D9 vtable index for CreateDevice
#define D3D9_VTABLE_CREATEDEVICE 16

typedef IDirect3D9* (__stdcall* tDirect3DCreate9)(UINT);
typedef long(__stdcall* tCreateDevice)(IDirect3D9*, UINT, int, HWND, DWORD,
	D3DPRESENT_PARAMETERS_MINIMAL*, IDirect3DDevice9**);
typedef ULONG(__stdcall* tRelease)(void*);

// ======================================================================
// Static members
// ======================================================================
IDirect3DDevice9* D3D9Hook::s_device = nullptr;
bool D3D9Hook::s_installed = false;
bool D3D9Hook::s_deviceLost = false;
HWND D3D9Hook::s_hwnd = nullptr;
WNDPROC D3D9Hook::s_originalWndProc = nullptr;
tEndScene D3D9Hook::s_oEndScene = nullptr;
tReset D3D9Hook::s_oReset = nullptr;

// ======================================================================
// Window enumeration callback -- find the SWG client window
// ======================================================================
struct FindWindowData {
	DWORD processId;
	HWND result;
};

static BOOL CALLBACK enumWindowsCallback(HWND hwnd, LPARAM lParam) {
	FindWindowData* data = reinterpret_cast<FindWindowData*>(lParam);
	DWORD windowProcessId = 0;
	GetWindowThreadProcessId(hwnd, &windowProcessId);

	if (windowProcessId == data->processId && IsWindowVisible(hwnd)) {
		// Check that it's a top-level window with a title
		char title[256];
		GetWindowTextA(hwnd, title, sizeof(title));
		if (strlen(title) > 0) {
			data->result = hwnd;
			return FALSE; // stop enumeration
		}
	}
	return TRUE;
}

HWND D3D9Hook::findGameWindow() {
	FindWindowData data;
	data.processId = GetCurrentProcessId();
	data.result = nullptr;
	EnumWindows(enumWindowsCallback, reinterpret_cast<LPARAM>(&data));
	return data.result;
}

// ======================================================================
// Find the D3D9 device by creating a temporary one to extract vtable
// ======================================================================
bool D3D9Hook::findD3D9Device() {
	HMODULE d3d9Module = GetModuleHandleA("d3d9.dll");
	if (!d3d9Module) {
		OutputDebugStringA("[Deco D3D9] d3d9.dll not loaded");
		return false;
	}

	tDirect3DCreate9 pDirect3DCreate9 = reinterpret_cast<tDirect3DCreate9>(
		GetProcAddress(d3d9Module, "Direct3DCreate9"));
	if (!pDirect3DCreate9) {
		OutputDebugStringA("[Deco D3D9] Could not find Direct3DCreate9");
		return false;
	}

	// Create a temporary hidden window for the device
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
		OutputDebugStringA("[Deco D3D9] Could not create temp window");
		return false;
	}

	IDirect3D9* d3d = pDirect3DCreate9(D3D_SDK_VERSION_CONST);
	if (!d3d) {
		DestroyWindow(tempHwnd);
		UnregisterClassA("DecoD3D9TempWnd", wc.hInstance);
		OutputDebugStringA("[Deco D3D9] Direct3DCreate9 failed");
		return false;
	}

	D3DPRESENT_PARAMETERS_MINIMAL pp = {};
	pp.Windowed = TRUE;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.hDeviceWindow = tempHwnd;
	pp.BackBufferFormat = D3DFMT_UNKNOWN;

	IDirect3DDevice9* tempDevice = nullptr;

	// Get CreateDevice from d3d9 vtable
	void** d3dVtable = *reinterpret_cast<void***>(d3d);
	tCreateDevice pCreateDevice = reinterpret_cast<tCreateDevice>(d3dVtable[D3D9_VTABLE_CREATEDEVICE]);

	long hr = pCreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, tempHwnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &tempDevice);

	if (hr != 0 || !tempDevice) {
		tRelease pRelease = reinterpret_cast<tRelease>(d3dVtable[2]);
		pRelease(d3d);
		DestroyWindow(tempHwnd);
		UnregisterClassA("DecoD3D9TempWnd", wc.hInstance);
		OutputDebugStringA("[Deco D3D9] CreateDevice failed");
		return false;
	}

	// Extract EndScene and Reset addresses from the temp device vtable
	void** deviceVtable = *reinterpret_cast<void***>(tempDevice);
	s_oEndScene = reinterpret_cast<tEndScene>(deviceVtable[D3D9_VTABLE_ENDSCENE]);
	s_oReset = reinterpret_cast<tReset>(deviceVtable[D3D9_VTABLE_RESET]);

	// Clean up the temp device
	tRelease pReleaseDevice = reinterpret_cast<tRelease>(deviceVtable[2]);
	pReleaseDevice(tempDevice);

	tRelease pReleaseD3D = reinterpret_cast<tRelease>(d3dVtable[2]);
	pReleaseD3D(d3d);

	DestroyWindow(tempHwnd);
	UnregisterClassA("DecoD3D9TempWnd", wc.hInstance);

	return (s_oEndScene != nullptr);
}

// ======================================================================
// Install hooks
// ======================================================================
bool D3D9Hook::install() {
	if (s_installed) return true;

	if (!findD3D9Device()) {
		OutputDebugStringA("[Deco D3D9] Failed to find D3D9 device vtable");
		return false;
	}

	// Hook EndScene and Reset via Detours
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(reinterpret_cast<PVOID*>(&s_oEndScene), hkEndScene);
	DetourAttach(reinterpret_cast<PVOID*>(&s_oReset), hkReset);
	LONG err = DetourTransactionCommit();

	if (err != NO_ERROR) {
		OutputDebugStringA("[Deco D3D9] Detour hook failed");
		return false;
	}

	// Subclass the game window for mouse input
	s_hwnd = findGameWindow();
	if (s_hwnd) {
		s_originalWndProc = reinterpret_cast<WNDPROC>(
			SetWindowLongPtrA(s_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hkWndProc)));
	}

	s_installed = true;
	OutputDebugStringA("[Deco D3D9] Hooks installed successfully");
	Game::debugPrintUi("[Deco] 3D Gizmo overlay initialized.");
	return true;
}

// ======================================================================
// Remove hooks
// ======================================================================
void D3D9Hook::remove() {
	if (!s_installed) return;

	// Restore window procedure
	if (s_hwnd && s_originalWndProc) {
		SetWindowLongPtrA(s_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(s_originalWndProc));
		s_originalWndProc = nullptr;
	}

	// Unhook D3D9
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(reinterpret_cast<PVOID*>(&s_oEndScene), hkEndScene);
	DetourDetach(reinterpret_cast<PVOID*>(&s_oReset), hkReset);
	DetourTransactionCommit();

	s_installed = false;
	s_device = nullptr;
	OutputDebugStringA("[Deco D3D9] Hooks removed");
}

// ======================================================================
// Hooked EndScene - called every frame after the game finishes rendering
// ======================================================================
long __stdcall D3D9Hook::hkEndScene(IDirect3DDevice9* pDevice) {
	s_device = pDevice;

	if (!s_deviceLost) {
		auto& deco = DecorationMode::getInstance();
		if (deco.isActive() && deco.isGizmoEnabled()) {
			GizmoRenderer::render(pDevice);
		}
	}

	return s_oEndScene(pDevice);
}

// ======================================================================
// Hooked Reset - handle device lost/restored
// ======================================================================
long __stdcall D3D9Hook::hkReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS_MINIMAL* pp) {
	s_deviceLost = true;
	GizmoRenderer::onDeviceLost();

	long result = s_oReset(pDevice, pp);

	if (result == 0) { // D3D_OK
		s_deviceLost = false;
		GizmoRenderer::onDeviceRestored(pDevice);
	}

	return result;
}

// ======================================================================
// Hooked WndProc - intercept mouse events for gizmo interaction
// ======================================================================
LRESULT CALLBACK D3D9Hook::hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	auto& deco = DecorationMode::getInstance();

	if (deco.isActive() && deco.isGizmoEnabled()) {
		int mouseX = LOWORD(lParam);
		int mouseY = HIWORD(lParam);

		switch (msg) {
		case WM_LBUTTONDOWN:
			if (deco.getHoveredAxis() != DecorationMode::GizmoAxis::None) {
				deco.beginDrag(deco.getHoveredAxis(), mouseX, mouseY);
				return 0; // consume the message
			}
			break;

		case WM_MOUSEMOVE:
			if (deco.getState() == DecorationMode::State::Dragging) {
				deco.updateDrag(mouseX, mouseY);
				return 0;
			} else if (deco.getState() == DecorationMode::State::Active && s_device) {
				// Update axis hover detection
				GizmoRenderer::updateHoverTest(s_device, mouseX, mouseY);
			}
			break;

		case WM_LBUTTONUP:
			if (deco.getState() == DecorationMode::State::Dragging) {
				deco.endDrag();
				return 0;
			}
			break;

		case WM_RBUTTONDOWN:
			if (deco.getState() == DecorationMode::State::Dragging) {
				deco.cancelDrag();
				return 0;
			}
			break;

		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE && deco.getState() == DecorationMode::State::Dragging) {
				deco.cancelDrag();
				return 0;
			}
			break;
		}
	}

	return CallWindowProcA(s_originalWndProc, hwnd, msg, wParam, lParam);
}
