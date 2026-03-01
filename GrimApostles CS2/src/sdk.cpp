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
}


// ─── Map ─────────────────────────────────────────────────────────────────────

void CGame::getMap() {
	uintptr_t mBase  = DMADevice::getModuleBase(_strdup("matchmaking.dll"));
	uintptr_t mapPtr = DMADevice::MemReadPtr<uint64_t>(mBase + matchmaking_dll::dwGameTypes + matchmaking_dll::dwGameTypes_mapName);
	DMADevice::MemRead(mapPtr + 0x4, &map, sizeof(map));
}

void CGame::getEntityList() {
	entityList = DMADevice::MemReadPtr<uint64_t>(DMADevice::moduleBase + client_dll::dwEntityList);
}


// ─── Local player ─────────────────────────────────────────────────────────────

void CGame::getLocalPlayer() {
	localPlayer.controller = DMADevice::MemReadPtr<uint64_t>(DMADevice::moduleBase + client_dll::dwLocalPlayerController);
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
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].pawn       + client_dll::C_CSPlayerPawn::m_pClippingWeapon,          &players[i].weaponPtr, sizeof(uint64_t));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);

	// Pass 2: dereference pointers obtained in pass 1 (name string and weapon definition index).
	for (int i = 0; i < 64; i++) {
		if (!players[i].controller) continue;
		DMADevice::PrepareEX(DMADevice::hScatter, players[i].nameAddr, &players[i].name, sizeof(char[36]));
		DMADevice::PrepareEX(DMADevice::hScatter,
			players[i].weaponPtr + client_dll::C_EconEntity::m_AttributeManager
			                     + client_dll::C_AttributeContainer::m_Item
			                     + client_dll::C_EconItemView::m_iItemDefinitionIndex,
			&players[i].weaponID, sizeof(uint16_t));
	}
	DMADevice::ExecuteRead(DMADevice::hScatter);
	DMADevice::Clear(DMADevice::hScatter);
}
