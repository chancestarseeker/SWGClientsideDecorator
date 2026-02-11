// SWGCommandExtension.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "soewrappers.h"
// #region agent log
static int _dbg_swgcmd = (aslrDebugLog("H12_SWGCmdExt_ok", 0, 0), 1);
// #endregion

// NOTE: DetourFinishHelperProcess is exported via SWGCommandExtension.def at ordinal #1
// This is required by withdll.exe / DetourCreateProcessWithDllEx for DLL injection.
// The actual implementation comes from detours.lib.