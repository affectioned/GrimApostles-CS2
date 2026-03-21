#include "pch.h"
#include "sdk.h"

void CGame::update() {
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

	int newCount = 0;
	for (int i = 0; i < 64; i++)
		if (players[i].controller) newCount++;
	if (newCount > 0)
		playerCount = newCount;
}


// ─── Player data (two consolidated scatter passes) ────────────────────────────

void CGame::getPlayerData() {
	for (int i = 0; i < playerCount; i++) {
		if (!players[i].controller || !players[i].pawn) continue;
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::CCSPlayerController::m_sSanitizedPlayerName, &players[i].nameAddr,  sizeof(uint64_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::C_BaseEntity::m_iTeamNum,                    &players[i].teamID,    sizeof(uint8_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::CCSPlayerController::m_iCompTeammateColor,   &players[i].color,     sizeof(DWORD));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_BaseEntity::m_iHealth,                    &players[i].health,    sizeof(uint32_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_CSPlayerPawn::m_angEyeAngles,             &players[i].eyeAngles, sizeof(Vector2));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_BasePlayerPawn::m_vOldOrigin,             &players[i].position,  sizeof(Vector3));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	for (int i = 0; i < playerCount; i++) {
		if (!players[i].nameAddr) continue;
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].nameAddr, &players[i].name, sizeof(players[i].name));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	for (int i = 0; i < playerCount; i++)
		players[i].name[sizeof(players[i].name) - 1] = '\0';

	static int prevCount = -1;
	if (playerCount != prevCount) {
		std::cout << "[SDK]: Active players -> " << playerCount << "\n";
		prevCount = playerCount;
	}
}

void CGame::getWeapons() {
	// ── Pass 1: pawn → activeWeapon + weaponServices ──
	for (int i = 0; i < playerCount; i++) {
		if (!players[i].pawn) continue;
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn + client_dll::C_CSPlayerPawn::m_pClippingWeapon,    &players[i].activeWeapon,    sizeof(uint64_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn + client_dll::C_BasePlayerPawn::m_pWeaponServices, &players[i].weaponServices, sizeof(uint64_t));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// ── Pass 2: activeWeapon → activeWeaponID, weaponServices → weaponCount + weaponData ──
	for (int i = 0; i < playerCount; i++) {
		if (players[i].activeWeapon)
			DMADevice::PrepareEX(DMADevice::hScatter,
				players[i].activeWeapon
				+ client_dll::C_EconEntity::m_AttributeManager
				+ client_dll::C_AttributeContainer::m_Item
				+ client_dll::C_EconItemView::m_iItemDefinitionIndex,
				&players[i].activeWeaponID, sizeof(uint16_t));
		if (players[i].weaponServices) {
			DMADevice::PrepareEX(DMADevice::hScatter, players[i].weaponServices + client_dll::CPlayer_WeaponServices::m_hMyWeapons,        &players[i].weaponCount, sizeof(int32_t));
			DMADevice::PrepareEX(DMADevice::hScatter, players[i].weaponServices + client_dll::CPlayer_WeaponServices::m_hMyWeapons + 0x8,  &players[i].weaponData,  sizeof(uint64_t));
		}
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// ── Pass 3: weaponData → weapon handles ──
	for (int i = 0; i < playerCount; i++) {
		if (players[i].weaponCount < 0 || players[i].weaponCount > 16) players[i].weaponCount = 0;
		if (!players[i].weaponData) continue;
		for (int j = 0; j < players[i].weaponCount; j++)
			DMADevice::PrepareEX(DMADevice::hScatter, players[i].weaponData + j * sizeof(uint32_t), &players[i].weapons[j].weaponHandle, sizeof(uint32_t));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// ── Pass 4: handle → list entry ──
	for (int i = 0; i < playerCount; i++) {
		for (int j = 0; j < players[i].weaponCount; j++) {
			if (!players[i].weapons[j].weaponHandle) continue;
			int index = players[i].weapons[j].weaponHandle & 0x7FFF;
			DMADevice::PrepareEX(DMADevice::hScatter, entityList + (0x8 * (index >> 9) + 16), &players[i].weapons[j].weaponEntry, sizeof(uint64_t));
		}
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// ── Pass 5: list entry → weapon controller ──
	for (int i = 0; i < playerCount; i++) {
		for (int j = 0; j < players[i].weaponCount; j++) {
			if (!players[i].weapons[j].weaponEntry) continue;
			int index = players[i].weapons[j].weaponHandle & 0x7FFF;
			DMADevice::PrepareEX(DMADevice::hScatter, players[i].weapons[j].weaponEntry + 0x70 * (index & 0x1FF), &players[i].weapons[j].weaponController, sizeof(uint64_t));
		}
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// ── Pass 6: weapon controller → weapon ID ──
	for (int i = 0; i < playerCount; i++) {
		for (int j = 0; j < players[i].weaponCount; j++) {
			if (!players[i].weapons[j].weaponController) continue;
			DMADevice::PrepareEX(DMADevice::hScatter,
				players[i].weapons[j].weaponController
				+ client_dll::C_EconEntity::m_AttributeManager
				+ client_dll::C_AttributeContainer::m_Item
				+ client_dll::C_EconItemView::m_iItemDefinitionIndex,
				&players[i].weapons[j].weaponID, sizeof(uint16_t));
		}
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);
}
