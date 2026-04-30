#pragma once
#include "GameGlobals.h"
#include "Const/Config.h"
#include "vec.h"
#include "offsets.h"
#include "CCSPlayerController.h"
#include "C_CSPlayerPawn.h"
#include "C_PlantedC4.h"
#include <unordered_map>

// Windows user-mode pointer range. Torn DMA reads during map transitions can
// produce values like 0xC3C3C3C3... that pass a null check but dereference to
// garbage; this bound rejects them.
inline bool isValidPtr(uint64_t p) { return p > 0x10000 && p < 0x7FFFFFFFFFFF; }

struct mapData {
	float xBound;
	float yBound;
	float scale;
	float lowerZThreshold;

	mapData(float x, float y, float s, float zThresh = std::numeric_limits<float>::infinity())
		: xBound(x), yBound(y), scale(s), lowerZThreshold(zThresh) {}
	mapData() : xBound(0), yBound(0), scale(0), lowerZThreshold(std::numeric_limits<float>::infinity()) {}
};

// ── Player ────────────────────────────────────────────────────────────────────

struct CPlayer {
	// DMA chain pointers
	uint64_t controllerBase = 0;
	uint64_t listEntry      = 0;
	uint64_t listEntry2     = 0;
	uint32_t pawnHandle     = 0;
	uint64_t pawnBase       = 0;

	// Game entity data
	CCSPlayerController ctrl;
	C_CSPlayerPawn      pawn;
};

// ── Game ──────────────────────────────────────────────────────────────────────
// Pure data class — all DMA reads are driven by CS2Context timer methods.

class CGame {
public:
	CPlayer      localPlayer;
	char         mapName[32] = {};
	uintptr_t    entityList  = 0;
	CPlayer      players[MAX_ENTITIES];
	C_PlantedC4  bomb;

	// engine2.dll: SIGNONSTATE_FULL (6) means we're connected and in-game.
	// Polled by t_NetworkState (500 ms). Render code may use this to gate
	// behaviour without parsing entity-list/map-name heuristics.
	uint8_t      signOnState = 0;
};
