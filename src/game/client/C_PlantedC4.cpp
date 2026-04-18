#include "pch.h"
#include "sdk.h"

static bool isValidPtr(uint64_t p) {
	return p > 0x10000ULL && p < 0x7FFFFFFFFFFF0000ULL;
}

void CGame::getBombData(uint64_t c4) {
	if (!c4) {
		// Reset all bomb state — clears stale terminal flags (hasDefused/hasExploded)
		// so they don't bleed into the next round when someone picks up the C4.
		bomb.entity         = 0;
		bomb.sceneNode      = 0;
		bomb.position       = {};
		bomb.isTicking      = false;
		bomb.isBeingDefused = false;
		bomb.hasExploded    = false;
		bomb.hasDefused     = false;
		bomb.plantTimeSet   = false;
		return;
	}

	bomb.entity = c4;

	// Bomb scatter 1: state fields + scene node pointer
	bool    newTicking  = false;
	bool    newDefusing = false;
	bool    newExploded = false;
	bool    newDefused  = false;
	int32_t newSite     = -1;

	g_DMA.PrepareEX(c4 + client_dll::C_BaseEntity::m_pGameSceneNode, &bomb.sceneNode, sizeof(bomb.sceneNode));
	g_DMA.PrepareEX(c4 + client_dll::C_PlantedC4::m_bBombTicking,    &newTicking,     sizeof(newTicking));
	g_DMA.PrepareEX(c4 + client_dll::C_PlantedC4::m_nBombSite,       &newSite,        sizeof(newSite));
	g_DMA.PrepareEX(c4 + client_dll::C_PlantedC4::m_bBeingDefused,   &newDefusing,    sizeof(newDefusing));
	g_DMA.PrepareEX(c4 + client_dll::C_PlantedC4::m_bHasExploded,    &newExploded,    sizeof(newExploded));
	g_DMA.PrepareEX(c4 + client_dll::C_PlantedC4::m_bBombDefused,    &newDefused,     sizeof(newDefused));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	if (newTicking && !bomb.isTicking) {
		bomb.plantTime    = C_PlantedC4::Clock::now();
		bomb.plantTimeSet = true;
		std::cout << "[Bomb]: Planted — site " << newSite << "\n";
	}
	if (!newTicking)
		bomb.plantTimeSet = false;

	bomb.isTicking      = newTicking;
	bomb.isBeingDefused = newDefusing;
	bomb.hasExploded    = newExploded;
	bomb.hasDefused     = newDefused;
	bomb.site           = newSite;

	// Bomb scatter 2: world position from scene node
	if (isValidPtr(bomb.sceneNode)) {
		g_DMA.PrepareEX(bomb.sceneNode + client_dll::CGameSceneNode::m_vecAbsOrigin, &bomb.position, sizeof(bomb.position));
		g_DMA.ExecuteRead();
		g_DMA.Clear();
	}
}
