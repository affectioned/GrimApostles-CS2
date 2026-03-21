#include "pch.h"
#include "gui.h"
#include "updater.h"

int main() {
	// Redirect std::cout to a timestamped log file next to the exe
	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	std::wstring exeDir(exePath);
	exeDir = exeDir.substr(0, exeDir.find_last_of(L"\\/") + 1);

	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::tm tm{};
	localtime_s(&tm, &t);
	std::wostringstream logName;
	logName << exeDir << L"log_" << std::put_time(&tm, L"%Y%m%d_%H%M%S") << L".txt";

	static std::ofstream logFile(logName.str());
	logFile.rdbuf()->pubsetbuf(nullptr, 0);  // disable buffering — writes go to disk immediately
	if (logFile.is_open())
		std::cout.rdbuf(logFile.rdbuf());

	std::cout << "[Main]: GrimApostles CS2 starting\n";

	// Stage 1: Fetch latest class offsets from remote (falls back to hardcoded defaults on failure)
	// dw* offsets are resolved locally via signature scan after DMA connects (gui::ConnectButton)
	std::cout << "[Main]: Stage 1 - Fetching class offsets\n";
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
