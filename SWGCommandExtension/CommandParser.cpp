#include "stdafx.h"

#include "CommandParser.h"

uint32_t CommandParser::newVtable[3] = {};
static bool s_commandParserVtableInited = false;

void CommandParser::ctor(const char* cmd, size_t min, const char* args, const char* help, CommandParser* arg5) {
	runMethod<0xA83EF0, void>(cmd, min, args, help, arg5);

	if (!s_commandParserVtableInited) {
		newVtable[0] = (uint32_t)aslr(0x161E6CC);
		newVtable[1] = (uint32_t)aslr(0x15EA3C4);
		newVtable[2] = (uint32_t)aslr(0x15EA3C8);
		s_commandParserVtableInited = true;
	}

	SETVTABLE(newVtable);
}