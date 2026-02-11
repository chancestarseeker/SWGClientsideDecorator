#include "stdafx.h"

#include "TerrainObject.h"

// #region agent log
static int _dbg_terrain_pre = (aslrDebugLog("H11_TerrainObj_preCall", 0, 0), 1);
// #endregion
float TerrainObject::newHighLoDValue = 6.0f;  // safe default, was getHighLevelOfDetailThreshold() which crashes during DLL static init
// #region agent log
static int _dbg_terrain_mid = (aslrDebugLog("H11_TerrainObj_midCall", 0, 0), 1);
// #endregion
float TerrainObject::newLoDThresholdValue = 1.0f;  // safe default, was getLevelOfDetailThreshold() which crashes during DLL static init
// #region agent log
static int _dbg_terrain_post = (aslrDebugLog("H11_TerrainObj_postCall", 0, 0), 1);
// #endregion

SET_CLIENT_STATIC(TerrainObject::ms_instance, 0x01947194);

void TerrainObject::setHighLevelOfDetailThresholdHook(float ) {
	// #region agent log
	static bool s_loggedHiLoD = false;
	if (!s_loggedHiLoD) { aslrDebugLog("HOOK_TerrainHiLoD_first"); s_loggedHiLoD = true; }
	// #endregion
	oldSetHighLoDThreshold::run(newHighLoDValue);
}

void TerrainObject::setLevelOfDetailThresholdHook(float) {
	// #region agent log
	static bool s_loggedLoD = false;
	if (!s_loggedLoD) { aslrDebugLog("HOOK_TerrainLoD_first"); s_loggedLoD = true; }
	// #endregion
	oldSetLoDThreshold::run(newLoDThresholdValue);
}