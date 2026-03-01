#include "pch.h"
#include "offsets.h"

// Default values are the last known good offsets.
// updater::fetchOffsets() / fetchClassOffsets() overwrites these at startup.

// ─── offsets.hpp defaults ─────────────────────────────────────────────────────

namespace client_dll {
	std::ptrdiff_t dwEntityList            = 0x21C8070;
	std::ptrdiff_t dwLocalPlayerController = 0x22F0188;
	std::ptrdiff_t dwLocalPlayerPawn       = 0x2065AF0;
}

namespace matchmaking_dll {
	std::ptrdiff_t dwGameTypes = 0x1B8000;
}

// ─── client_dll.hpp defaults ──────────────────────────────────────────────────

namespace client_dll {
	namespace C_BaseEntity {
		std::ptrdiff_t m_iTeamNum = 0x3F3;
		std::ptrdiff_t m_iHealth  = 0x354;
	}

	namespace C_BasePlayerPawn {
		std::ptrdiff_t m_vOldOrigin = 0x1588;
	}

	namespace CCSPlayerController {
		std::ptrdiff_t m_hPlayerPawn          = 0x90C;
		std::ptrdiff_t m_sSanitizedPlayerName = 0x860;
		std::ptrdiff_t m_iCompTeammateColor   = 0x848;
	}

	namespace C_CSPlayerPawn {
		std::ptrdiff_t m_angEyeAngles    = 0x3DD0;
		std::ptrdiff_t m_pClippingWeapon = 0x3DC0;
	}

	namespace C_EconEntity {
		std::ptrdiff_t m_AttributeManager = 0x1378;
	}

	namespace C_AttributeContainer {
		std::ptrdiff_t m_Item = 0x50;
	}

	namespace C_EconItemView {
		std::ptrdiff_t m_iItemDefinitionIndex = 0x1BA;
	}
}
