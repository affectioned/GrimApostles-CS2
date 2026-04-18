#include "pch.h"
#include "offsets.h"

// All offsets are resolved at startup via sigscan (dw*) or remote fetch (class members).
// No hardcoded fallbacks — if resolution fails, the offset stays 0 and reads return zeros.

// ─── offsets.hpp ──────────────────────────────────────────────────────────────

namespace client_dll {
	std::ptrdiff_t dwEntityList            = 0;
	std::ptrdiff_t dwLocalPlayerController = 0;
	std::ptrdiff_t dwLocalPlayerPawn       = 0;
	std::ptrdiff_t dwGlobalVars            = 0;
	std::ptrdiff_t dwPlantedC4             = 0;
}

namespace matchmaking_dll {
	std::ptrdiff_t dwGameTypes = 0;
}

// ─── client_dll.hpp ───────────────────────────────────────────────────────────

namespace client_dll {
	namespace C_BaseEntity {
		std::ptrdiff_t m_iTeamNum       = 0;
		std::ptrdiff_t m_iHealth        = 0;
		std::ptrdiff_t m_lifeState      = 0;
		std::ptrdiff_t m_pGameSceneNode = 0;
	}

	namespace CGameSceneNode {
		std::ptrdiff_t m_vecAbsOrigin = 0;
	}

	namespace C_PlantedC4 {
		std::ptrdiff_t m_bBombTicking  = 0;
		std::ptrdiff_t m_nBombSite     = 0;
		std::ptrdiff_t m_bHasExploded  = 0;
		std::ptrdiff_t m_bBeingDefused = 0;
		std::ptrdiff_t m_bBombDefused  = 0;
	}

	namespace C_BasePlayerPawn {
		std::ptrdiff_t m_vOldOrigin      = 0;
		std::ptrdiff_t m_pWeaponServices = 0;
	}

	namespace CPlayer_WeaponServices {
		std::ptrdiff_t m_hMyWeapons = 0;
	}

	namespace CCSPlayerController {
		std::ptrdiff_t m_hPlayerPawn          = 0;
		std::ptrdiff_t m_sSanitizedPlayerName = 0;
		std::ptrdiff_t m_iCompTeammateColor   = 0;
		std::ptrdiff_t m_iPing                = 0;
		std::ptrdiff_t m_iPawnArmor           = 0;
		std::ptrdiff_t m_bPawnHasDefuser      = 0;
		std::ptrdiff_t m_bPawnHasHelmet       = 0;
	}

	namespace C_CSPlayerPawn {
		std::ptrdiff_t m_angEyeAngles    = 0;
		std::ptrdiff_t m_pClippingWeapon = 0;
		std::ptrdiff_t m_bIsDefusing     = 0;
		std::ptrdiff_t m_szLastPlaceName = 0;
	}

	namespace C_EconEntity {
		std::ptrdiff_t m_AttributeManager = 0;
	}

	namespace C_AttributeContainer {
		std::ptrdiff_t m_Item = 0;
	}

	namespace C_EconItemView {
		std::ptrdiff_t m_iItemDefinitionIndex = 0;
	}
}
