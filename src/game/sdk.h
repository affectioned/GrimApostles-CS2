#pragma once
#include "GameGlobals.h"
#include "Const/Config.h"
#include "vec.h"
#include "offsets.h"
#include "CCSPlayerController.h"
#include "C_CSPlayerPawn.h"
#include "C_PlantedC4.h"
#include <unordered_map>

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

};
