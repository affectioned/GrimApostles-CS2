#include "pch.h"
#include "gui.h"
#include "sdk.h"
#include "updater.h"

namespace gui {
	ID3D11Device* g_pd3dDevice = nullptr;
	ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
	IDXGISwapChain* g_pSwapChain = nullptr;
	UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
	ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
	bool exitRequested = false;
	WNDCLASSEXW wc = {};
	HWND hwnd = nullptr;
}

namespace maps {
	float radarSize = 1080;
	float vertigoZBound = 11700;
	float nukeZBound = -495;
	std::unordered_map<std::string, ID3D11ShaderResourceView*> mapTextures;
	std::unordered_map<std::string, mapData> mapBounds;
}

namespace icons {
	float scale = 0.4f;
	std::unordered_map<int, ID3D11ShaderResourceView*> iconTextures;
	std::unordered_map<int, int> iconWidths;
	std::unordered_map<int, int> iconHeights;
}

void gui::CreateAppWindow() {
	wc = { sizeof(wc), CS_VREDRAW | CS_HREDRAW, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"GrimApostles", nullptr };
	::RegisterClassExW(&wc);
	hwnd = ::CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"GrimApostles CS2", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, wc.hInstance, nullptr);
	std::cout << "[GUI]: Window created\n";
}

bool gui::InitD3D() {
	if (!CreateDeviceD3D(hwnd)) {
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		std::cout << "[GUI]: Direct3D initialization failed\n";
		return false;
	}
	std::cout << "[GUI]: Direct3D initialized\n";
	return true;
}

void gui::ShowAppWindow() {
	::ShowWindow(hwnd, SW_MAXIMIZE);
	::UpdateWindow(hwnd);
}

void gui::InitImGui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
	std::cout << "[GUI]: ImGui " << IMGUI_VERSION << " initialized\n";
}

void gui::RunLoop() {
	std::cout << "[GUI]: Render loop started\n";
	CGame game;
	bool done = false;

	while (!done) {
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT || exitRequested) done = true;
		}
		if (done) break;

		if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_ResizeWidth = g_ResizeHeight = 0;
			CreateRenderTarget();
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ConnectButton();
		ExitButton();
		FpsOverlay();

		if (DMADevice::bConnected) {
			game.update();
			gameLoop(game);
		}

		ImGui::Render();
		constexpr float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, black);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		g_pSwapChain->Present(1, 0);
	}
	std::cout << "[GUI]: Render loop exited\n";
}

void gui::Cleanup() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);
	DMADevice::ShowKeyPress();
}

// Helper: anchored fixed overlay window, no decoration.
static bool BeginOverlay(const char* id, float x, float y, ImGuiWindowFlags extra = 0) {
	ImGui::SetNextWindowPos({ x, y });
	return ImGui::Begin(id, nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | extra);
}

void gui::ConnectButton() {
	BeginOverlay("##connect", ImGui::GetIO().DisplaySize.x - 185, 35);
	if (ImGui::Button(DMADevice::bConnected ? "Disconnect" : "Connect", { 150, 20 })) {
		if (!DMADevice::bConnected) {
			DMADevice::Connect();
			DMADevice::AttachToProcessId();
			DMADevice::moduleBase = DMADevice::getModuleBase(MODULE);
			std::cout << "[DMA]: " << PROCESS << " PID=" << std::dec << DMADevice::dwAttachedProcessId
				<< " client.dll=0x" << std::hex << DMADevice::moduleBase << "\n";
			updater::sigscanOffsets();
		} else {
			DMADevice::Disconnect();
		}
	}
	ImGui::End();
}

void gui::ExitButton() {
	BeginOverlay("##exit", ImGui::GetIO().DisplaySize.x - 185, 80);
	if (ImGui::Button("Exit", { 150, 20 })) {
		exitRequested = true;
		DMADevice::Disconnect();
	}
	ImGui::End();
}

void gui::FpsOverlay() {
	ImGui::SetNextWindowBgAlpha(0.0f);
	BeginOverlay("##fps", ImGui::GetIO().DisplaySize.x - 185, 125,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("%.0f FPS", ImGui::GetIO().Framerate);
	ImGui::End();
}
