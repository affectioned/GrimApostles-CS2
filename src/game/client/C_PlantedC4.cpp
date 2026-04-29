#include "pch.h"
#include "C_PlantedC4.h"
#include "offsets.h"
#include "GameGlobals.h"
#include "../CS2Context.h"

// ── t_BombState — 16 ms ───────────────────────────────────────────────────────
// Dereferences dwPlantedC4 → entity, then reads bomb flags, site, scene node,
// and (if ticking) world position. Latches plantTime on the rising edge of
// m_bBombTicking so timeRemaining() works off a steady clock.

void CS2Context::t_BombState()
{
	// Re-read dwPlantedC4 fresh — never cache this pointer across map transitions.
	uint64_t plantedC4Ptr = 0;
	g_Scatter->Add(g_ClientBase + client_dll::dwPlantedC4, &plantedC4Ptr);
	g_Scatter->Execute();
	g_Scatter->Clear();

	if (!plantedC4Ptr || plantedC4Ptr < 0x10000 || plantedC4Ptr > 0x7FFFFFFFFFFF) {
		m_Local->bomb      = {};
		m_SuppressedC4Blow = 0.0f;
		return;
	}
	m_PlantedC4Ptr = plantedC4Ptr;

	uint64_t c4 = 0;
	g_Scatter->Add(plantedC4Ptr, &c4);
	g_Scatter->Execute();
	g_Scatter->Clear();

	if (!c4) {
		m_Local->bomb.entity      = 0;
		m_Local->bomb.isTicking   = false;
		m_Local->bomb.plantTimeSet = false;
		return;
	}

	// Reset plant timer and position if the entity pointer changed (new round)
	if (c4 != m_Local->bomb.entity) {
		m_Local->bomb.plantTimeSet = false;
		m_Local->bomb.position     = {};
	}
	m_Local->bomb.entity = c4;

	bool    newActivated = false, newTicking  = false;
	bool    newDefusing  = false, newExploded = false, newDefused = false;
	int32_t newSite      = -1;
	float   newC4Blow    = 0.0f;

	g_Scatter->Add(c4 + client_dll::C_BaseEntity::m_pGameSceneNode, &m_Local->bomb.sceneNode);
	g_Scatter->Add(c4 + client_dll::C_PlantedC4::m_bC4Activated,    &newActivated);
	g_Scatter->Add(c4 + client_dll::C_PlantedC4::m_bBombTicking,    &newTicking);
	g_Scatter->Add(c4 + client_dll::C_PlantedC4::m_nBombSite,       &newSite);
	g_Scatter->Add(c4 + client_dll::C_PlantedC4::m_bBeingDefused,   &newDefusing);
	g_Scatter->Add(c4 + client_dll::C_PlantedC4::m_bHasExploded,    &newExploded);
	g_Scatter->Add(c4 + client_dll::C_PlantedC4::m_bBombDefused,    &newDefused);
	g_Scatter->Add(c4 + client_dll::C_PlantedC4::m_flC4Blow,        &newC4Blow);
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Stale entity from previous round — clear entity so t_CarrierScan isn't suppressed
	if (!newActivated) {
		m_Local->bomb.entity       = 0;
		m_Local->bomb.isTicking    = false;
		m_Local->bomb.plantTimeSet = false;
		return;
	}

	// Suppressed after a respawn: ignore this entity until its m_flC4Blow changes,
	// which only happens when a new bomb is planted (each plant gets a unique blow time).
	// We suppress by blow-time rather than pointer because CS2 reuses the entity address.
	if (m_SuppressedC4Blow != 0.0f && newC4Blow == m_SuppressedC4Blow) {
		m_Local->bomb = {};
		return;
	}
	m_SuppressedC4Blow = 0.0f;

	// Reject corrupt reads: site must be 0 (A) or 1 (B) once ticking
	if (newTicking && newSite != 0 && newSite != 1) return;

	// Latch plant time on rising edge
	if (newTicking && !m_Local->bomb.isTicking) {
		m_Local->bomb.plantTime    = C_PlantedC4::Clock::now();
		m_Local->bomb.plantTimeSet = true;
		Log::Info("[Bomb]: Planted - site {}", newSite == 0 ? "A" : "B");
	}
	if (!newTicking)
		m_Local->bomb.plantTimeSet = false;

	m_Local->bomb.isTicking      = newTicking;
	m_Local->bomb.isBeingDefused = newDefusing;
	m_Local->bomb.hasExploded    = newExploded;
	m_Local->bomb.hasDefused     = newDefused;
	m_Local->bomb.site           = newSite;
	m_Local->bomb.c4Blow         = newC4Blow;

	if (newTicking && m_Local->bomb.sceneNode > 0x10000 && m_Local->bomb.sceneNode < 0x7FFFFFFFFFFF) {
		g_Scatter->Add(m_Local->bomb.sceneNode + client_dll::CGameSceneNode::m_vecAbsOrigin,
		               &m_Local->bomb.position);
		g_Scatter->Execute();
		g_Scatter->Clear();
	}
}

// ── t_CarrierScan — 100 ms ───────────────────────────────────────────────────
// Scans each T player's weapon list for item definition 49 (C4).
// Skipped entirely once the bomb is planted to save 4 round trips.

void CS2Context::t_CarrierScan()
{
	// Once planted, no carrier to find
	if (m_Local->bomb.entity) {
		if (m_Local->bomb.carrierSlot >= 0)
			m_Local->bomb.planterSlot = m_Local->bomb.carrierSlot;
		m_Local->bomb.isCarried   = false;
		m_Local->bomb.carrierSlot = -1;
		return;
	}
	if (!m_Local->entityList) return;

	static constexpr int kSlots = 8;

	// Pass 1: weaponServicesPtr → m_pData of C_NetworkUtlVectorBase<CHandle<>>
	// Layout at m_hMyWeapons: +0x00=m_Size(int32), +0x04=m_nMaxSize, +0x08=m_pData(ptr)
	static uint64_t weaponListPtrs[MAX_ENTITIES];
	memset(weaponListPtrs, 0, sizeof(weaponListPtrs));

	for (int i = 0; i < MAX_ENTITIES; i++) {
		if (!m_Local->players[i].pawnBase || m_Local->players[i].pawn.lifeState != 0) continue;
		if (m_Local->players[i].ctrl.teamID != TEAM_T) continue;
		if (!m_Local->players[i].pawn.weaponServicesPtr) continue;
		g_Scatter->Add(m_Local->players[i].pawn.weaponServicesPtr + client_dll::CPlayer_WeaponServices::m_hMyWeapons + 0x8,
		               &weaponListPtrs[i]);
	}
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Pass 2: weapon list base → array of CHandle<C_BaseCombatWeapon>
	static uint32_t weaponHandles[MAX_ENTITIES][kSlots];
	memset(weaponHandles, 0, sizeof(weaponHandles));

	for (int i = 0; i < MAX_ENTITIES; i++) {
		if (weaponListPtrs[i] < 0x10000 || weaponListPtrs[i] > 0x7FFFFFFFFFFF) continue;
		g_Scatter->AddRaw(weaponListPtrs[i], kSlots * sizeof(uint32_t), weaponHandles[i]);
	}
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Pass 3: each handle → entity list chunk entry
	static uint64_t weaponListEntries[MAX_ENTITIES][kSlots];
	memset(weaponListEntries, 0, sizeof(weaponListEntries));

	for (int i = 0; i < MAX_ENTITIES; i++) {
		for (int j = 0; j < kSlots; j++) {
			uint32_t h = weaponHandles[i][j];
			if (!h || h == 0xFFFFFFFF) continue;
			g_Scatter->Add(m_Local->entityList + 0x8 * ((h & 0x7FFF) >> 9) + 16,
			               &weaponListEntries[i][j]);
		}
	}
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Pass 4: chunk entry + slot offset → weapon entity pointer
	static uint64_t weaponEntityPtrs[MAX_ENTITIES][kSlots];
	memset(weaponEntityPtrs, 0, sizeof(weaponEntityPtrs));

	for (int i = 0; i < MAX_ENTITIES; i++) {
		for (int j = 0; j < kSlots; j++) {
			uint32_t h = weaponHandles[i][j];
			if (!h || h == 0xFFFFFFFF || !weaponListEntries[i][j]) continue;
			g_Scatter->Add(weaponListEntries[i][j] + 0x70 * (h & 0x1FF),
			               &weaponEntityPtrs[i][j]);
		}
	}
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Pass 5: weapon entity → item definition index
	static uint16_t weaponDefIds[MAX_ENTITIES][kSlots];
	memset(weaponDefIds, 0, sizeof(weaponDefIds));

	for (int i = 0; i < MAX_ENTITIES; i++) {
		for (int j = 0; j < kSlots; j++) {
			if (!weaponEntityPtrs[i][j]) continue;
			uint64_t itemAddr = weaponEntityPtrs[i][j]
				+ client_dll::C_EconEntity::m_AttributeManager
				+ client_dll::C_AttributeContainer::m_Item
				+ client_dll::C_EconItemView::m_iItemDefinitionIndex;
			g_Scatter->Add(itemAddr, &weaponDefIds[i][j]);
		}
	}
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Find the slot holding C4 (item def 49)
	m_Local->bomb.isCarried   = false;
	m_Local->bomb.carrierSlot = -1;
	for (int i = 0; i < MAX_ENTITIES; i++) {
		if (!m_Local->players[i].controllerBase || m_Local->players[i].pawn.lifeState != 0) continue;
		if (m_Local->players[i].ctrl.teamID != TEAM_T) continue;
		for (int j = 0; j < kSlots; j++) {
			if (weaponDefIds[i][j] == 49) {
				m_Local->bomb.isCarried   = true;
				m_Local->bomb.carrierSlot = i;
				return;
			}
		}
	}
}
