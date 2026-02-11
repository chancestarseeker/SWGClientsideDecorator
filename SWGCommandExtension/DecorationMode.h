#pragma once

#include "Object.h"
#include "NetworkId.h"
#include "Vector.h"
#include "Transform.h"

// ======================================================================
// DecorationMode - Core state machine for the furniture decoration plugin.
//
// Phase 1: Keyboard nudge commands via /deco
// Phase 2: D3D9 overlay HUD + wireframe highlight
// Phase 3: 3D gizmo with mouse drag
// Phase 4: Rotation rings, undo/redo, snap-to-grid
// ======================================================================

class DecorationMode {
public:
	enum class State {
		Idle,        // Not decorating
		Active,      // Decoration mode on, keyboard nudge
		Dragging,    // Mouse drag in progress (Phase 3)
		Committing   // Sending command to server
	};

	enum class GizmoAxis {
		None,
		AxisX,
		AxisY,
		AxisZ
	};

	enum class GizmoTool {
		Translate,
		Rotate
	};

	static DecorationMode& getInstance();

	// --- State management ---
	bool isActive() const { return m_state != State::Idle; }
	State getState() const { return m_state; }

	// --- Activation ---
	bool activate();
	void deactivate();
	void toggle();

	// --- Keyboard movement (Phase 1) ---
	void moveForward(float distance);
	void moveBack(float distance);
	void moveLeft(float distance);
	void moveRight(float distance);
	void moveUp(float distance);
	void moveDown(float distance);
	void rotate(float degrees);
	void rotatePitch(float degrees);
	void rotateRoll(float degrees);

	// --- Step size ---
	float getStepSize() const { return m_stepSize; }
	void setStepSize(float size);
	float getRotateStep() const { return m_rotateStep; }
	void setRotateStep(float deg);

	// --- Target info ---
	Object* getTargetObject() const;
	uint64_t getTargetId() const { return m_targetId; }
	Vector getTargetPosition() const;
	void showPositionInfo() const;

	// --- Gizmo (Phase 3) ---
	bool isGizmoEnabled() const { return m_gizmoEnabled; }
	void setGizmoEnabled(bool enabled) { m_gizmoEnabled = enabled; }
	GizmoAxis getSelectedAxis() const { return m_selectedAxis; }
	GizmoAxis getHoveredAxis() const { return m_hoveredAxis; }
	void setHoveredAxis(GizmoAxis axis) { m_hoveredAxis = axis; }
	GizmoTool getGizmoTool() const { return m_gizmoTool; }
	void setGizmoTool(GizmoTool tool) { m_gizmoTool = tool; }

	// --- Mouse drag (Phase 3) ---
	void beginDrag(GizmoAxis axis, int mouseX, int mouseY);
	void updateDrag(int mouseX, int mouseY);
	void endDrag();
	void cancelDrag();

	// --- Undo/Redo (Phase 4) ---
	void undo();
	void redo();

	// --- Snap to grid (Phase 4) ---
	bool isSnapEnabled() const { return m_snapEnabled; }
	void setSnapEnabled(bool enabled) { m_snapEnabled = enabled; }
	float getSnapSize() const { return m_snapSize; }
	void setSnapSize(float size) { m_snapSize = size; }

	// --- Command parsing ---
	bool parseCommand(const wchar_t* args);

private:
	DecorationMode();

	void sendMoveCommand(const char* direction, int distance);
	void sendRotateCommand(float degrees);
	void sendRotateAxisCommand(const char* axis, float degrees);
	void pushUndoState();
	void showHelp() const;

	State m_state = State::Idle;
	uint64_t m_targetId = 0;
	float m_stepSize = 1.0f;
	float m_rotateStep = 15.0f;
	bool m_gizmoEnabled = false;
	bool m_snapEnabled = false;
	float m_snapSize = 0.5f;

	GizmoAxis m_selectedAxis = GizmoAxis::None;
	GizmoAxis m_hoveredAxis = GizmoAxis::None;
	GizmoTool m_gizmoTool = GizmoTool::Translate;

	// Drag state
	Vector m_dragStartPos;
	int m_dragStartMouseX = 0;
	int m_dragStartMouseY = 0;
	Transform m_originalTransform;

	// Undo stack (Phase 4)
	static const int MAX_UNDO = 50;
	struct UndoEntry {
		uint64_t targetId = 0;
		Vector position;
		bool valid = false;
	};
	UndoEntry m_undoStack[MAX_UNDO];
	int m_undoIndex = -1;
	int m_undoCount = 0;
};
