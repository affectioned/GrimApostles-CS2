#include "pch.h"
#include "gui.h"
#include "sdk.h"
#include "updater.h"

extern std::atomic<bool> bRunning;

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
	wc = { sizeof(wc), CS_VREDRAW | CS_HREDRAW, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"CS2_DMA_RADAR", nullptr };
	::RegisterClassExW(&wc);
	hwnd = ::CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"CS2_DMA_RADAR", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, wc.hInstance, nullptr);
	Log::Info("[GUI]: Window created");
}

bool gui::InitD3D() {
	if (!CreateDeviceD3D(hwnd)) {
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		Log::Error("[GUI]: Direct3D initialization failed");
		return false;
	}
	Log::Info("[GUI]: Direct3D initialized");
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
	ImGui::GetIO().IniFilename = "CS2_DMA_RADAR.ini";

	ImGuiSettingsHandler h;
	h.TypeName   = "CS2_DMA_RADAR";
	h.TypeHash   = ImHashStr("CS2_DMA_RADAR");
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

	// Load a font with broad Unicode coverage (Latin + Cyrillic) for player names
	{
		static const ImWchar kRanges[] = {
			0x0020, 0x00FF,  // Latin Basic + Latin-1 Supplement
			0x0400, 0x052F,  // Cyrillic + Cyrillic Supplement
			0,
		};
		ImFontConfig cfg;
		cfg.OversampleH = 1; cfg.OversampleV = 1; cfg.PixelSnapH = true;
		if (!ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 13.0f, &cfg, kRanges))
			ImGui::GetIO().Fonts->AddFontDefault();
	}

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
	Log::Info("[GUI]: ImGui {} initialized", IMGUI_VERSION);
}

void gui::OnFrame(CGame& game, std::mutex& gameMutex) {
	MSG msg;
	while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
		if (msg.message == WM_QUIT) {
			Log::Info("Exit: WM_QUIT received");
			bRunning.store(false, std::memory_order_release);
			return;
		}
	}

	if (exitRequested) {
		Log::Info("Exit: exitRequested");
		bRunning.store(false, std::memory_order_release);
		return;
	}

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

	if (g_Connected.load(std::memory_order_acquire)) {
		CGame snapshot;
		{
			std::lock_guard<std::mutex> lock(gameMutex);
			snapshot = game;
		}
		gameLoop(snapshot);
		if (settings::showTeamPanels)
			RenderTeamPanels(snapshot);
		RenderBombPanel(snapshot);
	}

	ImGui::Render();
	constexpr float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
	g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, black);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	g_pSwapChain->Present(0, 0);
}

void gui::Cleanup() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	for (auto& [k, v] : maps::mapTextures)  { if (v) { v->Release(); v = nullptr; } }
	for (auto& [k, v] : icons::iconTextures) { if (v) { v->Release(); v = nullptr; } }
	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);
	Log::Info("[GUI]: Cleanup complete");
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
	ImGui::SetCursorPosX((panelW - ImGui::CalcTextSize("CS2_DMA_RADAR").x) * 0.5f);
	ImGui::TextColored(ImVec4(0.5f, 0.78f, 1.0f, 1.0f), "CS2_DMA_RADAR");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// FPS counter
	char fpsBuf[16];
	snprintf(fpsBuf, sizeof(fpsBuf), "%.0f FPS", io.Framerate);
	ImGui::TextDisabled("%s", fpsBuf);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Exit — neutral until hovered, then red tint
	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.14f, 0.14f, 0.18f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.50f, 0.12f, 0.12f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.60f, 0.10f, 0.10f, 1.0f));
	if (ImGui::Button("Exit", ImVec2(-1.0f, 22.0f))) {
		Log::Info("Exit: button clicked");
		exitRequested = true;
		bRunning.store(false, std::memory_order_release);
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

static void renderPlayerEntry(const CPlayer& p, const CGame& game, bool isEnemy, bool isCarrier = false) {
	constexpr float kDotR   = 5.0f;
	constexpr float kIndent = kDotR * 2.0f + 8.0f;
	constexpr float kBarH   = 3.0f;
	constexpr float kIconH  = 14.0f;

	bool        alive  = p.pawn.lifeState == 0;
	float       alpha  = alive ? 1.0f : 0.38f;
	ImDrawList* dl     = ImGui::GetWindowDrawList();
	ImVec2      origin = ImGui::GetCursorScreenPos();
	float       lineH  = ImGui::GetTextLineHeight();
	float       winW   = ImGui::GetWindowWidth();

	// Dot
	ImU32 dotCol;
	if      (p.controllerBase == game.localPlayer.controllerBase) dotCol = IM_COL32(255, 255, 255, (int)(alpha * 255));
	else if (!isEnemy)                                             dotCol = gui::setColor(p.ctrl.color, alpha * 255.0f);
	else                                                           dotCol = IM_COL32(255, 60,  60,  (int)(alpha * 255));

	ImVec2 dotCtr = { origin.x + kDotR + 2.0f, origin.y + lineH * 0.5f + 1.0f };
	dl->AddCircleFilled(dotCtr, kDotR + 1.0f, IM_COL32(0, 0, 0, (int)(alpha * 180)));
	dl->AddCircleFilled(dotCtr, kDotR,         dotCol);

	// Name (left) + HP number (right)
	char hpBuf[8];
	if (alive) snprintf(hpBuf, sizeof(hpBuf), "%d", p.pawn.health);
	else       snprintf(hpBuf, sizeof(hpBuf), "---");

	float hpTextW  = ImGui::CalcTextSize(hpBuf).x;
	float nameMaxW = winW - kIndent - hpTextW - 20.0f;

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kIndent);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.95f, alpha));

	ImVec2 nameScreen = ImGui::GetCursorScreenPos();
	ImGui::PushClipRect(nameScreen, { nameScreen.x + nameMaxW, nameScreen.y + lineH + 2.0f }, true);
	ImGui::TextUnformatted(p.ctrl.name[0] ? p.ctrl.name : "...");
	ImGui::PopClipRect();

	ImGui::SameLine(winW - hpTextW - 14.0f);
	if (alive) ImGui::Text("%d", p.pawn.health);
	else       ImGui::TextDisabled("---");
	ImGui::PopStyleColor();

	// Health bar
	float barStartX = origin.x + kIndent;
	float barW      = winW - kIndent - 14.0f;
	ImVec2 barTL    = { barStartX, ImGui::GetCursorScreenPos().y };
	float filled    = alive ? (float)p.pawn.health / 100.0f : 0.0f;
	if (filled > 1.0f) filled = 1.0f;
	float r = 255.0f * (1.0f - filled);
	float g = 255.0f * filled;
	dl->AddRectFilled(barTL, { barTL.x + barW,          barTL.y + kBarH }, IM_COL32(30, 30, 30, (int)(alpha * 200)), 1.5f);
	dl->AddRectFilled(barTL, { barTL.x + barW * filled, barTL.y + kBarH }, IM_COL32((int)r, (int)g, 0, (int)(alpha * 255)), 1.5f);
	ImGui::Dummy({ 0.0f, kBarH + 3.0f });

	// Weapon icon (left-indented)
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kIndent);
	if (alive) {
		auto texIt = icons::iconTextures.find(p.pawn.activeWeaponID);
		if (texIt != icons::iconTextures.end() && texIt->second) {
			int iw = icons::iconWidths.count(p.pawn.activeWeaponID)  ? icons::iconWidths[p.pawn.activeWeaponID]  : 1;
			int ih = icons::iconHeights.count(p.pawn.activeWeaponID) ? icons::iconHeights[p.pawn.activeWeaponID] : 1;
			float iconW = (ih > 0) ? kIconH * (float)iw / (float)ih : kIconH;
			ImGui::Image((ImTextureID)texIt->second, { iconW, kIconH });
		} else {
			ImGui::Dummy({ 0.0f, kIconH });
		}
	} else {
		ImGui::Dummy({ 0.0f, kIconH });
	}

	// Last place name (center of the row)
	if (alive && p.pawn.lastPlaceName[0]) {
		ImGui::SameLine();
		ImGui::TextDisabled("%s", p.pawn.lastPlaceName);
	}

	// Right-aligned: DEFUSING > C4 carrier > armor/defuser
	if (alive && p.pawn.isDefusing) {
		constexpr const char* kLabel = "DEFUSING";
		ImGui::SameLine(winW - ImGui::CalcTextSize(kLabel).x - 14.0f);
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.1f, 1.0f), kLabel);
	} else if (alive && isCarrier) {
		auto it = icons::iconTextures.find(49);
		if (it != icons::iconTextures.end() && it->second) {
			int iw = icons::iconWidths.count(49)  ? icons::iconWidths[49]  : 1;
			int ih = icons::iconHeights.count(49) ? icons::iconHeights[49] : 1;
			float iconW = (ih > 0) ? kIconH * (float)iw / (float)ih : kIconH;
			ImGui::SameLine(winW - iconW - 14.0f);
			ImGui::Image((ImTextureID)it->second, { iconW, kIconH }, {0,0}, {1,1},
			             ImVec4(1.0f, 0.88f, 0.0f, 1.0f));
		} else {
			constexpr const char* kLabel = "C4";
			ImGui::SameLine(winW - ImGui::CalcTextSize(kLabel).x - 14.0f);
			ImGui::TextColored(ImVec4(1.0f, 0.88f, 0.0f, 1.0f), kLabel);
		}
	} else {
		char status[32] = {};
		if (alive && p.ctrl.armor > 0)
			snprintf(status, sizeof(status), "%s", p.ctrl.hasHelmet ? "H" : "K");
		if (alive && p.ctrl.hasDefuser) {
			size_t l = strlen(status);
			snprintf(status + l, sizeof(status) - l, "%sD", l ? " " : "");
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

	TeamID myTeam = game.localPlayer.ctrl.teamID;

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
		if (!p.controllerBase || p.ctrl.teamID < TEAM_T || p.ctrl.teamID == myTeam || p.pawn.health == 0) continue;
		bool isCarrier = (game.bomb.isCarried && game.bomb.carrierSlot == i)
		              || (game.bomb.entity && !game.bomb.hasDefused && !game.bomb.hasExploded && game.bomb.planterSlot == i);
		renderPlayerEntry(p, game, true, isCarrier);
	}
	ImGui::End();
}

// ─── Bomb Panel ───────────────────────────────────────────────────────────────

void gui::RenderBombPanel(const CGame& game) {
	const C_PlantedC4& b = game.bomb;

	// Only show when bomb is planted and local player is CT-side
	if (!b.entity || game.localPlayer.ctrl.teamID == TEAM_T) return;

	constexpr float kPanelW = 200.0f;
	constexpr float kPad    = 10.0f;
	constexpr ImGuiWindowFlags kFlags =
		ImGuiWindowFlags_NoTitleBar        |
		ImGuiWindowFlags_NoResize          |
		ImGuiWindowFlags_NoMove            |
		ImGuiWindowFlags_NoScrollbar       |
		ImGuiWindowFlags_AlwaysAutoResize  |
		ImGuiWindowFlags_NoSavedSettings;

	// Anchor to bottom-left corner
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(kPad, io.DisplaySize.y - kPad), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
	ImGui::SetNextWindowSize(ImVec2(kPanelW, 0.0f), ImGuiCond_Always);
	ImGui::Begin("##bomb", nullptr, kFlags);

	// Title
	ImGui::SetCursorPosX((kPanelW - ImGui::CalcTextSize("BOMB").x) * 0.5f);
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "BOMB");
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (b.hasDefused) {
		// Defused
		ImGui::SetCursorPosX((kPanelW - ImGui::CalcTextSize("DEFUSED").x) * 0.5f);
		ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "DEFUSED");

	} else if (b.hasExploded) {
		// Exploded
		ImGui::SetCursorPosX((kPanelW - ImGui::CalcTextSize("EXPLODED").x) * 0.5f);
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "EXPLODED");

	} else if (b.entity) {
		// Planted — show site + countdown
		const char* siteStr = (b.site == 0) ? "A" : (b.site == 1) ? "B" : "?";
		char plantBuf[24];
		snprintf(plantBuf, sizeof(plantBuf), "PLANTED - %s", siteStr);
		ImGui::SetCursorPosX((kPanelW - ImGui::CalcTextSize(plantBuf).x) * 0.5f);
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", plantBuf);

		// Timer — turns red below 10 seconds
		float   rem = b.timeRemaining();
		ImVec4  timerCol = (rem > 10.0f) ? ImVec4(1.0f, 0.9f, 0.3f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
		char    timerBuf[12];
		snprintf(timerBuf, sizeof(timerBuf), "%.1fs", rem);
		ImGui::SetCursorPosX((kPanelW - ImGui::CalcTextSize(timerBuf).x) * 0.5f);
		ImGui::TextColored(timerCol, "%s", timerBuf);

		if (b.isBeingDefused) {
			constexpr const char* kDefusing = "DEFUSING";
			ImGui::SetCursorPosX((kPanelW - ImGui::CalcTextSize(kDefusing).x) * 0.5f);
			ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), kDefusing);
		}

	}

	ImGui::Spacing();
	ImGui::End();
}
