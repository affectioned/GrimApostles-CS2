#pragma once

namespace GameModules
{
	inline constexpr const char* ProcessName = "cs2.exe";
	inline constexpr const char* ClientDll   = "client.dll";

	inline std::vector<std::string> ModuleList()
	{
		return { ProcessName, ClientDll };
	}
}
