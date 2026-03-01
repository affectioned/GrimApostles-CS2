#include "pch.h"
#include "gui.h"
#include "updater.h"

int main() {
	// Stage 1: Fetch latest offsets from remote (falls back to hardcoded defaults on failure)
	updater::fetchOffsets();       // offsets.hpp     → client_dll::dw* + matchmaking_dll::dwGameTypes
	updater::fetchClassOffsets();  // client_dll.hpp  → client_dll::ClassName::m_*

	// Stage 2: Create the Win32 window
	gui::CreateAppWindow();

	// Stage 3: Initialize Direct3D device and swap chain
	if (!gui::InitD3D()) {
		gui::Cleanup();
		return 1;
	}

	// Stage 4: Show and maximize the window
	gui::ShowAppWindow();

	// Stage 5: Create ImGui context and bind backends
	gui::InitImGui();

	// Stage 6: Load map bounds and textures
	gui::loadMapBounds();
	gui::loadTextures();

	// Stage 7: Run the render loop (blocks until exit)
	gui::RunLoop();

	// Stage 8: Shut everything down
	gui::Cleanup();

	return 0;
}
