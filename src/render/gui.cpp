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
	std::unordered_map<std::string, ID3D11ShaderResourceView*> mapTextures;
	std::unordered_map<std::string, mapData> mapBounds;
}

namespace icons {
	std::unordered_map<int, ID3D11ShaderResourceView*> iconTextures;
	std::unordered_map<int, int> iconWidths;
	std::unordered_map<int, int> iconHeights;
}

namespace settings {
	bool  showWeaponIcons = false;
	bool  showPlayerNames = false;
	bool  showHealthBars  = false;
	bool  showAimLines    = true;
	bool  showTeamPanels  = true;
	float iconScale       = 0.2f;
	float aimLineLength   = 40.0f;
	float dotRadius       = 6.0f;
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

	// Hook our settings into ImGui's .ini system — loads on first NewFrame(), saves on DestroyContext()
	ImGui::GetIO().IniFilename = "GrimApostles.ini";

	ImGuiSettingsHandler h;
	h.TypeName   = "GrimApostles";
	h.TypeHash   = ImHashStr("GrimApostles");
	h.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char*) -> void* { return (void*)1; };
	h.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line) {
		int i; float f;
		if      (sscanf_s(line, "WeaponIcons=%d", &i) == 1) settings::showWeaponIcons = i != 0;
		else if (sscanf_s(line, "PlayerNames=%d", &i) == 1) settings::showPlayerNames = i != 0;
		else if (sscanf_s(line, "HealthBars=%d",  &i) == 1) settings::showHealthBars  = i != 0;
		else if (sscanf_s(line, "AimLines=%d",    &i) == 1) settings::showAimLines    = i != 0;
		else if (sscanf_s(line, "TeamPanels=%d",  &i) == 1) settings::showTeamPanels  = i != 0;
		else if (sscanf_s(line, "IconScale=%f",   &f) == 1) settings::iconScale       = f;
		else if (sscanf_s(line, "AimLength=%f",   &f) == 1) settings::aimLineLength   = f;
		else if (sscanf_s(line, "DotRadius=%f",   &f) == 1) settings::dotRadius       = f;
	};
	h.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* buf) {
		buf->appendf("[%s][Settings]\n", h->TypeName);
		buf->appendf("WeaponIcons=%d\n", settings::showWeaponIcons);
		buf->appendf("PlayerNames=%d\n", settings::showPlayerNames);
		buf->appendf("HealthBars=%d\n",  settings::showHealthBars);
		buf->appendf("AimLines=%d\n",    settings::showAimLines);
		buf->appendf("TeamPanels=%d\n",  settings::showTeamPanels);
		buf->appendf("IconScale=%.3f\n", settings::iconScale);
		buf->appendf("AimLength=%.1f\n", settings::aimLineLength);
		buf->appendf("DotRadius=%.2f\n", settings::dotRadius);
		buf->append("\n");
	};
	ImGui::AddSettingsHandler(&h);

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding    = 6.0f;
	style.FrameRounding     = 4.0f;
	style.GrabRounding      = 4.0f;
	style.WindowBorderSize  = 0.0f;
	style.FrameBorderSize   = 0.0f;
	style.ItemSpacing       = ImVec2(8.0f, 6.0f);
	style.WindowPadding     = ImVec2(12.0f, 10.0f);
	style.FramePadding      = ImVec2(8.0f, 4.0f);

	ImVec4* c = style.Colors;
	c[ImGuiCol_WindowBg]          = ImVec4(0.08f, 0.08f, 0.10f, 0.92f);
	c[ImGuiCol_Text]              = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
	c[ImGuiCol_TextDisabled]      = ImVec4(0.50f, 0.50f, 0.58f, 1.00f);
	c[ImGuiCol_Separator]         = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
	c[ImGuiCol_Button]            = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
	c[ImGuiCol_ButtonHovered]     = ImVec4(0.28f, 0.28f, 0.36f, 1.00f);
	c[ImGuiCol_ButtonActive]      = ImVec4(0.20f, 0.20f, 0.28f, 1.00f);
	c[ImGuiCol_FrameBg]           = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
	c[ImGuiCol_FrameBgHovered]    = ImVec4(0.20f, 0.20f, 0.26f, 1.00f);
	c[ImGuiCol_Header]            = ImVec4(0.20f, 0.20f, 0.26f, 1.00f);
	c[ImGuiCol_HeaderHovered]     = ImVec4(0.28f, 0.28f, 0.36f, 1.00f);

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
	std::cout << "[GUI]: ImGui " << IMGUI_VERSION << " initialized\n";
}

void gui::RunLoop() {
	std::cout << "[GUI]: Render loop started\n";
	CGame game = {};
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

		RenderControlPanel();

		if (DMADevice::bConnected) {
			game.update();
			gameLoop(game);
			if (settings::showTeamPanels)
				RenderTeamPanels(game);
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

void gui::RenderControlPanel() {
	ImGuiIO& io = ImGui::GetIO();
	constexpr float panelW = 220.0f;

	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - panelW - 10.0f, 10.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(panelW, 0.0f), ImGuiCond_Always);
	ImGui::Begin("##panel", nullptr,
		ImGuiWindowFlags_NoTitleBar        |
		ImGuiWindowFlags_NoResize          |
		ImGuiWindowFlags_NoMove            |
		ImGuiWindowFlags_NoScrollbar       |
		ImGuiWindowFlags_AlwaysAutoResize  |
		ImGuiWindowFlags_NoSavedSettings
	);

	// Title
	ImGui::SetCursorPosX((panelW - ImGui::CalcTextSize("GrimApostles CS2").x) * 0.5f);
	ImGui::TextColored(ImVec4(0.5f, 0.78f, 1.0f, 1.0f), "GrimApostles CS2");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Status indicator + FPS on the same row
	if (DMADevice::bConnected)
		ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "● Connected");
	else
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "● Disconnected");

	char fpsBuf[16];
	snprintf(fpsBuf, sizeof(fpsBuf), "%.0f FPS", io.Framerate);
	ImGui::SameLine(panelW - ImGui::CalcTextSize(fpsBuf).x - 12.0f);
	ImGui::TextDisabled("%s", fpsBuf);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Connect / Disconnect button
	if (DMADevice::bConnected) {
		ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.12f, 0.12f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.20f, 0.20f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.62f, 0.14f, 0.14f, 1.0f));
		if (ImGui::Button("Disconnect", ImVec2(-1.0f, 28.0f)))
			DMADevice::Disconnect();
		ImGui::PopStyleColor(3);
	} else {
		ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.12f, 0.45f, 0.22f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.60f, 0.30f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.14f, 0.52f, 0.25f, 1.0f));
		if (ImGui::Button("Connect", ImVec2(-1.0f, 28.0f))) {
			if (!DMADevice::Connect() || !DMADevice::AttachToProcessId()) {
				DMADevice::Disconnect();
			} else {
				DMADevice::moduleBase = DMADevice::getModuleBase(DMADevice::kModule);
				if (!DMADevice::moduleBase) {
					std::cout << "[DMA]: client.dll not found — disconnecting\n";
					DMADevice::Disconnect();
				} else {
					std::cout << "[DMA]: " << DMADevice::kProcess << " PID=" << std::dec << DMADevice::dwAttachedProcessId
						<< " client.dll=0x" << std::hex << DMADevice::moduleBase << "\n";
					updater::sigscanOffsets();
				}
			}
		}
		ImGui::PopStyleColor(3);
	}

	ImGui::Spacing();

	// Exit — neutral until hovered, then red tint
	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.14f, 0.14f, 0.18f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.50f, 0.12f, 0.12f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.60f, 0.10f, 0.10f, 1.0f));
	if (ImGui::Button("Exit", ImVec2(-1.0f, 22.0f))) {
		exitRequested = true;
		DMADevice::Disconnect();
	}
	ImGui::PopStyleColor(3);

	// Settings
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::CollapsingHeader("Settings")) {
		ImGui::Spacing();

		ImGui::Checkbox("Weapon Icons", &settings::showWeaponIcons);
		ImGui::Checkbox("Player Names", &settings::showPlayerNames);
		ImGui::Checkbox("Health Bars",  &settings::showHealthBars);
		ImGui::Checkbox("Aim Lines",    &settings::showAimLines);
		ImGui::Checkbox("Team Panels",  &settings::showTeamPanels);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SliderFloat("##iconscale", &settings::iconScale,    0.05f, 0.50f,  "Icons  %.2fx");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SliderFloat("##aimlength", &settings::aimLineLength, 10.0f, 100.0f, "Aim  %.0fpx");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SliderFloat("##dotradius", &settings::dotRadius,     3.0f,  14.0f,  "Dot  %.1fpx");

		ImGui::Spacing();
	}

	ImGui::End();
}

// ─── Team Panels ──────────────────────────────────────────────────────────────

static void renderPlayerEntry(const CPlayer& p, const CGame& game, bool isEnemy) {
	constexpr float kDotR   = 5.0f;
	constexpr float kIndent = kDotR * 2.0f + 8.0f;
	constexpr float kBarH   = 3.0f;
	constexpr float kIconH  = 14.0f;

	bool        alive  = p.lifeState == 0;
	float       alpha  = alive ? 1.0f : 0.38f;
	ImDrawList* dl     = ImGui::GetWindowDrawList();
	ImVec2      origin = ImGui::GetCursorScreenPos();
	float       lineH  = ImGui::GetTextLineHeight();
	float       winW   = ImGui::GetWindowWidth();

	// Dot
	ImU32 dotCol;
	if      (p.controller == game.localPlayer.controller) dotCol = IM_COL32(255, 255, 255, (int)(alpha * 255));
	else if (!isEnemy)                                     dotCol = gui::setColor(p.color, alpha * 255.0f);
	else                                                   dotCol = IM_COL32(255, 60,  60,  (int)(alpha * 255));

	ImVec2 dotCtr = { origin.x + kDotR + 2.0f, origin.y + lineH * 0.5f + 1.0f };
	dl->AddCircleFilled(dotCtr, kDotR + 1.0f, IM_COL32(0, 0, 0, (int)(alpha * 180)));
	dl->AddCircleFilled(dotCtr, kDotR,         dotCol);

	// Name (left) + HP number (right)
	char hpBuf[8];
	if (alive) snprintf(hpBuf, sizeof(hpBuf), "%d", p.health);
	else       snprintf(hpBuf, sizeof(hpBuf), "---");

	float hpTextW  = ImGui::CalcTextSize(hpBuf).x;
	float nameMaxW = winW - kIndent - hpTextW - 20.0f;

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kIndent);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.95f, alpha));

	ImVec2 nameScreen = ImGui::GetCursorScreenPos();
	ImGui::PushClipRect(nameScreen, { nameScreen.x + nameMaxW, nameScreen.y + lineH + 2.0f }, true);
	ImGui::TextUnformatted(p.name[0] ? p.name : "...");
	ImGui::PopClipRect();

	ImGui::SameLine(winW - hpTextW - 14.0f);
	if (alive) ImGui::Text("%d", p.health);
	else       ImGui::TextDisabled("---");
	ImGui::PopStyleColor();

	// Health bar
	float barStartX = origin.x + kIndent;
	float barW      = winW - kIndent - 14.0f;
	ImVec2 barTL    = { barStartX, ImGui::GetCursorScreenPos().y };
	float filled    = alive ? (float)p.health / 100.0f : 0.0f;
	if (filled > 1.0f) filled = 1.0f;
	float r = 255.0f * (1.0f - filled);
	float g = 255.0f * filled;
	dl->AddRectFilled(barTL, { barTL.x + barW,          barTL.y + kBarH }, IM_COL32(30, 30, 30, (int)(alpha * 200)), 1.5f);
	dl->AddRectFilled(barTL, { barTL.x + barW * filled, barTL.y + kBarH }, IM_COL32((int)r, (int)g, 0, (int)(alpha * 255)), 1.5f);
	ImGui::Dummy({ 0.0f, kBarH + 3.0f });

	// Weapon icon (left-indented)
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kIndent);
	if (alive) {
		auto texIt = icons::iconTextures.find(p.activeWeaponID);
		if (texIt != icons::iconTextures.end() && texIt->second) {
			int iw = icons::iconWidths.count(p.activeWeaponID)  ? icons::iconWidths[p.activeWeaponID]  : 1;
			int ih = icons::iconHeights.count(p.activeWeaponID) ? icons::iconHeights[p.activeWeaponID] : 1;
			float iconW = (ih > 0) ? kIconH * (float)iw / (float)ih : kIconH;
			ImGui::Image((ImTextureID)texIt->second, { iconW, kIconH });
		} else {
			ImGui::Dummy({ 0.0f, kIconH });
		}
	} else {
		ImGui::Dummy({ 0.0f, kIconH });
	}

	// Last place name (center of the row)
	if (alive && p.lastPlaceName[0]) {
		ImGui::SameLine();
		ImGui::TextDisabled("%s", p.lastPlaceName);
	}

	// Right-aligned: DEFUSING takes priority, otherwise armor/defuser/ping
	if (alive && p.isDefusing) {
		constexpr const char* kLabel = "DEFUSING";
		ImGui::SameLine(winW - ImGui::CalcTextSize(kLabel).x - 14.0f);
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.1f, 1.0f), kLabel);
	} else {
		char status[32] = {};
		if (alive && p.armor > 0)
			snprintf(status, sizeof(status), "%s", p.hasHelmet ? "H" : "K");
		if (alive && p.hasDefuser) {
			size_t l = strlen(status);
			snprintf(status + l, sizeof(status) - l, "%sD", l ? " " : "");
		}
		if (p.ping > 0) {
			size_t l = strlen(status);
			snprintf(status + l, sizeof(status) - l, "%s%dms", l ? " " : "", (int)p.ping);
		}
		if (status[0]) {
			float sw = ImGui::CalcTextSize(status).x;
			ImGui::SameLine(winW - sw - 14.0f);
			ImGui::TextDisabled("%s", status);
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
}

void gui::RenderTeamPanels(const CGame& game) {
	constexpr float kPanelW = 240.0f;
	constexpr float kPad    = 10.0f;
	constexpr ImGuiWindowFlags kFlags =
		ImGuiWindowFlags_NoTitleBar        |
		ImGuiWindowFlags_NoResize          |
		ImGuiWindowFlags_NoMove            |
		ImGuiWindowFlags_NoScrollbar       |
		ImGuiWindowFlags_AlwaysAutoResize  |
		ImGuiWindowFlags_NoSavedSettings;

	uint8_t myTeam = game.localPlayer.teamID;

	ImGui::SetNextWindowPos(ImVec2(kPad, kPad), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(kPanelW, 0.0f), ImGuiCond_Always);
	ImGui::Begin("##enemies", nullptr, kFlags);

	ImGui::SetCursorPosX((kPanelW - ImGui::CalcTextSize("ENEMIES").x) * 0.5f);
	ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "ENEMIES");
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	for (int i = 0; i < 64; i++) {
		const CPlayer& p = game.players[i];
		if (!p.controller || p.teamID == myTeam || p.ping == 0) continue;
		renderPlayerEntry(p, game, true);
	}
	ImGui::End();
}
