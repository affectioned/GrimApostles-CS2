#pragma once

namespace updater {
	// Scans cs2.exe modules for all offsets (module-level dw* and class members).
	// Called from CS2Context::Initialize() after the process is found.
	bool sigscanOffsets(DMA_Connection* conn, Process* proc);
}
