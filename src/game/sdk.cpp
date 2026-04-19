#include "pch.h"
#include "sdk.h"
#include "updater.h"

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

	// Scatter A: module-level pointers (1 round trip)
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

	// Transient DMA failure — skip frame, keep last good state
	if (!newEntityList && entityList) return;

	static uint64_t prevEL = 0, prevCtrl = 0;
	if (newEntityList != prevEL)   { std::cout << "[SDK]: Entity list -> 0x"      << std::hex << newEntityList << std::dec << "\n"; prevEL   = newEntityList; }
	if (newController != prevCtrl) { std::cout << "[SDK]: Local controller -> 0x" << std::hex << newController << std::dec << "\n"; prevCtrl = newController; }

	if (newEntityList) entityList = newEntityList;
	if (newController) localPlayer.controllerBase = newController;
	if (newPawn)       localPlayer.pawnBase       = newPawn;

	// Scatter B: map ptr, local player, C4 entity dereference, entity chunk ptr (1 round trip)
	// All 64 player slots (indices 1-64) live in chunk 0 — read that one pointer once.
	uint64_t mapPtr = 0, plantedC4Entity = 0, entityChunk = 0;
	memset(players, 0, sizeof(players));

	if (isValidPtr(globalVarsPtr))
		g_DMA.PrepareEX(globalVarsPtr + 0x0188, &mapPtr, sizeof(uint64_t));
	if (isValidPtr(localPlayer.controllerBase))
		localPlayer.ctrl.Read(localPlayer.controllerBase);
	if (isValidPtr(localPlayer.pawnBase))
		g_DMA.PrepareEX(localPlayer.pawnBase + client_dll::C_BasePlayerPawn::m_vOldOrigin, &localPlayer.pawn.position, sizeof(Vector3));
	if (isValidPtr(plantedC4Ptr))
		g_DMA.PrepareEX(plantedC4Ptr, &plantedC4Entity, sizeof(uint64_t));
	g_DMA.PrepareEX(entityList + 16, &entityChunk, sizeof(uint64_t));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	if (!isValidPtr(plantedC4Entity)) plantedC4Entity = 0;

	static uint64_t prevC4 = 0;
	if (plantedC4Entity != prevC4) { std::cout << "[SDK]: PlantedC4 entity -> 0x" << std::hex << plantedC4Entity << std::dec << "\n"; prevC4 = plantedC4Entity; }

	// Propagate chunk ptr to all 64 player slots (replaces 64 identical PrepareEX calls)
	if (isValidPtr(entityChunk))
		for (int i = 0; i < kMaxPlayers; i++) players[i].listEntry = entityChunk;

	// Scatter C: string data (1 round trip)
	char mapNameBuf[32] = {};
	if (isValidPtr(mapPtr))                   g_DMA.PrepareEX(mapPtr,                    mapNameBuf,             sizeof(mapNameBuf));
	if (isValidPtr(localPlayer.ctrl.nameAddr)) g_DMA.PrepareEX(localPlayer.ctrl.nameAddr, localPlayer.ctrl.name, sizeof(localPlayer.ctrl.name));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	mapNameBuf[sizeof(mapNameBuf) - 1] = '\0';
	auto logOnChange = [](char* dst, size_t n, const char* src, const char* label) {
		if (!isValidAscii(src, n)) return;
		if (strncmp(dst, src, n) != 0) { std::cout << "[SDK]: " << label << " -> " << src << "\n"; memcpy(dst, src, n); }
	};
	if (isValidPtr(mapPtr)) logOnChange(mapName, sizeof(mapName), mapNameBuf, "Map");

	// Entity chain: listEntry → controllerBase → pawnHandle → listEntry2 → pawnBase (4 round trips)
	resolveEntityChain();

	// Player data pass 1: ctrl + pawn fields (1 round trip)
	// Player data pass 2: names + weapon IDs batched together (1 round trip)
	getPlayerData();

	// Carrier detection: 4 round trips — skipped entirely when bomb is planted
	if (!bomb.entity) {
		getCarrier();
	} else {
		bomb.isCarried   = false;
		bomb.carrierSlot = -1;
	}

	// Bomb state + position (1-2 round trips)
	getBombData(plantedC4Entity);
	applyCache();
}

// ─── resolveEntityChain ───────────────────────────────────────────────────────

void CGame::resolveEntityChain() {
	for (int i = 0; i < kMaxPlayers; i++)
		if (isValidPtr(players[i].listEntry))
			g_DMA.PrepareEX(players[i].listEntry + 0x70 * ((i + 1) & 0x1FF), &players[i].controllerBase, sizeof(uint64_t));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	for (int i = 0; i < kMaxPlayers; i++)
		if (isValidPtr(players[i].controllerBase))
			g_DMA.PrepareEX(players[i].controllerBase + client_dll::CCSPlayerController::m_hPlayerPawn, &players[i].pawnHandle, sizeof(uint32_t));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	for (int i = 0; i < kMaxPlayers; i++)
		if (players[i].pawnHandle) // handle, not a pointer — zero check is correct
			g_DMA.PrepareEX(entityList + 0x8 * ((players[i].pawnHandle & 0x7FFF) >> 9) + 16, &players[i].listEntry2, sizeof(uint64_t));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	for (int i = 0; i < kMaxPlayers; i++)
		if (isValidPtr(players[i].listEntry2))
			g_DMA.PrepareEX(players[i].listEntry2 + 0x70 * (players[i].pawnHandle & 0x1FF), &players[i].pawnBase, sizeof(uint64_t));
	g_DMA.ExecuteRead();
	g_DMA.Clear();
}

// ─── getPlayerData ────────────────────────────────────────────────────────────

void CGame::getPlayerData() {
	// Pass 1: ctrl fields + pawn fields
	for (int i = 0; i < kMaxPlayers; i++) {
		if (!isValidPtr(players[i].controllerBase) || !isValidPtr(players[i].pawnBase)) continue;
		players[i].ctrl.Read(players[i].controllerBase);
		players[i].pawn.Read(players[i].pawnBase);
	}
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	// Pass 2: name strings + active weapon IDs — both independent, batched together
	for (int i = 0; i < kMaxPlayers; i++) {
		if (isValidPtr(players[i].ctrl.nameAddr))
			players[i].ctrl.ReadName();

		if (isValidPtr(players[i].pawn.activeWeapon)) {
			uint64_t itemAddr = players[i].pawn.activeWeapon
				+ client_dll::C_EconEntity::m_AttributeManager
				+ client_dll::C_AttributeContainer::m_Item
				+ client_dll::C_EconItemView::m_iItemDefinitionIndex;
			g_DMA.PrepareEX(itemAddr, &players[i].pawn.activeWeaponID, sizeof(players[i].pawn.activeWeaponID));
		}
	}
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	for (int i = 0; i < kMaxPlayers; i++) {
		players[i].ctrl.name[sizeof(players[i].ctrl.name) - 1]                  = '\0';
		players[i].pawn.lastPlaceName[sizeof(players[i].pawn.lastPlaceName) - 1] = '\0';
	}
}

// ─── getCarrier ───────────────────────────────────────────────────────────────

void CGame::getCarrier() {
	static constexpr int kSlots = 8;

	static uint64_t weaponListPtrs[kMaxPlayers];
	memset(weaponListPtrs, 0, sizeof(weaponListPtrs));

	for (int i = 0; i < kMaxPlayers; i++) {
		if (!isValidPtr(players[i].pawnBase) || players[i].pawn.lifeState != 0) continue;
		if (players[i].ctrl.teamID != 2) continue;
		if (!isValidPtr(players[i].pawn.weaponServicesPtr)) continue;
		g_DMA.PrepareEX(players[i].pawn.weaponServicesPtr + client_dll::CPlayer_WeaponServices::m_hMyWeapons,
		                &weaponListPtrs[i], sizeof(uint64_t));
	}
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	static uint32_t weaponHandles[kMaxPlayers][kSlots];
	memset(weaponHandles, 0, sizeof(weaponHandles));

	for (int i = 0; i < kMaxPlayers; i++) {
		if (!isValidPtr(weaponListPtrs[i])) continue;
		g_DMA.PrepareEX(weaponListPtrs[i], weaponHandles[i], kSlots * sizeof(uint32_t));
	}
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	static uint64_t weaponListEntries[kMaxPlayers][kSlots];
	memset(weaponListEntries, 0, sizeof(weaponListEntries));

	for (int i = 0; i < kMaxPlayers; i++) {
		for (int j = 0; j < kSlots; j++) {
			uint32_t h = weaponHandles[i][j];
			if (!h || h == 0xFFFFFFFF) continue;
			g_DMA.PrepareEX(entityList + 0x8 * ((h & 0x7FFF) >> 9) + 16, &weaponListEntries[i][j], sizeof(uint64_t));
		}
	}
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	static uint64_t weaponEntityPtrs[kMaxPlayers][kSlots];
	memset(weaponEntityPtrs, 0, sizeof(weaponEntityPtrs));

	for (int i = 0; i < kMaxPlayers; i++) {
		for (int j = 0; j < kSlots; j++) {
			uint32_t h = weaponHandles[i][j];
			if (!h || h == 0xFFFFFFFF || !isValidPtr(weaponListEntries[i][j])) continue;
			g_DMA.PrepareEX(weaponListEntries[i][j] + 0x70 * (h & 0x1FF), &weaponEntityPtrs[i][j], sizeof(uint64_t));
		}
	}
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	static uint16_t weaponDefIds[kMaxPlayers][kSlots];
	memset(weaponDefIds, 0, sizeof(weaponDefIds));

	for (int i = 0; i < kMaxPlayers; i++) {
		for (int j = 0; j < kSlots; j++) {
			if (!isValidPtr(weaponEntityPtrs[i][j])) continue;
			uint64_t itemAddr = weaponEntityPtrs[i][j]
				+ client_dll::C_EconEntity::m_AttributeManager
				+ client_dll::C_AttributeContainer::m_Item
				+ client_dll::C_EconItemView::m_iItemDefinitionIndex;
			g_DMA.PrepareEX(itemAddr, &weaponDefIds[i][j], sizeof(uint16_t));
		}
	}
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	bomb.isCarried   = false;
	bomb.carrierSlot = -1;
	for (int i = 0; i < kMaxPlayers; i++) {
		if (!isValidPtr(players[i].controllerBase) || players[i].pawn.lifeState != 0) continue;
		if (players[i].ctrl.teamID != 2) continue;
		for (int j = 0; j < kSlots; j++) {
			if (weaponDefIds[i][j] == 49) {
				bomb.isCarried   = true;
				bomb.carrierSlot = i;
				return;
			}
		}
	}
}

// ─── applyCache ───────────────────────────────────────────────────────────────

void CGame::applyCache() {
	for (int i = 0; i < kMaxPlayers; i++) {
		if (isValidPtr(players[i].pawnBase)) {
			playerCache[i]    = players[i];
			playerCacheAge[i] = 0;
		} else if (playerCacheAge[i] < kMaxCacheAge) {
			players[i] = playerCache[i];
			playerCacheAge[i]++;
		} else {
			playerCache[i]    = {};
			playerCacheAge[i] = 0;
		}
	}
}
