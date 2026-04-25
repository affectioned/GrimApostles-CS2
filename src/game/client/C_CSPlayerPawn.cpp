#include "pch.h"
#include "C_CSPlayerPawn.h"
#include "offsets.h"
#include "GameGlobals.h"
#include "../CS2Context.h"

void C_CSPlayerPawn::Read(uint64_t base) {
	g_Scatter->Add(base + client_dll::C_BaseEntity::m_iHealth,                   &health);
	g_Scatter->Add(base + client_dll::C_BaseEntity::m_lifeState,                 &lifeState);
	g_Scatter->Add(base + client_dll::C_CSPlayerPawn::m_angEyeAngles,            &eyeAngles);
	g_Scatter->Add(base + client_dll::C_BasePlayerPawn::m_vOldOrigin,            &position);
	g_Scatter->Add(base + client_dll::C_CSPlayerPawn::m_bIsDefusing,             &isDefusing);
	g_Scatter->AddRaw(base + client_dll::C_CSPlayerPawn::m_szLastPlaceName,      sizeof(lastPlaceName), lastPlaceName);
	g_Scatter->Add(base + client_dll::C_BasePlayerPawn::m_pWeaponServices,       &weaponServicesPtr);
}

// ── t_LocalPlayerPos — 8 ms ───────────────────────────────────────────────────
// Reads only the local player's world position for low-latency self-tracking.

void CS2Context::t_LocalPlayerPos()
{
	if (!m_Local->localPlayer.pawnBase) return;

	uint8_t curLifeState = 255;
	g_Scatter->Add(m_Local->localPlayer.pawnBase + client_dll::C_BasePlayerPawn::m_vOldOrigin,
	               &m_Local->localPlayer.pawn.position);
	g_Scatter->Add(m_Local->localPlayer.pawnBase + client_dll::C_BaseEntity::m_lifeState,
	               &curLifeState);
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Dead → alive transition = new round; suppress the stale bomb entity so
	// t_BombState doesn't re-populate from the old entity (m_bC4Activated and
	// m_bBombTicking stay true after T-elimination round end). We identify the
	// stale bomb by its m_flC4Blow value — unique to each plant event — rather
	// than by pointer (CS2 reuses the same entity address every round).
	static uint8_t prevLifeState = 0;
	if (prevLifeState != 0 && curLifeState == 0) {
		m_SuppressedC4Blow = m_Local->bomb.c4Blow;
		m_Local->bomb      = {};
		m_PlantedC4Ptr     = 0;
	}
	prevLifeState = curLifeState;
}

// ── t_PlayerPositions — 8 ms ──────────────────────────────────────────────────
// Reads pawn fields (health, lifeState, position, angles, defusing, etc.)
// for every valid player slot.

void CS2Context::t_PlayerPositions()
{
	for (int i = 0; i < MAX_ENTITIES; i++)
		if (m_Local->players[i].pawnBase)
			m_Local->players[i].pawn.Read(m_Local->players[i].pawnBase);
	g_Scatter->Execute();
	g_Scatter->Clear();

	for (int i = 0; i < MAX_ENTITIES; i++)
		m_Local->players[i].pawn.lastPlaceName[sizeof(m_Local->players[i].pawn.lastPlaceName) - 1] = '\0';
}

// ── t_PlayerWeapons — 100 ms ──────────────────────────────────────────────────
// Reads active weapon definition IDs (item definition index) for all players.

void CS2Context::t_PlayerWeapons()
{
	if (!m_Local->entityList) return;

	// Pass 1: weapon services → active weapon handle
	static uint32_t handles[MAX_ENTITIES];
	memset(handles, 0, sizeof(handles));
	for (int i = 0; i < MAX_ENTITIES; i++) {
		if (!m_Local->players[i].pawnBase || !m_Local->players[i].pawn.weaponServicesPtr) continue;
		g_Scatter->Add(m_Local->players[i].pawn.weaponServicesPtr + client_dll::CPlayer_WeaponServices::m_hActiveWeapon,
		               &handles[i]);
	}
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Pass 2: handle → entity list chunk
	static uint64_t listEntries[MAX_ENTITIES];
	memset(listEntries, 0, sizeof(listEntries));
	for (int i = 0; i < MAX_ENTITIES; i++) {
		uint32_t h = handles[i];
		if (!h || h == 0xFFFFFFFF) continue;
		g_Scatter->Add(m_Local->entityList + 0x8 * ((h & 0x7FFF) >> 9) + 16, &listEntries[i]);
	}
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Pass 3: list chunk + slot → weapon entity pointer
	static uint64_t weaponPtrs[MAX_ENTITIES];
	memset(weaponPtrs, 0, sizeof(weaponPtrs));
	for (int i = 0; i < MAX_ENTITIES; i++) {
		uint32_t h = handles[i];
		if (!h || h == 0xFFFFFFFF || !listEntries[i]) continue;
		g_Scatter->Add(listEntries[i] + 0x70 * (h & 0x1FF), &weaponPtrs[i]);
	}
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Pass 4: weapon entity → item definition index
	for (int i = 0; i < MAX_ENTITIES; i++) {
		if (!weaponPtrs[i]) continue;
		uint64_t itemAddr = weaponPtrs[i]
			+ client_dll::C_EconEntity::m_AttributeManager
			+ client_dll::C_AttributeContainer::m_Item
			+ client_dll::C_EconItemView::m_iItemDefinitionIndex;
		g_Scatter->Add(itemAddr, &m_Local->players[i].pawn.activeWeaponID);
	}
	g_Scatter->Execute();
	g_Scatter->Clear();
}
