#include "pch.h"
#include "C_CSPlayerPawn.h"
#include "offsets.h"
#include "dma.h"

void C_CSPlayerPawn::Read(uint64_t base) {
	g_DMA.PrepareEX(base + client_dll::C_BaseEntity::m_iHealth,           &health,        sizeof(health));
	g_DMA.PrepareEX(base + client_dll::C_BaseEntity::m_lifeState,         &lifeState,     sizeof(lifeState));
	g_DMA.PrepareEX(base + client_dll::C_CSPlayerPawn::m_angEyeAngles,    &eyeAngles,     sizeof(eyeAngles));
	g_DMA.PrepareEX(base + client_dll::C_BasePlayerPawn::m_vOldOrigin,    &position,      sizeof(position));
	g_DMA.PrepareEX(base + client_dll::C_CSPlayerPawn::m_pClippingWeapon, &activeWeapon,  sizeof(activeWeapon));
	g_DMA.PrepareEX(base + client_dll::C_CSPlayerPawn::m_bIsDefusing,     &isDefusing,    sizeof(isDefusing));
	g_DMA.PrepareEX(base + client_dll::C_CSPlayerPawn::m_szLastPlaceName, &lastPlaceName, sizeof(lastPlaceName));
}
