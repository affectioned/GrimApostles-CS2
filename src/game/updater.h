#pragma once
#include <atomic>

namespace updater {
	// Set to true (with release semantics) after fetchClassOffsets() completes its writes.
	// Acquire-load this before reading class offsets to ensure visibility on the main thread.
	extern std::atomic<bool> classOffsetsReady;

	// Fetches cs2_dumper/output/client_dll.hpp and updates all client_dll::ClassName::m_* offsets.
	// Falls back to hardcoded defaults in offsets.cpp on failure.
	bool fetchClassOffsets();

	// Scans cs2.exe modules for signatures to resolve dw* offsets locally.
	// Requires DMA to be connected and cs2.exe to be running.
	// Updates only offsets whose signatures are found; leaves others unchanged.
	bool sigscanOffsets();
}
