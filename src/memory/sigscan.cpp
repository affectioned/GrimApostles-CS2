#include "pch.h"
#include "sigscan.h"
#include "dma.h"

#include <vector>

static const uint8_t s_hexLUT[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
    0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

struct PatByte { uint8_t value; bool wildcard; };

static std::vector<PatByte> ParsePattern(const char* pat) {
    std::vector<PatByte> out;
    for (; *pat; pat++) {
        if (*pat == ' ') continue;
        if (*pat == '?') {
            out.push_back({ 0, true });
            if (pat[1] == '?') pat++;
        } else {
            out.push_back({ uint8_t((s_hexLUT[(uint8_t)pat[0]] << 4) | s_hexLUT[(uint8_t)pat[1]]), false });
            pat++;
        }
    }
    return out;
}

static uint64_t FindSignature(uint64_t start, uint64_t end, const char* pattern) {
    auto pat = ParsePattern(pattern);
    if (pat.empty()) return 0;

    std::vector<uint8_t> buf(end - start);
    DWORD read = 0;
    if (!VMMDLL_MemReadEx(DMADevice::hVMM, DMADevice::dwAttachedProcessId, start,
        buf.data(), (DWORD)buf.size(), &read, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_ZEROPAD_ON_FAIL))
        return 0;

    for (size_t i = 0; i + pat.size() <= buf.size(); i++) {
        bool match = true;
        for (size_t j = 0; j < pat.size(); j++)
            if (!pat[j].wildcard && buf[i + j] != pat[j].value) { match = false; break; }
        if (match) return start + i;
    }
    return 0;
}

// Gets module bounds and scans for pattern. Returns match VA and base via out params.
static bool ScanModule(const char* name, const char* pattern, uint64_t& match, uint64_t& base) {
    PVMMDLL_MAP_MODULEENTRY e = nullptr;
    if (!VMMDLL_Map_GetModuleFromNameU(DMADevice::hVMM, DMADevice::dwAttachedProcessId, (LPSTR)name, &e, 0) || !e)
        return false;
    base = e->vaBase;
    uint64_t size = e->cbImageSize;
    VMMDLL_MemFree(e);
    match = FindSignature(base, base + size, pattern);
    return match != 0;
}

std::ptrdiff_t FindRIPOffset(const char* module_name, const char* pattern, int rip_offset) {
    uint64_t match, base;
    if (!ScanModule(module_name, pattern, match, base)) return 0;
    int32_t disp = 0;
    if (DMADevice::MemRead(match + rip_offset, &disp, 4) != 4) return 0;
    return (std::ptrdiff_t)((match + rip_offset + 4) + (int64_t)disp - base);
}

uint32_t FindU4InModule(const char* module_name, const char* pattern, int u4_offset) {
    uint64_t match, base;
    if (!ScanModule(module_name, pattern, match, base)) return 0;
    uint32_t value = 0;
    DMADevice::MemRead(match + u4_offset, &value, 4);
    return value;
}
