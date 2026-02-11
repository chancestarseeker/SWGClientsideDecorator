#include "stdafx.h"
#include "DecorationMode.h"

#include "Game.h"
#include "CreatureObject.h"
#include "ClientCommandQueue.h"
#include "CachedNetworkId.h"

#include <cmath>
#include <cstdio>
#include <cwchar>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ======================================================================
// Helper: wide string to float
// ======================================================================
static float wstof(const wchar_t* str) {
	try { return std::stof(str); }
	catch (...) { return 0.0f; }
}

// ======================================================================
// Helper: wide string to int
// ======================================================================
static int wstoi(const wchar_t* str) {
	try { return std::stoi(str); }
	catch (...) { return 0; }
}

// ======================================================================
// Helper: case-insensitive wide string compare
// ======================================================================
static bool wcieq(const wchar_t* a, const wchar_t* b) {
	return _wcsicmp(a, b) == 0;
}

// ======================================================================
// Singleton
// ======================================================================
DecorationMode& DecorationMode::getInstance() {
	static DecorationMode instance;
	return instance;
}

DecorationMode::DecorationMode() {
	memset(m_undoStack, 0, sizeof(m_undoStack));
}

// ======================================================================
// Activation
// ======================================================================
bool DecorationMode::activate() {
	CreatureObject* creature = Game::getPlayerCreature();
	if (!creature) {
		Game::debugPrintUi("[Deco] Error: player creature not found.");
		return false;
	}

	auto& lookAtTarget = creature->getLookAtTarget();
	Object* target = lookAtTarget.getObject();

	if (!target) {
		Game::debugPrintUi("[Deco] No target selected. Look at a piece of furniture first.");
		return false;
	}

	m_targetId = target->getObjectID().get();
	m_state = State::Active;
	m_selectedAxis = GizmoAxis::None;
	m_hoveredAxis = GizmoAxis::None;
	m_originalTransform = target->getTransform_o2p();

	auto pos = target->getPosition_p();
	char msg[512];
	sprintf_s(msg, sizeof(msg),
		"[Deco] \\#00ff00Decoration mode ACTIVATED\\#ffffff on object %lld\n"
		"[Deco] Position: (%.2f, %.2f, %.2f)  Step: %.0f  RotateStep: %.0f\n"
		"[Deco] Type /deco help for commands.",
		m_targetId, pos.getX(), pos.getY(), pos.getZ(), m_stepSize, m_rotateStep);
	Game::debugPrintUi(msg);

	return true;
}

void DecorationMode::deactivate() {
	if (m_state == State::Dragging) {
		cancelDrag();
	}
	m_state = State::Idle;
	m_targetId = 0;
	m_selectedAxis = GizmoAxis::None;
	m_hoveredAxis = GizmoAxis::None;
	Game::debugPrintUi("[Deco] \\#ff4444Decoration mode DEACTIVATED.\\#ffffff");
}

void DecorationMode::toggle() {
	if (isActive()) {
		deactivate();
	} else {
		activate();
	}
}

// ======================================================================
// Target access
// ======================================================================
Object* DecorationMode::getTargetObject() const {
	if (!isActive()) return nullptr;

	CreatureObject* creature = Game::getPlayerCreature();
	if (!creature) return nullptr;

	auto& lookAtTarget = creature->getLookAtTarget();
	Object* obj = lookAtTarget.getObject();

	if (obj && obj->getObjectID().get() == m_targetId) {
		return obj;
	}

	return nullptr;
}

Vector DecorationMode::getTargetPosition() const {
	Object* target = getTargetObject();
	if (target) {
		return target->getPosition_p();
	}
	return Vector(0, 0, 0);
}

void DecorationMode::showPositionInfo() const {
	Object* target = getTargetObject();
	if (target) {
		auto pos = target->getPosition_p();
		const Transform& tr = target->getTransform_o2p();

		float yaw = atan2f(tr.getLocalFrameK_p().getX(), tr.getLocalFrameK_p().getZ()) * (180.0f / M_PI);

		char msg[512];
		sprintf_s(msg, sizeof(msg),
			"[Deco] Object %lld\n"
			"[Deco]   Position: (%.3f, %.3f, %.3f)\n"
			"[Deco]   Yaw: %.1f deg  |  Step: %.0f  |  RotStep: %.0f\n"
			"[Deco]   Snap: %s (%.1f)  |  Tool: %s",
			m_targetId,
			pos.getX(), pos.getY(), pos.getZ(),
			yaw, m_stepSize, m_rotateStep,
			m_snapEnabled ? "ON" : "OFF", m_snapSize,
			m_gizmoTool == GizmoTool::Translate ? "Translate" : "Rotate");
		Game::debugPrintUi(msg);
	} else {
		Game::debugPrintUi("[Deco] Target object not found. Make sure you are looking at it.");
	}
}

// ======================================================================
// Step sizes
// ======================================================================
void DecorationMode::setStepSize(float size) {
	if (size < 1.0f) size = 1.0f;
	if (size > 500.0f) size = 500.0f;
	m_stepSize = size;
	char msg[128];
	sprintf_s(msg, sizeof(msg), "[Deco] Move step size set to %.0f", m_stepSize);
	Game::debugPrintUi(msg);
}

void DecorationMode::setRotateStep(float deg) {
	if (deg < 1.0f) deg = 1.0f;
	if (deg > 360.0f) deg = 360.0f;
	m_rotateStep = deg;
	char msg[128];
	sprintf_s(msg, sizeof(msg), "[Deco] Rotate step size set to %.1f degrees", m_rotateStep);
	Game::debugPrintUi(msg);
}

// ======================================================================
// Server commands
// ======================================================================
void DecorationMode::sendMoveCommand(const char* direction, int distance) {
	if (distance < 1) distance = 1;
	if (distance > 500) distance = 500;

	constexpr uint32_t moveHash = soe::string::hashCode("moveFurniture");

	char paramStr[64];
	sprintf_s(paramStr, sizeof(paramStr), "%s %d", direction, distance);

	soe::unicode params = paramStr;
	ClientCommandQueue::enqueueCommand(moveHash, NetworkId(m_targetId), params);

	char msg[128];
	sprintf_s(msg, sizeof(msg), "[Deco] \\#aaaaff Move %s %d\\#ffffff", direction, distance);
	Game::debugPrintUi(msg);
}

void DecorationMode::sendRotateCommand(float degrees) {
	constexpr uint32_t rotateHash = soe::string::hashCode("rotateFurniture");

	char paramStr[64];
	sprintf_s(paramStr, sizeof(paramStr), "%.1f", degrees);

	soe::unicode params = paramStr;
	ClientCommandQueue::enqueueCommand(rotateHash, NetworkId(m_targetId), params);

	char msg[128];
	sprintf_s(msg, sizeof(msg), "[Deco] \\#aaffaa Rotate yaw %.1f deg\\#ffffff", degrees);
	Game::debugPrintUi(msg);
}

void DecorationMode::sendRotateAxisCommand(const char* axis, float degrees) {
	constexpr uint32_t rotateHash = soe::string::hashCode("rotateFurniture");

	char paramStr[64];
	sprintf_s(paramStr, sizeof(paramStr), "%s %.1f", axis, degrees);

	soe::unicode params = paramStr;
	ClientCommandQueue::enqueueCommand(rotateHash, NetworkId(m_targetId), params);

	char msg[128];
	sprintf_s(msg, sizeof(msg), "[Deco] \\#aaffaa Rotate %s %.1f deg\\#ffffff", axis, degrees);
	Game::debugPrintUi(msg);
}

// ======================================================================
// Movement (Phase 1)
// ======================================================================
void DecorationMode::moveForward(float distance) {
	if (!isActive()) return;
	pushUndoState();
	sendMoveCommand("forward", static_cast<int>(distance));
}

void DecorationMode::moveBack(float distance) {
	if (!isActive()) return;
	pushUndoState();
	sendMoveCommand("back", static_cast<int>(distance));
}

void DecorationMode::moveLeft(float distance) {
	if (!isActive()) return;
	pushUndoState();
	sendMoveCommand("left", static_cast<int>(distance));
}

void DecorationMode::moveRight(float distance) {
	if (!isActive()) return;
	pushUndoState();
	sendMoveCommand("right", static_cast<int>(distance));
}

void DecorationMode::moveUp(float distance) {
	if (!isActive()) return;
	pushUndoState();
	sendMoveCommand("up", static_cast<int>(distance));
}

void DecorationMode::moveDown(float distance) {
	if (!isActive()) return;
	pushUndoState();
	sendMoveCommand("down", static_cast<int>(distance));
}

void DecorationMode::rotate(float degrees) {
	if (!isActive()) return;
	pushUndoState();
	sendRotateCommand(degrees);
}

void DecorationMode::rotatePitch(float degrees) {
	if (!isActive()) return;
	pushUndoState();
	sendRotateAxisCommand("pitch", degrees);
}

void DecorationMode::rotateRoll(float degrees) {
	if (!isActive()) return;
	pushUndoState();
	sendRotateAxisCommand("roll", degrees);
}

// ======================================================================
// Mouse drag (Phase 3)
// ======================================================================
void DecorationMode::beginDrag(GizmoAxis axis, int mouseX, int mouseY) {
	if (!isActive()) return;

	Object* target = getTargetObject();
	if (!target) return;

	pushUndoState();

	m_state = State::Dragging;
	m_selectedAxis = axis;
	m_dragStartMouseX = mouseX;
	m_dragStartMouseY = mouseY;
	m_dragStartPos = target->getPosition_p();
	m_originalTransform = target->getTransform_o2p();
}

void DecorationMode::updateDrag(int mouseX, int mouseY) {
	if (m_state != State::Dragging) return;

	Object* target = getTargetObject();
	if (!target) {
		cancelDrag();
		return;
	}

	int dx = mouseX - m_dragStartMouseX;
	int dy = mouseY - m_dragStartMouseY;

	float sensitivity = 0.02f;
	float deltaH = dx * sensitivity;
	float deltaV = -dy * sensitivity;

	Vector newPos = m_dragStartPos;
	switch (m_selectedAxis) {
	case GizmoAxis::AxisX:
		newPos.setX(m_dragStartPos.getX() + deltaH);
		break;
	case GizmoAxis::AxisY:
		newPos.setY(m_dragStartPos.getY() + deltaV);
		break;
	case GizmoAxis::AxisZ:
		newPos.setZ(m_dragStartPos.getZ() + deltaH);
		break;
	default:
		break;
	}

	if (m_snapEnabled && m_snapSize > 0.0f) {
		newPos.setX(floorf(newPos.getX() / m_snapSize + 0.5f) * m_snapSize);
		newPos.setY(floorf(newPos.getY() / m_snapSize + 0.5f) * m_snapSize);
		newPos.setZ(floorf(newPos.getZ() / m_snapSize + 0.5f) * m_snapSize);
	}

	// Client-side preview
	target->setPosition_p(newPos);
}

void DecorationMode::endDrag() {
	if (m_state != State::Dragging) return;

	Object* target = getTargetObject();
	if (target) {
		Vector currentPos = target->getPosition_p();
		Vector delta = currentPos - m_dragStartPos;

		// Revert client-side preview before sending server commands
		target->setPosition_p(m_dragStartPos);

		m_state = State::Committing;

		// Decompose world-space delta into axis-aligned move commands
		int dx = static_cast<int>(fabsf(delta.getX()) + 0.5f);
		int dy = static_cast<int>(fabsf(delta.getY()) + 0.5f);
		int dz = static_cast<int>(fabsf(delta.getZ()) + 0.5f);

		if (dx > 0) sendMoveCommand(delta.getX() > 0 ? "right" : "left", dx);
		if (dy > 0) sendMoveCommand(delta.getY() > 0 ? "up" : "down", dy);
		if (dz > 0) sendMoveCommand(delta.getZ() > 0 ? "forward" : "back", dz);
	}

	m_state = State::Active;
	m_selectedAxis = GizmoAxis::None;
}

void DecorationMode::cancelDrag() {
	if (m_state != State::Dragging) return;

	Object* target = getTargetObject();
	if (target) {
		target->setTransform_o2p(m_originalTransform);
	}

	m_state = State::Active;
	m_selectedAxis = GizmoAxis::None;
	Game::debugPrintUi("[Deco] Drag cancelled.");
}

// ======================================================================
// Undo (Phase 4)
// ======================================================================
void DecorationMode::pushUndoState() {
	Object* target = getTargetObject();
	if (!target) return;

	m_undoIndex = (m_undoIndex + 1) % MAX_UNDO;
	m_undoStack[m_undoIndex].targetId = m_targetId;
	m_undoStack[m_undoIndex].position = target->getPosition_p();
	m_undoStack[m_undoIndex].valid = true;

	if (m_undoCount < MAX_UNDO)
		m_undoCount++;
}

void DecorationMode::undo() {
	if (m_undoCount <= 0 || m_undoIndex < 0) {
		Game::debugPrintUi("[Deco] Nothing to undo.");
		return;
	}

	auto& entry = m_undoStack[m_undoIndex];
	if (entry.valid) {
		char msg[256];
		sprintf_s(msg, sizeof(msg),
			"[Deco] Previous position was (%.2f, %.2f, %.2f). Use /deco move commands to return.",
			entry.position.getX(), entry.position.getY(), entry.position.getZ());
		Game::debugPrintUi(msg);

		m_undoIndex--;
		if (m_undoIndex < 0) m_undoIndex = MAX_UNDO - 1;
		m_undoCount--;
	}
}

void DecorationMode::redo() {
	Game::debugPrintUi("[Deco] Redo not yet implemented.");
}

// ======================================================================
// Help text
// ======================================================================
void DecorationMode::showHelp() const {
	Game::debugPrintUi("\\#00ccff[Decoration Mode Help]\\#ffffff");
	Game::debugPrintUi("/deco              - Toggle decoration mode on look-at target");
	Game::debugPrintUi("/deco on           - Activate decoration mode");
	Game::debugPrintUi("/deco off          - Deactivate decoration mode");
	Game::debugPrintUi("\\#aaaaff--- Movement ---\\#ffffff");
	Game::debugPrintUi("/deco f [dist]     - Move forward (or: forward)");
	Game::debugPrintUi("/deco b [dist]     - Move back");
	Game::debugPrintUi("/deco l [dist]     - Move left");
	Game::debugPrintUi("/deco r [dist]     - Move right");
	Game::debugPrintUi("/deco u [dist]     - Move up");
	Game::debugPrintUi("/deco d [dist]     - Move down");
	Game::debugPrintUi("\\#aaffaa--- Rotation ---\\#ffffff");
	Game::debugPrintUi("/deco rot [deg]    - Rotate yaw (or: rotate)");
	Game::debugPrintUi("/deco pitch [deg]  - Rotate pitch");
	Game::debugPrintUi("/deco roll [deg]   - Rotate roll");
	Game::debugPrintUi("\\#ffaaaa--- Settings ---\\#ffffff");
	Game::debugPrintUi("/deco step [n]     - Set move step size (1-500, default 1)");
	Game::debugPrintUi("/deco rstep [deg]  - Set rotation step (1-360, default 15)");
	Game::debugPrintUi("/deco pos          - Show current position info");
	Game::debugPrintUi("/deco snap [size]  - Toggle snap-to-grid / set snap size");
	Game::debugPrintUi("/deco gizmo        - Toggle 3D gizmo overlay");
	Game::debugPrintUi("/deco tool         - Toggle translate/rotate gizmo tool");
	Game::debugPrintUi("/deco undo         - Show previous position for undo");
	Game::debugPrintUi("/deco help         - Show this help");
}

// ======================================================================
// Command parser
// ======================================================================
bool DecorationMode::parseCommand(const wchar_t* args) {
	// Tokenize args
	wchar_t buffer[256];
	wcsncpy_s(buffer, args, _TRUNCATE);

	wchar_t* context = nullptr;
	wchar_t* subcmd = wcstok_s(buffer, L" ", &context);
	wchar_t* param1 = subcmd ? wcstok_s(nullptr, L" ", &context) : nullptr;

	// /deco (no args) - toggle
	if (!subcmd || wcslen(subcmd) == 0) {
		toggle();
		return true;
	}

	// /deco help
	if (wcieq(subcmd, L"help")) {
		showHelp();
		return true;
	}

	// /deco on
	if (wcieq(subcmd, L"on")) {
		if (!isActive()) activate();
		else Game::debugPrintUi("[Deco] Already active.");
		return true;
	}

	// /deco off
	if (wcieq(subcmd, L"off")) {
		if (isActive()) deactivate();
		else Game::debugPrintUi("[Deco] Not active.");
		return true;
	}

	// /deco pos
	if (wcieq(subcmd, L"pos") || wcieq(subcmd, L"info")) {
		if (isActive()) showPositionInfo();
		else Game::debugPrintUi("[Deco] Not active. Use /deco to activate.");
		return true;
	}

	// /deco step [size]
	if (wcieq(subcmd, L"step")) {
		if (param1) setStepSize(wstof(param1));
		else {
			char msg[64];
			sprintf_s(msg, sizeof(msg), "[Deco] Current step size: %.0f", m_stepSize);
			Game::debugPrintUi(msg);
		}
		return true;
	}

	// /deco rstep [deg]
	if (wcieq(subcmd, L"rstep")) {
		if (param1) setRotateStep(wstof(param1));
		else {
			char msg[64];
			sprintf_s(msg, sizeof(msg), "[Deco] Current rotation step: %.1f deg", m_rotateStep);
			Game::debugPrintUi(msg);
		}
		return true;
	}

	// /deco snap [size]
	if (wcieq(subcmd, L"snap")) {
		if (param1) {
			m_snapSize = wstof(param1);
			if (m_snapSize < 0.1f) m_snapSize = 0.1f;
			m_snapEnabled = true;
			char msg[64];
			sprintf_s(msg, sizeof(msg), "[Deco] Snap enabled, size: %.1f", m_snapSize);
			Game::debugPrintUi(msg);
		} else {
			m_snapEnabled = !m_snapEnabled;
			char msg[64];
			sprintf_s(msg, sizeof(msg), "[Deco] Snap %s (size: %.1f)", m_snapEnabled ? "ON" : "OFF", m_snapSize);
			Game::debugPrintUi(msg);
		}
		return true;
	}

	// /deco gizmo
	if (wcieq(subcmd, L"gizmo")) {
		m_gizmoEnabled = !m_gizmoEnabled;
		char msg[64];
		sprintf_s(msg, sizeof(msg), "[Deco] 3D Gizmo %s", m_gizmoEnabled ? "ENABLED" : "DISABLED");
		Game::debugPrintUi(msg);
		return true;
	}

	// /deco tool
	if (wcieq(subcmd, L"tool")) {
		m_gizmoTool = (m_gizmoTool == GizmoTool::Translate) ? GizmoTool::Rotate : GizmoTool::Translate;
		Game::debugPrintUi(m_gizmoTool == GizmoTool::Translate
			? "[Deco] Tool: Translate"
			: "[Deco] Tool: Rotate");
		return true;
	}

	// /deco undo
	if (wcieq(subcmd, L"undo")) {
		undo();
		return true;
	}

	// --- Movement commands (require active mode) ---
	if (!isActive()) {
		Game::debugPrintUi("[Deco] Not active. Use /deco to activate first.");
		return true;
	}

	float dist = param1 ? wstof(param1) : m_stepSize;
	if (dist < 1.0f) dist = m_stepSize;

	float deg = param1 ? wstof(param1) : m_rotateStep;
	if (deg == 0.0f) deg = m_rotateStep;

	// /deco f|forward [dist]
	if (wcieq(subcmd, L"f") || wcieq(subcmd, L"forward")) {
		moveForward(dist);
		return true;
	}

	// /deco b|back [dist]
	if (wcieq(subcmd, L"b") || wcieq(subcmd, L"back")) {
		moveBack(dist);
		return true;
	}

	// /deco l|left [dist]
	if (wcieq(subcmd, L"l") || wcieq(subcmd, L"left")) {
		moveLeft(dist);
		return true;
	}

	// /deco r|right [dist]
	if (wcieq(subcmd, L"r") || wcieq(subcmd, L"right")) {
		moveRight(dist);
		return true;
	}

	// /deco u|up [dist]
	if (wcieq(subcmd, L"u") || wcieq(subcmd, L"up")) {
		moveUp(dist);
		return true;
	}

	// /deco d|down [dist]
	if (wcieq(subcmd, L"d") || wcieq(subcmd, L"down")) {
		moveDown(dist);
		return true;
	}

	// /deco move <direction> [distance]
	if (wcieq(subcmd, L"move")) {
		if (!param1) {
			Game::debugPrintUi("[Deco] Usage: /deco move <forward|back|left|right|up|down> [distance]");
			return true;
		}
		wchar_t* param2 = wcstok_s(nullptr, L" ", &context);
		float moveDist = param2 ? wstof(param2) : m_stepSize;
		if (moveDist < 1.0f) moveDist = m_stepSize;

		if (wcieq(param1, L"forward") || wcieq(param1, L"f")) moveForward(moveDist);
		else if (wcieq(param1, L"back") || wcieq(param1, L"b")) moveBack(moveDist);
		else if (wcieq(param1, L"left") || wcieq(param1, L"l")) moveLeft(moveDist);
		else if (wcieq(param1, L"right") || wcieq(param1, L"r")) moveRight(moveDist);
		else if (wcieq(param1, L"up") || wcieq(param1, L"u")) moveUp(moveDist);
		else if (wcieq(param1, L"down") || wcieq(param1, L"d")) moveDown(moveDist);
		else Game::debugPrintUi("[Deco] Unknown direction. Use forward/back/left/right/up/down.");
		return true;
	}

	// /deco rot|rotate [deg]
	if (wcieq(subcmd, L"rot") || wcieq(subcmd, L"rotate")) {
		rotate(deg);
		return true;
	}

	// /deco pitch [deg]
	if (wcieq(subcmd, L"pitch")) {
		rotatePitch(deg);
		return true;
	}

	// /deco roll [deg]
	if (wcieq(subcmd, L"roll")) {
		rotateRoll(deg);
		return true;
	}

	Game::debugPrintUi("[Deco] Unknown command. Type /deco help for a list of commands.");
	return true;
}
