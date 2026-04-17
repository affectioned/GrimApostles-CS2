#include "pch.h"
#include "dma.h"

bool DMADevice::bConnected = false;
DWORD DMADevice::dwAttachedProcessId = 0;
VMM_HANDLE DMADevice::hVMM = NULL;
uint64_t DMADevice::moduleBase = 0;
VMMDLL_SCATTER_HANDLE DMADevice::hScatter = NULL;

void DMADevice::ShowKeyPress() {
	std::cout << "\nPress any key to exit\n";
	Sleep(250);
	_getch();
}

bool DMADevice::Connect() {
	if (bConnected) return true;

	char arg0[] = "";
	char arg1[] = "-device";
	char arg2[] = "fpga://algo=0";
	LPSTR args[] = { arg0, arg1, arg2 };
	hVMM = VMMDLL_Initialize(3, args);
	if (!hVMM) {
		std::cout << "[DMA]: Failed to initialize VMM\n";
		return false;
	}
	hScatter = VMMDLL_Scatter_Initialize(hVMM, dwAttachedProcessId,
		VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_ZEROPAD_ON_FAIL | VMMDLL_FLAG_NOPAGING_IO);

	ULONG64 id, major, minor;
	if (!VMMDLL_ConfigGet(hVMM, LC_OPT_FPGA_FPGA_ID, &id) ||
		!VMMDLL_ConfigGet(hVMM, LC_OPT_FPGA_VERSION_MAJOR, &major) ||
		!VMMDLL_ConfigGet(hVMM, LC_OPT_FPGA_VERSION_MINOR, &minor)) {
		std::cout << "[DMA]: Failed to read FPGA config\n";
		VMMDLL_Close(hVMM);
		if (hScatter) VMMDLL_Scatter_CloseHandle(hScatter);
		hVMM = NULL; hScatter = NULL;
		return false;
	}
	std::cout << "[DMA]: FPGA ID=" << id << " v" << major << "." << minor << "\n";

	if (major >= 4 && (major >= 5 || minor >= 7)) {
		LC_CONFIG cfg = {};
		cfg.dwVersion = LC_CONFIG_VERSION;
		strcpy_s(cfg.szDevice, "existing");
		if (HANDLE lc = LcCreate(&cfg)) {
			BYTE bytes[4] = { 0x10, 0x00, 0x10, 0x00 };
			LcCommand(lc, LC_CMD_FPGA_CFGREGPCIE_MARKWR | 0x002, 4, bytes, NULL, NULL);
			std::cout << "[DMA]: Auto abort clearing on\n";
			LcClose(lc);
		}
	}

	bConnected = true;
	return true;
}

void DMADevice::Disconnect() {
	std::cout << "[DMA]: Disconnecting\n";
	bConnected = false;
	VMMDLL_Close(hVMM);
	VMMDLL_Scatter_CloseHandle(hScatter);
	hVMM = NULL; hScatter = NULL;
	dwAttachedProcessId = 0; moduleBase = 0;
}

bool DMADevice::AttachToProcessId() {
	if (!bConnected) return false;
	if (!VMMDLL_PidGetFromName(hVMM, const_cast<LPSTR>(kProcess), &dwAttachedProcessId)) {
		std::cout << "[DMA]: Failed to find PID\n";
		return false;
	}
	VMMDLL_PROCESS_INFORMATION info = {};
	info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
	info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
	SIZE_T size = sizeof(info);
	if (!VMMDLL_ProcessGetInformation(hVMM, dwAttachedProcessId, &info, &size)) {
		std::cout << "[DMA]: Failed to get process information\n";
		return false;
	}
	std::cout << "[DMA]: Attached to " << kProcess << " (PID: " << dwAttachedProcessId << ")\n";
	return true;
}

uint64_t DMADevice::getModuleBase(const char* moduleName) {
	return VMMDLL_ProcessGetModuleBaseU(hVMM, dwAttachedProcessId, const_cast<LPSTR>(moduleName));
}

bool DMADevice::Clear(VMMDLL_SCATTER_HANDLE hSCATTER) {
	return VMMDLL_Scatter_Clear(hSCATTER, dwAttachedProcessId,
		VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_ZEROPAD_ON_FAIL | VMMDLL_FLAG_NOPAGING_IO);
}

bool DMADevice::ExecuteRead(VMMDLL_SCATTER_HANDLE hSCATTER) {
	return VMMDLL_Scatter_ExecuteRead(hSCATTER);
}
