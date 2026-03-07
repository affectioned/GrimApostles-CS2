#include "pch.h"
#include "gui.h"

void gui::loadMapBounds() {
	maps::mapBounds["cs_italy"]        = mapData(-2647.0f, 2592.0f, 4.6f);
	maps::mapBounds["cs_office"]       = mapData(-1838.0f, 1858.0f, 4.1f);
	maps::mapBounds["de_ancient"]      = mapData(-2953.0f, 2164.0f, 5.0f);
	maps::mapBounds["de_anubis"]       = mapData(-2796.0f, 3328.0f, 5.22f);
	maps::mapBounds["de_dust2"]        = mapData(-2476.0f, 3239.0f, 4.4f);
	maps::mapBounds["de_inferno"]      = mapData(-2087.0f, 3870.0f, 4.9f);
	maps::mapBounds["de_mirage"]       = mapData(-3230.0f, 1713.0f, 5.0f);
	maps::mapBounds["de_nuke"]         = mapData(-3453.0f, 2887.0f, 7.0f);
	maps::mapBounds["de_overpass"]     = mapData(-4831.0f, 1781.0f, 5.2f);
	maps::mapBounds["de_vertigo"]      = mapData(-3168.0f, 1762.0f, 4.0f);
	maps::mapBounds["de_train"]        = mapData(-2308.0f, 2078.0f, 4.082077f);
	std::cout << "[Resources]: Loaded " << maps::mapBounds.size() << " map bounds\n";
}

void gui::loadTextures() {
	std::cout << "[Resources]: Loading textures...\n";
	int failed = 0;
	auto load = [&](ID3D11ShaderResourceView** dst, ID3D11Device* dev, const wchar_t* path) {
		*dst = LoadImageTexture(dev, path);
		if (!*dst) failed++;
	};
	auto loadDim = [&](ID3D11ShaderResourceView** dst, ID3D11Device* dev, const wchar_t* path, int* w, int* h) {
		*dst = LoadImageTexture(dev, path, w, h);
		if (!*dst) failed++;
	};

	// Maps
	load(&maps::mapTextures["cs_italy"],         g_pd3dDevice, L".\\textures\\maps\\cs_italy_radar.png");
	load(&maps::mapTextures["cs_office"],        g_pd3dDevice, L".\\textures\\maps\\cs_office_radar.png");
	load(&maps::mapTextures["de_ancient"],       g_pd3dDevice, L".\\textures\\maps\\de_ancient_radar.png");
	load(&maps::mapTextures["de_anubis"],        g_pd3dDevice, L".\\textures\\maps\\de_anubis_radar.png");
	load(&maps::mapTextures["de_dust2"],         g_pd3dDevice, L".\\textures\\maps\\de_dust2_radar.png");
	load(&maps::mapTextures["de_inferno"],       g_pd3dDevice, L".\\textures\\maps\\de_inferno_radar.png");
	load(&maps::mapTextures["de_mirage"],        g_pd3dDevice, L".\\textures\\maps\\de_mirage_radar.png");
	load(&maps::mapTextures["de_nuke_lower"],    g_pd3dDevice, L".\\textures\\maps\\de_nuke_lower_radar.png");
	load(&maps::mapTextures["de_nuke"],          g_pd3dDevice, L".\\textures\\maps\\de_nuke_radar.png");
	load(&maps::mapTextures["de_overpass"],      g_pd3dDevice, L".\\textures\\maps\\de_overpass_radar.png");
	load(&maps::mapTextures["de_vertigo_lower"], g_pd3dDevice, L".\\textures\\maps\\de_vertigo_lower_radar.png");
	load(&maps::mapTextures["de_vertigo"],       g_pd3dDevice, L".\\textures\\maps\\de_vertigo_radar.png");
	load(&maps::mapTextures["de_train"],         g_pd3dDevice, L".\\textures\\maps\\de_train_radar.png");

	// Weapon icons
	loadDim(&icons::iconTextures[1],   g_pd3dDevice, L".\\textures\\icons\\deagle.png",               &icons::iconWidths[1],   &icons::iconHeights[1]);
	loadDim(&icons::iconTextures[2],   g_pd3dDevice, L".\\textures\\icons\\elite.png",                &icons::iconWidths[2],   &icons::iconHeights[2]);
	loadDim(&icons::iconTextures[3],   g_pd3dDevice, L".\\textures\\icons\\fiveseven.png",            &icons::iconWidths[3],   &icons::iconHeights[3]);
	loadDim(&icons::iconTextures[4],   g_pd3dDevice, L".\\textures\\icons\\glock.png",                &icons::iconWidths[4],   &icons::iconHeights[4]);
	loadDim(&icons::iconTextures[7],   g_pd3dDevice, L".\\textures\\icons\\ak47.png",                 &icons::iconWidths[7],   &icons::iconHeights[7]);
	loadDim(&icons::iconTextures[8],   g_pd3dDevice, L".\\textures\\icons\\aug.png",                  &icons::iconWidths[8],   &icons::iconHeights[8]);
	loadDim(&icons::iconTextures[9],   g_pd3dDevice, L".\\textures\\icons\\awp.png",                  &icons::iconWidths[9],   &icons::iconHeights[9]);
	loadDim(&icons::iconTextures[10],  g_pd3dDevice, L".\\textures\\icons\\famas.png",                &icons::iconWidths[10],  &icons::iconHeights[10]);
	loadDim(&icons::iconTextures[11],  g_pd3dDevice, L".\\textures\\icons\\g3sg1.png",                &icons::iconWidths[11],  &icons::iconHeights[11]);
	loadDim(&icons::iconTextures[13],  g_pd3dDevice, L".\\textures\\icons\\galilar.png",              &icons::iconWidths[13],  &icons::iconHeights[13]);
	loadDim(&icons::iconTextures[14],  g_pd3dDevice, L".\\textures\\icons\\m249.png",                 &icons::iconWidths[14],  &icons::iconHeights[14]);
	loadDim(&icons::iconTextures[16],  g_pd3dDevice, L".\\textures\\icons\\m4a1.png",                 &icons::iconWidths[16],  &icons::iconHeights[16]);
	loadDim(&icons::iconTextures[17],  g_pd3dDevice, L".\\textures\\icons\\mac10.png",                &icons::iconWidths[17],  &icons::iconHeights[17]);
	loadDim(&icons::iconTextures[19],  g_pd3dDevice, L".\\textures\\icons\\p90.png",                  &icons::iconWidths[19],  &icons::iconHeights[19]);
	loadDim(&icons::iconTextures[23],  g_pd3dDevice, L".\\textures\\icons\\mp5sd.png",                &icons::iconWidths[23],  &icons::iconHeights[23]);
	loadDim(&icons::iconTextures[24],  g_pd3dDevice, L".\\textures\\icons\\ump45.png",                &icons::iconWidths[24],  &icons::iconHeights[24]);
	loadDim(&icons::iconTextures[25],  g_pd3dDevice, L".\\textures\\icons\\xm1014.png",               &icons::iconWidths[25],  &icons::iconHeights[25]);
	loadDim(&icons::iconTextures[26],  g_pd3dDevice, L".\\textures\\icons\\bizon.png",                &icons::iconWidths[26],  &icons::iconHeights[26]);
	loadDim(&icons::iconTextures[27],  g_pd3dDevice, L".\\textures\\icons\\mag7.png",                 &icons::iconWidths[27],  &icons::iconHeights[27]);
	loadDim(&icons::iconTextures[28],  g_pd3dDevice, L".\\textures\\icons\\negev.png",                &icons::iconWidths[28],  &icons::iconHeights[28]);
	loadDim(&icons::iconTextures[29],  g_pd3dDevice, L".\\textures\\icons\\sawedoff.png",             &icons::iconWidths[29],  &icons::iconHeights[29]);
	loadDim(&icons::iconTextures[30],  g_pd3dDevice, L".\\textures\\icons\\tec9.png",                 &icons::iconWidths[30],  &icons::iconHeights[30]);
	loadDim(&icons::iconTextures[31],  g_pd3dDevice, L".\\textures\\icons\\taser.png",                &icons::iconWidths[31],  &icons::iconHeights[31]);
	loadDim(&icons::iconTextures[32],  g_pd3dDevice, L".\\textures\\icons\\hkp2000.png",              &icons::iconWidths[32],  &icons::iconHeights[32]);
	loadDim(&icons::iconTextures[33],  g_pd3dDevice, L".\\textures\\icons\\mp7.png",                  &icons::iconWidths[33],  &icons::iconHeights[33]);
	loadDim(&icons::iconTextures[34],  g_pd3dDevice, L".\\textures\\icons\\mp9.png",                  &icons::iconWidths[34],  &icons::iconHeights[34]);
	loadDim(&icons::iconTextures[35],  g_pd3dDevice, L".\\textures\\icons\\nova.png",                 &icons::iconWidths[35],  &icons::iconHeights[35]);
	loadDim(&icons::iconTextures[36],  g_pd3dDevice, L".\\textures\\icons\\p250.png",                 &icons::iconWidths[36],  &icons::iconHeights[36]);
	loadDim(&icons::iconTextures[38],  g_pd3dDevice, L".\\textures\\icons\\scar20.png",               &icons::iconWidths[38],  &icons::iconHeights[38]);
	loadDim(&icons::iconTextures[39],  g_pd3dDevice, L".\\textures\\icons\\sg556.png",                &icons::iconWidths[39],  &icons::iconHeights[39]);
	loadDim(&icons::iconTextures[40],  g_pd3dDevice, L".\\textures\\icons\\ssg08.png",                &icons::iconWidths[40],  &icons::iconHeights[40]);
	loadDim(&icons::iconTextures[41],  g_pd3dDevice, L".\\textures\\icons\\knifegg.png",              &icons::iconWidths[41],  &icons::iconHeights[41]);
	loadDim(&icons::iconTextures[42],  g_pd3dDevice, L".\\textures\\icons\\knife.png",                &icons::iconWidths[42],  &icons::iconHeights[42]);
	loadDim(&icons::iconTextures[43],  g_pd3dDevice, L".\\textures\\icons\\flashbang.png",            &icons::iconWidths[43],  &icons::iconHeights[43]);
	loadDim(&icons::iconTextures[44],  g_pd3dDevice, L".\\textures\\icons\\hegrenade.png",            &icons::iconWidths[44],  &icons::iconHeights[44]);
	loadDim(&icons::iconTextures[45],  g_pd3dDevice, L".\\textures\\icons\\smokegrenade.png",         &icons::iconWidths[45],  &icons::iconHeights[45]);
	loadDim(&icons::iconTextures[46],  g_pd3dDevice, L".\\textures\\icons\\molotov.png",              &icons::iconWidths[46],  &icons::iconHeights[46]);
	loadDim(&icons::iconTextures[47],  g_pd3dDevice, L".\\textures\\icons\\decoy.png",                &icons::iconWidths[47],  &icons::iconHeights[47]);
	loadDim(&icons::iconTextures[48],  g_pd3dDevice, L".\\textures\\icons\\incgrenade.png",           &icons::iconWidths[48],  &icons::iconHeights[48]);
	loadDim(&icons::iconTextures[49],  g_pd3dDevice, L".\\textures\\icons\\c4.png",                   &icons::iconWidths[49],  &icons::iconHeights[49]);
	loadDim(&icons::iconTextures[55],  g_pd3dDevice, L".\\textures\\icons\\defuser.png",              &icons::iconWidths[55],  &icons::iconHeights[55]);
	loadDim(&icons::iconTextures[59],  g_pd3dDevice, L".\\textures\\icons\\knife_t.png",              &icons::iconWidths[59],  &icons::iconHeights[59]);
	loadDim(&icons::iconTextures[60],  g_pd3dDevice, L".\\textures\\icons\\m4a1_silencer.png",        &icons::iconWidths[60],  &icons::iconHeights[60]);
	loadDim(&icons::iconTextures[61],  g_pd3dDevice, L".\\textures\\icons\\usp_silencer.png",         &icons::iconWidths[61],  &icons::iconHeights[61]);
	loadDim(&icons::iconTextures[63],  g_pd3dDevice, L".\\textures\\icons\\cz75a.png",                &icons::iconWidths[63],  &icons::iconHeights[63]);
	loadDim(&icons::iconTextures[64],  g_pd3dDevice, L".\\textures\\icons\\revolver.png",             &icons::iconWidths[64],  &icons::iconHeights[64]);
	loadDim(&icons::iconTextures[500], g_pd3dDevice, L".\\textures\\icons\\bayonet.png",              &icons::iconWidths[500], &icons::iconHeights[500]);
	loadDim(&icons::iconTextures[503], g_pd3dDevice, L".\\textures\\icons\\knife_css.png",            &icons::iconWidths[503], &icons::iconHeights[503]);
	loadDim(&icons::iconTextures[505], g_pd3dDevice, L".\\textures\\icons\\knife_flip.png",           &icons::iconWidths[505], &icons::iconHeights[505]);
	loadDim(&icons::iconTextures[506], g_pd3dDevice, L".\\textures\\icons\\knife_gut.png",            &icons::iconWidths[506], &icons::iconHeights[506]);
	loadDim(&icons::iconTextures[507], g_pd3dDevice, L".\\textures\\icons\\knife_karambit.png",       &icons::iconWidths[507], &icons::iconHeights[507]);
	loadDim(&icons::iconTextures[508], g_pd3dDevice, L".\\textures\\icons\\knife_m9_bayonet.png",     &icons::iconWidths[508], &icons::iconHeights[508]);
	loadDim(&icons::iconTextures[509], g_pd3dDevice, L".\\textures\\icons\\knife_tactical.png",       &icons::iconWidths[509], &icons::iconHeights[509]);
	loadDim(&icons::iconTextures[512], g_pd3dDevice, L".\\textures\\icons\\knife_falchion.png",       &icons::iconWidths[512], &icons::iconHeights[512]);
	loadDim(&icons::iconTextures[514], g_pd3dDevice, L".\\textures\\icons\\knife_survival_bowie.png", &icons::iconWidths[514], &icons::iconHeights[514]);
	loadDim(&icons::iconTextures[515], g_pd3dDevice, L".\\textures\\icons\\knife_butterfly.png",      &icons::iconWidths[515], &icons::iconHeights[515]);
	loadDim(&icons::iconTextures[516], g_pd3dDevice, L".\\textures\\icons\\knife_push.png",           &icons::iconWidths[516], &icons::iconHeights[516]);
	loadDim(&icons::iconTextures[517], g_pd3dDevice, L".\\textures\\icons\\knife_cord.png",           &icons::iconWidths[517], &icons::iconHeights[517]);
	loadDim(&icons::iconTextures[518], g_pd3dDevice, L".\\textures\\icons\\knife_canis.png",          &icons::iconWidths[518], &icons::iconHeights[518]);
	loadDim(&icons::iconTextures[519], g_pd3dDevice, L".\\textures\\icons\\knife_ursus.png",          &icons::iconWidths[519], &icons::iconHeights[519]);
	loadDim(&icons::iconTextures[520], g_pd3dDevice, L".\\textures\\icons\\knife_gypsy_jackknife.png",&icons::iconWidths[520], &icons::iconHeights[520]);
	loadDim(&icons::iconTextures[521], g_pd3dDevice, L".\\textures\\icons\\knife_outdoor.png",        &icons::iconWidths[521], &icons::iconHeights[521]);
	loadDim(&icons::iconTextures[522], g_pd3dDevice, L".\\textures\\icons\\knife_stiletto.png",       &icons::iconWidths[522], &icons::iconHeights[522]);
	loadDim(&icons::iconTextures[523], g_pd3dDevice, L".\\textures\\icons\\knife_widowmaker.png",     &icons::iconWidths[523], &icons::iconHeights[523]);
	loadDim(&icons::iconTextures[525], g_pd3dDevice, L".\\textures\\icons\\knife_skeleton.png",       &icons::iconWidths[525], &icons::iconHeights[525]);
	loadDim(&icons::iconTextures[526], g_pd3dDevice, L".\\textures\\icons\\knife_kukri.png",          &icons::iconWidths[526], &icons::iconHeights[526]);

	int total = (int)(maps::mapTextures.size() + icons::iconTextures.size());
	std::cout << "[Resources]: Loaded " << (total - failed) << "/" << total << " textures";
	if (failed > 0) std::cout << " (" << failed << " failed — check textures/ folder)";
	std::cout << "\n";
}
