#include "pch.h"
#include "offsets.h"

// Compiled-in offset values. Used at startup until updater::sigscanOffsets()
// (for dw*) and updater::fetchClassOffsets() (for class members) overwrite
// them. Values are taken from the cs2-dumper snapshot at the time of build,
// and serve as a fallback when both the cs2-dumper HTTP fetch and the local
// disk cache are unavailable.

namespace client_dll {
	// ── Module-level RVA pointers ────────────────────────────────────────────
	std::ptrdiff_t dwEntityList            = 0;
	std::ptrdiff_t dwLocalPlayerController = 0;
	std::ptrdiff_t dwLocalPlayerPawn       = 0;
	std::ptrdiff_t dwGlobalVars            = 0;
	std::ptrdiff_t dwPlantedC4             = 0;
	std::ptrdiff_t dwWeaponC4              = 0x22A6C68;
	std::ptrdiff_t dwGameRules             = 0x2328E38;

	// ── Class member offsets ─────────────────────────────────────────────────

	namespace C_BaseEntity {
		std::ptrdiff_t m_iHealth          = 0x34C;
		std::ptrdiff_t m_lifeState        = 0x354;
		std::ptrdiff_t m_iTeamNum         = 0x3EB;
		std::ptrdiff_t m_pGameSceneNode   = 0x330;
		std::ptrdiff_t m_hOwnerEntity     = 0x520;
	}

	namespace CGameSceneNode {
		std::ptrdiff_t m_vecAbsOrigin     = 0xC8;
	}

	namespace C_BasePlayerPawn {
		std::ptrdiff_t m_pWeaponServices    = 0x11E0;
		std::ptrdiff_t m_pObserverServices  = 0x11F8;
		std::ptrdiff_t m_vOldOrigin         = 0x1390;
	}

	namespace CPlayer_WeaponServices {
		std::ptrdiff_t m_hActiveWeapon    = 0x60;
	}

	namespace CPlayer_ObserverServices {
		std::ptrdiff_t m_hObserverTarget  = 0x4C;
	}

	namespace CCSPlayerController {
		std::ptrdiff_t m_sSanitizedPlayerName = 0x858;
		std::ptrdiff_t m_iCompTeammateColor   = 0x840;
		std::ptrdiff_t m_hPlayerPawn          = 0x904;
		std::ptrdiff_t m_iPawnArmor           = 0x914;
		std::ptrdiff_t m_bPawnHasDefuser      = 0x918;
		std::ptrdiff_t m_bPawnHasHelmet       = 0x919;
	}

	namespace C_CSPlayerPawn {
		std::ptrdiff_t m_szLastPlaceName  = 0x14B4;
		std::ptrdiff_t m_bIsDefusing      = 0x1C4A;
		std::ptrdiff_t m_angEyeAngles     = 0x3300;
	}

	namespace C_CSGameRulesProxy {
		std::ptrdiff_t m_pGameRules       = 0x600;
	}

	namespace C_CSGameRules {
		std::ptrdiff_t m_iRoundEndWinnerTeam = 0xF08;
	}

	namespace C_PlantedC4 {
		std::ptrdiff_t m_bBombTicking     = 0x1160;
		std::ptrdiff_t m_nBombSite        = 0x1164;
		std::ptrdiff_t m_flC4Blow         = 0x1190;
		std::ptrdiff_t m_bHasExploded     = 0x1195;
		std::ptrdiff_t m_bBeingDefused    = 0x119C;
		std::ptrdiff_t m_bC4Activated     = 0x11A8;
		std::ptrdiff_t m_bBombDefused     = 0x11B4;
	}

	namespace C_EconEntity {
		std::ptrdiff_t m_AttributeManager = 0x1180;
	}

	namespace C_AttributeContainer {
		std::ptrdiff_t m_Item             = 0x50;
	}

	namespace C_EconItemView {
		std::ptrdiff_t m_iItemDefinitionIndex = 0x1BA;
	}
}

namespace engine2_dll {
	std::ptrdiff_t dwBuildNumber                   = 0x60CC74;
	std::ptrdiff_t dwNetworkGameClient             = 0x90A0C0;
	std::ptrdiff_t dwNetworkGameClient_signOnState = 0x230;
}
