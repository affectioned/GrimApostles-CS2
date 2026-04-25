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