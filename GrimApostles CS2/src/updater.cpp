#include "pch.h"
#include "updater.h"
#include "offsets.h"
#include "SigScan.h"

#include <wininet.h>
#include <regex>
#pragma comment(lib, "wininet")

static const wchar_t* CLIENT_DLL_URL = L"https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.hpp";

static std::string fetchURL(const wchar_t* url) {
	std::string result;
	HINTERNET hNet = InternetOpenW(L"GrimApostles", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	if (!hNet) return result;
	HINTERNET hUrl = InternetOpenUrlW(hNet, url, nullptr, 0,
		INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (hUrl) {
		char buf[4096]; DWORD n = 0;
		while (InternetReadFile(hUrl, buf, sizeof(buf), &n) && n > 0)
			result.append(buf, n);
		InternetCloseHandle(hUrl);
	}
	InternetCloseHandle(hNet);
	return result;
}

using OffsetMap = std::unordered_map<std::string, std::ptrdiff_t>;

static OffsetMap parseOffsets(const std::string& content) {
	static const std::regex rx(R"((\w+)\s*=\s*0x([0-9A-Fa-f]+))");
	OffsetMap map;
	for (auto it = std::sregex_iterator(content.begin(), content.end(), rx); it != std::sregex_iterator(); ++it)
		map[(*it)[1].str()] = (std::ptrdiff_t)std::stoull((*it)[2].str(), nullptr, 16);
	return map;
}

static std::string getClassBlock(const std::string& content, const char* ns) {
	std::smatch m;
	std::regex rx(std::string("namespace ") + ns + R"(\s*\{([^}]*)\})");
	return std::regex_search(content, m, rx) ? m[1].str() : std::string{};
}

// ─── fetchClassOffsets ────────────────────────────────────────────────────────

bool updater::fetchClassOffsets() {
	std::cout << "[Updater]: Fetching client_dll.hpp..." << std::endl;
	std::string content = fetchURL(CLIENT_DLL_URL);
	if (content.empty()) {
		std::cout << "[Updater]: Failed — using defaults for class offsets." << std::endl;
		return false;
	}

	int updated = 0;
	std::unordered_map<std::string, OffsetMap> cache;
	auto assign = [&](std::ptrdiff_t& t, const char* cls, const char* field) {
		auto& m = cache[cls];
		if (m.empty()) m = parseOffsets(getClassBlock(content, cls));
		if (auto it = m.find(field); it != m.end()) { t = it->second; updated++; }
	};

	assign(client_dll::C_BaseEntity::m_iTeamNum,                    "C_BaseEntity",             "m_iTeamNum");
	assign(client_dll::C_BaseEntity::m_iHealth,                     "C_BaseEntity",             "m_iHealth");
	assign(client_dll::C_BasePlayerPawn::m_vOldOrigin,              "C_BasePlayerPawn",         "m_vOldOrigin");
	assign(client_dll::C_BasePlayerPawn::m_pWeaponServices,         "C_BasePlayerPawn",         "m_pWeaponServices");
	assign(client_dll::CCSPlayerController::m_hPlayerPawn,          "CCSPlayerController",      "m_hPlayerPawn");
	assign(client_dll::CCSPlayerController::m_sSanitizedPlayerName, "CCSPlayerController",      "m_sSanitizedPlayerName");
	assign(client_dll::CCSPlayerController::m_iCompTeammateColor,   "CCSPlayerController",      "m_iCompTeammateColor");
	assign(client_dll::C_CSPlayerPawn::m_angEyeAngles,              "C_CSPlayerPawn",           "m_angEyeAngles");
	assign(client_dll::C_CSPlayerPawn::m_pClippingWeapon,           "C_CSPlayerPawn",           "m_pClippingWeapon");
	assign(client_dll::C_EconEntity::m_AttributeManager,            "C_EconEntity",             "m_AttributeManager");
	assign(client_dll::C_AttributeContainer::m_Item,                "C_AttributeContainer",     "m_Item");
	assign(client_dll::C_EconItemView::m_iItemDefinitionIndex,      "C_EconItemView",           "m_iItemDefinitionIndex");
	assign(client_dll::CPlayer_WeaponServices::m_hMyWeapons,        "CPlayer_WeaponServices",   "m_hMyWeapons");

	std::cout << "[Updater]: " << updated << "/13 class offsets updated." << std::endl;
	return updated > 0;
}

// ─── sigscanOffsets ───────────────────────────────────────────────────────────

bool updater::sigscanOffsets() {
	std::cout << "[Updater]: Running signature scan for dw* offsets..." << std::endl;

	int updated = 0;
	auto set  = [&](std::ptrdiff_t& t, std::ptrdiff_t v, const char* name) {
		t = v; updated++;
		std::cout << "[Updater]: " << name << " = 0x" << std::hex << v << std::dec << "\n";
	};
	auto scan = [&](std::ptrdiff_t& t, const char* mod, const char* sig, const char* name) {
		if (auto r = FindRIPOffset(mod, sig, 3)) set(t, r, name);
		else std::cout << "[Updater]: " << name << " — sig not found\n";
	};

	scan(client_dll::dwEntityList,            "client.dll",      "48 89 0D ?? ?? ?? ?? E9 ?? ?? ?? ?? CC", "dwEntityList");
	scan(client_dll::dwLocalPlayerController, "client.dll",      "48 8B 05 ?? ?? ?? ?? 41 89 BE",          "dwLocalPlayerController");
	scan(client_dll::dwGlobalVars,            "client.dll",      "48 89 15 ?? ?? ?? ?? 48 89 42",          "dwGlobalVars");
	scan(matchmaking_dll::dwGameTypes,        "matchmaking.dll", "48 8D 0D ?? ?? ?? ?? FF 90",             "dwGameTypes");

	// dwLocalPlayerPawn: rva of dwPrediction global + struct member offset from inner u4 scan
	if (auto rva = FindRIPOffset("client.dll", "48 8D 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 40 53 56 41 54", 3))
		if (auto off = FindU4InModule("client.dll", "4C 39 B6 ?? ?? ?? ?? 74 ?? 44 88 BE", 3))
			set(client_dll::dwLocalPlayerPawn, rva + (std::ptrdiff_t)off, "dwLocalPlayerPawn");

	std::cout << "[Updater]: " << updated << "/5 offsets resolved via sigscan." << std::endl;
	return updated > 0;
}
