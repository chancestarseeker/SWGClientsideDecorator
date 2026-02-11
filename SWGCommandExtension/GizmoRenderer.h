#pragma once

#include "D3D9Hook.h"
#include "Vector.h"

// ======================================================================
// GizmoRenderer - Draws the 3D translate/rotate gizmo overlay using D3D9.
//
// Renders colored arrows (X=red, Y=green, Z=blue) at the target object.
// Also handles screen-space hit testing for axis selection.
// ======================================================================

// Vertex format: position + diffuse color
struct GizmoVertex {
	float x, y, z;
	DWORD color;
};

// Screen-space vertex for 2D HUD elements
struct HudVertex {
	float x, y, z, rhw;
	DWORD color;
};

#define GIZMO_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)
#define HUD_FVF   (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

// Colors (ARGB)
#define COLOR_AXIS_X          0xFFFF2222  // Red
#define COLOR_AXIS_Y          0xFF22FF22  // Green
#define COLOR_AXIS_Z          0xFF4444FF  // Blue
#define COLOR_AXIS_X_HOVER    0xFFFF8888
#define COLOR_AXIS_Y_HOVER    0xFF88FF88
#define COLOR_AXIS_Z_HOVER    0xFF8888FF
#define COLOR_AXIS_X_ACTIVE   0xFFFFFF44
#define COLOR_AXIS_Y_ACTIVE   0xFFFFFF44
#define COLOR_AXIS_Z_ACTIVE   0xFFFFFF44
#define COLOR_CENTER          0xFFFFFFFF  // White
#define COLOR_WIREFRAME       0x88FFFFFF  // Semi-transparent white
#define COLOR_HUD_BG          0xAA000000  // Semi-transparent black
#define COLOR_HUD_TEXT_NORMAL 0xFFCCCCCC
#define COLOR_ROTATION_RING   0x88FFAA00  // Semi-transparent orange

class GizmoRenderer {
public:
	// Called every frame from EndScene hook
	static void render(IDirect3DDevice9* device);

	// Called on device lost/restored
	static void onDeviceLost();
	static void onDeviceRestored(IDirect3DDevice9* device);

	// Hit testing: check which axis the mouse is hovering over
	static void updateHoverTest(IDirect3DDevice9* device, int mouseX, int mouseY);

private:
	// Rendering sub-functions
	static void renderTranslateGizmo(IDirect3DDevice9* device, const Vector& worldPos);
	static void renderRotateGizmo(IDirect3DDevice9* device, const Vector& worldPos);
	static void renderWireframeHighlight(IDirect3DDevice9* device, const Vector& worldPos);
	static void renderHUD(IDirect3DDevice9* device);

	// Geometry generation
	static void generateArrow(GizmoVertex* verts, int& count,
		const Vector& origin, const Vector& direction, float length, DWORD color);
	static void generateCone(GizmoVertex* verts, int& count,
		const Vector& tip, const Vector& direction, float height, float radius,
		int segments, DWORD color);
	static void generateCircle(GizmoVertex* verts, int& count,
		const Vector& center, int axisIdx, float radius, int segments, DWORD color);
	static void generateWireBox(GizmoVertex* verts, int& count,
		const Vector& center, float halfSize, DWORD color);

	// Matrix helpers
	static bool projectToScreen(IDirect3DDevice9* device, const Vector& worldPos,
		float& screenX, float& screenY);
	static float distanceToScreenLine(float px, float py,
		float x1, float y1, float x2, float y2);

	// State
	static float s_gizmoScale;
	static const float ARROW_LENGTH;
	static const float CONE_HEIGHT;
	static const float CONE_RADIUS;
	static const int CONE_SEGMENTS;
	static const float RING_RADIUS;
	static const int RING_SEGMENTS;
	static const float HIT_THRESHOLD;
	static const float WIREFRAME_SIZE;
};
