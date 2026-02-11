#include "stdafx.h"
#include "GizmoRenderer.h"
#include "DecorationMode.h"
#include "Game.h"

#include <cmath>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ======================================================================
// Constants
// ======================================================================
float GizmoRenderer::s_gizmoScale = 1.0f;
const float GizmoRenderer::ARROW_LENGTH = 2.0f;
const float GizmoRenderer::CONE_HEIGHT = 0.4f;
const float GizmoRenderer::CONE_RADIUS = 0.12f;
const int   GizmoRenderer::CONE_SEGMENTS = 8;
const float GizmoRenderer::RING_RADIUS = 1.8f;
const int   GizmoRenderer::RING_SEGMENTS = 32;
const float GizmoRenderer::HIT_THRESHOLD = 12.0f; // pixels
const float GizmoRenderer::WIREFRAME_SIZE = 0.5f;

// ======================================================================
// Helpers for calling D3D9 device methods through vtable
// ======================================================================
static void d3dSetRenderState(IDirect3DDevice9* dev, int state, DWORD value) {
	getD3D9Method<tSetRenderState>(dev, D3D9_VTABLE_SETRENDERSTATE)(dev, state, value);
}

static void d3dSetTexture(IDirect3DDevice9* dev, DWORD stage, void* tex) {
	getD3D9Method<tSetTexture>(dev, D3D9_VTABLE_SETTEXTURE)(dev, stage, tex);
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

static void d3dDrawPrimitiveUP(IDirect3DDevice9* dev, int type, UINT count,
	const void* data, UINT stride) {
	getD3D9Method<tDrawPrimitiveUP>(dev, D3D9_VTABLE_DRAWPRIMITIVEUP)(dev, type, count, data, stride);
}

// ======================================================================
// Matrix math helpers
// ======================================================================
static void matrixIdentity(D3DMATRIX_MINIMAL& m) {
	memset(&m, 0, sizeof(m));
	m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
}

static void matrixTranslation(D3DMATRIX_MINIMAL& m, float x, float y, float z) {
	matrixIdentity(m);
	m.m[3][0] = x;
	m.m[3][1] = y;
	m.m[3][2] = z;
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

// ======================================================================
// Project a world-space point to screen coordinates
// ======================================================================
bool GizmoRenderer::projectToScreen(IDirect3DDevice9* device,
	const Vector& worldPos, float& screenX, float& screenY) {

	D3DMATRIX_MINIMAL view, proj;
	D3DVIEWPORT9_MINIMAL vp;

	d3dGetTransform(device, D3DTS_VIEW, &view);
	d3dGetTransform(device, D3DTS_PROJECTION, &proj);
	d3dGetViewport(device, &vp);

	// Transform world -> view -> projection
	D3DMATRIX_MINIMAL viewProj;
	matrixMultiply(viewProj, view, proj);

	float x = worldPos.getX();
	float y = worldPos.getY();
	float z = worldPos.getZ();

	float clipX = x * viewProj.m[0][0] + y * viewProj.m[1][0] + z * viewProj.m[2][0] + viewProj.m[3][0];
	float clipY = x * viewProj.m[0][1] + y * viewProj.m[1][1] + z * viewProj.m[2][1] + viewProj.m[3][1];
	float clipW = x * viewProj.m[0][3] + y * viewProj.m[1][3] + z * viewProj.m[2][3] + viewProj.m[3][3];

	if (fabsf(clipW) < 0.0001f) return false;
	if (clipW < 0.0f) return false; // behind camera

	float ndcX = clipX / clipW;
	float ndcY = clipY / clipW;

	screenX = (ndcX * 0.5f + 0.5f) * vp.Width + vp.X;
	screenY = (-ndcY * 0.5f + 0.5f) * vp.Height + vp.Y;

	return true;
}

// ======================================================================
// Distance from point to line segment (2D, screen space)
// ======================================================================
float GizmoRenderer::distanceToScreenLine(float px, float py,
	float x1, float y1, float x2, float y2) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	float lenSq = dx * dx + dy * dy;

	if (lenSq < 0.0001f)
		return sqrtf((px - x1) * (px - x1) + (py - y1) * (py - y1));

	float t = ((px - x1) * dx + (py - y1) * dy) / lenSq;
	t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;

	float closestX = x1 + t * dx;
	float closestY = y1 + t * dy;

	return sqrtf((px - closestX) * (px - closestX) + (py - closestY) * (py - closestY));
}

// ======================================================================
// Generate arrow line (2 vertices for a line)
// ======================================================================
void GizmoRenderer::generateArrow(GizmoVertex* verts, int& count,
	const Vector& origin, const Vector& direction, float length, DWORD color) {
	verts[count++] = { origin.getX(), origin.getY(), origin.getZ(), color };
	verts[count++] = {
		origin.getX() + direction.getX() * length,
		origin.getY() + direction.getY() * length,
		origin.getZ() + direction.getZ() * length,
		color
	};
}

// ======================================================================
// Generate cone (arrow head) as triangle fan vertices
// ======================================================================
void GizmoRenderer::generateCone(GizmoVertex* verts, int& count,
	const Vector& tip, const Vector& direction, float height, float radius,
	int segments, DWORD color) {

	// Find two perpendicular vectors to the direction
	Vector up(0, 1, 0);
	if (fabsf(direction.dotProduct(up)) > 0.99f) {
		up = Vector(1, 0, 0);
	}
	Vector right = direction.crossProduct(up);
	float rightLen = right.length();
	if (rightLen > 0.0001f) {
		right = right * (1.0f / rightLen);
	}
	Vector forward = right.crossProduct(direction);
	float forwardLen = forward.length();
	if (forwardLen > 0.0001f) {
		forward = forward * (1.0f / forwardLen);
	}

	Vector base(
		tip.getX() - direction.getX() * height,
		tip.getY() - direction.getY() * height,
		tip.getZ() - direction.getZ() * height
	);

	// Triangle fan: tip, then circle around base
	for (int i = 0; i < segments; i++) {
		float angle1 = (2.0f * M_PI * i) / segments;
		float angle2 = (2.0f * M_PI * (i + 1)) / segments;

		float cos1 = cosf(angle1), sin1 = sinf(angle1);
		float cos2 = cosf(angle2), sin2 = sinf(angle2);

		Vector p1(
			base.getX() + (right.getX() * cos1 + forward.getX() * sin1) * radius,
			base.getY() + (right.getY() * cos1 + forward.getY() * sin1) * radius,
			base.getZ() + (right.getZ() * cos1 + forward.getZ() * sin1) * radius
		);

		Vector p2(
			base.getX() + (right.getX() * cos2 + forward.getX() * sin2) * radius,
			base.getY() + (right.getY() * cos2 + forward.getY() * sin2) * radius,
			base.getZ() + (right.getZ() * cos2 + forward.getZ() * sin2) * radius
		);

		verts[count++] = { tip.getX(), tip.getY(), tip.getZ(), color };
		verts[count++] = { p1.getX(), p1.getY(), p1.getZ(), color };
		verts[count++] = { p2.getX(), p2.getY(), p2.getZ(), color };
	}
}

// ======================================================================
// Generate circle (rotation ring) as line segments
// ======================================================================
void GizmoRenderer::generateCircle(GizmoVertex* verts, int& count,
	const Vector& center, int axisIdx, float radius, int segments, DWORD color) {

	for (int i = 0; i < segments; i++) {
		float angle1 = (2.0f * M_PI * i) / segments;
		float angle2 = (2.0f * M_PI * (i + 1)) / segments;

		float c1 = cosf(angle1) * radius, s1 = sinf(angle1) * radius;
		float c2 = cosf(angle2) * radius, s2 = sinf(angle2) * radius;

		Vector p1, p2;
		switch (axisIdx) {
		case 0: // X axis - ring in YZ plane
			p1 = Vector(center.getX(), center.getY() + c1, center.getZ() + s1);
			p2 = Vector(center.getX(), center.getY() + c2, center.getZ() + s2);
			break;
		case 1: // Y axis - ring in XZ plane
			p1 = Vector(center.getX() + c1, center.getY(), center.getZ() + s1);
			p2 = Vector(center.getX() + c2, center.getY(), center.getZ() + s2);
			break;
		case 2: // Z axis - ring in XY plane
			p1 = Vector(center.getX() + c1, center.getY() + s1, center.getZ());
			p2 = Vector(center.getX() + c2, center.getY() + s2, center.getZ());
			break;
		}

		verts[count++] = { p1.getX(), p1.getY(), p1.getZ(), color };
		verts[count++] = { p2.getX(), p2.getY(), p2.getZ(), color };
	}
}

// ======================================================================
// Generate wireframe box
// ======================================================================
void GizmoRenderer::generateWireBox(GizmoVertex* verts, int& count,
	const Vector& center, float halfSize, DWORD color) {

	float x = center.getX(), y = center.getY(), z = center.getZ();
	float h = halfSize;

	// 8 corners
	Vector corners[8] = {
		{ x - h, y - h, z - h }, { x + h, y - h, z - h },
		{ x + h, y + h, z - h }, { x - h, y + h, z - h },
		{ x - h, y - h, z + h }, { x + h, y - h, z + h },
		{ x + h, y + h, z + h }, { x - h, y + h, z + h }
	};

	// 12 edges
	int edges[][2] = {
		{0,1},{1,2},{2,3},{3,0},  // front face
		{4,5},{5,6},{6,7},{7,4},  // back face
		{0,4},{1,5},{2,6},{3,7}   // connecting edges
	};

	for (int i = 0; i < 12; i++) {
		verts[count++] = { corners[edges[i][0]].getX(), corners[edges[i][0]].getY(), corners[edges[i][0]].getZ(), color };
		verts[count++] = { corners[edges[i][1]].getX(), corners[edges[i][1]].getY(), corners[edges[i][1]].getZ(), color };
	}
}

// ======================================================================
// Main render function - called from EndScene hook
// ======================================================================
void GizmoRenderer::render(IDirect3DDevice9* device) {
	auto& deco = DecorationMode::getInstance();
	if (!deco.isActive()) return;

	Object* target = deco.getTargetObject();
	if (!target) return;

	Vector worldPos = target->getPosition_p();

	// Save render states we'll modify
	// We use a simple approach: set states, render, then restore
	// Note: In a production build, use CreateStateBlock for full save/restore

	// Set render states for gizmo rendering
	d3dSetTexture(device, 0, nullptr);
	d3dSetRenderState(device, D3DRS_LIGHTING, 0);
	d3dSetRenderState(device, D3DRS_ZENABLE, 0);         // Draw on top
	d3dSetRenderState(device, D3DRS_ZWRITEENABLE, 0);
	d3dSetRenderState(device, D3DRS_ALPHABLENDENABLE, 1);
	d3dSetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	d3dSetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	d3dSetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
	d3dSetRenderState(device, D3DRS_FILLMODE, D3DFILL_SOLID);
	d3dSetRenderState(device, D3DRS_ANTIALIASEDLINEENABLE, 1);

	// Set identity world transform (gizmo geometry is in world space)
	D3DMATRIX_MINIMAL identity;
	matrixIdentity(identity);
	d3dSetTransform(device, D3DTS_WORLD, &identity);

	// Render wireframe highlight around the object
	renderWireframeHighlight(device, worldPos);

	// Render the appropriate gizmo tool
	if (deco.getGizmoTool() == DecorationMode::GizmoTool::Translate) {
		renderTranslateGizmo(device, worldPos);
	} else {
		renderRotateGizmo(device, worldPos);
	}

	// Render HUD overlay with position info
	renderHUD(device);

	// Restore some key render states
	d3dSetRenderState(device, D3DRS_ZENABLE, 1);
	d3dSetRenderState(device, D3DRS_ZWRITEENABLE, 1);
	d3dSetRenderState(device, D3DRS_ALPHABLENDENABLE, 0);
	d3dSetRenderState(device, D3DRS_LIGHTING, 1);
	d3dSetRenderState(device, D3DRS_CULLMODE, 2); // D3DCULL_CCW
	d3dSetRenderState(device, D3DRS_ANTIALIASEDLINEENABLE, 0);
}

// ======================================================================
// Render the translate gizmo (3 arrows: X=red, Y=green, Z=blue)
// ======================================================================
void GizmoRenderer::renderTranslateGizmo(IDirect3DDevice9* device, const Vector& worldPos) {
	auto& deco = DecorationMode::getInstance();
	auto selected = deco.getSelectedAxis();
	auto hovered = deco.getHoveredAxis();

	float len = ARROW_LENGTH * s_gizmoScale;
	float coneH = CONE_HEIGHT * s_gizmoScale;
	float coneR = CONE_RADIUS * s_gizmoScale;

	// Determine colors based on hover/selection state
	DWORD colorX = (selected == DecorationMode::GizmoAxis::AxisX) ? COLOR_AXIS_X_ACTIVE :
		(hovered == DecorationMode::GizmoAxis::AxisX) ? COLOR_AXIS_X_HOVER : COLOR_AXIS_X;
	DWORD colorY = (selected == DecorationMode::GizmoAxis::AxisY) ? COLOR_AXIS_Y_ACTIVE :
		(hovered == DecorationMode::GizmoAxis::AxisY) ? COLOR_AXIS_Y_HOVER : COLOR_AXIS_Y;
	DWORD colorZ = (selected == DecorationMode::GizmoAxis::AxisZ) ? COLOR_AXIS_Z_ACTIVE :
		(hovered == DecorationMode::GizmoAxis::AxisZ) ? COLOR_AXIS_Z_HOVER : COLOR_AXIS_Z;

	// Arrow lines (6 vertices for 3 lines)
	GizmoVertex lines[6];
	int lineCount = 0;

	generateArrow(lines, lineCount, worldPos, Vector(1, 0, 0), len, colorX); // X axis
	generateArrow(lines, lineCount, worldPos, Vector(0, 1, 0), len, colorY); // Y axis
	generateArrow(lines, lineCount, worldPos, Vector(0, 0, 1), len, colorZ); // Z axis

	d3dSetFVF(device, GIZMO_FVF);
	d3dDrawPrimitiveUP(device, D3DPT_LINELIST, 3, lines, sizeof(GizmoVertex));

	// Arrow heads (cones)
	GizmoVertex cones[CONE_SEGMENTS * 3 * 3]; // 3 cones, each with CONE_SEGMENTS triangles
	int coneCount = 0;

	Vector tipX(worldPos.getX() + len, worldPos.getY(), worldPos.getZ());
	Vector tipY(worldPos.getX(), worldPos.getY() + len, worldPos.getZ());
	Vector tipZ(worldPos.getX(), worldPos.getY(), worldPos.getZ() + len);

	generateCone(cones, coneCount, tipX, Vector(1, 0, 0), coneH, coneR, CONE_SEGMENTS, colorX);
	generateCone(cones, coneCount, tipY, Vector(0, 1, 0), coneH, coneR, CONE_SEGMENTS, colorY);
	generateCone(cones, coneCount, tipZ, Vector(0, 0, 1), coneH, coneR, CONE_SEGMENTS, colorZ);

	d3dDrawPrimitiveUP(device, D3DPT_TRIANGLELIST, coneCount / 3, cones, sizeof(GizmoVertex));

	// Center point (small wireframe box)
	GizmoVertex center[24]; // 12 edges * 2 vertices
	int centerCount = 0;
	generateWireBox(center, centerCount, worldPos, 0.08f * s_gizmoScale, COLOR_CENTER);
	d3dDrawPrimitiveUP(device, D3DPT_LINELIST, centerCount / 2, center, sizeof(GizmoVertex));
}

// ======================================================================
// Render the rotation gizmo (3 rings: X=red, Y=green, Z=blue)
// ======================================================================
void GizmoRenderer::renderRotateGizmo(IDirect3DDevice9* device, const Vector& worldPos) {
	auto& deco = DecorationMode::getInstance();
	auto selected = deco.getSelectedAxis();
	auto hovered = deco.getHoveredAxis();

	float radius = RING_RADIUS * s_gizmoScale;

	DWORD colorX = (selected == DecorationMode::GizmoAxis::AxisX) ? COLOR_AXIS_X_ACTIVE :
		(hovered == DecorationMode::GizmoAxis::AxisX) ? COLOR_AXIS_X_HOVER : COLOR_AXIS_X;
	DWORD colorY = (selected == DecorationMode::GizmoAxis::AxisY) ? COLOR_AXIS_Y_ACTIVE :
		(hovered == DecorationMode::GizmoAxis::AxisY) ? COLOR_AXIS_Y_HOVER : COLOR_AXIS_Y;
	DWORD colorZ = (selected == DecorationMode::GizmoAxis::AxisZ) ? COLOR_AXIS_Z_ACTIVE :
		(hovered == DecorationMode::GizmoAxis::AxisZ) ? COLOR_AXIS_Z_HOVER : COLOR_AXIS_Z;

	// 3 rings * RING_SEGMENTS * 2 vertices per segment
	const int maxVerts = RING_SEGMENTS * 2 * 3;
	GizmoVertex rings[RING_SEGMENTS * 2 * 3];
	int ringCount = 0;

	generateCircle(rings, ringCount, worldPos, 0, radius, RING_SEGMENTS, colorX); // X ring
	generateCircle(rings, ringCount, worldPos, 1, radius, RING_SEGMENTS, colorY); // Y ring
	generateCircle(rings, ringCount, worldPos, 2, radius, RING_SEGMENTS, colorZ); // Z ring

	d3dSetFVF(device, GIZMO_FVF);
	d3dDrawPrimitiveUP(device, D3DPT_LINELIST, ringCount / 2, rings, sizeof(GizmoVertex));

	// Center marker
	GizmoVertex center[24];
	int centerCount = 0;
	generateWireBox(center, centerCount, worldPos, 0.08f * s_gizmoScale, COLOR_CENTER);
	d3dDrawPrimitiveUP(device, D3DPT_LINELIST, centerCount / 2, center, sizeof(GizmoVertex));
}

// ======================================================================
// Render wireframe highlight around the selected object
// ======================================================================
void GizmoRenderer::renderWireframeHighlight(IDirect3DDevice9* device, const Vector& worldPos) {
	GizmoVertex box[24];
	int count = 0;
	generateWireBox(box, count, worldPos, WIREFRAME_SIZE * s_gizmoScale, COLOR_WIREFRAME);

	d3dSetFVF(device, GIZMO_FVF);
	d3dDrawPrimitiveUP(device, D3DPT_LINELIST, count / 2, box, sizeof(GizmoVertex));
}

// ======================================================================
// Render HUD overlay with position/state info
// ======================================================================
void GizmoRenderer::renderHUD(IDirect3DDevice9* device) {
	auto& deco = DecorationMode::getInstance();
	Object* target = deco.getTargetObject();
	if (!target) return;

	D3DVIEWPORT9_MINIMAL vp;
	d3dGetViewport(device, &vp);

	// Draw a small background box in the top-left corner
	float hudX = 10.0f;
	float hudY = 10.0f;
	float hudW = 280.0f;
	float hudH = 60.0f;

	HudVertex bgQuad[6] = {
		{ hudX,        hudY,        0.0f, 1.0f, COLOR_HUD_BG },
		{ hudX + hudW, hudY,        0.0f, 1.0f, COLOR_HUD_BG },
		{ hudX,        hudY + hudH, 0.0f, 1.0f, COLOR_HUD_BG },
		{ hudX + hudW, hudY,        0.0f, 1.0f, COLOR_HUD_BG },
		{ hudX + hudW, hudY + hudH, 0.0f, 1.0f, COLOR_HUD_BG },
		{ hudX,        hudY + hudH, 0.0f, 1.0f, COLOR_HUD_BG },
	};

	// Save and set 2D rendering states
	D3DMATRIX_MINIMAL savedView, savedProj, savedWorld;
	d3dGetTransform(device, D3DTS_VIEW, &savedView);
	d3dGetTransform(device, D3DTS_PROJECTION, &savedProj);
	d3dGetTransform(device, D3DTS_WORLD, &savedWorld);

	D3DMATRIX_MINIMAL identity;
	matrixIdentity(identity);
	d3dSetTransform(device, D3DTS_VIEW, &identity);
	d3dSetTransform(device, D3DTS_PROJECTION, &identity);
	d3dSetTransform(device, D3DTS_WORLD, &identity);

	d3dSetFVF(device, HUD_FVF);
	d3dDrawPrimitiveUP(device, D3DPT_TRIANGLELIST, 2, bgQuad, sizeof(HudVertex));

	// Draw border
	HudVertex border[8] = {
		{ hudX,        hudY,        0.0f, 1.0f, COLOR_AXIS_Y },
		{ hudX + hudW, hudY,        0.0f, 1.0f, COLOR_AXIS_Y },
		{ hudX + hudW, hudY,        0.0f, 1.0f, COLOR_AXIS_Y },
		{ hudX + hudW, hudY + hudH, 0.0f, 1.0f, COLOR_AXIS_Y },
		{ hudX + hudW, hudY + hudH, 0.0f, 1.0f, COLOR_AXIS_Y },
		{ hudX,        hudY + hudH, 0.0f, 1.0f, COLOR_AXIS_Y },
		{ hudX,        hudY + hudH, 0.0f, 1.0f, COLOR_AXIS_Y },
		{ hudX,        hudY,        0.0f, 1.0f, COLOR_AXIS_Y },
	};
	d3dDrawPrimitiveUP(device, D3DPT_LINELIST, 4, border, sizeof(HudVertex));

	// Restore transforms
	d3dSetTransform(device, D3DTS_VIEW, &savedView);
	d3dSetTransform(device, D3DTS_PROJECTION, &savedProj);
	d3dSetTransform(device, D3DTS_WORLD, &savedWorld);
}

// ======================================================================
// Update hover test - determine which axis the mouse is closest to
// ======================================================================
void GizmoRenderer::updateHoverTest(IDirect3DDevice9* device, int mouseX, int mouseY) {
	auto& deco = DecorationMode::getInstance();
	if (!deco.isActive() || deco.getState() == DecorationMode::State::Dragging) return;

	Object* target = deco.getTargetObject();
	if (!target) return;

	Vector worldPos = target->getPosition_p();
	float len = ARROW_LENGTH * s_gizmoScale;

	// Project gizmo origin and axis endpoints to screen space
	float originSX, originSY;
	if (!projectToScreen(device, worldPos, originSX, originSY)) {
		deco.setHoveredAxis(DecorationMode::GizmoAxis::None);
		return;
	}

	Vector tipX(worldPos.getX() + len, worldPos.getY(), worldPos.getZ());
	Vector tipY(worldPos.getX(), worldPos.getY() + len, worldPos.getZ());
	Vector tipZ(worldPos.getX(), worldPos.getY(), worldPos.getZ() + len);

	float tipXsx, tipXsy, tipYsx, tipYsy, tipZsx, tipZsy;
	bool validX = projectToScreen(device, tipX, tipXsx, tipXsy);
	bool validY = projectToScreen(device, tipY, tipYsx, tipYsy);
	bool validZ = projectToScreen(device, tipZ, tipZsx, tipZsy);

	float mx = static_cast<float>(mouseX);
	float my = static_cast<float>(mouseY);

	float distX = validX ? distanceToScreenLine(mx, my, originSX, originSY, tipXsx, tipXsy) : 9999.0f;
	float distY = validY ? distanceToScreenLine(mx, my, originSX, originSY, tipYsx, tipYsy) : 9999.0f;
	float distZ = validZ ? distanceToScreenLine(mx, my, originSX, originSY, tipZsx, tipZsy) : 9999.0f;

	float minDist = 9999.0f;
	DecorationMode::GizmoAxis closestAxis = DecorationMode::GizmoAxis::None;

	if (distX < minDist && distX < HIT_THRESHOLD) { minDist = distX; closestAxis = DecorationMode::GizmoAxis::AxisX; }
	if (distY < minDist && distY < HIT_THRESHOLD) { minDist = distY; closestAxis = DecorationMode::GizmoAxis::AxisY; }
	if (distZ < minDist && distZ < HIT_THRESHOLD) { minDist = distZ; closestAxis = DecorationMode::GizmoAxis::AxisZ; }

	deco.setHoveredAxis(closestAxis);
}

// ======================================================================
// Device lost/restored handlers
// ======================================================================
void GizmoRenderer::onDeviceLost() {
	// Nothing to release since we use DrawPrimitiveUP (no persistent buffers)
}

void GizmoRenderer::onDeviceRestored(IDirect3DDevice9* device) {
	// Nothing to recreate
}
