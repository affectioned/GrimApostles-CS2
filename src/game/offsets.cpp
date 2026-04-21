#include "pch.h"
#include "offsets.h"

// ── Module-level RVA pointers ─────────────────────────────────────────────────
// These are absolute addresses inside client.dll that shift on every game update.
// Resolved at startup via signature scan (updater::sigscanOffsets).

namespace client_dll {
	std::ptrdiff_t dwEntityList            = 0;
	std::ptrdiff_t dwLocalPlayerController = 0;
	std::ptrdiff_t dwLocalPlayerPawn       = 0;
	std::ptrdiff_t dwGlobalVars            = 0;
	std::ptrdiff_t dwPlantedC4             = 0;
}

// ── Class member offsets ──────────────────────────────────────────────────────
// Schema-defined struct field offsets. These rarely change between CS2 updates.
// Update values here when a game patch breaks reads; verify against cs2-dumper.

namespace client_dll {
	namespace C_BaseEntity {
		std::ptrdiff_t m_pGameSceneNode = 0x328;
		std::ptrdiff_t m_iTeamNum       = 0x3CB;
		std::ptrdiff_t m_lifeState      = 0x338;
		std::ptrdiff_t m_iHealth        = 0x344;
	}

	namespace CGameSceneNode {
		std::ptrdiff_t m_vecAbsOrigin = 0x90;
	}

	namespace C_BasePlayerPawn {
		std::ptrdiff_t m_pWeaponServices = 0x11F8;
		std::ptrdiff_t m_vOldOrigin      = 0x1224;
	}

	namespace CCSPlayerController {
		std::ptrdiff_t m_sSanitizedPlayerName = 0x640;
		std::ptrdiff_t m_hPlayerPawn          = 0x7E4;
		std::ptrdiff_t m_iCompTeammateColor   = 0x7EC;
		std::ptrdiff_t m_iPing                = 0x7F8;
		std::ptrdiff_t m_iPawnArmor           = 0x7FC;
		std::ptrdiff_t m_bPawnHasDefuser      = 0x800;
		std::ptrdiff_t m_bPawnHasHelmet       = 0x801;
	}

	namespace C_CSPlayerPawn {
		std::ptrdiff_t m_pClippingWeapon  = 0x12A0;
		std::ptrdiff_t m_angEyeAngles     = 0x1510;
		std::ptrdiff_t m_bIsDefusing      = 0x1448;
		std::ptrdiff_t m_szLastPlaceName  = 0x1798;
	}

	namespace CPlayer_WeaponServices {
		std::ptrdiff_t m_hMyWeapons = 0x8;
	}

	namespace C_EconEntity {
		std::ptrdiff_t m_AttributeManager = 0x9E8;
	}

	namespace C_AttributeContainer {
		std::ptrdiff_t m_Item = 0x38;
	}

	namespace C_EconItemView {
		std::ptrdiff_t m_iItemDefinitionIndex = 0x1B4;
	}

	namespace C_PlantedC4 {
		std::ptrdiff_t m_bC4Activated   = 0x19B;
		std::ptrdiff_t m_nBombSite      = 0x19C;
		std::ptrdiff_t m_bHasExploded   = 0x1A0;
		std::ptrdiff_t m_bBombTicking   = 0x1A2;
		std::ptrdiff_t m_bBeingDefused  = 0x1A4;
		std::ptrdiff_t m_bBombDefused   = 0x1AA;
	}
}
