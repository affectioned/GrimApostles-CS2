#pragma once
#include <cstdint>
#include "vec.h"

struct C_CSPlayerPawn {
	uint32_t health            = 0;
	uint8_t  lifeState         = 0;
	Vector2  eyeAngles         = {};
	Vector3  position          = {};
	uint64_t activeWeapon      = 0;
	uint16_t activeWeaponID    = 0;
	bool     isDefusing        = false;
	char     lastPlaceName[18] = {};

	void Read(uint64_t base);
};
