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

static float parseKVFloat(const std::string& content, const char* key, size_t from = 0) {
	std::string search = std::string("\"") + key + "\"";
	auto pos = content.find(search, from);
	if (pos == std::string::npos) return 0.0f;
	pos = content.find('"', pos + search.size());
	if (pos == std::string::npos) return 0.0f;
	++pos;
	auto end = content.find('"', pos);
	if (end == std::string::npos) return 0.0f;
	try { return std::stof(content.substr(pos, end - pos)); }
	catch (...) { return 0.0f; }
}

// Returns the Z threshold below which the _lower radar variant should be shown,
// or infinity if the map has no vertical sections.
static float parseLowerZThreshold(const std::string& content) {
	auto vsPos = content.find("\"verticalsections\"");
	if (vsPos == std::string::npos) return std::numeric_limits<float>::infinity();
	auto defPos = content.find("\"default\"", vsPos);
	if (defPos == std::string::npos) return std::numeric_limits<float>::infinity();
	float v = parseKVFloat(content, "AltitudeMin", defPos);
	return (v == 0.0f) ? std::numeric_limits<float>::infinity() : v;
}

void gui::loadMapBounds() {
	namespace fs = std::filesystem;
	const fs::path mapDir = L".\\textures\\maps";
	int count = 0;
	try {
		for (const auto& entry : fs::directory_iterator(mapDir)) {
			if (entry.path().extension() != L".txt") continue;
			std::string stem = entry.path().stem().string();
			const std::string suffix = "_radar";
			if (stem.size() <= suffix.size() ||
			    stem.compare(stem.size() - suffix.size(), suffix.size(), suffix) != 0) continue;
			std::string mapName = stem.substr(0, stem.size() - suffix.size());

			std::ifstream f(entry.path());
			if (!f) continue;
			std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

			float posX   = parseKVFloat(content, "pos_x");
			float posY   = parseKVFloat(content, "pos_y");
			float scale  = parseKVFloat(content, "scale");
			if (scale == 0.0f) continue;

			float zThresh = parseLowerZThreshold(content);
			maps::mapBounds[mapName] = mapData(posX, posY, scale, zThresh);
			count++;
		}
	} catch (const fs::filesystem_error& e) {
		std::cerr << "[Resources]: Could not scan maps folder: " << e.what() << "\n";
	}

	// Lower variants: same bounds as the base map but no further sub-level switching
	for (auto& [name, data] : maps::mapBounds) {
		if (data.lowerZThreshold == std::numeric_limits<float>::infinity()) continue;
		std::string lowerName = name + "_lower";
		if (!maps::mapBounds.count(lowerName)) {
			mapData lower = data;
			lower.lowerZThreshold = std::numeric_limits<float>::infinity();
			maps::mapBounds[lowerName] = lower;
		}
	}

	std::cout << "[Resources]: Loaded " << count << " map bounds\n";
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
