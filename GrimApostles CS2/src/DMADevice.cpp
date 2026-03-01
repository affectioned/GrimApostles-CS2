#include "pch.h"
#include "DMADevice.h"

using namespace std;

bool DMADevice::bConnected = false;
DWORD DMADevice::dwAttachedProcessId = NULL;
VMM_HANDLE DMADevice::hVMM = NULL;
uint64_t DMADevice::moduleBase = NULL;
VMMDLL_SCATTER_HANDLE DMADevice::hScatter = NULL;

void DMADevice::ShowKeyPress() {
	cout << "\nPress any key to exit loader\n";
	Sleep(250);
	_getch();
}

bool DMADevice::Connect() {
	//Making sure we aren't already connected
	if (bConnected) return true;
	bool bReturnStatus = false;
	unsigned int iArgumentCount = 3;
	bool result = false;

	LPSTR args[] = { _strdup(""), _strdup("-device"), _strdup("fpga://algo =0") };


	/*Initializing VMM and checking*/
	hVMM = VMMDLL_Initialize(iArgumentCount, args);
	hScatter = VMMDLL_Scatter_Initialize(hVMM, dwAttachedProcessId, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_ZEROPAD_ON_FAIL | VMMDLL_FLAG_NOPAGING_IO);

	if (hVMM) {
		cout << "[DMA]: DMA succesfully initialized\n";
	}
	else {
		cout << "[DMA]: DMA failed to initialize\n";
		bConnected = false;
		return false;
	}
	//Getting the config
	ULONG64 qwID, qwVersionMajor, qwVersionMinor;
	result =
		VMMDLL_ConfigGet(hVMM, LC_OPT_FPGA_FPGA_ID, &qwID) &&
		VMMDLL_ConfigGet(hVMM, LC_OPT_FPGA_VERSION_MAJOR, &qwVersionMajor) &&
		VMMDLL_ConfigGet(hVMM, LC_OPT_FPGA_VERSION_MINOR, &qwVersionMinor);

	if (result) {

		//printf("[DMA]: DMA configuration found\n");
		//printf("         ID = %lli\n", qwID);
		//printf("         VERSION = %lli.%lli\n", qwVersionMajor, qwVersionMinor);

	}
	else {
		printf("[DMA]: Failed to find DMA Config\n");
		bConnected = false;
		VMMDLL_Close(hVMM);
		VMMDLL_Scatter_CloseHandle(hScatter);
		return false;
	}

	//Clearing master aborts
	if ((qwVersionMajor >= 4) && ((qwVersionMajor >= 5) || (qwVersionMinor >= 7)))
	{
		HANDLE leechCoreHandle;
		LC_CONFIG LcConfig = {};
		LcConfig.dwVersion = LC_CONFIG_VERSION;
		strcpy_s(LcConfig.szDevice, "existing");

		// fetch already existing leechcore handle.
		leechCoreHandle = LcCreate(&LcConfig);
		//Bytes for our command
		BYTE bytes[4] = { 0x10, 0x00, 0x10, 0x00 };

		if (leechCoreHandle) {
			// enable auto-clear of status register [master abort].
			LcCommand(leechCoreHandle, LC_CMD_FPGA_CFGREGPCIE_MARKWR | 0x002, 4, bytes, NULL, NULL);
			//printf("[DMA]: AUTO ABORT CLEARING ON\n");
			// close leechcore handle.
			LcClose(leechCoreHandle);
		}
	}
	//Set our connection status and return
	bConnected = true;
	return bReturnStatus;
}

void DMADevice::Disconnect() {
	bConnected = false;
	//close our VMM handle
	VMMDLL_Close(hVMM);
	VMMDLL_Scatter_CloseHandle(hScatter);
	hVMM = NULL;
	hScatter = NULL;
	dwAttachedProcessId = NULL;
	moduleBase = NULL;
}

bool DMADevice::AttachToProcessId() {
	bool result = false;
	if (bConnected == false) {
		return false;
	}
	//we grab a process id from the name some games use 2 process id's or processes like pubg it runs 2 of the same exe names so you have to filter them
	if (!(VMMDLL_PidGetFromName(hVMM, PROCESS, &dwAttachedProcessId))) {
		cout << "[DMA]: Failed to find PID\n";
		return false;
	}
	VMMDLL_PROCESS_INFORMATION ProcessInformation;
	SIZE_T cbProcessInformation = sizeof(VMMDLL_PROCESS_INFORMATION);
	ZeroMemory(&ProcessInformation, sizeof(VMMDLL_PROCESS_INFORMATION));
	ProcessInformation.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
	ProcessInformation.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
	result = VMMDLL_ProcessGetInformation(hVMM, dwAttachedProcessId, &ProcessInformation, &cbProcessInformation);
	if (result) {
		return true;
	}
	else {
		printf("[DMA]: Failed to get process information\n");
		return false;
	}
}

uint64_t DMADevice::getModuleBase(LPSTR moduleName) {
	return VMMDLL_ProcessGetModuleBaseU(hVMM, dwAttachedProcessId, moduleName);
}

//SCATTER FUNCTIONALITY
bool DMADevice::Clear(VMMDLL_SCATTER_HANDLE hSCATTER){
	return VMMDLL_Scatter_Clear(hSCATTER, dwAttachedProcessId, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_ZEROPAD_ON_FAIL | VMMDLL_FLAG_NOPAGING_IO);
}
bool DMADevice::ExecuteRead(VMMDLL_SCATTER_HANDLE hSCATTER) {
	return VMMDLL_Scatter_ExecuteRead(hSCATTER);
}
