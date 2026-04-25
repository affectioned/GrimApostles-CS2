# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

A DMA-based radar overlay for Counter-Strike 2. It reads game memory via FPGA hardware (PCILeech/MemProcFS — read-only, no writes) and renders a live radar using ImGui + DirectX 11. Written in C++ targeting Windows x64.

## Building

Open `CS2_DMA_RADAR.sln` in **Visual Studio 2022 (v143)**. Set configuration to **Release x64** and build. Output goes to `bin/Release/`.

There is no test suite and no script-based build — MSBuild via Visual Studio is the only build path. The CI (`release.yml`) uses:
```
msbuild "CS2_DMA_RADAR.sln" /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /m /nologo
```

## Runtime requirements (not in repo)

Place alongside the `.exe` at runtime:
- `vmm.dll`, `leechcore.dll`, `FTD3XX.dll` — from PCILeech releases
- `textures/maps/` — radar PNG images per map (e.g. `de_dust2_radar.png`); all `*_radar.png` files are loaded automatically
- `textures/icons/` — weapon icon PNGs by name (e.g. `ak47.png`); any `.png` matching `kWeaponIDs` in `resources.cpp` is loaded automatically

## Architecture

### Startup sequence (`src/main.cpp`)
1. Create Win32 window → init D3D11 → show window → init ImGui
2. `gui::loadMapBounds()` — populates `maps::mapBounds` with world-space extents per map name
3. `gui::loadTextures()` — loads all PNGs from disk into DX11 `ID3D11ShaderResourceView*`, stored in `maps::mapTextures` and `icons::iconTextures`
4. `updater::fetchClassOffsets()` — background `std::thread` (detached) fetches `client_dll.hpp` from a2x/cs2-dumper; no hardcoded fallbacks (offset stays 0 on failure, reads return zeros)
5. `DMAThreadMain()` — started as `std::thread` in main; handles connection (with retry) then drives the update loop flat-out
6. `gui::RunLoop()` — main render loop at 144fps cap (blocks until exit)

### Source layout
Follows Valve Source 2 folder naming conventions:
- `src/tier0/` — platform layer: `dma.h/.cpp` (MemProcFS wrapper), `sigscan.h/.cpp`, `dma_thread.h/.cpp` (DMA thread entry point)
- `src/game/` — game state: `sdk.h/.cpp` (`CGame`, `CPlayer`, `mapData`), `offsets.h/.cpp`, `updater.h/.cpp`
- `src/game/client/` — CS2 client entity mirrors (one `.h`+`.cpp` per class, named after the CS2 class):
  - `CCSPlayerController.h/.cpp` — controller fields + `Read(base)` (queues scatter reads) + `ReadName()` (reads name string after nameAddr resolves)
  - `C_CSPlayerPawn.h/.cpp` — pawn fields + `Read(base)` (queues scatter reads)
  - `C_PlantedC4.h/.cpp` — planted bomb struct + `CGame::getBombData()`
- `src/vgui/` — rendering/UI: `gui.h/.cpp`, `render.cpp`, `dx11.cpp`, `resources.cpp`

**Coding convention:** file names and type names match CS2's actual class names exactly. Folder names follow Source 2 conventions (`tier0`, `vgui`, `game/client`). Always follow this pattern when adding new classes. Use `class` for types with private members (e.g. `CGame`, `DMADevice`); use `struct` for plain data/entity mirrors (e.g. `CPlayer`, `CCSPlayerController`, `C_PlantedC4`).

### Rendering pipeline
- `src/vgui/render.cpp`: `gameLoop()` → `renderMap()` → `renderPlayers()` → `renderBomb()`
- `renderPlayers()` draws an orange ring around enemies who are actively defusing (`p.pawn.isDefusing`)
- `renderBomb()` draws: yellow carrier halo on C4 holder (pre-plant) or on the planter (post-plant, while bomb is live), yellow dot for planted bomb, cyan defuse ring
- `src/vgui/gui.cpp`: `RenderTeamPanels()` (enemy list, top-left), `RenderBombPanel()` (bottom-left: planted site/timer/defusing states — CT-side only, only shown when bomb entity exists)
- `worldToRadar()` converts CS2 world coordinates to radar pixel space using the per-map bounds/scale from `mapData`
- Textures are loaded via `DirectX::CreateWICTextureFromFile` (WIC, from DirectXTK)

### Key namespaces (declared in `src/vgui/gui.h`)
- `gui::` — all rendering, D3D lifecycle, resource loading
- `maps::` — `mapTextures`, `mapBounds`, radar sizing constants
- `icons::` — `iconTextures`, `iconWidths`, `iconHeights`
- `settings::` — runtime-configurable render options (`showWeaponIcons`, `showPlayerNames`, `showHealthBars`, `showAimLines`, `showTeamPanels`, `iconScale`, `aimLineLength`, `dotRadius`); defined with defaults in `gui.cpp`, exposed in the collapsible Settings panel of `RenderControlPanel()`. Radar-only overlays (weapon icons, names, health bars) default **off** since the enemy panel covers them; aim lines and team panels default on.

### CPlayer structure
`CPlayer` composes two sub-structs matching CS2's class hierarchy:
- `p.ctrl` — `CCSPlayerController`: `teamID` (`TeamID` enum), `color`, `ping`, `armor`, `hasDefuser`, `hasHelmet`, `name`, `nameAddr`
- `p.pawn` — `C_CSPlayerPawn`: `health`, `lifeState`, `position`, `eyeAngles`, `activeWeapon`, `activeWeaponID`, `isDefusing`, `lastPlaceName`
- `p.controllerBase`, `p.pawnBase` — DMA chain pointers

Valid player filter: `p.controllerBase && p.pawn.lifeState == 0 && p.ctrl.teamID >= TEAM_T && p.pawn.health > 0`

### TeamID enum
All team ID comparisons use the `TeamID` enum (defined in `src/game/Const/Config.h`):
```cpp
enum TeamID : uint8_t { TEAM_UNASSIGNED=0, TEAM_SPECTATOR=1, TEAM_T=2, TEAM_CT=3 };
```
`CCSPlayerController::teamID` is of type `TeamID`. Never compare against raw integers — use the enum values.

### CS2Context timer architecture
All DMA reads are driven by `CS2Context` timer methods registered in `Initialize()`:
- `t_ModulePtrs` (1000ms) — reads `dwEntityList`, `dwLocalPlayerController`, `dwLocalPlayerPawn`, `dwGlobalVars`, `dwPlantedC4`; clears `m_Local->players` and `m_EntityChunk` on either entity list **or local controller** pointer change — `dwEntityList` is a static pointer inside client.dll that does **not** change between map loads, so the controller pointer is the reliable map-change signal
- `t_EntityChain` (100ms) — 4-pass entity chain: chunk → controllerBase → pawnHandle → listEntry2 → pawnBase. On every run, saves the previous `controllerBase` per slot before resetting; after Pass 1, if a slot now holds a different controller, `ctrl` and `pawn` are cleared to prevent stale `teamID` from miscoloring the new occupant until `t_PlayerCtrl` refreshes it
- `t_PlayerCtrl` (50ms) — reads all controller fields **including `localPlayer.ctrl`** (must be read explicitly here or `localPlayer.ctrl.teamID` stays 0)
- `t_PlayerPositions` / `t_LocalPlayerPos` (8ms) — hot pawn fields. `t_LocalPlayerPos` also reads `lifeState` for the local player; a dead→alive transition (respawn) clears `m_Local->bomb` and resets `m_PlantedC4Ptr` to invalidate stale bomb state from the previous round
- `t_PlayerWeapons` (100ms) — resolves active weapon via 4-pass handle chain (weaponServicesPtr → m_hActiveWeapon handle → entity list → weapon ptr → item def index); `m_pClippingWeapon` no longer exists in CS2
- `t_CarrierScan` (100ms) — scans T-side weapon lists for C4 (item def 49); skipped once bomb is planted. On the first run after `bomb.entity` becomes non-zero, latches `bomb.planterSlot = bomb.carrierSlot` (before clearing carrierSlot) so the planter can be tracked post-plant. Pass 1 reads `weaponServicesPtr + m_hMyWeapons + 0x8` — the `+0x8` skips the `C_NetworkUtlVectorBase` size fields (+0x0=m_Size, +0x4=m_nMaxSize) to reach `m_pData`
- `t_BombState` (16ms) — reads planted bomb fields

Timers use `CTimer` (interval + lambda). `m_Local` is `std::unique_ptr<CGame>`; published to `m_Game` under `m_Mutex` at the end of each `Tick()`.

### Pointer validation
Always validate pointers as `p > 0x10000 && p < 0x7FFFFFFFFFFF` — a null check alone is insufficient. During map transitions, torn reads can produce garbage values (e.g. `0xC3C3C3C3...`) that pass a null check and cause cascading bad reads or entity list corruption.

### Bomb gotcha: site vs ticking ordering
`m_nBombSite` reads as -1 before `m_bBombTicking` goes true. Gate site validation on `newTicking`: `if (newTicking && newSite != 0 && newSite != 1) return;`. Applying that check unconditionally blocks all bomb state writes until ticking starts, preventing position and flags from ever being populated.

### Bomb tracking (`C_PlantedC4`)
`CGame::bomb` is a `C_PlantedC4`. Key fields: `entity`, `position`, `isTicking`, `isBeingDefused`, `hasExploded`, `hasDefused`, `site` (0=A, 1=B), `isCarried`, `carrierSlot`, `planterSlot`. Timer is wall-clock (`std::chrono::steady_clock`) latched when `isTicking` first goes true — `timeRemaining()` counts 40s from that point. `dwPlantedC4` is sigscan-only (no hardcoded fallback); pattern in `updater::sigscanOffsets()`.

`planterSlot` is latched by `t_CarrierScan` on the first run after `bomb.entity` becomes non-zero (from the last known `carrierSlot`). It is used by `renderBomb` (yellow halo on the planter while bomb is live) and `RenderTeamPanels` (C4 icon on the planter's row). Cleared when `bomb = {}` is reset.

**Stale bomb after round end**: when CTs win by T-elimination with the bomb still planted, `m_bBombTicking` and `m_bC4Activated` remain true on the entity and `dwPlantedC4` keeps pointing to it. The bomb timer would keep running into the next round. Fix: `t_LocalPlayerPos` detects the local player's dead→alive lifeState transition (new round) and immediately clears `m_Local->bomb` and `m_PlantedC4Ptr`.

**Bomb panel** (`RenderBombPanel`) only renders when `bomb.entity != 0` AND the local player is CT-side (`teamID == TEAM_CT`). Carrier info (who holds C4 pre-plant) is shown exclusively in the team panel, not the bomb panel.

### Memory reading
- `src/dma/DMA.h/.cpp` — `DMA_Connection` class. Global scatter object is `g_Scatter` (a `ScatterRead*`, set in `CS2Context::Initialize`). Use `g_Scatter->Add(addr, &val)` / `g_Scatter->Execute()` / `g_Scatter->Clear()` for all reads.
- `src/game/sdk.h` — `CGame` (class), `CPlayer`, `mapData` structs
- `src/game/sdk.cpp` — utility methods only; all DMA reads are driven by CS2Context timer methods
- `src/game/offsets.h/.cpp` — all CS2 struct offsets; no hardcoded fallbacks (offset stays 0 on failure)
- `src/game/updater.cpp` — fetches `client_dll.hpp` from a2x/cs2-dumper via WinINet (`fetchURL`), also does signature scanning via `sigscan.cpp`

### Threading model
- Single DMA thread: `std::thread` in `main.cpp` calls `DMAThreadMain(game, gameMutex, dmaRun)` — handles connect+retry then update loop
- Double-buffer: update thread writes to private `CGame local` (heap-allocated via `std::make_unique`), locks briefly to publish `game = *local`; render thread snapshots under the same lock
- No custom Thread class — plain `std::thread` + `std::atomic<bool> dmaRun`
- GUI has no connect/disconnect UI — DMA thread auto-connects on startup, retries every 3s on failure
- `g_DMA.Disconnect()` called on Exit button; DMA thread yields until stop flag fires

### Large stack allocations
- `CGame` is ~23KB (two `CPlayer[64]` arrays) — always heap-allocate when declaring as a local variable (`std::make_unique<CGame>()`)

### DMA reliability and read batching
The FPGA hardware runs at ~200MB/s; PCIe round-trip latency matters more than bandwidth. Guidelines:
- **Never use individual `MemReadPtr`/`MemRead` calls in `update()`** — always batch into `g_DMA.PrepareEX(...)` + `g_DMA.ExecuteRead()` + `g_DMA.Clear()` scatter passes.
- Entity class `Read(base)` methods only *queue* scatter reads — they call `g_DMA.PrepareEX` but do NOT call `ExecuteRead`. The caller (`sdk.cpp`) batches multiple entities then executes once.
- `update()` is structured as three scatter batches before the entity chain: **Scatter A** (5 module-level offsets including `dwPlantedC4`), **Scatter B** (mapPtr + local player fields + 64 entity chunk pointers), **Scatter C** (mapName + local player name strings).
- Entity chain passes are **guarded** (`if (players[i].listEntry)` etc.) so empty slots don't generate reads.
- `VMMDLL_FLAG_ZEROPAD_ON_FAIL` handles complete read failures (returns zeros). `sdk.cpp` has an `isValidPtr()` helper for validation.
- **Valid player filter**: `p.pawn.lifeState == 0 && p.ctrl.teamID >= TEAM_T && p.pawn.health > 0`. Do not filter by `ping` (too unreliable mid-scatter).

### Texture loading
`LoadImageTexture()` in `src/vgui/dx11.cpp` — two overloads, one optionally outputs pixel dimensions. Used exclusively in `src/vgui/resources.cpp`.

### Dynamic texture loading system
Both maps and icons are loaded by scanning their folders at startup via `std::filesystem::directory_iterator` — no hardcoded load calls.
- **Maps**: scans `textures/maps/` for `*_radar.png` (textures) and `*_radar.txt` (bounds). The txt files are CS2's overview KeyValues files extracted from the VPK by AssetExtractor (`resource/overviews/*.txt`) and renamed `<mapname>_radar.txt`. `loadMapBounds()` parses `pos_x`/`pos_y`/`scale` from each — no hardcoded bounds. Multi-level variants (`de_nuke_lower`, `de_vertigo_lower`) inherit the base map's bounds after the scan. Note: the overviews folder does **not** exist on the filesystem — files are inside `pak01_dir.vpk` and extracted via the `vpk` Python library.
- **Icons**: scans `textures/icons/` for `.png` files whose stem matches a key in the static `kWeaponIDs` table in `resources.cpp`. To add a new weapon, add its name→ID entry there — do not add `loadDim()` calls.
- `<filesystem>` is included in `pch.h`.

### Settings persistence
Settings are saved/loaded via ImGui's built-in `.ini` system using a registered `ImGuiSettingsHandler` (in `InitImGui()`, `gui.cpp`). File is `CS2_DMA_RADAR.ini` next to the `.exe`. Loads automatically on first `NewFrame()`, saves on `DestroyContext()`. To add a new setting: add a line to both `ReadLineFn` and `WriteAllFn` — no new files or explicit save calls needed. `ImGuiSettingsHandler` and `ImHashStr` come from `imgui_internal.h` (already included).

### Unicode player names
Player names are read as raw UTF-8 bytes from `m_sSanitizedPlayerName` (CS2's own sanitized name — preserves Unicode, strips control characters). No filtering is applied in the game layer. ImGui is initialized with Segoe UI (`C:\Windows\Fonts\segoeui.ttf`) covering Latin + Cyrillic glyph ranges; falls back to ImGui's built-in font if the file is missing. To add more script support (CJK etc.), merge additional fonts with glyph ranges in `InitImGui()` (`gui.cpp`).

### DMA initialization
`VMMDLL_Initialize` is called with `"-device", "FPGA", "-memmap", "auto", "-waitinitialize"`. `-memmap auto` lets LeechCore auto-detect the physical memory map; `-waitinitialize` blocks until VMM async init completes before the first scatter read fires.

### Log output
All log lines include a wall-clock timestamp: `HH:MM:SS.mmm [INFO] message`. Generated in `Log::Write` via `localtime_s` + `std::chrono::system_clock`. Always use `Log::Info` / `Log::Warn` / `Log::Error` — never `std::println` directly.

### WinINet is already linked
`updater.cpp` and `dx11.cpp` both `#pragma comment(lib, "wininet")`. Any new HTTP fetching can reuse the existing `fetchURL()` static helper in `updater.cpp`.

## Offset update strategy
- Module-level offsets (`dwEntityList`, etc.) are resolved via signature scanning (`updater::sigscanOffsets()`)
- Class member offsets are fetched from `https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.hpp` and parsed with regex
- No hardcoded fallbacks — `src/game/offsets.cpp` initialises all offsets to 0; sigscan/fetch fills them at startup
- `dwGameTypes` (`matchmaking_dll`) was removed — only resolves when in a match, and unused in this codebase
- Log strings use ASCII hyphens (`-`), not em dashes (`—`), to avoid CP1252/UTF-8 garbling in the Windows console
