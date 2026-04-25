#pragma once
#include <cstdint>
#include <Windows.h>
#include "../Const/Config.h"

struct CCSPlayerController {
	uint64_t nameAddr   = 0;
	char     name[32]   = {};
	TeamID   teamID     = TEAM_UNASSIGNED;
	DWORD    color      = 0;
	int32_t  armor      = 0;
	bool     hasDefuser = false;
	bool     hasHelmet  = false;

	void Read(uint64_t base);
	void ReadName();
};
