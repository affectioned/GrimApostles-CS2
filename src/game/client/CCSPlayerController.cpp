#include "pch.h"
#include "CCSPlayerController.h"
#include "offsets.h"
#include "dma.h"

void CCSPlayerController::Read(uint64_t base) {
	g_DMA.PrepareEX(base + client_dll::CCSPlayerController::m_sSanitizedPlayerName, &nameAddr,   sizeof(nameAddr));
	g_DMA.PrepareEX(base + client_dll::C_BaseEntity::m_iTeamNum,                    &teamID,     sizeof(teamID));
	g_DMA.PrepareEX(base + client_dll::CCSPlayerController::m_iCompTeammateColor,   &color,      sizeof(color));
	g_DMA.PrepareEX(base + client_dll::CCSPlayerController::m_iPing,                &ping,       sizeof(ping));
	g_DMA.PrepareEX(base + client_dll::CCSPlayerController::m_iPawnArmor,           &armor,      sizeof(armor));
	g_DMA.PrepareEX(base + client_dll::CCSPlayerController::m_bPawnHasDefuser,      &hasDefuser, sizeof(hasDefuser));
	g_DMA.PrepareEX(base + client_dll::CCSPlayerController::m_bPawnHasHelmet,       &hasHelmet,  sizeof(hasHelmet));
}

void CCSPlayerController::ReadName() {
	if (nameAddr)
		g_DMA.PrepareEX(nameAddr, &name, sizeof(name));
}
