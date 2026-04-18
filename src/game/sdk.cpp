#include "pch.h"
#include "sdk.h"
#include "updater.h"

void CGame::update() {
	// Acquire-load: ensures all class offset writes from fetchClassOffsets() (release-store)
	// are visible on this thread before we use them in the reads below.
	(void)updater::classOffsetsReady.load(std::memory_order_acquire);

	getMap();
	getLocalPlayer();
	getEntityList();
	getPlayers();
	getPlayerData();
	getWeapons();
}


// ─── Map ─────────────────────────────────────────────────────────────────────

void CGame::getMap() {
	auto globalVarsPtr = DMADevice::MemReadPtr<uint64_t>(DMADevice::moduleBase + client_dll::dwGlobalVars);
	if (!globalVarsPtr) return;

	uintptr_t mapPtr = DMADevice::MemReadPtr<uint64_t>(globalVarsPtr + 0x0188);
	if (!mapPtr) return;

	char newMapName[32] = {};
	DMADevice::MemRead(mapPtr, newMapName, sizeof(newMapName));
	newMapName[sizeof(newMapName) - 1] = '\0';

	if (newMapName[0]) {
		static char prevMap[32] = {};
		if (strncmp(newMapName, prevMap, sizeof(newMapName)) != 0) {
			std::cout << "[SDK]: Map -> " << newMapName << "\n";
			memcpy(prevMap, newMapName, sizeof(newMapName));
		}
		memcpy(mapName, newMapName, sizeof(mapName));
	}
}

void CGame::getEntityList() {
	entityList = DMADevice::MemReadPtr<uint64_t>(DMADevice::moduleBase + client_dll::dwEntityList);

	static uint64_t prevEntityList = 0;
	if (entityList != prevEntityList) {
		std::cout << "[SDK]: Entity list -> 0x" << std::hex << entityList << std::dec << "\n";
		prevEntityList = entityList;
	}
}


// ─── Local player ─────────────────────────────────────────────────────────────

void CGame::getLocalPlayer() {
	localPlayer.controller = DMADevice::MemReadPtr<uint64_t>(DMADevice::moduleBase + client_dll::dwLocalPlayerController);

	static uint64_t prevController = 0;
	if (localPlayer.controller != prevController) {
		std::cout << "[SDK]: Local controller -> 0x" << std::hex << localPlayer.controller << std::dec << "\n";
		prevController = localPlayer.controller;
	}
	if (!localPlayer.controller) return;

	localPlayer.pawn   = DMADevice::MemReadPtr<uint64_t>(DMADevice::moduleBase + client_dll::dwLocalPlayerPawn);
	localPlayer.teamID = DMADevice::MemReadPtr<uint8_t>(localPlayer.controller + client_dll::C_BaseEntity::m_iTeamNum);

	if (localPlayer.pawn)
		DMADevice::MemRead(localPlayer.pawn + client_dll::C_BasePlayerPawn::m_vOldOrigin, &localPlayer.position, sizeof(Vector3));

	localPlayer.nameAddr = DMADevice::MemReadPtr<uint64_t>(localPlayer.controller + client_dll::CCSPlayerController::m_sSanitizedPlayerName);
	if (!localPlayer.nameAddr) return;

	DMADevice::MemRead(localPlayer.nameAddr, &localPlayer.name, sizeof(localPlayer.name));
	localPlayer.name[sizeof(localPlayer.name) - 1] = '\0';

	static char prevName[32] = {};
	if (strncmp(localPlayer.name, prevName, sizeof(prevName)) != 0) {
		std::cout << "[SDK]: Local player name -> " << (localPlayer.name[0] ? localPlayer.name : "(empty)") << "\n";
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
