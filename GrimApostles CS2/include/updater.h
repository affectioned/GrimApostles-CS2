#pragma once

namespace updater {
	// Fetches cs2_dumper/output/offsets.hpp and updates client_dll::dw* and matchmaking_dll::dwGameTypes.
	// Falls back to hardcoded defaults in offsets.cpp on failure.
	bool fetchOffsets();

	// Fetches cs2_dumper/output/client_dll.hpp and updates all client_dll::ClassName::m_* offsets.
	// Falls back to hardcoded defaults in offsets.cpp on failure.
	bool fetchClassOffsets();
}
