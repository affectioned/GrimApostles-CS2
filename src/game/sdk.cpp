#include "pch.h"
#include "sdk.h"
#include "updater.h"

// Returns true for a plausible Windows user-space pointer.
static bool isValidPtr(uint64_t p) {
	return p > 0x10000ULL && p < 0x7FFFFFFFFFFF0000ULL;
}

// Returns true if the string is non-empty and contains only printable ASCII.
static bool isValidAscii(const char* s, size_t n) {
	if (!s || !s[0]) return false;
	for (size_t i = 0; i < n && s[i]; i++) {
		unsigned char c = (unsigned char)s[i];
		if (c < 0x20 || c > 0x7E) return false;
	}
	return true;
}

void CGame::update() {
	// Acquire-load: ensures all class offset writes from fetchClassOffsets() (release-store)
	// are visible on this thread before we use them in the reads below.
	(void)updater::classOffsetsReady.load(std::memory_order_acquire);

	// Snapshot last-good player state before getPlayers() zeroes the array.
	CPlayer prev[64];
	memcpy(prev, players, sizeof(prev));

	getMap();
	getLocalPlayer();
	getEntityList();
	getPlayers();
	getPlayerData();
	getWeapons();

	// Restore cached data for any slot where the new read produced garbage.
	// VMMDLL_FLAG_ZEROPAD_ON_FAIL handles full read failures (zeros), but
	// partial/corrupted reads can produce non-zero garbage addresses.
	for (int i = 0; i < 64; i++) {
		bool ctrlGarbage = players[i].controller && !isValidPtr(players[i].controller);
		bool pawnGarbage = players[i].pawn       && !isValidPtr(players[i].pawn);
		if (ctrlGarbage || pawnGarbage) {
			if (isValidPtr(prev[i].controller))
				players[i] = prev[i];
			continue;
		}
		// Even if pointers look valid, restore name if it came back as garbage.
		if (!isValidAscii(players[i].name, sizeof(players[i].name)) &&
			isValidAscii(prev[i].name, sizeof(prev[i].name)))
			memcpy(players[i].name, prev[i].name, sizeof(players[i].name));
	}
}


// ─── Map ─────────────────────────────────────────────────────────────────────

void CGame::getMap() {
	auto globalVarsPtr = DMADevice::MemReadPtr<uint64_t>(DMADevice::moduleBase + client_dll::dwGlobalVars);
	if (!isValidPtr(globalVarsPtr)) return;

	uintptr_t mapPtr = DMADevice::MemReadPtr<uint64_t>(globalVarsPtr + 0x0188);
	if (!isValidPtr(mapPtr)) return;

	char newMapName[32] = {};
	DMADevice::MemRead(mapPtr, newMapName, sizeof(newMapName));
	newMapName[sizeof(newMapName) - 1] = '\0';

	// Only commit if it looks like a real map name (printable ASCII, starts with "de_"/"cs_" etc.).
	if (!isValidAscii(newMapName, sizeof(newMapName))) return;

	static char prevMap[32] = {};
	if (strncmp(newMapName, prevMap, sizeof(newMapName)) != 0) {
		std::cout << "[SDK]: Map -> " << newMapName << "\n";
		memcpy(prevMap, newMapName, sizeof(newMapName));
	}
	memcpy(mapName, newMapName, sizeof(mapName));
}

void CGame::getEntityList() {
	uint64_t newEntityList = DMADevice::MemReadPtr<uint64_t>(DMADevice::moduleBase + client_dll::dwEntityList);
	if (!isValidPtr(newEntityList)) return;

	static uint64_t prevEntityList = 0;
	if (newEntityList != prevEntityList) {
		std::cout << "[SDK]: Entity list -> 0x" << std::hex << newEntityList << std::dec << "\n";
		prevEntityList = newEntityList;
	}
	entityList = newEntityList;
}


// ─── Local player ─────────────────────────────────────────────────────────────

void CGame::getLocalPlayer() {
	uint64_t newController = DMADevice::MemReadPtr<uint64_t>(DMADevice::moduleBase + client_dll::dwLocalPlayerController);

	// Accept zero (player not in-game) but reject garbage pointers.
	if (newController && !isValidPtr(newController)) return;

	static uint64_t prevController = 0;
	if (newController != prevController) {
		std::cout << "[SDK]: Local controller -> 0x" << std::hex << newController << std::dec << "\n";
		prevController = newController;
	}
	localPlayer.controller = newController;
	if (!localPlayer.controller) return;

	uint64_t newPawn = DMADevice::MemReadPtr<uint64_t>(DMADevice::moduleBase + client_dll::dwLocalPlayerPawn);
	if (newPawn && !isValidPtr(newPawn)) return;
	localPlayer.pawn   = newPawn;
	localPlayer.teamID = DMADevice::MemReadPtr<uint8_t>(localPlayer.controller + client_dll::C_BaseEntity::m_iTeamNum);

	if (localPlayer.pawn)
		DMADevice::MemRead(localPlayer.pawn + client_dll::C_BasePlayerPawn::m_vOldOrigin, &localPlayer.position, sizeof(Vector3));

	localPlayer.nameAddr = DMADevice::MemReadPtr<uint64_t>(localPlayer.controller + client_dll::CCSPlayerController::m_sSanitizedPlayerName);
	if (!isValidPtr(localPlayer.nameAddr)) return;

	char newName[32] = {};
	DMADevice::MemRead(localPlayer.nameAddr, &newName, sizeof(newName));
	newName[sizeof(newName) - 1] = '\0';

	// Only update name if it looks valid; keep previous on garbage reads.
	if (!isValidAscii(newName, sizeof(newName))) return;
	memcpy(localPlayer.name, newName, sizeof(localPlayer.name));

	static char prevName[32] = {};
	if (strncmp(localPlayer.name, prevName, sizeof(prevName)) != 0) {
		std::cout << "[SDK]: Local player name -> " << localPlayer.name << "\n";
		memcpy(prevName, localPlayer.name, sizeof(prevName));
	}
}


// ─── Entity pointer chain ─────────────────────────────────────────────────────
// Each pass depends on the previous result, so they must remain sequential.

void CGame::getPlayers() {
	memset(players, 0, sizeof(players));

	for (int i = 0; i < 64; i++)
		DMADevice::PrepareEX(DMADevice::hScatter, entityList + (0x8 * ((i + 1) >> 9) + 16), &players[i].listEntry, sizeof(uint64_t));
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	for (int i = 0; i < 64; i++)
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].listEntry + 0x70 * ((i + 1) & 0x1FF), &players[i].controller, sizeof(uint64_t));
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	for (int i = 0; i < 64; i++)
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::CCSPlayerController::m_hPlayerPawn, &players[i].pawnAddr, sizeof(uint32_t));
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	for (int i = 0; i < 64; i++)
		DMADevice::PrepareEX(DMADevice::hScatter, entityList + 0x8 * ((players[i].pawnAddr & 0x7FFF) >> 9) + 16, &players[i].listEntry2, sizeof(uint64_t));
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	for (int i = 0; i < 64; i++)
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].listEntry2 + 0x70 * (players[i].pawnAddr & 0x1FF), &players[i].pawn, sizeof(uint64_t));
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);
}


// ─── Player data (two consolidated scatter passes) ────────────────────────────

void CGame::getPlayerData() {
	for (int i = 0; i < 64; i++) {
		if (!players[i].controller || !players[i].pawn) continue;
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::CCSPlayerController::m_sSanitizedPlayerName, &players[i].nameAddr,     sizeof(uint64_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::C_BaseEntity::m_iTeamNum,                    &players[i].teamID,       sizeof(uint8_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::CCSPlayerController::m_iCompTeammateColor,   &players[i].color,        sizeof(DWORD));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_BaseEntity::m_iHealth,                    &players[i].health,       sizeof(uint32_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_BaseEntity::m_lifeState,                  &players[i].lifeState,    sizeof(uint8_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_CSPlayerPawn::m_angEyeAngles,             &players[i].eyeAngles,    sizeof(Vector2));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_BasePlayerPawn::m_vOldOrigin,             &players[i].position,     sizeof(Vector3));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_CSPlayerPawn::m_pClippingWeapon,          &players[i].activeWeapon,    sizeof(uint64_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_CSPlayerPawn::m_bIsDefusing,              &players[i].isDefusing,      sizeof(bool));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_CSPlayerPawn::m_szLastPlaceName,          &players[i].lastPlaceName,   sizeof(players[i].lastPlaceName));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::CCSPlayerController::m_iPing,               &players[i].ping,         sizeof(uint32_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::CCSPlayerController::m_iPawnArmor,          &players[i].armor,        sizeof(int32_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::CCSPlayerController::m_bPawnHasDefuser,     &players[i].hasDefuser,   sizeof(bool));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::CCSPlayerController::m_bPawnHasHelmet,      &players[i].hasHelmet,    sizeof(bool));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	for (int i = 0; i < 64; i++) {
		if (!players[i].nameAddr) continue;
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].nameAddr, &players[i].name, sizeof(players[i].name));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	for (int i = 0; i < 64; i++) {
		players[i].name[sizeof(players[i].name) - 1]          = '\0';
		players[i].lastPlaceName[sizeof(players[i].lastPlaceName) - 1] = '\0';
	}
}

// ─── Weapons ─────────────────────────────────────────────────────────────────

void CGame::getWeapons() {
	// activeWeapon handle was already fetched in getPlayerData pass 1.
	// Single pass: resolve handle → item definition index (weapon ID for icon lookup).
	for (int i = 0; i < 64; i++) {
		if (!players[i].activeWeapon) continue;
		DMADevice::PrepareEX(DMADevice::hScatter,
			players[i].activeWeapon
			+ client_dll::C_EconEntity::m_AttributeManager
			+ client_dll::C_AttributeContainer::m_Item
			+ client_dll::C_EconItemView::m_iItemDefinitionIndex,
			&players[i].activeWeaponID, sizeof(uint16_t));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);
}
