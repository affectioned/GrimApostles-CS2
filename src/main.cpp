#include "pch.h"
#include "gui.h"
#include "sdk.h"
#include "updater.h"
#include "dma_thread.h"

// Tee streambuf: duplicates all writes to two underlying streambufs.
class TeeBuf : public std::streambuf {
	std::streambuf* a;
	std::streambuf* b;
public:
	TeeBuf(std::streambuf* a, std::streambuf* b) : a(a), b(b) {}
protected:
	int overflow(int c) override {
		if (c == EOF) return !EOF;
		if (a->sputc((char)c) == EOF) return EOF;
		if (b->sputc((char)c) == EOF) return EOF;
		return c;
	}
	std::streamsize xsputn(const char* s, std::streamsize n) override {
		a->sputn(s, n);
		b->sputn(s, n);
		return n;
	}
};

int main() {
	// Tee std::cout to both the console and a timestamped log file next to the exe
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
	logFile.rdbuf()->pubsetbuf(nullptr, 0);
	static TeeBuf teeBuf(std::cout.rdbuf(), logFile.rdbuf());
	if (logFile.is_open())
		std::cout.rdbuf(&teeBuf);

	std::cout << "[Main]: GrimApostles CS2 starting\n";

	// Shared game state — written by the update thread, read by the render thread
	CGame      game;
	std::mutex gameMutex;

	// Stage 1: Create the Win32 window (must be on the main thread)
	std::cout << "[Main]: Stage 1 - Creating window\n";
	gui::CreateAppWindow();

	// Stage 2: Initialize Direct3D
	std::cout << "[Main]: Stage 2 - Initializing Direct3D\n";
	if (!gui::InitD3D()) {
		std::cout << "[Main]: Failed to initialize Direct3D, aborting\n";
		gui::Cleanup();
		return 1;
	}

	// Stage 3: Show window
	std::cout << "[Main]: Stage 3 - Showing window\n";
	gui::ShowAppWindow();

	// Stage 4: Initialize ImGui
	std::cout << "[Main]: Stage 4 - Initializing ImGui\n";
	gui::InitImGui();

	// Stage 5: Load resources
	std::cout << "[Main]: Stage 5 - Loading resources\n";
	gui::loadMapBounds();
	gui::loadTextures();

	// Stage 6: Fetch class offsets in background
	std::cout << "[Main]: Stage 6 - Fetching class offsets (background)\n";
	std::thread([] { updater::fetchClassOffsets(); }).detach();

	// Stage 7: Start DMA thread — handles connection then drives the update loop
	std::cout << "[Main]: Stage 7 - Starting DMA thread\n";
	std::atomic<bool> dmaRun{true};
	std::thread dmaThread([&] { DMAThreadMain(game, gameMutex, dmaRun); });

	// Stage 8: Run the render loop on the main thread (blocks until exit)
	std::cout << "[Main]: Stage 8 - Entering render loop\n";
	gui::RunLoop(game, gameMutex);

	// Stage 9: Shut down — stop DMA thread first, then cleanup rendering
	std::cout << "[Main]: Stage 9 - Shutting down\n";
	dmaRun = false;
	dmaThread.join();
	gui::Cleanup();

	std::cout << "[Main]: Exited cleanly\n";
	return 0;
}
