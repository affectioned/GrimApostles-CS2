#include "pch.h"
#include "gui.h"
#include "sdk.h"

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
	//You can change radar size based on your resolution. Set at 1080p right now for 1920x1080
	float radarSize = 1080;

	float vertigoZBound = 11700;
	float nukeZBound = -495;
	std::unordered_map<std::string, ID3D11ShaderResourceView*> mapTextures;
	std::unordered_map<std::string, mapData> mapBounds;
}

namespace icons {
	int id;
	float scale = 0.4f;
	std::unordered_map<int, ID3D11ShaderResourceView*> iconTextures;
	std::unordered_map<int, int> iconWidths;
	std::unordered_map<int, int> iconHeights;
}


// Stage 1: Register window class and create the Win32 window
void gui::CreateAppWindow() {
	wc = { sizeof(wc), CS_VREDRAW | CS_HREDRAW, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"GrimApostles", nullptr };
	::RegisterClassExW(&wc);
	hwnd = ::CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"GrimApostles CS2", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, wc.hInstance, nullptr);
}

// Stage 2: Initialize Direct3D device and swap chain
bool gui::InitD3D() {
	if (!CreateDeviceD3D(hwnd)) {
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return false;
	}
	return true;
}

// Stage 3: Make the window visible
void gui::ShowAppWindow() {
	::ShowWindow(hwnd, SW_MAXIMIZE);
	::UpdateWindow(hwnd);
}

// Stage 4: Create ImGui context and bind platform/renderer backends
void gui::InitImGui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
}

// Stage 5: Main render loop
void gui::RunLoop() {
	ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.f);
	CGame game;

	bool done = false;
	while (!done)
	{
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT || exitRequested)
				done = true;
		}
		if (done)
			break;

		// Handle window resize
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		{
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

		if (DMADevice::bConnected) {
			game.update();
			gameLoop(game);
		}

		ImGui::Render();

		const float clear_color_with_alpha[4] = {
			clear_color.x * clear_color.w,
			clear_color.y * clear_color.w,
			clear_color.z * clear_color.w,
			clear_color.w
		};
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		g_pSwapChain->Present(1, 0); // vsync
	}
}

// Stage 6: Shut down ImGui, D3D, and the window
void gui::Cleanup() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);
	DMADevice::ShowKeyPress();
}


void gui::ConnectButton() {
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 185, 35));
	ImGui::StyleColorsDark();
	ImGui::Begin("Connect", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	if (ImGui::Button(DMADevice::bConnected ? "Disconnect" : "Connect", { 150, 20 })) {
		if (!DMADevice::bConnected) {
			DMADevice::Connect();
			DMADevice::AttachToProcessId();
			DMADevice::moduleBase = DMADevice::getModuleBase(MODULE);
			std::cout << "\nProcess Information..." << std::endl;
			std::cout << "[DMA]:          CLIENT: " << PROCESS << std::endl;
			std::cout << "[DMA]:             PID: " << std::dec << DMADevice::dwAttachedProcessId << std::endl;
			std::cout << "[DMA]: CLIENT.DLL BASE: 0x" << std::hex << DMADevice::moduleBase << std::endl;
		}
		else {
			std::cout << "[DEBUG OUTPUT]: " << std::endl;
			DMADevice::Disconnect();
		}
	}
	ImGui::End();
}

void gui::ExitButton() {
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 185, 80));
	ImGui::StyleColorsDark();
	ImGui::Begin("EXIT", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	if (ImGui::Button("Exit", { 150 , 20 })) {
		exitRequested = true;
		DMADevice::Disconnect();
		std::cout << "Exiting Program... " << std::endl;
	}
	ImGui::End();
}
