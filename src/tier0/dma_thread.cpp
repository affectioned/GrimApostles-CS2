#include "pch.h"
#include "dma_thread.h"
#include "dma.h"
#include "updater.h"

void DMAThreadMain(CGame& game, std::mutex& gameMutex, const std::atomic<bool>& run) {
	// ── Connection phase ──────────────────────────────────────────────────────
	std::cout << "[DMA]: Connecting...\n";

	if (!g_DMA.Connect() || !g_DMA.AttachToProcessId()) {
		g_DMA.Disconnect();
		std::cout << "[DMA]: Connection failed\n";
		return;
	}

	g_DMA.moduleBase = g_DMA.getModuleBase(DMADevice::kModule);
	if (!g_DMA.moduleBase) {
		std::cout << "[DMA]: client.dll not found\n";
		g_DMA.Disconnect();
		return;
	}

	std::cout << "[DMA]: " << DMADevice::kProcess
	          << " PID="          << std::dec << g_DMA.dwAttachedProcessId
	          << " client.dll=0x" << std::hex << g_DMA.moduleBase << std::dec << "\n";

	updater::sigscanOffsets();

	// ── Update loop ───────────────────────────────────────────────────────────
	// Runs flat-out; sdk.cpp's scatter batches are the natural rate limiter.
	// Heap-allocated to avoid a ~23KB stack frame (CGame holds two CPlayer[64] arrays).
	auto local = std::make_unique<CGame>();
	while (run) {
		if (!g_DMA.bConnected) {
			std::this_thread::yield();
			continue;
		}
		local->update();
		std::lock_guard<std::mutex> lock(gameMutex);
		game = *local;
	}
}
