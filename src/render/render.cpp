#include "pch.h"
#include "gui.h"
#include "sdk.h"

static constexpr float PI = 3.14159265f;

void gui::gameLoop(const CGame& game) {
	std::string mapName = game.mapName;

	// Resolve multi-level map variants by Z position
	if (mapName == "de_nuke"    && game.localPlayer.position.z <= maps::nukeZBound)    mapName = "de_nuke_lower";
	if (mapName == "de_vertigo" && game.localPlayer.position.z <= maps::vertigoZBound) mapName = "de_vertigo_lower";

	auto texIt = maps::mapTextures.find(mapName);
	if (texIt == maps::mapTextures.end() || !texIt->second) return;

	renderMap(texIt->second);
	renderPlayers(game);
	ImGui::End();
}

void gui::renderMap(ID3D11ShaderResourceView* texture) {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::SetNextWindowPos(ImVec2(
		(ImGui::GetIO().DisplaySize.x / 2 - maps::radarSize / 2),
		(ImGui::GetIO().DisplaySize.y / 2 - maps::radarSize / 2)
	));
	ImGui::SetNextWindowSize(ImVec2(maps::radarSize, maps::radarSize));
	ImGui::Begin("MAP", nullptr,
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoScrollbar  |
		ImGuiWindowFlags_NoTitleBar   |
		ImGuiWindowFlags_NoResize
	);
	ImGui::Image((void*)texture, ImVec2(maps::radarSize, maps::radarSize));
	ImGui::PopStyleVar(2);
}

// Single pass over all players: aim line -> weapon icon (enemies only) -> dot -> health bar
void gui::renderPlayers(const CGame& game) {
	ImVec2      windowPos  = ImGui::GetWindowPos();
	const float aimLength  = 40.0f;
	const float localZ     = game.localPlayer.position.z;

	for (int i = 0; i < 64; i++) {
		const CPlayer& p = game.players[i];
		if (!p.controller || !p.health) continue;

		float x     = p.position.x;
		float y     = p.position.y;
		float z     = p.position.z;
		float angle = p.eyeAngles.y * PI / 180.0f;
		worldToRadar(x, y, game);

		float  opacity = setOpacity(localZ, z, game);
		ImVec2 pos     = ImVec2(windowPos.x + x, windowPos.y + y);

		// Aim line
		ImVec2 endpoint = ImVec2(pos.x + aimLength * cos(angle) + 1.0f,
		                         pos.y + aimLength * sin(angle) * -1.0f + 1.0f);
		ImGui::GetForegroundDrawList()->AddLine(pos, endpoint, IM_COL32(0,   0,   0,   (int)opacity), 6.5f);
		ImGui::GetForegroundDrawList()->AddLine(pos, endpoint, IM_COL32(255, 255, 255, (int)opacity), 4.0f);

		// Weapon icon -- enemies only
		if (p.teamID != game.localPlayer.teamID) {
			int weaponID = p.activeWeaponID;
			auto texIt = icons::iconTextures.find(weaponID);
			if (texIt != icons::iconTextures.end() && texIt->second) {
				float  iconW   = (float)icons::iconWidths[weaponID]  * icons::scale;
				float  iconH   = (float)icons::iconHeights[weaponID] * icons::scale;
				ImVec2 iconPos = (angle >= 0 && angle <= PI)
					? ImVec2(pos.x - iconW / 2, pos.y + 20.f)
					: ImVec2(pos.x - iconW / 2, pos.y - 20.f - iconH);
				ImGui::GetForegroundDrawList()->AddImage(
					(ImTextureID)texIt->second,
					iconPos, ImVec2(iconPos.x + iconW, iconPos.y + iconH),
					ImVec2(0, 0), ImVec2(1, 1),
					IM_COL32(255, 255, 255, 255)
				);
			}
		}

		// Player dot -- local player white, teammates colored, enemies red
		ImU32 dotColor;
		if      (p.controller == game.localPlayer.controller) dotColor = IM_COL32(255, 255, 255, 255);
		else if (p.teamID     == game.localPlayer.teamID)     dotColor = setColor(p.color, opacity);
		else                                                   dotColor = IM_COL32(255, 9, 9, (int)opacity);

		ImGui::GetForegroundDrawList()->AddCircleFilled(pos, 9.25f, IM_COL32(0, 0, 0, 255));
		ImGui::GetForegroundDrawList()->AddCircleFilled(pos, 8.0f,  dotColor);

		// Health bar -- enemies only, vertical bar on the left side of the dot
		if (p.teamID != game.localPlayer.teamID) {
			constexpr float barW = 3.0f, barH = 18.0f, barX = 14.0f;
			float filled = (float)p.health / 100.0f;
			float r = 255.0f * (1.0f - filled);
			float g = 255.0f * filled;
			ImVec2 barTL = ImVec2(pos.x - barX - barW, pos.y - barH / 2);
			ImVec2 barBR = ImVec2(pos.x - barX,        pos.y + barH / 2);
			float  fillY = barBR.y - barH * filled;
			ImGui::GetForegroundDrawList()->AddRectFilled(barTL, barBR, IM_COL32(0, 0, 0, (int)opacity));
			ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(barTL.x, fillY), barBR, IM_COL32((int)r, (int)g, 0, (int)opacity));
		}
	}
}


void gui::worldToRadar(float& x, float& y, const CGame& game) {
	auto it = maps::mapBounds.find(game.mapName);
	if (it == maps::mapBounds.end() || it->second.scale == 0.0f) return;
	mapData data = it->second;
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

float gui::setOpacity(float localZ, float entZ, const CGame& game) {
	std::string mapName = game.mapName;
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
