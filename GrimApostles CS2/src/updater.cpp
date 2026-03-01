#include "pch.h"
#include "updater.h"
#include "offsets.h"

#include <wininet.h>
#pragma comment(lib, "wininet")

static const wchar_t* OFFSETS_URL     = L"https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/offsets.hpp";
static const wchar_t* CLIENT_DLL_URL  = L"https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.hpp";

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

// ─── Line parser ─────────────────────────────────────────────────────────────
// Matches: constexpr std::ptrdiff_t NAME = 0xVALUE;

static bool parseLine(const std::string& line, std::string& name, std::ptrdiff_t& value) {
	const char* prefix = "constexpr std::ptrdiff_t ";
	auto pos = line.find(prefix);
	if (pos == std::string::npos) return false;
	pos += strlen(prefix);

	auto eq = line.find('=', pos);
	if (eq == std::string::npos) return false;

	name = line.substr(pos, eq - pos);
	name.erase(0, name.find_first_not_of(" \t"));
	name.erase(name.find_last_not_of(" \t") + 1);

	auto hex = line.find("0x", eq);
	if (hex == std::string::npos) return false;
	auto end = line.find(';', hex);
	if (end == std::string::npos) return false;

	try {
		value = static_cast<std::ptrdiff_t>(std::stoull(line.substr(hex + 2, end - hex - 2), nullptr, 16));
		return true;
	}
	catch (...) {
		return false;
	}
}

// Returns true if the first non-whitespace character on the line is '}'
static bool isClosingBrace(const std::string& line) {
	auto first = line.find_first_not_of(" \t");
	return first != std::string::npos && line[first] == '}';
}

// ─── fetchOffsets ─────────────────────────────────────────────────────────────
// Parses cs2_dumper::offsets::client_dll and cs2_dumper::offsets::matchmaking_dll

bool updater::fetchOffsets() {
	std::cout << "[Updater]: Fetching offsets.hpp..." << std::endl;

	std::string content = fetchURL(OFFSETS_URL);
	if (content.empty()) {
		std::cout << "[Updater]: Failed — using defaults for dw* offsets." << std::endl;
		return false;
	}

	enum class NS { None, ClientDLL, MatchmakingDLL };
	NS current = NS::None;
	int updated = 0;

	std::istringstream ss(content);
	std::string line;

	while (std::getline(ss, line)) {
		if      (line.find("namespace client_dll")     != std::string::npos) { current = NS::ClientDLL;     continue; }
		else if (line.find("namespace matchmaking_dll") != std::string::npos) { current = NS::MatchmakingDLL; continue; }

		if (current == NS::None) continue;
		if (isClosingBrace(line)) { current = NS::None; continue; }

		std::string name;
		std::ptrdiff_t value;
		if (!parseLine(line, name, value)) continue;

		if (current == NS::ClientDLL) {
			if      (name == "dwCSGOInput")                           { client_dll::dwCSGOInput = value; }
			else if (name == "dwEntityList")                          { client_dll::dwEntityList = value; }
			else if (name == "dwGameEntitySystem")                    { client_dll::dwGameEntitySystem = value; }
			else if (name == "dwGameEntitySystem_highestEntityIndex") { client_dll::dwGameEntitySystem_highestEntityIndex = value; }
			else if (name == "dwGameRules")                           { client_dll::dwGameRules = value; }
			else if (name == "dwGlobalVars")                          { client_dll::dwGlobalVars = value; }
			else if (name == "dwGlowManager")                         { client_dll::dwGlowManager = value; }
			else if (name == "dwLocalPlayerController")               { client_dll::dwLocalPlayerController = value; }
			else if (name == "dwLocalPlayerPawn")                     { client_dll::dwLocalPlayerPawn = value; }
			else if (name == "dwPlantedC4")                           { client_dll::dwPlantedC4 = value; }
			else if (name == "dwPrediction")                          { client_dll::dwPrediction = value; }
			else if (name == "dwSensitivity")                         { client_dll::dwSensitivity = value; }
			else if (name == "dwSensitivity_sensitivity")             { client_dll::dwSensitivity_sensitivity = value; }
			else if (name == "dwViewAngles")                          { client_dll::dwViewAngles = value; }
			else if (name == "dwViewMatrix")                          { client_dll::dwViewMatrix = value; }
			else if (name == "dwViewRender")                          { client_dll::dwViewRender = value; }
			else if (name == "dwWeaponC4")                            { client_dll::dwWeaponC4 = value; }
			else { continue; }
			updated++;
		}
		else if (current == NS::MatchmakingDLL) {
			if (name == "dwGameTypes") { matchmaking_dll::dwGameTypes = value; updated++; }
		}
	}

	std::cout << "[Updater]: " << updated << "/18 dw* offsets updated." << std::endl;
	return updated > 0;
}

// ─── fetchClassOffsets ────────────────────────────────────────────────────────
// Parses cs2_dumper::schemas::client_dll::<ClassName> for member offsets

bool updater::fetchClassOffsets() {
	std::cout << "[Updater]: Fetching client_dll.hpp..." << std::endl;

	std::string content = fetchURL(CLIENT_DLL_URL);
	if (content.empty()) {
		std::cout << "[Updater]: Failed — using defaults for class offsets." << std::endl;
		return false;
	}

	enum class Class {
		None,
		C_BaseEntity,
		C_BasePlayerPawn,
		CCSPlayerController,
		C_CSPlayerPawn,
		C_EconEntity,
		C_AttributeContainer,
		C_EconItemView
	};
	Class current = Class::None;
	int updated = 0;

	std::istringstream ss(content);
	std::string line;

	while (std::getline(ss, line)) {
		// Detect class namespace entry — order matters for subclasses sharing name fragments
		if      (line.find("namespace CCSPlayerController") != std::string::npos) { current = Class::CCSPlayerController; continue; }
		else if (line.find("namespace C_CSPlayerPawn")      != std::string::npos) { current = Class::C_CSPlayerPawn;      continue; }
		else if (line.find("namespace C_BasePlayerPawn")    != std::string::npos) { current = Class::C_BasePlayerPawn;    continue; }
		else if (line.find("namespace C_BaseEntity")        != std::string::npos) { current = Class::C_BaseEntity;        continue; }
		else if (line.find("namespace C_EconEntity")        != std::string::npos) { current = Class::C_EconEntity;        continue; }
		else if (line.find("namespace C_AttributeContainer") != std::string::npos){ current = Class::C_AttributeContainer; continue; }
		else if (line.find("namespace C_EconItemView")      != std::string::npos) { current = Class::C_EconItemView;      continue; }

		if (current == Class::None) continue;
		if (isClosingBrace(line)) { current = Class::None; continue; }

		std::string name;
		std::ptrdiff_t value;
		if (!parseLine(line, name, value)) continue;

		switch (current) {
		case Class::C_BaseEntity:
			if      (name == "m_iTeamNum") { client_dll::C_BaseEntity::m_iTeamNum = value; updated++; }
			else if (name == "m_iHealth")  { client_dll::C_BaseEntity::m_iHealth  = value; updated++; }
			break;
		case Class::C_BasePlayerPawn:
			if (name == "m_vOldOrigin") { client_dll::C_BasePlayerPawn::m_vOldOrigin = value; updated++; }
			break;
		case Class::CCSPlayerController:
			if      (name == "m_hPlayerPawn")          { client_dll::CCSPlayerController::m_hPlayerPawn          = value; updated++; }
			else if (name == "m_sSanitizedPlayerName") { client_dll::CCSPlayerController::m_sSanitizedPlayerName = value; updated++; }
			else if (name == "m_iCompTeammateColor")   { client_dll::CCSPlayerController::m_iCompTeammateColor   = value; updated++; }
			break;
		case Class::C_CSPlayerPawn:
			if      (name == "m_angEyeAngles")    { client_dll::C_CSPlayerPawn::m_angEyeAngles    = value; updated++; }
			else if (name == "m_pClippingWeapon") { client_dll::C_CSPlayerPawn::m_pClippingWeapon = value; updated++; }
			break;
		case Class::C_EconEntity:
			if (name == "m_AttributeManager") { client_dll::C_EconEntity::m_AttributeManager = value; updated++; }
			break;
		case Class::C_AttributeContainer:
			if (name == "m_Item") { client_dll::C_AttributeContainer::m_Item = value; updated++; }
			break;
		case Class::C_EconItemView:
			if (name == "m_iItemDefinitionIndex") { client_dll::C_EconItemView::m_iItemDefinitionIndex = value; updated++; }
			break;
		default:
			break;
		}
	}

	std::cout << "[Updater]: " << updated << "/11 class offsets updated." << std::endl;
	return updated > 0;
}
