#include "stdafx.h"

#include "SwgCuiToolbar.h"
#include "UIPage.h"
#include "CuiActionManager.h"
// #region agent log
static int _dbg_toolbar = (aslrDebugLog("H13_Toolbar_ok", 0, 0), 1);
// #endregion

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

SwgCuiToolbar* SwgCuiToolbar::ctor(UIPage* page, int sceneType) {
	// #region agent log
	hookLog("H40", "HOOK_Toolbar_entry");
	// #endregion
	originalCtor::run(this, page, sceneType);
	// #region agent log
	aslrDebugLog("HOOK_Toolbar_postOrigCtor");
	// #endregion

	int newNumberOfButtons = getNumberOfToolbarButtons();

	for (int i = DEFAULT_NUMBER_OF_BUTTONS; i < newNumberOfButtons; ++i) {
		const char* format = "toolbarSlot%d";
		char str[64];

		sprintf_s(str, sizeof(str), format, i);

		CuiActionManager::addAction(str, getAction(), false);
	}

	// #region agent log
	aslrDebugLog("HOOK_Toolbar_exit");
	// #endregion
	return this;
}

int SwgCuiToolbar::getNumberOfToolbarButtons() {
	static const int val = MAX(ConfigFile::getKeyInt("GalaxyExtender", "toolbarButtons", DEFAULT_NUMBER_OF_BUTTONS), DEFAULT_NUMBER_OF_BUTTONS);

	return val;
}

bool SwgCuiToolbarAction::performAction(const soe::string& id, const soe::unicode& u) {
	// #region agent log
	static bool s_loggedPerformAction = false;
	if (!s_loggedPerformAction) { aslrDebugLog("HOOK_ToolbarAction_first"); s_loggedPerformAction = true; }
	// #endregion
	int newNumberOfButtons = SwgCuiToolbar::getNumberOfToolbarButtons();

	for (int i = SwgCuiToolbar::DEFAULT_NUMBER_OF_BUTTONS; i < newNumberOfButtons; ++i) {
		const char* format = "toolbarSlot%d";
		char str[64];

		sprintf_s(str, sizeof(str), format, i);

		if (id == str) {
			getToolbar()->performToolbarAction(i);

			return true;
		}
	}

	return originalPerformAction::run(this, id, u);
}