#include "pch.h"
#include "../CS2Context.h"

// ── t_EntityChain — 500 ms ────────────────────────────────────────────────────
// 4-pass entity chain resolution: chunk ptr → controllerBase → pawnHandle
// → listEntry2 → pawnBase. Only the five chain pointer fields are reset so that
// ctrl/pawn data written by the slower timers persists between chain runs.

void CS2Context::t_EntityChain()
{
	if (!m_Local->entityList) return;

	// Pass 0: read chunk 0 pointer (player slots 1-64 all live in chunk 0)
	uint64_t newChunk = 0;
	g_Scatter->Add(m_Local->entityList + 16, &newChunk);
	g_Scatter->Execute();
	g_Scatter->Clear();

	if (!newChunk) return;
	m_EntityChunk = newChunk;

	// Reset only chain pointer fields — preserve ctrl/pawn data for slower timers
	for (int i = 0; i < MAX_ENTITIES; i++) {
		m_Local->players[i].listEntry      = m_EntityChunk;
		m_Local->players[i].controllerBase = 0;
		m_Local->players[i].pawnHandle     = 0;
		m_Local->players[i].listEntry2     = 0;
		m_Local->players[i].pawnBase       = 0;
	}

	// Pass 1: listEntry + slot offset → controllerBase
	for (int i = 0; i < MAX_ENTITIES; i++)
		if (m_Local->players[i].listEntry)
			g_Scatter->Add(m_Local->players[i].listEntry + 0x70 * ((i + 1) & 0x1FF),
			               &m_Local->players[i].controllerBase);
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Pass 2: controllerBase → pawnHandle (uint32_t CHandle)
	for (int i = 0; i < MAX_ENTITIES; i++)
		if (m_Local->players[i].controllerBase)
			g_Scatter->Add(m_Local->players[i].controllerBase + client_dll::CCSPlayerController::m_hPlayerPawn,
			               &m_Local->players[i].pawnHandle);
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Pass 3: pawnHandle → listEntry2 (handle is not a pointer — zero check)
	for (int i = 0; i < MAX_ENTITIES; i++)
		if (m_Local->players[i].pawnHandle)
			g_Scatter->Add(m_Local->entityList + 0x8 * ((m_Local->players[i].pawnHandle & 0x7FFF) >> 9) + 16,
			               &m_Local->players[i].listEntry2);
	g_Scatter->Execute();
	g_Scatter->Clear();

	// Pass 4: listEntry2 + pawn slot offset → pawnBase
	for (int i = 0; i < MAX_ENTITIES; i++)
		if (m_Local->players[i].listEntry2)
			g_Scatter->Add(m_Local->players[i].listEntry2 + 0x70 * (m_Local->players[i].pawnHandle & 0x1FF),
			               &m_Local->players[i].pawnBase);
	g_Scatter->Execute();
	g_Scatter->Clear();

	m_Local->applyCache();
}
