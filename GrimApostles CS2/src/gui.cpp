#include "gui.h"
#include "sdk.h"

namespace gui {
	ID3D11Device* g_pd3dDevice = NULL;
	ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
	IDXGISwapChain* g_pSwapChain = NULL;
	UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
	ID3D11RenderTargetView* g_mainRenderTargetView = NULL;
	bool exitRequested = false;
	WNDCLASSEXW wc = {};
	HWND hwnd = NULL;
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




//Init
void gui::initializeGUI() {
	// Create application window
	wc = { sizeof(wc), CS_VREDRAW | CS_HREDRAW, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"GrimApostles", nullptr };
	::RegisterClassExW(&wc);
	gui::hwnd = ::CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"GrimApostles CS2", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, wc.hInstance, nullptr);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return;
	}
	// Show the window
	::ShowWindow(hwnd, SW_MAXIMIZE);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;


	// Setup Dear ImGui style
	ImGui::StyleColorsDark();


	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

}

//All of our rendering
void gui::Render() {
	ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.f);
	CGame game;
	loadTextures();
	loadMapBounds();
	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT || exitRequested) {
				done = true;
			}

		}
		if (done)
			break;

		// Handle window resize (we don't resize directly in the WM_SIZE handler)
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_ResizeWidth = g_ResizeHeight = 0;
			CreateRenderTarget();
		}

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		//Where the magic happens
		gui::ConnectButton();
		gui::ExitButton();
		//Read in all info
		if (DMADevice::bConnected) {
			game.update();
			//Render
			gameLoop(game);
		}

		ImGui::Render();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(0, 0); // Present without vsync
	}


	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);
	DMADevice::ShowKeyPress();
	return;
}
//Here is where we initialize our DMA and connect to process
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
			//closing our vmm & scatter handle
			DMADevice::Disconnect();
		}
	}
	ImGui::End();
}
//Exit button
void gui::ExitButton() {
	//Setting up exit button 
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


//Map and player rendering. We have seperate functions for map and players but the same imgui window between the two
void gui::gameLoop(CGame game) {
	std::string mapName = game.map;
	//checking bounds for vertigo/nuke
	if (mapName == "de_nuke" && game.localPlayer.position.z <= maps::nukeZBound) mapName = "de_nuke_lower";
	if (mapName == "de_vertigo" && game.localPlayer.position.z <= maps::vertigoZBound) mapName = "de_vertigo_lower";
	//ID3D11ShaderResourceView* texture = fetchMap(mapName);
	ID3D11ShaderResourceView* texture = maps::mapTextures[mapName];
	renderMap(texture);
	renderIcons(game);
	renderAimLines(game);
	renderPlayers(game);
	ImGui::End();
}

void gui::renderMap(ID3D11ShaderResourceView* texture) {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::SetNextWindowPos(ImVec2((ImGui::GetIO().DisplaySize.x / 2 - maps::radarSize / 2), (ImGui::GetIO().DisplaySize.y / 2 - maps::radarSize / 2)));
	ImGui::Begin("MAP", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	ImGui::Image((void*)texture, ImVec2(maps::radarSize, maps::radarSize));
}

void gui::renderIcons(CGame game) {
	for (int i = 1; i <= 64; i++) {
		if (game.players[i - 1].controller == NULL) continue;
		if (game.players[i - 1].health == 0) continue;
		//Not drawing team weapons/util
		if (game.players[i - 1].teamID == game.localPlayer.teamID) continue;
		float x, y, z, angle;
		ImVec2 windowPos = ImGui::GetWindowPos();
		x = game.players[i - 1].position.x;
		y = game.players[i - 1].position.y;
		z = game.players[i - 1].position.z;
		//aim line\angle data
		angle = game.players[i - 1].eyeAngles.y;
		angle = angle * 3.14159265f / 180.0f;
		worldToRadar(x, y, game);

		int weaponID = game.players[i - 1].weaponID;
		float iconW = (float)icons::iconWidths[weaponID] * icons::scale;
		float iconH = (float)icons::iconHeights[weaponID] * icons::scale;
		ImVec2 iconPos;
		if (angle <= 3.14159265f && angle >= 0) {
			// below the dot
			iconPos = ImVec2(windowPos.x + x - (iconW / 2), (windowPos.y + y) + 10.f);
		}
		else {
			// above the dot
			iconPos = ImVec2(windowPos.x + x - (iconW / 2), (windowPos.y + y) - 10.f - iconH);
		}

		ImGui::GetForegroundDrawList()->AddImage(
			(ImTextureID)icons::iconTextures[weaponID],
			iconPos,
			ImVec2(iconPos.x + iconW, iconPos.y + iconH),
			ImVec2(0, 0), ImVec2(1, 1),
			IM_COL32(255, 255, 255, 255)
		);
	}
}
void gui::renderAimLines(CGame game) {
	for (int i = 1; i <= 64; i++) {
		if (game.players[i - 1].controller == NULL) continue;
		if (game.players[i - 1].health == 0) continue;
		float x, y, z, angle, length;
		float opacity;
		ImVec2 windowPos = ImGui::GetWindowPos();
		x = game.players[i - 1].position.x;
		y = game.players[i - 1].position.y;
		z = game.players[i - 1].position.z;
		//aim line\angle data
		angle = game.players[i - 1].eyeAngles.y;
		angle = angle * 3.14159265f / 180.0f;
		worldToRadar(x, y, game);
		opacity = setOpacity(game.localPlayer.position.z, z, game);
		length = 40.0f;
		//Aim lines
		ImVec2 endpoint = ImVec2(windowPos.x + x + length * cos(angle) + 1.0f, windowPos.y + y + length * sin(angle) * -1 + 1.0f);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2((windowPos.x + x), (windowPos.y + y)), endpoint, IM_COL32(0, 0, 0, opacity), 6.5f);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2((windowPos.x + x), (windowPos.y + y)), endpoint, IM_COL32(255, 255, 255, opacity), 4.0f);
	}
}
void gui::renderPlayers(CGame game) {
	for (int i = 1; i <= 64; i++) {
		if (game.players[i - 1].controller == NULL) continue;
		if (game.players[i - 1].health == 0) continue;
		float x, y, z;
		ImU32 dotColor;
		float opacity;
		ImVec2 windowPos = ImGui::GetWindowPos();
		x = game.players[i - 1].position.x;
		y = game.players[i - 1].position.y;
		z = game.players[i - 1].position.z;
		worldToRadar(x, y, game);
		opacity = setOpacity(game.localPlayer.position.z, z, game);
		//Only fetching colors for team - no need for enemy. they will all be red
		if (game.players[i - 1].teamID == game.localPlayer.teamID) {
			dotColor = setColor(game.players[i - 1].color, opacity);
		}
		else {
			dotColor = IM_COL32(255, 9, 9, opacity);
		}
		//LocalPlayer - white dot
		if (game.players[i - 1].controller == game.localPlayer.controller) {
			dotColor = IM_COL32(255, 255, 255, 255);
		}
		//Players
		ImGui::GetForegroundDrawList()->AddCircleFilled(ImVec2((windowPos.x + x), (windowPos.y + y)), 9.25f, IM_COL32(0, 0, 0, 255));
		ImGui::GetForegroundDrawList()->AddCircleFilled(ImVec2((windowPos.x + x), (windowPos.y + y)), 8.0f, dotColor);
	}
}



//Helpers - game related
void gui::worldToRadar(float& x, float& y, CGame game) {
	std::string mapName = game.map;
	mapData data = maps::mapBounds[mapName];
	x -= data.xBound;
	y -= data.yBound;
	x /= data.scale;
	y /= data.scale;
	//radar size conversion
	x *= maps::radarSize / 1024.f;
	y *= maps::radarSize / 1024.f;
	y *= -1;
}

ImU32 gui::setColor(DWORD color, float opacity) {
	switch (color) {
	//Grey
	case -1: 
		return IM_COL32(142, 212, 210, opacity);
	//Blue
	case 0:
		return IM_COL32(0, 255, 251, opacity);
	//Green
	case 1:
		return IM_COL32(47, 255, 0, opacity);
	//Yellow
	case 2:
		return IM_COL32(255, 255, 0, opacity);
	//Orange
	case 3:
		return IM_COL32(250, 130, 2, opacity);
	//Purple
	case 4:
		return IM_COL32(250, 2, 182, opacity);
	default:
		return IM_COL32(133, 204, 148, opacity);
	}
}

float gui::setOpacity(float localZ, float entZ, CGame game) {
	std::string mapName = game.map;
	if (mapName == "de_nuke") {
		if (localZ < -495 && entZ >= -495) return 155;
		if (localZ >= -495 && entZ < -495) return 155;
	}
	if (mapName == "de_vertigo") {
		if (localZ < 11700 && entZ >= 11700) return 155;
		if (localZ >= 11700 && entZ < 11700) return 155;
	}
	return 255;
}





//Loading all resources
void gui::loadMapBounds() {
	maps::mapBounds["cs_italy"] = mapData(-2647.0f, 2592.0f, 4.6f);
	maps::mapBounds["cs_office"] = mapData(-1838.0f, 1858.0f, 4.1f);
	maps::mapBounds["de_ancient"] = mapData(-2953.0f, 2164.0f, 5.0f);
	maps::mapBounds["de_anubis"] = mapData(-2796.0f, 3328.0f, 5.22f);
	maps::mapBounds["de_dust2"] = mapData(-2476.0f, 3239.0f, 4.4f);
	maps::mapBounds["de_inferno"] = mapData(-2087.0f, 3870.0f, 4.9f);
	maps::mapBounds["de_mirage"] = mapData(-3230.0f, 1713.0f, 5.0f);
	maps::mapBounds["de_nuke"] = mapData(-3453.0f, 2887.0f, 7.0f);
	maps::mapBounds["de_overpass"] = mapData(-4831.0f, 1781.0f, 5.2f);
	maps::mapBounds["de_vertigo"] = mapData(-3168.0f, 1762.0f, 4.0f);
	maps::mapBounds["de_train"] = mapData(-2308.0f, 2078.0f, 4.082077f);
}
void gui::loadTextures() {
	//Maps
	maps::mapTextures["cs_italy"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\cs_italy_radar.png");
	maps::mapTextures["cs_office"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\cs_office_radar.png");
	maps::mapTextures["de_ancient"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\de_ancient_radar.png");
	maps::mapTextures["de_anubis"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\de_anubis_radar.png");
	maps::mapTextures["de_dust2"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\de_dust2_radar.png");
	maps::mapTextures["de_inferno"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\de_inferno_radar.png");
	maps::mapTextures["de_mirage"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\de_mirage_radar.png");
	maps::mapTextures["de_nuke_lower"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\de_nuke_lower_radar.png");
	maps::mapTextures["de_nuke"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\de_nuke_radar.png");
	maps::mapTextures["de_overpass"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\de_overpass_radar.png");
	maps::mapTextures["de_vertigo_lower"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\de_vertigo_lower_radar.png");
	maps::mapTextures["de_vertigo"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\de_vertigo_radar.png");
	maps::mapTextures["de_train"] = LoadImageTexture(g_pd3dDevice, L".\\textures\\maps\\de_train_radar.png");

	// Icons
	icons::iconTextures[1] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\deagle.png", &icons::iconWidths[1], &icons::iconHeights[1]);
	icons::iconTextures[2] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\elite.png", &icons::iconWidths[2], &icons::iconHeights[2]);
	icons::iconTextures[3] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\fiveseven.png", &icons::iconWidths[3], &icons::iconHeights[3]);
	icons::iconTextures[4] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\glock.png", &icons::iconWidths[4], &icons::iconHeights[4]);
	icons::iconTextures[7] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\ak47.png", &icons::iconWidths[7], &icons::iconHeights[7]);
	icons::iconTextures[8] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\aug.png", &icons::iconWidths[8], &icons::iconHeights[8]);
	icons::iconTextures[9] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\awp.png", &icons::iconWidths[9], &icons::iconHeights[9]);
	icons::iconTextures[10] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\famas.png", &icons::iconWidths[10], &icons::iconHeights[10]);
	icons::iconTextures[11] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\g3sg1.png", &icons::iconWidths[11], &icons::iconHeights[11]);
	icons::iconTextures[13] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\galilar.png", &icons::iconWidths[13], &icons::iconHeights[13]);
	icons::iconTextures[14] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\m249.png", &icons::iconWidths[14], &icons::iconHeights[14]);
	icons::iconTextures[16] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\m4a1.png", &icons::iconWidths[16], &icons::iconHeights[16]);
	icons::iconTextures[17] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\mac10.png", &icons::iconWidths[17], &icons::iconHeights[17]);
	icons::iconTextures[19] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\p90.png", &icons::iconWidths[19], &icons::iconHeights[19]);
	icons::iconTextures[23] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\mp5sd.png", &icons::iconWidths[23], &icons::iconHeights[23]);
	icons::iconTextures[24] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\ump45.png", &icons::iconWidths[24], &icons::iconHeights[24]);
	icons::iconTextures[25] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\xm1014.png", &icons::iconWidths[25], &icons::iconHeights[25]);
	icons::iconTextures[26] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\bizon.png", &icons::iconWidths[26], &icons::iconHeights[26]);
	icons::iconTextures[27] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\mag7.png", &icons::iconWidths[27], &icons::iconHeights[27]);
	icons::iconTextures[28] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\negev.png", &icons::iconWidths[28], &icons::iconHeights[28]);
	icons::iconTextures[29] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\sawedoff.png", &icons::iconWidths[29], &icons::iconHeights[29]);
	icons::iconTextures[30] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\tec9.png", &icons::iconWidths[30], &icons::iconHeights[30]);
	icons::iconTextures[31] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\taser.png", &icons::iconWidths[31], &icons::iconHeights[31]);
	icons::iconTextures[32] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\hkp2000.png", &icons::iconWidths[32], &icons::iconHeights[32]);
	icons::iconTextures[33] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\mp7.png", &icons::iconWidths[33], &icons::iconHeights[33]);
	icons::iconTextures[34] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\mp9.png", &icons::iconWidths[34], &icons::iconHeights[34]);
	icons::iconTextures[35] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\nova.png", &icons::iconWidths[35], &icons::iconHeights[35]);
	icons::iconTextures[36] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\p250.png", &icons::iconWidths[36], &icons::iconHeights[36]);
	icons::iconTextures[38] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\scar20.png", &icons::iconWidths[38], &icons::iconHeights[38]);
	icons::iconTextures[39] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\sg556.png", &icons::iconWidths[39], &icons::iconHeights[39]);
	icons::iconTextures[40] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\ssg08.png", &icons::iconWidths[40], &icons::iconHeights[40]);
	icons::iconTextures[41] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knifegg.png", &icons::iconWidths[41], &icons::iconHeights[41]);
	icons::iconTextures[42] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife.png", &icons::iconWidths[42], &icons::iconHeights[42]);
	icons::iconTextures[43] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\flashbang.png", &icons::iconWidths[43], &icons::iconHeights[43]);
	icons::iconTextures[44] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\hegrenade.png", &icons::iconWidths[44], &icons::iconHeights[44]);
	icons::iconTextures[45] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\smokegrenade.png", &icons::iconWidths[45], &icons::iconHeights[45]);
	icons::iconTextures[46] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\molotov.png", &icons::iconWidths[46], &icons::iconHeights[46]);
	icons::iconTextures[47] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\decoy.png", &icons::iconWidths[47], &icons::iconHeights[47]);
	icons::iconTextures[48] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\incgrenade.png", &icons::iconWidths[48], &icons::iconHeights[48]);
	icons::iconTextures[49] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\c4.png", &icons::iconWidths[49], &icons::iconHeights[49]);
	icons::iconTextures[55] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\defuser.png", &icons::iconWidths[55], &icons::iconHeights[55]);
	icons::iconTextures[59] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_t.png", &icons::iconWidths[59], &icons::iconHeights[59]);
	icons::iconTextures[60] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\m4a1_silencer.png", &icons::iconWidths[60], &icons::iconHeights[60]);
	icons::iconTextures[61] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\usp_silencer.png", &icons::iconWidths[61], &icons::iconHeights[61]);
	icons::iconTextures[63] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\cz75a.png", &icons::iconWidths[63], &icons::iconHeights[63]);
	icons::iconTextures[64] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\revolver.png", &icons::iconWidths[64], &icons::iconHeights[64]);
	icons::iconTextures[500] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\bayonet.png", &icons::iconWidths[500], &icons::iconHeights[500]);
	icons::iconTextures[503] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_css.png", &icons::iconWidths[503], &icons::iconHeights[503]);
	icons::iconTextures[505] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_flip.png", &icons::iconWidths[505], &icons::iconHeights[505]);
	icons::iconTextures[506] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_gut.png", &icons::iconWidths[506], &icons::iconHeights[506]);
	icons::iconTextures[507] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_karambit.png", &icons::iconWidths[507], &icons::iconHeights[507]);
	icons::iconTextures[508] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_m9_bayonet.png", &icons::iconWidths[508], &icons::iconHeights[508]);
	icons::iconTextures[509] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_tactical.png", &icons::iconWidths[509], &icons::iconHeights[509]);
	icons::iconTextures[512] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_falchion.png", &icons::iconWidths[512], &icons::iconHeights[512]);
	icons::iconTextures[514] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_survival_bowie.png", &icons::iconWidths[514], &icons::iconHeights[514]);
	icons::iconTextures[515] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_butterfly.png", &icons::iconWidths[515], &icons::iconHeights[515]);
	icons::iconTextures[516] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_push.png", &icons::iconWidths[516], &icons::iconHeights[516]);
	icons::iconTextures[517] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_cord.png", &icons::iconWidths[517], &icons::iconHeights[517]);
	icons::iconTextures[518] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_canis.png", &icons::iconWidths[518], &icons::iconHeights[518]);
	icons::iconTextures[519] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_ursus.png", &icons::iconWidths[519], &icons::iconHeights[519]);
	icons::iconTextures[520] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_gypsy_jackknife.png", &icons::iconWidths[520], &icons::iconHeights[520]);
	icons::iconTextures[521] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_outdoor.png", &icons::iconWidths[521], &icons::iconHeights[521]);
	icons::iconTextures[522] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_stiletto.png", &icons::iconWidths[522], &icons::iconHeights[522]);
	icons::iconTextures[523] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_widowmaker.png", &icons::iconWidths[523], &icons::iconHeights[523]);
	icons::iconTextures[525] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_skeleton.png", &icons::iconWidths[525], &icons::iconHeights[525]);
	icons::iconTextures[526] = LoadImageTexture(g_pd3dDevice, L".\\textures\\icons\\knife_kukri.png", &icons::iconWidths[526], &icons::iconHeights[526]);

}

//Helpers - imgui
bool gui::CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}
void gui::CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}
void gui::CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}
void gui::CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}
//Loading images
ID3D11ShaderResourceView* gui::LoadImageTexture(ID3D11Device* device, const wchar_t* filename)
{
	ID3D11ShaderResourceView* texture = nullptr;

	HRESULT hr = DirectX::CreateWICTextureFromFile(
		device,
		filename,
		nullptr,
		&texture
	);

	if (FAILED(hr))
	{
		// Failed to load - return null
		return nullptr;
	}

	return texture;
}
ID3D11ShaderResourceView* gui::LoadImageTexture(ID3D11Device* device, const wchar_t* filename, int* outWidth, int* outHeight)
{
	ID3D11ShaderResourceView* texture = nullptr;
	ID3D11Resource* resource = nullptr;

	HRESULT hr = DirectX::CreateWICTextureFromFile(
		device,
		filename,
		&resource,
		&texture
	);

	if (FAILED(hr))
		return nullptr;

	if (resource && outWidth && outHeight)
	{
		ID3D11Texture2D* tex2D = nullptr;
		if (SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex2D)))
		{
			D3D11_TEXTURE2D_DESC desc;
			tex2D->GetDesc(&desc);
			*outWidth = (int)desc.Width;
			*outHeight = (int)desc.Height;
			tex2D->Release();
		}
		resource->Release();
	}

	return texture;
}
//Handles resizing, minimizing, exiting window
LRESULT WINAPI gui::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	case WM_GETMINMAXINFO:
		MINMAXINFO* minMaxInfo = (MINMAXINFO*)lParam;
		minMaxInfo->ptMinTrackSize.x = 960;  // Set minimum width
		minMaxInfo->ptMinTrackSize.y = 540;  // Set minimum height
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);

}