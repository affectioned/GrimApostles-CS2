#include "pch.h"
#include "sdk.h"

void CGame::getBombData(uint64_t c4) {
	if (!c4) {
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

	// Reset timer if the entity pointer changed (e.g. two different C4s across rounds)
	if (c4 != bomb.entity)
		bomb.plantTimeSet = false;
	bomb.entity = c4;

	bool    newActivated = false;
	bool    newTicking  = false;
	bool    newDefusing = false;
	bool    newExploded = false;
	bool    newDefused  = false;
	int32_t newSite     = -1;

	g_DMA.PrepareEX(c4 + client_dll::C_BaseEntity::m_pGameSceneNode, &bomb.sceneNode,  sizeof(bomb.sceneNode));
	g_DMA.PrepareEX(c4 + client_dll::C_PlantedC4::m_bC4Activated,    &newActivated,    sizeof(newActivated));
	g_DMA.PrepareEX(c4 + client_dll::C_PlantedC4::m_bBombTicking,    &newTicking,      sizeof(newTicking));
	g_DMA.PrepareEX(c4 + client_dll::C_PlantedC4::m_nBombSite,       &newSite,         sizeof(newSite));
	g_DMA.PrepareEX(c4 + client_dll::C_PlantedC4::m_bBeingDefused,   &newDefusing,     sizeof(newDefusing));
	g_DMA.PrepareEX(c4 + client_dll::C_PlantedC4::m_bHasExploded,    &newExploded,     sizeof(newExploded));
	g_DMA.PrepareEX(c4 + client_dll::C_PlantedC4::m_bBombDefused,    &newDefused,      sizeof(newDefused));
	g_DMA.ExecuteRead();
	g_DMA.Clear();

	// Stale entity from previous round — treat as no bomb until activated
	if (!newActivated) {
		bomb.isTicking    = false;
		bomb.plantTimeSet = false;
		return;
	}

	// Reject corrupt reads: site must be 0 (A) or 1 (B) once ticking
	if (newTicking && newSite != 0 && newSite != 1) return;

	if (newTicking && !bomb.isTicking) {
		bomb.plantTime    = C_PlantedC4::Clock::now();
		bomb.plantTimeSet = true;
		std::cout << "[Bomb]: Planted — site " << (newSite == 0 ? "A" : "B") << "\n";
	}
	if (!newTicking)
		bomb.plantTimeSet = false;

	bomb.isTicking      = newTicking;
	bomb.isBeingDefused = newDefusing;
	bomb.hasExploded    = newExploded;
	bomb.hasDefused     = newDefused;
	bomb.site           = newSite;

	if (isValidPtr(bomb.sceneNode)) {
		g_DMA.PrepareEX(bomb.sceneNode + client_dll::CGameSceneNode::m_vecAbsOrigin, &bomb.position, sizeof(bomb.position));
		g_DMA.ExecuteRead();
		g_DMA.Clear();
	}
}
