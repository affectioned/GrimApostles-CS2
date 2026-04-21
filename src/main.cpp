#include "pch.h"
#include <filesystem>
#include "gui.h"
#include "sdk.h"
#include "updater.h"
#include "CS2Context.h"
#include "DMA/DMA Thread.h"

std::atomic<bool> bRunning{ true };

int main() {
	{
		wchar_t exePath[MAX_PATH]{};
		GetModuleFileNameW(nullptr, exePath, MAX_PATH);
		auto logPath = std::filesystem::path(exePath).parent_path() / "client.log";
		Log::Init(logPath.wstring());
	}

	Log::Info("GrimApostles CS2 starting");

	// Stage 1: Create the Win32 window (must be on the main thread)
	Log::Info("Stage 1 - Creating window");
	gui::CreateAppWindow();

	// Stage 2: Initialize Direct3D
	Log::Info("Stage 2 - Initializing Direct3D");
	if (!gui::InitD3D()) {
		Log::Error("Failed to initialize Direct3D, aborting");
		gui::Cleanup();
		return 1;
	}

	// Stage 3: Show window
	Log::Info("Stage 3 - Showing window");
	gui::ShowAppWindow();

	// Stage 4: Initialize ImGui
	Log::Info("Stage 4 - Initializing ImGui");
	gui::InitImGui();

	// Stage 5: Load resources
	Log::Info("Stage 5 - Loading resources");
	gui::loadMapBounds();
	gui::loadTextures();

	// Stage 6: Create game context and start DMA thread
	Log::Info("Stage 6 - Starting DMA thread");
	CGame      game;
	std::mutex gameMutex;
	g_GameContext = new CS2Context(game, gameMutex);
	std::thread dmaThread(DMA_Thread_Main);

	// Stage 7: Run the render loop on the main thread (blocks until exit)
	Log::Info("Stage 7 - Entering render loop");
	gui::RunLoop(game, gameMutex);

	// Stage 8: Shut down — stop DMA thread first, then cleanup rendering
	Log::Info("Stage 8 - Shutting down");
	bRunning = false;
	dmaThread.join();
	gui::Cleanup();

	Log::Info("Exited cleanly");
	return 0;
}
