#include "pch.h"
#include "sdk.h"
#include "updater.h"

static bool isValidPtr(uint64_t p) {
	return p > 0x10000ULL && p < 0x7FFFFFFFFFFF0000ULL;
}

static bool isValidAscii(const char* s, size_t n) {
	if (!s || !s[0]) return false;
	for (size_t i = 0; i < n && s[i]; i++) {
		unsigned char c = (unsigned char)s[i];
		if (c < 0x20 || c > 0x7E) return false;
	}
	return true;
}

// ─── update ───────────────────────────────────────────────────────────────────

void CGame::update() {
	(void)updater::classOffsetsReady.load(std::memory_order_acquire);

	uint64_t base = g_DMA.moduleBase;

	// Scatter A: module-level pointers
	uint64_t globalVarsPtr = 0, newController = 0, newPawn = 0, newEntityList = 0, plantedC4Ptr = 0;
	g_DMA.PrepareEX(base + client_dll::dwGlobalVars,            &globalVarsPtr, sizeof(uint64_t));
	g_DMA.PrepareEX(base + client_dll::dwLocalPlayerController, &newController, sizeof(uint64_t));
	g_DMA.PrepareEX(base + client_dll::dwLocalPlayerPawn,       &newPawn,       sizeof(uint64_t));
	g_DMA.PrepareEX(base + client_dll::dwEntityList,            &newEntityList, sizeof(uint64_t));
	g_DMA.PrepareEX(base + client_dll::dwPlantedC4,             &plantedC4Ptr,  sizeof(uint64_t));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	if (!isValidPtr(globalVarsPtr)) globalVarsPtr = 0;
	if (!isValidPtr(newEntityList)) newEntityList  = 0;
	if (!isValidPtr(newController)) newController  = 0;
	if (!isValidPtr(newPawn))       newPawn        = 0;
	if (!isValidPtr(plantedC4Ptr))  plantedC4Ptr   = 0;

	static uint64_t prevEL = 0, prevCtrl = 0;
	if (newEntityList != prevEL)   { std::cout << "[SDK]: Entity list -> 0x"      << std::hex << newEntityList << std::dec << "\n"; prevEL   = newEntityList; }
	if (newController != prevCtrl) { std::cout << "[SDK]: Local controller -> 0x" << std::hex << newController << std::dec << "\n"; prevCtrl = newController; }

	if (newEntityList) entityList = newEntityList;
	localPlayer.controllerBase = newController;
	localPlayer.pawnBase       = newPawn;

	// Scatter B: map ptr, local player fields, entity list chunk roots
	uint64_t mapPtr = 0;
	memset(players, 0, sizeof(players));

	if (globalVarsPtr)
		g_DMA.PrepareEX(globalVarsPtr + 0x0188, &mapPtr, sizeof(uint64_t));

	if (localPlayer.controllerBase)
		localPlayer.ctrl.Read(localPlayer.controllerBase);

	if (localPlayer.pawnBase)
		g_DMA.PrepareEX(localPlayer.pawnBase + client_dll::C_BasePlayerPawn::m_vOldOrigin, &localPlayer.pawn.position, sizeof(Vector3));

	for (int i = 0; i < 64; i++)
		g_DMA.PrepareEX(entityList + (0x8 * ((i + 1) >> 9) + 16), &players[i].listEntry, sizeof(uint64_t));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	// Scatter C: string data
	char mapNameBuf[32] = {};
	if (isValidPtr(mapPtr))                   g_DMA.PrepareEX(mapPtr,                    mapNameBuf,                sizeof(mapNameBuf));
	if (isValidPtr(localPlayer.ctrl.nameAddr)) g_DMA.PrepareEX(localPlayer.ctrl.nameAddr, localPlayer.ctrl.name,    sizeof(localPlayer.ctrl.name));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	mapNameBuf[sizeof(mapNameBuf) - 1] = '\0';

	auto logOnChange = [](char* dst, size_t n, const char* src, const char* label) {
		if (!isValidAscii(src, n)) return;
		if (strncmp(dst, src, n) != 0) {
			std::cout << "[SDK]: " << label << " -> " << src << "\n";
			memcpy(dst, src, n);
		}
	};
	if (isValidPtr(mapPtr)) logOnChange(mapName, sizeof(mapName), mapNameBuf, "Map");

	// Entity chain: listEntry → controllerBase → pawnHandle → listEntry2 → pawnBase
	resolveEntityChain();

	getPlayerData();
	getWeapons();
	getBombData(plantedC4Ptr);
}

// ─── resolveEntityChain ───────────────────────────────────────────────────────

void CGame::resolveEntityChain() {
	for (int i = 0; i < 64; i++)
		if (players[i].listEntry)
			g_DMA.PrepareEX(players[i].listEntry + 0x70 * ((i + 1) & 0x1FF), &players[i].controllerBase, sizeof(uint64_t));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	for (int i = 0; i < 64; i++)
		if (players[i].controllerBase)
			g_DMA.PrepareEX(players[i].controllerBase + client_dll::CCSPlayerController::m_hPlayerPawn, &players[i].pawnHandle, sizeof(uint32_t));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	for (int i = 0; i < 64; i++)
		if (players[i].pawnHandle)
			g_DMA.PrepareEX(entityList + 0x8 * ((players[i].pawnHandle & 0x7FFF) >> 9) + 16, &players[i].listEntry2, sizeof(uint64_t));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	for (int i = 0; i < 64; i++)
		if (players[i].listEntry2)
			g_DMA.PrepareEX(players[i].listEntry2 + 0x70 * (players[i].pawnHandle & 0x1FF), &players[i].pawnBase, sizeof(uint64_t));
	g_DMA.ExecuteRead();
	g_DMA.Clear();
}

// ─── getPlayerData ────────────────────────────────────────────────────────────

void CGame::getPlayerData() {
	for (int i = 0; i < 64; i++) {
		if (!players[i].controllerBase || !players[i].pawnBase) continue;
		players[i].ctrl.Read(players[i].controllerBase);
		players[i].pawn.Read(players[i].pawnBase);
	}
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	for (int i = 0; i < 64; i++) {
		if (!players[i].ctrl.nameAddr) continue;
		players[i].ctrl.ReadName();
	}
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	for (int i = 0; i < 64; i++) {
		players[i].ctrl.name[sizeof(players[i].ctrl.name) - 1]                  = '\0';
		players[i].pawn.lastPlaceName[sizeof(players[i].pawn.lastPlaceName) - 1] = '\0';
	}
}

// ─── getWeapons ───────────────────────────────────────────────────────────────

void CGame::getWeapons() {
	for (int i = 0; i < 64; i++) {
		if (!players[i].pawn.activeWeapon) continue;
		uint64_t itemAddr = players[i].pawn.activeWeapon
			+ client_dll::C_EconEntity::m_AttributeManager
			+ client_dll::C_AttributeContainer::m_Item
			+ client_dll::C_EconItemView::m_iItemDefinitionIndex;
		g_DMA.PrepareEX(itemAddr, &players[i].pawn.activeWeaponID, sizeof(players[i].pawn.activeWeaponID));
	}
	g_DMA.ExecuteRead();
	g_DMA.Clear();
}
