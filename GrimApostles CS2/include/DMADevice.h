#pragma once
#include <Windows.h>
#include "vmm/vmmdll.h"
#include "vmm/leechcore.h"
#include <iostream>
#include <string.h>
#include <cstdio>
#include <conio.h>
#include <cstdint>
#pragma comment(lib,"leechcore")
#pragma comment(lib,"vmm")

#define PROCESS  _strdup("cs2.exe")
#define MODULE	 _strdup("client.dll")


namespace DMADevice
{
	extern bool bConnected;
	extern DWORD dwAttachedProcessId;
	extern uint64_t moduleBase;
	extern VMM_HANDLE hVMM;
	extern VMMDLL_SCATTER_HANDLE hScatter;

	//Primary functionality
	bool Connect();
	void Disconnect();
	bool AttachToProcessId();
	void ShowKeyPress();
	uint64_t getModuleBase(LPSTR);
	//Scatter functions
	bool Clear(VMMDLL_SCATTER_HANDLE);
	bool ExecuteRead(VMMDLL_SCATTER_HANDLE);

	template<typename U, typename P>DWORD MemRead(U lpAddress, P lpOutput, size_t uiSize, bool bFullReadRequired = true)
	{
		if (!dwAttachedProcessId || !bConnected || !lpAddress || !hVMM) {
			return 0;
		}
		DWORD dwBytesRead = 0;
		BOOL bRetn = (VMMDLL_MemReadEx(hVMM, dwAttachedProcessId, (ULONG64)lpAddress, (PBYTE)lpOutput, uiSize, &dwBytesRead, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_ZEROPAD_ON_FAIL | VMMDLL_FLAG_NOPAGING_IO) && dwBytesRead != 0);
		if (!bRetn || (bFullReadRequired && dwBytesRead != uiSize)) {
			return dwBytesRead;
		}
		return dwBytesRead;
	}

	template<typename Var, typename U>Var MemReadPtr(U lpAddress)
	{
		Var lpPtr = 0;
		if (MemRead(lpAddress, &lpPtr, sizeof(Var))) {
			return lpPtr;
		}
		return 0;
	}

	// Add a entry to the scatter handle for a address to be read and specify the output buffer for the data to be stored
	template<typename U, typename P>BOOL PrepareEX(VMMDLL_SCATTER_HANDLE hSCATTER, U vAddress, P pOutput, size_t uiSize)
	{
		if (!hSCATTER || !vAddress || !uiSize) {
			return false;
		}

		return VMMDLL_Scatter_PrepareEx(hSCATTER, vAddress, uiSize, (PBYTE)pOutput, NULL);
	}


}
