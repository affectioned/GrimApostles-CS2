#pragma once
#include <cstdint>
#include <Windows.h>

struct CCSPlayerController {
	uint64_t nameAddr   = 0;
	char     name[32]   = {};
	uint8_t  teamID     = 0;
	DWORD    color      = 0;
	uint32_t ping       = 0;
	int32_t  armor      = 0;
	bool     hasDefuser = false;
	bool     hasHelmet  = false;

	void Read(uint64_t base);
	void ReadName();
};
