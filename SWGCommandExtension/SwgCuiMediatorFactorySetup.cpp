#include "stdafx.h"

#include "UIPage.h"
#include "SwgCuiMediatorFactorySetup.h"
#include "SwgCuiConsole.h"
#include "CuiMediatorFactory.h"

void SwgCuiMediatorFactorySetup::install() {
	// #region agent log
	hookLog("H40", "HOOK_MedSetup_entry");
	// #endregion
	/*typedef CuiMediatorFactory::Constructor<SwgCuiConsole> ctor_t;

	auto ctor = create_object<ctor_t>("/Console");

	CuiMediatorFactory::addConstructor("WS_Console", ctor);
	*/
	originalInstall::run();
	// #region agent log
	aslrDebugLog("HOOK_MedSetup_exit");
	// #endregion
}