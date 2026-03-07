#include "pch.h"
#include "sdk.h"

void CGame::update() {
	getMap();

	// Local player
	getLocalPlayer();
	getLocalPawn();
	getLocalTeam();
	getLocalPos();
	getLocalName();

	// Entities
	getEntityList();
	getPlayers();     // resolves pointer chain: listEntry -> controller -> pawnAddr -> listEntry2 -> pawn
	getPlayerData();  // two batched passes for all remaining per-player fields
	getWeapons();
}


// ─── Map ─────────────────────────────────────────────────────────────────────

void CGame::getMap() {
	uintptr_t mBase  = DMADevice::getModuleBase(_strdup("matchmaking.dll"));
	uintptr_t mapPtr = DMADevice::MemReadPtr<uint64_t>(mBase + matchmaking_dll::dwGameTypes + matchmaking_dll::dwGameTypes_mapName);
	DMADevice::MemRead(mapPtr + 0x4, &map, sizeof(map));

	static char prevMap[36] = {};
	if (strncmp(map, prevMap, sizeof(map)) != 0) {
		std::cout << "[SDK]: Map -> " << (map[0] ? map : "(none)") << "\n";
		memcpy(prevMap, map, sizeof(map));
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
}
void CGame::getLocalPawn() {
	localPlayer.pawn = DMADevice::MemReadPtr<uint64_t>(DMADevice::moduleBase + client_dll::dwLocalPlayerPawn);
}
void CGame::getLocalTeam() {
	localPlayer.teamID = DMADevice::MemReadPtr<uint8_t>(localPlayer.controller + client_dll::C_BaseEntity::m_iTeamNum);
}
void CGame::getLocalPos() {
	DMADevice::MemRead(localPlayer.pawn + client_dll::C_BasePlayerPawn::m_vOldOrigin, &localPlayer.position, sizeof(Vector3));
}
void CGame::getLocalName() {
	localPlayer.nameAddr = DMADevice::MemReadPtr<uint64_t>(localPlayer.controller + client_dll::CCSPlayerController::m_sSanitizedPlayerName);
	DMADevice::MemRead(localPlayer.nameAddr, &localPlayer.name, sizeof(char[36]));

	static char prevName[36] = {};
	if (strncmp(localPlayer.name, prevName, sizeof(prevName)) != 0) {
		std::cout << "[SDK]: Local player name -> " << (localPlayer.name[0] ? localPlayer.name : "(empty)") << "\n";
		memcpy(prevName, localPlayer.name, sizeof(prevName));
	}
}


// ─── Entity pointer chain ─────────────────────────────────────────────────────
// Each pass depends on the previous result, so they must remain sequential.

void CGame::getPlayers() {
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
	// Pass 1: everything readable from the already-resolved controller and pawn.
	//         nameAddr and weaponPtr are fetched here so pass 2 can dereference them.
	for (int i = 0; i < 64; i++) {
		if (!players[i].controller) continue;
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::CCSPlayerController::m_sSanitizedPlayerName, &players[i].nameAddr,  sizeof(uint64_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::C_BaseEntity::m_iTeamNum,                    &players[i].teamID,    sizeof(uint8_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].controller + client_dll::CCSPlayerController::m_iCompTeammateColor,   &players[i].color,     sizeof(DWORD));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_BaseEntity::m_iHealth,                    &players[i].health,    sizeof(uint32_t));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_CSPlayerPawn::m_angEyeAngles,             &players[i].eyeAngles, sizeof(Vector2));
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_BasePlayerPawn::m_vOldOrigin,             &players[i].position,  sizeof(Vector3));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// Pass 2: dereference pointers obtained in pass 1 (name string).
	for (int i = 0; i < 64; i++) {
		if (!players[i].controller) continue;
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].nameAddr, &players[i].name, sizeof(char[36]));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	int count = 0;
	for (int i = 0; i < 64; i++)
		if (players[i].controller) count++;
	static int prevCount = -1;
	if (count != prevCount) {
		std::cout << "[SDK]: Active players -> " << count << "\n";
		prevCount = count;
	}
}

// getActiveWeapons() is eliminated — merged here.
void CGame::getWeapons()
{
	// ── Pass 1: pawn → activeWeapon + weaponServices (was 2 separate passes) ──
	for (int i = 0; i < 64; i++) {
		DMADevice::PrepareEX(DMADevice::hScatter,
			players[i].pawn + client_dll::C_CSPlayerPawn::m_pClippingWeapon,
			&players[i].activeWeapon, sizeof(uint64_t));
		DMADevice::PrepareEX(DMADevice::hScatter,
			players[i].pawn + client_dll::C_BasePlayerPawn::m_pWeaponServices,
			&players[i].weaponServices, sizeof(uint64_t));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// ── Pass 2: activeWeapon → activeWeaponID
	//           weaponServices → weaponCount + weaponData  (was 3 separate passes) ──
	for (int i = 0; i < 64; i++) {
		DMADevice::PrepareEX(DMADevice::hScatter,
			players[i].activeWeapon
			+ client_dll::C_EconEntity::m_AttributeManager
			+ client_dll::C_AttributeContainer::m_Item
			+ client_dll::C_EconItemView::m_iItemDefinitionIndex,
			&players[i].activeWeaponID, sizeof(uint16_t));
		DMADevice::PrepareEX(DMADevice::hScatter,
			players[i].weaponServices + client_dll::CPlayer_WeaponServices::m_hMyWeapons,
			&players[i].weaponCount, sizeof(int32_t));
		DMADevice::PrepareEX(DMADevice::hScatter,
			players[i].weaponServices + client_dll::CPlayer_WeaponServices::m_hMyWeapons + 0x8,
			&players[i].weaponData, sizeof(uint64_t));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// ── Pass 3: weaponData → weapon handles ──
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < players[i].weaponCount; j++) {
			DMADevice::PrepareEX(DMADevice::hScatter,
				players[i].weaponData + j * sizeof(uint32_t),
				&players[i].weapons[j].weaponHandle, sizeof(uint32_t));
		}
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// ── Pass 4: handle → list entry ──
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < players[i].weaponCount; j++) {
			int index = players[i].weapons[j].weaponHandle & 0x7FFF;
			DMADevice::PrepareEX(DMADevice::hScatter,
				entityList + (0x8 * (index >> 9) + 16),
				&players[i].weapons[j].weaponEntry, sizeof(uint64_t));
		}
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// ── Pass 5: list entry → weapon controller ──
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < players[i].weaponCount; j++) {
			int index = players[i].weapons[j].weaponHandle & 0x7FFF;
			DMADevice::PrepareEX(DMADevice::hScatter,
				players[i].weapons[j].weaponEntry + 0x70 * (index & 0x1FF),
				&players[i].weapons[j].weaponController, sizeof(uint64_t));
		}
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// ── Pass 6: weapon controller → weapon ID ──
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < players[i].weaponCount; j++) {
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