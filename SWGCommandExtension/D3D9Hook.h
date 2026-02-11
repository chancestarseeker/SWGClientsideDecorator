#pragma once

// ======================================================================
// D3D9Hook - Hooks IDirect3DDevice9::EndScene to render decoration overlays.
//
// Uses a temporary D3D9 device to discover the vtable layout, then
// hooks EndScene via Detours. When our EndScene fires, we capture the
// real device pointer and delegate rendering to GizmoRenderer.
//
// Also subclasses the SWG window procedure for mouse input capture.
// ======================================================================

#include <windows.h>

// Opaque forward declarations -- we define minimal D3D9 types inline
// so we don't require the DirectX SDK to be installed.
struct IDirect3D9;
struct IDirect3DDevice9;

// D3D9 constants we use
#define D3D_SDK_VERSION_CONST 32
#define D3DADAPTER_DEFAULT 0
#define D3DDEVTYPE_HAL 1
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0x00000020L
#define D3DSWAPEFFECT_DISCARD 1
#define D3DFMT_UNKNOWN 0

// D3D9 vtable indices for IDirect3DDevice9
#define D3D9_VTABLE_ENDSCENE     42
#define D3D9_VTABLE_RESET        16
#define D3D9_VTABLE_SETTRANSFORM 44
#define D3D9_VTABLE_GETTRANSFORM 45
#define D3D9_VTABLE_SETVIEWPORT  47
#define D3D9_VTABLE_GETVIEWPORT  48
#define D3D9_VTABLE_SETRENDERSTATE 57
#define D3D9_VTABLE_SETTEXTURE   65
#define D3D9_VTABLE_SETFVF       89
#define D3D9_VTABLE_DRAWPRIMITIVEUP 83
#define D3D9_VTABLE_SETSTREAMOURCE 100

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
#define D3DFILL_WIREFRAME        2
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

// Minimal D3D matrix
struct D3DMATRIX_MINIMAL {
	float m[4][4];
};

// Minimal D3D viewport
struct D3DVIEWPORT9_MINIMAL {
	DWORD X, Y, Width, Height;
	float MinZ, MaxZ;
};

// Minimal present parameters for temp device creation
struct D3DPRESENT_PARAMETERS_MINIMAL {
	UINT BackBufferWidth;
	UINT BackBufferHeight;
	UINT BackBufferFormat;
	UINT BackBufferCount;
	UINT MultiSampleType;
	DWORD MultiSampleQuality;
	UINT SwapEffect;
	HWND hDeviceWindow;
	BOOL Windowed;
	BOOL EnableAutoDepthStencil;
	UINT AutoDepthStencilFormat;
	DWORD Flags;
	UINT FullScreen_RefreshRateInHz;
	UINT PresentationInterval;
};

// Function pointer types for D3D9 device methods (stdcall COM convention)
typedef long(__stdcall* tEndScene)(IDirect3DDevice9*);
typedef long(__stdcall* tReset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS_MINIMAL*);
typedef long(__stdcall* tSetTransform)(IDirect3DDevice9*, int, const D3DMATRIX_MINIMAL*);
typedef long(__stdcall* tGetTransform)(IDirect3DDevice9*, int, D3DMATRIX_MINIMAL*);
typedef long(__stdcall* tSetViewport)(IDirect3DDevice9*, const D3DVIEWPORT9_MINIMAL*);
typedef long(__stdcall* tGetViewport)(IDirect3DDevice9*, D3DVIEWPORT9_MINIMAL*);
typedef long(__stdcall* tSetRenderState)(IDirect3DDevice9*, int, DWORD);
typedef long(__stdcall* tSetTexture)(IDirect3DDevice9*, DWORD, void*);
typedef long(__stdcall* tSetFVF)(IDirect3DDevice9*, DWORD);
typedef long(__stdcall* tDrawPrimitiveUP)(IDirect3DDevice9*, int, UINT, const void*, UINT);

// Helper to call a D3D9 device method by vtable index
inline void** getD3D9Vtable(IDirect3DDevice9* device) {
	return *reinterpret_cast<void***>(device);
}

template<typename FuncType>
inline FuncType getD3D9Method(IDirect3DDevice9* device, int vtableIndex) {
	void** vtable = getD3D9Vtable(device);
	return reinterpret_cast<FuncType>(vtable[vtableIndex]);
}

class D3D9Hook {
public:
	static bool install();
	static void remove();
	static bool isInstalled() { return s_installed; }
	static IDirect3DDevice9* getDevice() { return s_device; }
	static HWND getGameHwnd() { return s_hwnd; }

private:
	static long __stdcall hkEndScene(IDirect3DDevice9* pDevice);
	static long __stdcall hkReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS_MINIMAL* pp);
	static LRESULT CALLBACK hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static bool findD3D9Device();
	static HWND findGameWindow();

	static IDirect3DDevice9* s_device;
	static bool s_installed;
	static bool s_deviceLost;
	static HWND s_hwnd;
	static WNDPROC s_originalWndProc;
	static tEndScene s_oEndScene;
	static tReset s_oReset;
};
