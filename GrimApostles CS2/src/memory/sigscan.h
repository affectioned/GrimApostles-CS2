#pragma once
#include <cstdint>
#include <cstddef>
#include <Windows.h>

// Finds a RIP-relative instruction in module_name and resolves the embedded disp32.
// rip_offset = byte index within the match where the 4-byte disp32 begins (typically 3).
// Returns the module-relative offset, or 0 on failure.
std::ptrdiff_t FindRIPOffset(const char* module_name, const char* pattern, int rip_offset);

// Finds a pattern in module_name and reads the raw uint32 at u4_offset bytes into the match.
// Equivalent to pelite's 'u4' atom — captures a struct member offset, not a pointer.
// Returns the raw value, or 0 on failure.
uint32_t FindU4InModule(const char* module_name, const char* pattern, int u4_offset);
