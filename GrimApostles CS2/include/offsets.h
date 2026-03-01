#pragma once
#include <cstddef>

// ─── offsets.hpp  (cs2_dumper::offsets::*) ────────────────────────────────────
// Module-level pointer offsets. Fetched at startup by updater::fetchOffsets().

namespace client_dll {
	extern std::ptrdiff_t dwEntityList;
	extern std::ptrdiff_t dwLocalPlayerController;
	extern std::ptrdiff_t dwLocalPlayerPawn;
}

namespace matchmaking_dll {
	extern std::ptrdiff_t dwGameTypes;

	// Fixed sub-offset inside the GameTypes struct — not in the dumper output
	constexpr std::ptrdiff_t dwGameTypes_mapName = 0x120;
}

// ─── client_dll.hpp  (cs2_dumper::schemas::client_dll::*) ────────────────────
// Class member offsets. Fetched at startup by updater::fetchClassOffsets().

namespace client_dll {
	namespace C_BaseEntity {
		extern std::ptrdiff_t m_iTeamNum;  // uint8_t
		extern std::ptrdiff_t m_iHealth;   // int32_t
	}

	namespace C_BasePlayerPawn {
		extern std::ptrdiff_t m_vOldOrigin;  // Vector3
	}

	namespace CCSPlayerController {
		extern std::ptrdiff_t m_hPlayerPawn;            // CHandle<C_CSPlayerPawn>
		extern std::ptrdiff_t m_sSanitizedPlayerName;   // char*
		extern std::ptrdiff_t m_iCompTeammateColor;     // int32_t
	}

	namespace C_CSPlayerPawn {
		extern std::ptrdiff_t m_angEyeAngles;    // Vector2
		extern std::ptrdiff_t m_pClippingWeapon; // CHandle<C_WeaponBase>
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
