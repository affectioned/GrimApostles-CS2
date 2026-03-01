#include "pch.h"
#include "updater.h"
#include "offsets.h"

#include <wininet.h>
#include <regex>
#pragma comment(lib, "wininet")

static const wchar_t* OFFSETS_URL    = L"https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/offsets.hpp";
static const wchar_t* CLIENT_DLL_URL = L"https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.hpp";

// ─── HTTP fetch ───────────────────────────────────────────────────────────────

static std::string fetchURL(const wchar_t* url) {
	std::string result;

	HINTERNET hInternet = InternetOpenW(L"GrimApostles", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	if (!hInternet) return result;

	HINTERNET hUrl = InternetOpenUrlW(hInternet, url, nullptr, 0,
		INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (hUrl) {
		char buf[4096];
		DWORD bytesRead = 0;
		while (InternetReadFile(hUrl, buf, sizeof(buf), &bytesRead) && bytesRead > 0)
			result.append(buf, bytesRead);
		InternetCloseHandle(hUrl);
	}

	InternetCloseHandle(hInternet);
	return result;
}

// ─── Regex helpers ────────────────────────────────────────────────────────────

static std::ptrdiff_t getOffset(const std::string& content, const std::string& name) {
	std::regex rx(name + R"(\s*=\s*0x([0-9A-Fa-f]+))");
	std::smatch m;
	if (std::regex_search(content, m, rx))
		return static_cast<std::ptrdiff_t>(std::stoull(m[1].str(), nullptr, 16));
	return 0;
}

static std::string getClassBlock(const std::string& content, const std::string& ns) {
	std::regex rx("namespace " + ns + R"(\s*\{([^}]*)\})");
	std::smatch m;
	if (std::regex_search(content, m, rx)) return m[1].str();
	return {};
}

// ─── fetchOffsets ─────────────────────────────────────────────────────────────

bool updater::fetchOffsets() {
	std::cout << "[Updater]: Fetching offsets.hpp..." << std::endl;

	std::string content = fetchURL(OFFSETS_URL);
	if (content.empty()) {
		std::cout << "[Updater]: Failed — using defaults for dw* offsets." << std::endl;
		return false;
	}

	int updated = 0;
	auto assign = [&](std::ptrdiff_t& target, const std::string& name) {
		if (auto v = getOffset(content, name)) { target = v; updated++; }
	};

	assign(client_dll::dwEntityList,            "dwEntityList");
	assign(client_dll::dwLocalPlayerController, "dwLocalPlayerController");
	assign(client_dll::dwLocalPlayerPawn,       "dwLocalPlayerPawn");
	assign(matchmaking_dll::dwGameTypes,        "dwGameTypes");

	std::cout << "[Updater]: " << updated << "/4 dw* offsets updated." << std::endl;
	return updated > 0;
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
	auto assign = [&](std::ptrdiff_t& target, const std::string& block, const std::string& name) {
		if (auto v = getOffset(block, name)) { target = v; updated++; }
	};

	std::string baseEntity     = getClassBlock(content, "C_BaseEntity");
	std::string basePlayerPawn = getClassBlock(content, "C_BasePlayerPawn");
	std::string playerCtrl     = getClassBlock(content, "CCSPlayerController");
	std::string csPlayerPawn   = getClassBlock(content, "C_CSPlayerPawn");
	std::string econEntity     = getClassBlock(content, "C_EconEntity");
	std::string attrContainer  = getClassBlock(content, "C_AttributeContainer");
	std::string econItemView   = getClassBlock(content, "C_EconItemView");

	assign(client_dll::C_BaseEntity::m_iTeamNum,                   baseEntity,     "m_iTeamNum");
	assign(client_dll::C_BaseEntity::m_iHealth,                    baseEntity,     "m_iHealth");
	assign(client_dll::C_BasePlayerPawn::m_vOldOrigin,             basePlayerPawn, "m_vOldOrigin");
	assign(client_dll::CCSPlayerController::m_hPlayerPawn,         playerCtrl,     "m_hPlayerPawn");
	assign(client_dll::CCSPlayerController::m_sSanitizedPlayerName,playerCtrl,     "m_sSanitizedPlayerName");
	assign(client_dll::CCSPlayerController::m_iCompTeammateColor,  playerCtrl,     "m_iCompTeammateColor");
	assign(client_dll::C_CSPlayerPawn::m_angEyeAngles,             csPlayerPawn,   "m_angEyeAngles");
	assign(client_dll::C_CSPlayerPawn::m_pClippingWeapon,          csPlayerPawn,   "m_pClippingWeapon");
	assign(client_dll::C_EconEntity::m_AttributeManager,           econEntity,     "m_AttributeManager");
	assign(client_dll::C_AttributeContainer::m_Item,               attrContainer,  "m_Item");
	assign(client_dll::C_EconItemView::m_iItemDefinitionIndex,     econItemView,   "m_iItemDefinitionIndex");

	std::cout << "[Updater]: " << updated << "/11 class offsets updated." << std::endl;
	return updated > 0;
}
