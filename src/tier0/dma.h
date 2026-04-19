#pragma once
#include <Windows.h>
#include "vmmdll.h"
#include "leechcore.h"
#include <iostream>
#include <string.h>
#include <conio.h>
#include <cstdint>
#pragma comment(lib,"leechcore")
#pragma comment(lib,"vmm")

class DMADevice {
public:
	static constexpr const char* kProcess = "cs2.exe";
	static constexpr const char* kModule  = "client.dll";

	bool     bConnected          = false;
	DWORD    dwAttachedProcessId = 0;
	uint64_t moduleBase          = 0;
	VMM_HANDLE            hVMM     = nullptr;
	VMMDLL_SCATTER_HANDLE hScatter = nullptr;

	bool     Connect();
	void     Disconnect();
	bool     AttachToProcessId();
	void     ShowKeyPress();
	uint64_t getModuleBase(const char* moduleName);
	bool     Clear();
	bool     ExecuteRead();

	template<typename U, typename P>
	DWORD MemRead(U lpAddress, P lpOutput, size_t uiSize, bool bFullReadRequired = true) {
		if (!dwAttachedProcessId || !bConnected || !lpAddress || !hVMM) return 0;
		DWORD dwBytesRead = 0;
		BOOL bRetn = (VMMDLL_MemReadEx(hVMM, dwAttachedProcessId, (ULONG64)lpAddress, (PBYTE)lpOutput, uiSize, &dwBytesRead,
			VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_ZEROPAD_ON_FAIL | VMMDLL_FLAG_NOPAGING_IO) && dwBytesRead != 0);
		if (!bRetn || (bFullReadRequired && dwBytesRead != uiSize)) return 0;
		return dwBytesRead;
	}

	template<typename U, typename P>
	BOOL PrepareEX(U vAddress, P pOutput, size_t uiSize) {
		if (!hScatter || !vAddress || !uiSize) return false;
		return VMMDLL_Scatter_PrepareEx(hScatter, vAddress, uiSize, (PBYTE)pOutput, NULL);
	}
};

extern DMADevice g_DMA;

// Valid x64 user-space pointer range — rejects nulls, kernel addresses, and DMA zero-pads.
inline bool isValidPtr(uint64_t p) {
	return p > 0x10000ULL && p < 0x7FFFFFFFFFFF0000ULL;
}
