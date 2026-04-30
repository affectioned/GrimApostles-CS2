#pragma once

namespace GameModules
{
	inline constexpr const char* ProcessName = "cs2.exe";
	inline constexpr const char* ClientDll   = "client.dll";
	inline constexpr const char* Engine2Dll  = "engine2.dll";

	inline std::vector<std::string> ModuleList()
	{
		return { ProcessName, ClientDll, Engine2Dll };
	}
}
