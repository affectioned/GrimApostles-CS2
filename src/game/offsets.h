#pragma once
#include <cstddef>

// ─── offsets.hpp  (cs2_dumper::offsets::*) ────────────────────────────────────
// Module-level pointer offsets. Fetched at startup by updater::fetchOffsets().

namespace client_dll {
	extern std::ptrdiff_t dwEntityList;
	extern std::ptrdiff_t dwLocalPlayerController;
	extern std::ptrdiff_t dwLocalPlayerPawn;
	extern std::ptrdiff_t dwGlobalVars;
	extern std::ptrdiff_t dwPlantedC4;
}

namespace matchmaking_dll {
	extern std::ptrdiff_t dwGameTypes;
}

// ─── client_dll.hpp  (cs2_dumper::schemas::client_dll::*) ────────────────────
// Class member offsets. Fetched at startup by updater::fetchClassOffsets().

namespace client_dll {
	namespace C_BaseEntity {
		extern std::ptrdiff_t m_iTeamNum;       // uint8_t
		extern std::ptrdiff_t m_iHealth;        // int32_t
		extern std::ptrdiff_t m_lifeState;      // uint8_t (0=alive, 1=dying, 2=dead)
		extern std::ptrdiff_t m_pGameSceneNode; // CGameSceneNode*
	}

	namespace CGameSceneNode {
		extern std::ptrdiff_t m_vecAbsOrigin;   // Vector3
	}

	namespace C_PlantedC4 {
		extern std::ptrdiff_t m_bBombTicking;  // bool
		extern std::ptrdiff_t m_nBombSite;     // int32 (0=A, 1=B)
		extern std::ptrdiff_t m_bHasExploded;  // bool
		extern std::ptrdiff_t m_bBeingDefused; // bool
		extern std::ptrdiff_t m_bBombDefused;  // bool
	}

	namespace C_BasePlayerPawn {
		extern std::ptrdiff_t m_vOldOrigin;  // Vector3
	}

	namespace CCSPlayerController {
		extern std::ptrdiff_t m_hPlayerPawn;            // CHandle<C_CSPlayerPawn>
		extern std::ptrdiff_t m_sSanitizedPlayerName;   // char*
		extern std::ptrdiff_t m_iCompTeammateColor;     // int32_t
		extern std::ptrdiff_t m_iPing;                  // uint32_t
		extern std::ptrdiff_t m_iPawnArmor;             // int32_t
		extern std::ptrdiff_t m_bPawnHasDefuser;        // bool
		extern std::ptrdiff_t m_bPawnHasHelmet;         // bool
	}

	namespace C_CSPlayerPawn {
		extern std::ptrdiff_t m_angEyeAngles;    // Vector2
		extern std::ptrdiff_t m_pClippingWeapon; // CHandle<C_WeaponBase>
		extern std::ptrdiff_t m_bIsDefusing;     // bool
		extern std::ptrdiff_t m_szLastPlaceName; // char[18]
	}

	namespace C_EconEntity {
		extern std::ptrdiff_t m_AttributeManager;  // C_AttributeContainer
	}

	namespace C_AttributeContainer {
		extern std::ptrdiff_t m_Item;  // C_EconItemView
	}

	namespace C_EconItemView {
		extern std::ptrdiff_t m_iItemDefinitionIndex;  // uint16_t
	}

}
