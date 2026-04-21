#include "pch.h"
#include "updater.h"
#include "offsets.h"
#include "DMA/Memory/SigScan.h"

// ── RIP-relative resolver ──────────────────────────────────────────────────────
// Resolve a RIP-relative MOV/LEA reference and return the module-relative RVA.
// sigHit   – absolute address of the first byte of the matched instruction
// dispOff  – byte offset within the instruction where the 32-bit displacement lives
// instrSz  – total size of the instruction (RIP advances past it before applying disp)
// Returns 0 if the resolved address is not readable (false-positive match).
static ptrdiff_t ResolveRIP(DMA_Connection* Conn, DWORD pid,
                             uintptr_t clientBase, uint64_t sigHit,
                             int dispOff, int instrSz)
{
    int32_t disp = ReadFromPID<int32_t>(Conn, sigHit + dispOff, pid);
    uintptr_t absAddr = static_cast<uintptr_t>(sigHit) + instrSz + disp;
    if (!IsAddressReadable(Conn, absAddr, pid))
        return 0;
    return static_cast<ptrdiff_t>(absAddr - clientBase);
}

// Scan for sig, resolve the RIP-relative displacement, and write target.
// Falls back to 'fallback' (pass 0 when no reliable hardcoded RVA is known).
static void ResolveOffset(DMA_Connection* Conn, DWORD pid, uintptr_t clientBase, uintptr_t clientEnd,
                           const char* name, ptrdiff_t& target, ptrdiff_t fallback,
                           const char* sig, int dispOff, int instrSz)
{
    uint64_t hit = FindSignature(Conn, sig, clientBase, clientEnd, pid);
    ptrdiff_t offset = hit ? ResolveRIP(Conn, pid, clientBase, hit, dispOff, instrSz) : 0;

    if (offset)
    {
        target = offset;
        Log::Info("[+] {} = 0x{:X}", name, target);
    }
    else
    {
        target = fallback;
        Log::Warn("[!] {} sig failed, using fallback 0x{:X}", name, target);
    }
}

// ── sigscanOffsets ────────────────────────────────────────────────────────────

bool updater::sigscanOffsets(DMA_Connection* conn, Process* proc)
{
    Log::Info("[Updater]: Scanning RVA pointers...");

    const DWORD     pid        = proc->GetPID();
    const uintptr_t clientBase = proc->GetModuleBase("client.dll");
    const uintptr_t clientEnd  = clientBase + proc->GetModuleSize("client.dll");

    if (!clientBase)
    {
        Log::Error("[Updater]: client.dll base not found, aborting.");
        return false;
    }

    int resolved = 0;

    auto scanRIP = [&](ptrdiff_t& t, ptrdiff_t fallback,
                       const char* sig, int dispOff, int instrSz, const char* name) {
        ResolveOffset(conn, pid, clientBase, clientEnd, name, t, fallback, sig, dispOff, instrSz);
        if (t) resolved++;
    };

    // ── Module-level RVA pointers ─────────────────────────────────────────────
    // All are 7-byte RIP-relative instructions (3-byte opcode+ModRM + 4-byte disp).

    scanRIP(client_dll::dwEntityList,            0, "48 89 0D ?? ?? ?? ?? E9 ?? ?? ?? ?? CC",       3, 7, "dwEntityList");
    scanRIP(client_dll::dwLocalPlayerController, 0, "48 8B 05 ?? ?? ?? ?? 41 89 BE",                3, 7, "dwLocalPlayerController");
    scanRIP(client_dll::dwGlobalVars,            0, "48 89 15 ?? ?? ?? ?? 48 89 42",                3, 7, "dwGlobalVars");
    scanRIP(client_dll::dwPlantedC4,             0, "48 8B 15 ?? ?? ?? ?? 41 FF C0 48 8D 4C 24 ??", 3, 7, "dwPlantedC4");

    // dwLocalPlayerPawn — two-step: resolve the list-entry base RVA then add the pawn field offset.
    {
        uint64_t hit = FindSignature(conn,
            "48 8D 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 40 53 56 41 54",
            clientBase, clientEnd, pid);
        ptrdiff_t rva = hit ? ResolveRIP(conn, pid, clientBase, hit, 3, 7) : 0;

        uint64_t hit2 = FindSignature(conn, "4C 39 B6 ?? ?? ?? ?? 74 ?? 44 88 BE", clientBase, clientEnd, pid);
        uint32_t off  = hit2 ? ReadFromPID<uint32_t>(conn, hit2 + 3, pid) : 0;

        if (rva && off)
        {
            client_dll::dwLocalPlayerPawn = rva + static_cast<ptrdiff_t>(off);
            Log::Info("[+] dwLocalPlayerPawn = 0x{:X}", client_dll::dwLocalPlayerPawn);
            resolved++;
        }
        else
        {
            Log::Warn("[!] dwLocalPlayerPawn sig failed (rva={} off={})", rva != 0, off != 0);
        }
    }

    Log::Info("[Updater]: {}/5 RVA pointers resolved.", resolved);
    return resolved > 0;
}
