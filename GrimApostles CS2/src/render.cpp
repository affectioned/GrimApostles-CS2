#include "pch.h"
#include "gui.h"
#include "sdk.h"

void gui::gameLoop(CGame game) {
	std::string mapName = game.map;

	// Resolve multi-level map variants by Z position
	if (mapName == "de_nuke"    && game.localPlayer.position.z <= maps::nukeZBound)    mapName = "de_nuke_lower";
	if (mapName == "de_vertigo" && game.localPlayer.position.z <= maps::vertigoZBound) mapName = "de_vertigo_lower";

	ID3D11ShaderResourceView* texture = maps::mapTextures[mapName];
	renderMap(texture);
	renderIcons(game);
	renderAimLines(game);
	renderPlayers(game);
	ImGui::End();
}

void gui::renderMap(ID3D11ShaderResourceView* texture) {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::SetNextWindowPos(ImVec2(
		(ImGui::GetIO().DisplaySize.x / 2 - maps::radarSize / 2),
		(ImGui::GetIO().DisplaySize.y / 2 - maps::radarSize / 2)
	));
	ImGui::Begin("MAP", nullptr,
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoScrollbar  |
		ImGuiWindowFlags_NoTitleBar   |
		ImGuiWindowFlags_NoResize
	);
	ImGui::Image((void*)texture, ImVec2(maps::radarSize, maps::radarSize));
}

void gui::renderIcons(CGame game) {
	ImVec2 windowPos = ImGui::GetWindowPos();

	for (int i = 1; i <= 64; i++) {
		if (game.players[i - 1].controller == NULL) continue;
		if (game.players[i - 1].health == 0)        continue;
		// Skip teammates
		if (game.players[i - 1].teamID == game.localPlayer.teamID) continue;

		float x     = game.players[i - 1].position.x;
		float y     = game.players[i - 1].position.y;
		float angle = game.players[i - 1].eyeAngles.y * 3.14159265f / 180.0f;
		worldToRadar(x, y, game);

		int   weaponID = game.players[i - 1].weaponID;
		float iconW    = (float)icons::iconWidths[weaponID]  * icons::scale;
		float iconH    = (float)icons::iconHeights[weaponID] * icons::scale;

		ImVec2 iconPos;
		if (angle >= 0 && angle <= 3.14159265f)
			iconPos = ImVec2(windowPos.x + x - (iconW / 2), (windowPos.y + y) + 10.f);
		else
			iconPos = ImVec2(windowPos.x + x - (iconW / 2), (windowPos.y + y) - 10.f - iconH);

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
	ImVec2 windowPos = ImGui::GetWindowPos();
	const float length = 40.0f;

	for (int i = 1; i <= 64; i++) {
		if (game.players[i - 1].controller == NULL) continue;
		if (game.players[i - 1].health == 0)        continue;

		float x       = game.players[i - 1].position.x;
		float y       = game.players[i - 1].position.y;
		float z       = game.players[i - 1].position.z;
		float angle   = game.players[i - 1].eyeAngles.y * 3.14159265f / 180.0f;
		worldToRadar(x, y, game);

		float   opacity  = setOpacity(game.localPlayer.position.z, z, game);
		ImVec2  origin   = ImVec2(windowPos.x + x, windowPos.y + y);
		ImVec2  endpoint = ImVec2(origin.x + length * cos(angle) + 1.0f, origin.y + length * sin(angle) * -1 + 1.0f);

		ImGui::GetForegroundDrawList()->AddLine(origin, endpoint, IM_COL32(0,   0,   0,   (int)opacity), 6.5f);
		ImGui::GetForegroundDrawList()->AddLine(origin, endpoint, IM_COL32(255, 255, 255, (int)opacity), 4.0f);
	}
}

void gui::renderPlayers(CGame game) {
	ImVec2 windowPos = ImGui::GetWindowPos();

	for (int i = 1; i <= 64; i++) {
		if (game.players[i - 1].controller == NULL) continue;
		if (game.players[i - 1].health == 0)        continue;

		float x = game.players[i - 1].position.x;
		float y = game.players[i - 1].position.y;
		float z = game.players[i - 1].position.z;
		worldToRadar(x, y, game);

		float  opacity = setOpacity(game.localPlayer.position.z, z, game);
		ImU32  dotColor;

		if (game.players[i - 1].teamID == game.localPlayer.teamID)
			dotColor = setColor(game.players[i - 1].color, opacity);
		else
			dotColor = IM_COL32(255, 9, 9, (int)opacity);

		// Local player is always white
		if (game.players[i - 1].controller == game.localPlayer.controller)
			dotColor = IM_COL32(255, 255, 255, 255);

		ImVec2 pos = ImVec2(windowPos.x + x, windowPos.y + y);
		ImGui::GetForegroundDrawList()->AddCircleFilled(pos, 9.25f, IM_COL32(0, 0, 0, 255));
		ImGui::GetForegroundDrawList()->AddCircleFilled(pos, 8.0f,  dotColor);
	}
}


void gui::worldToRadar(float& x, float& y, CGame game) {
	mapData data = maps::mapBounds[game.map];
	x -= data.xBound;
	y -= data.yBound;
	x /= data.scale;
	y /= data.scale;
	x *= maps::radarSize / 1024.f;
	y *= maps::radarSize / 1024.f;
	y *= -1;
}

ImU32 gui::setColor(DWORD color, float opacity) {
	switch (color) {
	case -1: return IM_COL32(142, 212, 210, (int)opacity); // Grey
	case  0: return IM_COL32(  0, 255, 251, (int)opacity); // Blue
	case  1: return IM_COL32( 47, 255,   0, (int)opacity); // Green
	case  2: return IM_COL32(255, 255,   0, (int)opacity); // Yellow
	case  3: return IM_COL32(250, 130,   2, (int)opacity); // Orange
	case  4: return IM_COL32(250,   2, 182, (int)opacity); // Purple
	default: return IM_COL32(133, 204, 148, (int)opacity);
	}
}

float gui::setOpacity(float localZ, float entZ, CGame game) {
	std::string mapName = game.map;
	if (mapName == "de_nuke") {
		if (localZ <  maps::nukeZBound && entZ >= maps::nukeZBound) return 155;
		if (localZ >= maps::nukeZBound && entZ <  maps::nukeZBound) return 155;
	}
	if (mapName == "de_vertigo") {
		if (localZ <  maps::vertigoZBound && entZ >= maps::vertigoZBound) return 155;
		if (localZ >= maps::vertigoZBound && entZ <  maps::vertigoZBound) return 155;
	}
	return 255;
}
