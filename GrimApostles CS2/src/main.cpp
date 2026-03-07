#include "pch.h"
#include "gui.h"
#include "updater.h"

int main() {
	std::cout << "[Main]: GrimApostles CS2 starting\n";

	// Stage 1: Fetch latest offsets from remote (falls back to hardcoded defaults on failure)
	std::cout << "[Main]: Stage 1 - Fetching offsets\n";
	updater::fetchOffsets();       // offsets.hpp     → client_dll::dw* + matchmaking_dll::dwGameTypes
	updater::fetchClassOffsets();  // client_dll.hpp  → client_dll::ClassName::m_*

	// Stage 2: Create the Win32 window
	std::cout << "[Main]: Stage 2 - Creating window\n";
	gui::CreateAppWindow();

	// Stage 3: Initialize Direct3D device and swap chain
	std::cout << "[Main]: Stage 3 - Initializing Direct3D\n";
	if (!gui::InitD3D()) {
		std::cout << "[Main]: Failed to initialize Direct3D, aborting\n";
		gui::Cleanup();
		return 1;
	}

	// Stage 4: Show and maximize the window
	std::cout << "[Main]: Stage 4 - Showing window\n";
	gui::ShowAppWindow();

	// Stage 5: Create ImGui context and bind backends
	std::cout << "[Main]: Stage 5 - Initializing ImGui\n";
	gui::InitImGui();

	// Stage 6: Load map bounds and textures
	std::cout << "[Main]: Stage 6 - Loading resources\n";
	gui::loadMapBounds();
	gui::loadTextures();

	// Stage 7: Run the render loop (blocks until exit)
	std::cout << "[Main]: Stage 7 - Entering render loop\n";
	gui::RunLoop();

	// Stage 8: Shut everything down
	std::cout << "[Main]: Stage 8 - Shutting down\n";
	gui::Cleanup();

	std::cout << "[Main]: Exited cleanly\n";
	return 0;
}
