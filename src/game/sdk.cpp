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
	(void)updater::classOffsetsReady.load(std::memory_order_acquire);

	CPlayer prev[64];
	memcpy(prev, players, sizeof(prev));

	// ── Scatter A: all module-level offsets in one batch (was 4 separate MemReadPtrs) ──
	uint64_t globalVarsPtr = 0, newController = 0, newPawn = 0, newEntityList = 0;
	DMADevice::PrepareEX(DMADevice::hScatter, DMADevice::moduleBase + client_dll::dwGlobalVars,            &globalVarsPtr, sizeof(uint64_t));
	DMADevice::PrepareEX(DMADevice::hScatter, DMADevice::moduleBase + client_dll::dwLocalPlayerController, &newController, sizeof(uint64_t));
	DMADevice::PrepareEX(DMADevice::hScatter, DMADevice::moduleBase + client_dll::dwLocalPlayerPawn,       &newPawn,       sizeof(uint64_t));
	DMADevice::PrepareEX(DMADevice::hScatter, DMADevice::moduleBase + client_dll::dwEntityList,            &newEntityList, sizeof(uint64_t));
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	if (newController && !isValidPtr(newController)) newController = 0;
	if (newPawn       && !isValidPtr(newPawn))       newPawn       = 0;
	if (!isValidPtr(newEntityList))                  newEntityList = 0;
	if (!isValidPtr(globalVarsPtr))                  globalVarsPtr = 0;

	if (newEntityList) {
		static uint64_t prevEL = 0;
		if (newEntityList != prevEL) { std::cout << "[SDK]: Entity list -> 0x" << std::hex << newEntityList << std::dec << "\n"; prevEL = newEntityList; }
		entityList = newEntityList;
	}
	{
		static uint64_t prevCtrl = 0;
		if (newController != prevCtrl) { std::cout << "[SDK]: Local controller -> 0x" << std::hex << newController << std::dec << "\n"; prevCtrl = newController; }
	}
	localPlayer.controller = newController;
	localPlayer.pawn       = newPawn;

	// ── Scatter B: one level deep + entity chunk pointers (was separate calls + scatter pass 1) ──
	// Batches: mapPtr, local player fields, and all 64 entity list chunk reads in one round trip.
	uint64_t mapPtr = 0;
	memset(players, 0, sizeof(players));

	if (globalVarsPtr)
		DMADevice::PrepareEX(DMADevice::hScatter, globalVarsPtr + 0x0188,                                                          &mapPtr,               sizeof(uint64_t));
	if (localPlayer.controller) {
		DMADevice::PrepareEX(DMADevice::hScatter, localPlayer.controller + client_dll::C_BaseEntity::m_iTeamNum,                   &localPlayer.teamID,   sizeof(uint8_t));
		DMADevice::PrepareEX(DMADevice::hScatter, localPlayer.controller + client_dll::CCSPlayerController::m_sSanitizedPlayerName, &localPlayer.nameAddr, sizeof(uint64_t));
	}
	if (localPlayer.pawn)
		DMADevice::PrepareEX(DMADevice::hScatter, localPlayer.pawn + client_dll::C_BasePlayerPawn::m_vOldOrigin,                   &localPlayer.position, sizeof(Vector3));
	for (int i = 0; i < 64; i++)
		DMADevice::PrepareEX(DMADevice::hScatter, entityList + (0x8 * ((i + 1) >> 9) + 16), &players[i].listEntry, sizeof(uint64_t));
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// ── Scatter C: string data (was 2 separate MemReads) ──────────────────────
	char mapNameBuf[32] = {}, nameBuf[32] = {};
	bool readingMap  = isValidPtr(mapPtr);
	bool readingName = isValidPtr(localPlayer.nameAddr);

	if (readingMap)  DMADevice::PrepareEX(DMADevice::hScatter, mapPtr,                  mapNameBuf, sizeof(mapNameBuf));
	if (readingName) DMADevice::PrepareEX(DMADevice::hScatter, localPlayer.nameAddr,    nameBuf,    sizeof(nameBuf));
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	if (readingMap) {
		mapNameBuf[sizeof(mapNameBuf) - 1] = '\0';
		if (isValidAscii(mapNameBuf, sizeof(mapNameBuf))) {
			static char prevMap[32] = {};
			if (strncmp(mapNameBuf, prevMap, sizeof(mapNameBuf)) != 0) {
				std::cout << "[SDK]: Map -> " << mapNameBuf << "\n";
				memcpy(prevMap, mapNameBuf, sizeof(mapNameBuf));
			}
			memcpy(mapName, mapNameBuf, sizeof(mapName));
		}
	}
	if (readingName) {
		nameBuf[sizeof(nameBuf) - 1] = '\0';
		if (isValidAscii(nameBuf, sizeof(nameBuf))) {
			memcpy(localPlayer.name, nameBuf, sizeof(localPlayer.name));
			static char prevName[32] = {};
			if (strncmp(localPlayer.name, prevName, sizeof(prevName)) != 0) {
				std::cout << "[SDK]: Local player name -> " << localPlayer.name << "\n";
				memcpy(prevName, localPlayer.name, sizeof(prevName));
			}
		}
	}

	// ── Entity chain passes 2–5 (guarded: only prepare reads for valid slots) ──
	for (int i = 0; i < 64; i++)
		if (players[i].listEntry)
			DMADevice::PrepareEX(DMADevice::hScatter, players[i].listEntry + 0x70 * ((i + 1) & 0x1FF), &players[i].controller, sizeof(uint64_t));
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	for (int i = 0; i < 64; i++)
		if (players[i].controller)
			DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::CCSPlayerController::m_hPlayerPawn, &players[i].pawnAddr, sizeof(uint32_t));
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	for (int i = 0; i < 64; i++)
		if (players[i].pawnAddr)
			DMADevice::PrepareEX(DMADevice::hScatter, entityList + 0x8 * ((players[i].pawnAddr & 0x7FFF) >> 9) + 16, &players[i].listEntry2, sizeof(uint64_t));
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	for (int i = 0; i < 64; i++)
		if (players[i].listEntry2)
			DMADevice::PrepareEX(DMADevice::hScatter, players[i].listEntry2 + 0x70 * (players[i].pawnAddr & 0x1FF), &players[i].pawn, sizeof(uint64_t));
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	getPlayerData();
	getWeapons();

	// ── Cache restore + drop logging ──────────────────────────────────────────
	for (int i = 0; i < 64; i++) {
		bool hadPlayer = isValidPtr(prev[i].controller);

		bool ctrlGarbage = players[i].controller && !isValidPtr(players[i].controller);
		bool pawnGarbage = players[i].pawn       && !isValidPtr(players[i].pawn);
		if (ctrlGarbage || pawnGarbage) {
			if (hadPlayer) {
				std::cout << "[SDK]: Slot " << i << " '" << prev[i].name << "' cache-restored ("
				          << (ctrlGarbage ? "garbage ctrl" : "garbage pawn") << ")\n";
				players[i] = prev[i];
			}
			continue;
		}

		// Slot zeroed out — valid prev entry silently dropped by a zero read.
		if (hadPlayer && !players[i].controller)
			std::cout << "[SDK]: Slot " << i << " '" << prev[i].name << "' dropped (zero read)\n";

		if (!isValidAscii(players[i].name, sizeof(players[i].name)) &&
			isValidAscii(prev[i].name, sizeof(prev[i].name)))
			memcpy(players[i].name, prev[i].name, sizeof(players[i].name));
	}
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
		players[i].name[sizeof(players[i].name) - 1]                  = '\0';
		players[i].lastPlaceName[sizeof(players[i].lastPlaceName) - 1] = '\0';
	}
}

// ─── Weapons ─────────────────────────────────────────────────────────────────

void CGame::getWeapons() {
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
