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
	g_Scatter->Add(base + client_dll::C_CSPlayerPawn::m_pClippingWeapon,         &activeWeapon);
	g_Scatter->Add(base + client_dll::C_CSPlayerPawn::m_bIsDefusing,             &isDefusing);
	g_Scatter->AddRaw(base + client_dll::C_CSPlayerPawn::m_szLastPlaceName,      sizeof(lastPlaceName), lastPlaceName);
	g_Scatter->Add(base + client_dll::C_BasePlayerPawn::m_pWeaponServices,       &weaponServicesPtr);
}

// ── t_LocalPlayerPos — 8 ms ───────────────────────────────────────────────────
// Reads only the local player's world position for low-latency self-tracking.

void CS2Context::t_LocalPlayerPos()
{
	if (!m_Local->localPlayer.pawnBase) return;
	g_Scatter->Add(m_Local->localPlayer.pawnBase + client_dll::C_BasePlayerPawn::m_vOldOrigin,
	               &m_Local->localPlayer.pawn.position);
	g_Scatter->Execute();
	g_Scatter->Clear();
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
	for (int i = 0; i < MAX_ENTITIES; i++) {
		if (!m_Local->players[i].pawnBase) continue;
		if (!m_Local->players[i].pawn.activeWeapon) continue;
		uint64_t itemAddr = m_Local->players[i].pawn.activeWeapon
			+ client_dll::C_EconEntity::m_AttributeManager
			+ client_dll::C_AttributeContainer::m_Item
			+ client_dll::C_EconItemView::m_iItemDefinitionIndex;
		g_Scatter->Add(itemAddr, &m_Local->players[i].pawn.activeWeaponID);
	}
	g_Scatter->Execute();
	g_Scatter->Clear();
}
