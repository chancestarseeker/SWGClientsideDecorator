#include "stdafx.h"

#include "SwgCuiLoginScreen.h"
#include "soewrappers.h"
#include "Game.h"

#define LOGINSCREEN_ACTIVATE 0xC8D190
#define GAME_APPLICATION_ADDRES 0x01908858

void SwgCuiLoginScreen::onButtonPressed(Object* uicontext) {
	// #region agent log
	hookLog("H40", "HOOK_LoginBtn_entry");
	// #endregion
	onButtonPressedOriginal::run(this, uicontext);
	// #region agent log
	hookLog("H40", "HOOK_LoginBtn_exit");
	// #endregion
}
