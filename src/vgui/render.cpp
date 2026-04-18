#include "pch.h"
#include "gui.h"
#include "sdk.h"

static constexpr float PI = 3.14159265f;

void gui::gameLoop(const CGame& game) {
	std::string mapName = game.mapName;

	auto boundsIt = maps::mapBounds.find(mapName);
	if (boundsIt != maps::mapBounds.end()) {
		float thresh = boundsIt->second.lowerZThreshold;
		if (game.localPlayer.pawn.position.z <= thresh) {
			std::string lower = mapName + "_lower";
			if (maps::mapTextures.count(lower))
				mapName = lower;
		}
	}

	auto texIt = maps::mapTextures.find(mapName);
	if (texIt == maps::mapTextures.end() || !texIt->second) return;

	renderMap(texIt->second);
	renderPlayers(game);
	renderBomb(game);
	ImGui::End();
}

void gui::renderMap(ID3D11ShaderResourceView* texture) {
	ImVec2 display = ImGui::GetIO().DisplaySize;
	maps::radarSize = std::min(display.x, display.y);

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

void gui::renderPlayers(const CGame& game) {
	ImVec2      windowPos = ImGui::GetWindowPos();
	const float localZ    = game.localPlayer.pawn.position.z;

	for (int i = 0; i < 64; i++) {
		const CPlayer& p = game.players[i];

		bool visible = p.controllerBase && p.pawn.lifeState == 0 && p.ctrl.teamID >= 2 && p.pawn.health > 0;
		if (!visible) continue;

		float x     = p.pawn.position.x;
		float y     = p.pawn.position.y;
		float z     = p.pawn.position.z;
		float angle = p.pawn.eyeAngles.y * PI / 180.0f;
		worldToRadar(x, y, game);

		float  opacity = setOpacity(localZ, z, game);
		ImVec2 pos     = ImVec2(windowPos.x + x, windowPos.y + y);

		// Aim line
		if (settings::showAimLines) {
			ImVec2 endpoint = ImVec2(pos.x + settings::aimLineLength * cos(angle) + 1.0f,
			                         pos.y + settings::aimLineLength * sin(angle) * -1.0f + 1.0f);
			ImGui::GetForegroundDrawList()->AddLine(pos, endpoint, IM_COL32(0,   0,   0,   (int)opacity), 6.5f);
			ImGui::GetForegroundDrawList()->AddLine(pos, endpoint, IM_COL32(255, 255, 255, (int)opacity), 4.0f);
		}

		// Weapon icon -- enemies only
		if (settings::showWeaponIcons && p.ctrl.teamID != game.localPlayer.ctrl.teamID) {
			int weaponID = p.pawn.activeWeaponID;
			auto texIt = icons::iconTextures.find(weaponID);
			if (texIt != icons::iconTextures.end() && texIt->second) {
				float  iconW   = (float)icons::iconWidths[weaponID]  * settings::iconScale;
				float  iconH   = (float)icons::iconHeights[weaponID] * settings::iconScale;
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

		// Player dot
		ImU32 dotColor;
		if      (p.controllerBase == game.localPlayer.controllerBase) dotColor = IM_COL32(255, 255, 255, 255);
		else if (p.ctrl.teamID    == game.localPlayer.ctrl.teamID)    dotColor = setColor(p.ctrl.color, opacity);
		else                                                           dotColor = IM_COL32(255, 9, 9, (int)opacity);

		// Defusing ring -- orange, enemies only
		if (p.pawn.isDefusing && p.ctrl.teamID != game.localPlayer.ctrl.teamID)
			ImGui::GetForegroundDrawList()->AddCircle(pos, settings::dotRadius + 3.5f, IM_COL32(255, 150, 20, (int)opacity), 0, 2.0f);

		ImGui::GetForegroundDrawList()->AddCircleFilled(pos, settings::dotRadius + 1.25f, IM_COL32(0, 0, 0, 255));
		ImGui::GetForegroundDrawList()->AddCircleFilled(pos, settings::dotRadius,         dotColor);

		// Health bar -- enemies only
		if (settings::showHealthBars && p.ctrl.teamID != game.localPlayer.ctrl.teamID) {
			constexpr float barW = 3.0f, barH = 18.0f, barX = 14.0f;
			float filled = (float)p.pawn.health / 100.0f;
			float r = 255.0f * (1.0f - filled);
			float g = 255.0f * filled;
			ImVec2 barTL = ImVec2(pos.x - barX - barW, pos.y - barH / 2);
			ImVec2 barBR = ImVec2(pos.x - barX,        pos.y + barH / 2);
			float  fillY = barBR.y - barH * filled;
			ImGui::GetForegroundDrawList()->AddRectFilled(barTL, barBR, IM_COL32(0, 0, 0, (int)opacity));
			ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(barTL.x, fillY), barBR, IM_COL32((int)r, (int)g, 0, (int)opacity));
		}

		// Player name
		if (settings::showPlayerNames && p.controllerBase != game.localPlayer.controllerBase && p.ctrl.name[0]) {
			ImVec2 textSize = ImGui::CalcTextSize(p.ctrl.name);
			ImVec2 namePos  = ImVec2(pos.x - textSize.x * 0.5f, pos.y + settings::dotRadius + 4.0f);
			ImGui::GetForegroundDrawList()->AddText(ImVec2(namePos.x + 1, namePos.y + 1), IM_COL32(0,   0,   0,   (int)opacity), p.ctrl.name);
			ImGui::GetForegroundDrawList()->AddText(namePos,                               IM_COL32(255, 255, 255, (int)opacity), p.ctrl.name);
		}
	}
}

void gui::renderBomb(const CGame& game) {
	ImVec2 windowPos = ImGui::GetWindowPos();
	const C_PlantedC4& b = game.bomb;

	// Carrier ring -- yellow halo on whoever holds the C4
	if (b.isCarried && b.carrierSlot >= 0) {
		const CPlayer& carrier = game.players[b.carrierSlot];
		if (carrier.controllerBase && carrier.pawn.lifeState == 0) {
			float x = carrier.pawn.position.x;
			float y = carrier.pawn.position.y;
			worldToRadar(x, y, game);
			ImVec2 pos = ImVec2(windowPos.x + x, windowPos.y + y);
			ImGui::GetForegroundDrawList()->AddCircle(pos, settings::dotRadius + 5.0f, IM_COL32(255, 220, 0, 210), 0, 1.5f);
		}
	}

	// Planted bomb dot
	if (!b.entity || b.hasDefused || b.hasExploded) return;

	float bx = b.position.x;
	float by = b.position.y;
	worldToRadar(bx, by, game);
	ImVec2 pos = ImVec2(windowPos.x + bx, windowPos.y + by);

	ImGui::GetForegroundDrawList()->AddCircleFilled(pos, settings::dotRadius + 1.25f, IM_COL32(0, 0, 0, 255));
	ImGui::GetForegroundDrawList()->AddCircleFilled(pos, settings::dotRadius,         IM_COL32(255, 200, 0, 255));

	if (b.isBeingDefused)
		ImGui::GetForegroundDrawList()->AddCircle(pos, settings::dotRadius + 3.5f, IM_COL32(0, 200, 255, 220), 0, 2.0f);
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
	case -1: return IM_COL32(142, 212, 210, (int)opacity);
	case  0: return IM_COL32(  0, 255, 251, (int)opacity);
	case  1: return IM_COL32( 47, 255,   0, (int)opacity);
	case  2: return IM_COL32(255, 255,   0, (int)opacity);
	case  3: return IM_COL32(250, 130,   2, (int)opacity);
	case  4: return IM_COL32(250,   2, 182, (int)opacity);
	default: return IM_COL32(133, 204, 148, (int)opacity);
	}
}

float gui::setOpacity(float localZ, float entZ, const CGame& game) {
	auto it = maps::mapBounds.find(game.mapName);
	if (it == maps::mapBounds.end()) return 255;
	float thresh = it->second.lowerZThreshold;
	if (thresh == std::numeric_limits<float>::infinity()) return 255;
	if ((localZ <= thresh) != (entZ <= thresh)) return 155;
	return 255;
}
