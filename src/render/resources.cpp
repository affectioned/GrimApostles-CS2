#include "pch.h"
#include "gui.h"

// Weapon icon filename stem -> CS2 weapon ID.
// Filenames are produced by tools/AssetExtractor and must match exactly.
static const std::unordered_map<std::string, int> kWeaponIDs = {
	{ "deagle",                1 },
	{ "elite",                 2 },
	{ "fiveseven",             3 },
	{ "glock",                 4 },
	{ "ak47",                  7 },
	{ "aug",                   8 },
	{ "awp",                   9 },
	{ "famas",                10 },
	{ "g3sg1",                11 },
	{ "galilar",              13 },
	{ "m249",                 14 },
	{ "m4a1",                 16 },
	{ "mac10",                17 },
	{ "p90",                  19 },
	{ "mp5sd",                23 },
	{ "ump45",                24 },
	{ "xm1014",               25 },
	{ "bizon",                26 },
	{ "mag7",                 27 },
	{ "negev",                28 },
	{ "sawedoff",             29 },
	{ "tec9",                 30 },
	{ "taser",                31 },
	{ "hkp2000",              32 },
	{ "mp7",                  33 },
	{ "mp9",                  34 },
	{ "nova",                 35 },
	{ "p250",                 36 },
	{ "scar20",               38 },
	{ "sg556",                39 },
	{ "ssg08",                40 },
	{ "knifegg",              41 },
	{ "knife",                42 },
	{ "flashbang",            43 },
	{ "hegrenade",            44 },
	{ "smokegrenade",         45 },
	{ "molotov",              46 },
	{ "decoy",                47 },
	{ "incgrenade",           48 },
	{ "c4",                   49 },
	{ "defuser",              55 },
	{ "knife_t",              59 },
	{ "m4a1_silencer",        60 },
	{ "usp_silencer",         61 },
	{ "cz75a",                63 },
	{ "revolver",             64 },
	{ "bayonet",             500 },
	{ "knife_css",           503 },
	{ "knife_flip",          505 },
	{ "knife_gut",           506 },
	{ "knife_karambit",      507 },
	{ "knife_m9_bayonet",    508 },
	{ "knife_tactical",      509 },
	{ "knife_falchion",      512 },
	{ "knife_survival_bowie",514 },
	{ "knife_butterfly",     515 },
	{ "knife_push",          516 },
	{ "knife_cord",          517 },
	{ "knife_canis",         518 },
	{ "knife_ursus",         519 },
	{ "knife_gypsy_jackknife",520 },
	{ "knife_outdoor",       521 },
	{ "knife_stiletto",      522 },
	{ "knife_widowmaker",    523 },
	{ "knife_skeleton",      525 },
	{ "knife_kukri",         526 },
};

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
	namespace fs = std::filesystem;
	int failed = 0;

	// Maps -- scan textures/maps/ for *_radar.png; strip the _radar suffix to get the map name.
	// Multi-level variants (de_nuke_lower, de_vertigo_lower) are loaded here and resolved in render.cpp.
	const fs::path mapDir = L".\\textures\\maps";
	try {
		for (const auto& entry : fs::directory_iterator(mapDir)) {
			if (entry.path().extension() != L".png") continue;
			std::string stem = entry.path().stem().string();
			const std::string suffix = "_radar";
			if (stem.size() <= suffix.size() || stem.compare(stem.size() - suffix.size(), suffix.size(), suffix) != 0) continue;
			std::string mapName = stem.substr(0, stem.size() - suffix.size());
			maps::mapTextures[mapName] = LoadImageTexture(g_pd3dDevice, entry.path().c_str());
			if (!maps::mapTextures[mapName]) failed++;
		}
	} catch (const fs::filesystem_error& e) {
		std::cerr << "[Resources]: Could not scan maps folder: " << e.what() << "\n";
	}

	// Weapon icons -- scan textures/icons/ and load any file whose name is in kWeaponIDs.
	// Icons are produced by tools/AssetExtractor; unknown filenames are silently ignored.
	const fs::path iconDir = L".\\textures\\icons";
	int iconLoaded = 0, iconFailed = 0;
	try {
		for (const auto& entry : fs::directory_iterator(iconDir)) {
			if (entry.path().extension() != L".png") continue;
			std::string stem = entry.path().stem().string();
			auto it = kWeaponIDs.find(stem);
			if (it == kWeaponIDs.end()) continue;
			int id = it->second;
			icons::iconTextures[id] = LoadImageTexture(
				g_pd3dDevice, entry.path().c_str(),
				&icons::iconWidths[id], &icons::iconHeights[id]);
			if (icons::iconTextures[id]) iconLoaded++;
			else                         iconFailed++;
		}
	} catch (const fs::filesystem_error& e) {
		std::cerr << "[Resources]: Could not scan icons folder: " << e.what() << "\n";
	}

	failed += iconFailed;
	int total = (int)maps::mapTextures.size() + iconLoaded + iconFailed;
	std::cout << "[Resources]: Loaded " << (total - failed) << "/" << total << " textures";
	if (failed > 0) std::cout << " (" << failed << " failed — check textures/ folder)";
	std::cout << "\n";
}
