// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

#include <windows.h>
#include <detours.h>
#include <cstdio>
#include <tlhelp32.h>

// #region agent log
static void debugLog(const char* hypothesisId, const char* message, const char* data = nullptr) {
    FILE* f = nullptr;
    fopen_s(&f, "c:\\AICode\\SWG Jedi\\.cursor\\debug.log", "a");
    if (f) {
        char exeName[MAX_PATH] = {};
        GetModuleFileNameA(NULL, exeName, MAX_PATH);
        fprintf(f, "{\"hypothesisId\":\"%s\",\"location\":\"dllmain.cpp\",\"message\":\"%s\",\"data\":{\"exe\":\"%s\"", hypothesisId, message, exeName);
        if (data) fprintf(f, ",\"extra\":\"%s\"", data);
        fprintf(f, "},\"timestamp\":%llu}\n", (unsigned long long)GetTickCount64());
        fclose(f);
    }
}
// #endregion

#include "CreatureObject.h"
#include "CuiMediatorFactory.h"
#include "CuiChatParser.h"
#include "Game.h"
#include "TerrainObject.h"
#include "SwgCuiLoginScreen.h"
#include "SwgCuiCommandParserDefault.h"
#include "SwgCuiMediatorFactorySetup.h"
#include "SwgCuiToolbar.h"
#include "ConfigFile.h"
#include "D3D9Hook.h"

using namespace std;

/// Memory Utilties
///
void writeJmp(BYTE* address, DWORD jumpTo, DWORD length) {
	DWORD oldProtect, newProtect, relativeAddress;

	VirtualProtect(address, length, PAGE_EXECUTE_READWRITE, &oldProtect);

	relativeAddress = (DWORD)(jumpTo - (DWORD)address) - 5;
	*address = 0xE9;
	*((DWORD *)(address + 0x1)) = relativeAddress;

	for (DWORD x = 0x5; x < length; x++)
	{
		*(address + x) = 0x90;
	}

	VirtualProtect(address, length, oldProtect, &newProtect);
}

void writeBytes(BYTE* address, const BYTE* values, int size) {
	DWORD oldProtect, newProtect;

	VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);

	memcpy(address, values, size);

	VirtualProtect(address, size, oldProtect, &newProtect);
}

#define ATTACH_HOOK(METHOD) METHOD##_hook_t::hookStorage_t::newMethod = &METHOD; \
											DetourAttach((PVOID*) &METHOD##_hook_t::hookStorage_t::original, (PVOID) METHOD##_hook_t::callHook);

#define DETACH_HOOK(METHOD) DetourDetach((PVOID*) &METHOD##_hook_t::hookStorage_t::original, (PVOID) METHOD##_hook_t::callHook);
	

// #region agent log
static DWORD WINAPI WatchdogThread(LPVOID) {
	for (int i = 0; i < 60; ++i) {
		Sleep(2000);
		char buf[128];
		DWORD tid = GetCurrentThreadId();
		sprintf_s(buf, "tick=%d,tid=%lu", i, tid);
		debugLog("H30_watchdog", "heartbeat", buf);
	}
	return 0;
}
// #endregion

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH: {
		// #region agent log
		debugLog("H1", "DLL_PROCESS_ATTACH entry");
		{ char buf[128]; sprintf_s(buf, "gameBase=0x%p, offset=0x%IX", (void*)(uintptr_t)GetModuleHandle(NULL), gameBaseOffset()); debugLog("ASLR", "ASLR offset computed", buf); }
		// #endregion

		// #region agent log — watchdog background thread
		CreateThread(NULL, 0, WatchdogThread, NULL, 0, NULL);
		debugLog("H30", "watchdog-thread-started");
		// #endregion

		DetourRestoreAfterWith();

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		// Collect other thread handles — store them so we can close AFTER commit
		LONG errorCode = 0;
		{
			DWORD myTid = GetCurrentThreadId();
			DWORD pid = GetCurrentProcessId();
			// #region agent log
			{ char buf[64]; sprintf_s(buf, "myTid=%lu,pid=%lu", myTid, pid); debugLog("H30", "thread-enum-start", buf); }
			// #endregion
			HANDLE threadHandles[256];
			int handleCount = 0;
			HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
			if (snap != INVALID_HANDLE_VALUE) {
				THREADENTRY32 te;
				te.dwSize = sizeof(te);
				if (Thread32First(snap, &te)) {
					do {
						if (te.th32OwnerProcessID == pid && te.th32ThreadID != myTid) {
							HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
							if (hThread) {
								DetourUpdateThread(hThread);
								if (handleCount < 256)
									threadHandles[handleCount++] = hThread;
								// #region agent log
								{ char buf[64]; sprintf_s(buf, "tid=%lu,h=0x%p", te.th32ThreadID, hThread); debugLog("H30", "thread-registered", buf); }
								// #endregion
							}
						}
					} while (Thread32Next(snap, &te));
				}
				CloseHandle(snap);
			}
			// #region agent log
			{ char buf[32]; sprintf_s(buf, "%d", handleCount); debugLog("H30", "threads-registered-count", buf); }
			// #endregion

		// Direct function hooks.
		ATTACH_HOOK(SwgCuiLoginScreen::onButtonPressed);
		ATTACH_HOOK(CuiChatParser::parse);
		ATTACH_HOOK(TerrainObject::setHighLevelOfDetailThresholdHook);
		ATTACH_HOOK(TerrainObject::setLevelOfDetailThresholdHook);
		ATTACH_HOOK(GroundScene::parseMessages);
		//ATTACH_HOOK(SwgCuiCommandParserDefault::ctorHook);
		//ATTACH_HOOK(SwgCuiCommandParserDefault::removeAliasStatic);
		ATTACH_HOOK(SwgCuiMediatorFactorySetup::install);
		ATTACH_HOOK(Transform::install);
		ATTACH_HOOK(Transform::invert);
		ATTACH_HOOK(Transform::reorthonormalize);
		ATTACH_HOOK(Transform::rotate_l2p);
		ATTACH_HOOK(Transform::rotateTranslate_l2p);
		ATTACH_HOOK(Transform::rotate_p2l);
		ATTACH_HOOK(Transform::rotateTranslate_l2pTr);
		ATTACH_HOOK(Transform::rotateTranslate_p2l);
		ATTACH_HOOK(Transform::yaw_l);
		ATTACH_HOOK(Transform::pitch_l);
		ATTACH_HOOK(Transform::roll_l);
		ATTACH_HOOK(SwgCuiToolbar::ctor);
		ATTACH_HOOK(SwgCuiToolbarAction::performAction);

		errorCode = DetourTransactionCommit();

		// #region agent log
		{ char buf[64]; sprintf_s(buf, "%ld", errorCode); debugLog("H2", "DetourTransactionCommit result", buf); }
		// #endregion

		// Close thread handles AFTER commit (H30: previously closed before commit!)
		for (int i = 0; i < handleCount; ++i) {
			CloseHandle(threadHandles[i]);
		}
		// #region agent log
		{ char buf[32]; sprintf_s(buf, "%d", handleCount); debugLog("H30", "thread-handles-closed-after-commit", buf); }
		// #endregion
		} // end thread enumeration block

		if (errorCode == NO_ERROR) {
			//Detour successful

			// #region agent log
			debugLog("H21", "pre-writeBytes-NOP");
			// #endregion
			const BYTE newData[7] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
			writeBytes((BYTE*)aslr(0xC8D258), newData, 7);
			// #region agent log
			debugLog("H21", "post-writeBytes-NOP");
			// #endregion

			// Toolbar button count patching is handled by SwgCuiToolbar::ctor hook
			// (calling ConfigFile::getKeyInt during DllMain crashes the game)
			// #region agent log
			debugLog("H21", "toolbar-patching-deferred-to-ctor-hook");
			// #endregion
						
			// Show our loaded message (only displays if chat is already present).
			// #region agent log
			debugLog("H2", "Detour hooks SUCCESS");
			// #endregion
			// Game::debugPrintUi crashes when injected before chat UI is ready
			// Game::debugPrintUi("[LOADED] Settings Override Extensions by N00854180T");
			// Game::debugPrintUi("Use /exthelp for details on extension command usage.");
			// #region agent log
			debugLog("H20", "post-debugPrintUi-skipped");
			// #endregion
		} else {
			// #region agent log
			debugLog("H2", "Detour hooks FAILED");
			// #endregion
			// Game::debugPrintUi("[LOAD] FAILED");
		}

		break;
	}
	case DLL_PROCESS_DETACH:
		// Remove D3D9 hooks (decoration gizmo)
		D3D9Hook::remove();

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		DETACH_HOOK(SwgCuiLoginScreen::onButtonPressed);
		DETACH_HOOK(CuiChatParser::parse);
		DETACH_HOOK(TerrainObject::setHighLevelOfDetailThresholdHook);
		DETACH_HOOK(TerrainObject::setLevelOfDetailThresholdHook);
		DETACH_HOOK(GroundScene::parseMessages);
		DETACH_HOOK(SwgCuiMediatorFactorySetup::install);
		DETACH_HOOK(Transform::install);
		DETACH_HOOK(Transform::invert);
		DETACH_HOOK(Transform::reorthonormalize);
		DETACH_HOOK(Transform::rotate_l2p);
		DETACH_HOOK(Transform::rotateTranslate_l2p);
		DETACH_HOOK(Transform::rotate_p2l);
		DETACH_HOOK(Transform::rotateTranslate_l2pTr);
		DETACH_HOOK(Transform::rotateTranslate_p2l);
		DETACH_HOOK(Transform::yaw_l);
		DETACH_HOOK(Transform::pitch_l);
		DETACH_HOOK(Transform::roll_l);
		DETACH_HOOK(SwgCuiToolbar::ctor);
		DETACH_HOOK(SwgCuiToolbarAction::performAction);

		DetourTransactionCommit();
		break;
	}

	return TRUE;
}
