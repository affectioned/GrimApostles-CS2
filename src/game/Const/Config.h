#pragma once
#include <cstdint>

// CS2-specific constants used as loop bounds and array sizes throughout the game layer.
// Do NOT include this in pch.h — game layer only.

inline constexpr size_t MAX_ENTITIES = 64;   // max entities in a single entity-list chunk

enum TeamID : uint8_t {
	TEAM_UNASSIGNED = 0,
	TEAM_SPECTATOR  = 1,
	TEAM_T          = 2,
	TEAM_CT         = 3,
};