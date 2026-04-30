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
4. `updater::fetchClassOffsets()` and `updater::fetchModuleOffsets()` — two synchronous HTTP fetches of `client_dll.hpp` and `offsets.hpp` from a2x/cs2-dumper, both with ETag-based conditional GET (304 = use disk cache, 200 = refresh). Cache files: `client_dll.hpp[.etag]` and `offsets.hpp[.etag]` next to the exe. Falls back to compiled-in defaults in `offsets.cpp` if both network and cache fail.
5. `DMAThreadMain()` — started as `std::thread` in main; handles connection (with retry) then drives the update loop flat-out
6. `gui::RunLoop()` — main render loop at 144fps cap (blocks until exit)

### Source layout
Follows Valve Source 2 folder naming conventions:
- `src/dma/` — platform layer: `DMA.h/.cpp` (`DMA_Connection`, MemProcFS wrapper), `DMA Thread.h/.cpp` (DMA thread entry point — note the space in the filename), `IGameContext.h`. Subfolders:
  - `src/dma/memory/` — `Process.h/.cpp`, `ScatterRead.h`, `SigScan.h/.cpp`
  - `src/dma/logging/` — `Log.h/.cpp`
  - `src/dma/input/` — `Input Manager.h/.cpp`
- `src/game/` — game state: `CS2Context.h/.cpp` (timer-driven update loop), `sdk.h/.cpp` (`CGame`, `CPlayer`, `mapData`), `offsets.h/.cpp`, `updater.h/.cpp`, `GameGlobals.h` (extern `g_Scatter`), `GameModules.h`, `vec.h`, `Const/Config.h` (`TeamID` enum)
- `src/game/client/` — CS2 client entity mirrors (one `.h`+`.cpp` per class, named after the CS2 class):
  - `CCSPlayerController.h/.cpp` — controller fields + `Read(base)` (queues scatter reads) + `ReadName()` (reads name string after nameAddr resolves)
  - `C_CSPlayerPawn.h/.cpp` — pawn fields + `Read(base)` (queues scatter reads)
  - `C_PlantedC4.h/.cpp` — planted bomb struct + `CGame::getBombData()`
  - `EntityList.cpp` — entity-list chunk/handle resolution helpers
- `src/vgui/` — rendering/UI: `gui.h/.cpp`, `render.cpp`, `dx11.cpp`, `resources.cpp`

**Coding convention:** file names and type names match CS2's actual class names exactly. Folder names follow Source 2 conventions (`dma`, `vgui`, `game/client`). Always follow this pattern when adding new classes. Use `class` for types with private members (e.g. `CGame`, `DMA_Connection`, `CS2Context`); use `struct` for plain data/entity mirrors (e.g. `CPlayer`, `CCSPlayerController`, `C_PlantedC4`).

### Rendering pipeline
- `src/vgui/render.cpp`: `gameLoop()` → `renderMap()` → `renderPlayers()` → `renderBomb()`
- `renderPlayers()` draws an orange ring around enemies who are actively defusing (`p.pawn.isDefusing`)
- `renderBomb()` draws: bomb-carrier halo on the current C4 holder (**pre-plant only** — the post-plant cue is the planted-bomb dot itself, not the planter), rendered as a black backdrop ring + bright yellow ring on top so it stays visible against any map background (the previous single-ring halo washed out on yellow ground textures); plus the yellow dot for the planted bomb and a cyan defuse ring. The halo applies regardless of local team
- `src/vgui/gui.cpp`: `RenderTeamPanels()` (enemy list, top-left), `RenderBombPanel()` (bottom-left: planted site/timer/defusing states — CT-side only, only shown when bomb entity exists)
- `worldToRadar()` converts CS2 world coordinates to radar pixel space using the per-map bounds/scale from `mapData`
- Textures are loaded via `DirectX::CreateWICTextureFromFile` (WIC, from DirectXTK)

### Rotating radar
Activated by `settings::rotateRadar` and gated on `localPlayer.controllerBase != 0`. `RadarFrame::compute()` reads the local yaw straight from `eyeAngles.y` — no smoothing — and publishes it as `RadarFrame::yawDeg`, which is also the source of `sinYaw`/`cosYaw`. Smoothing was tried and removed; the raw yaw matches the in-game crosshair 1:1 and feels more responsive at the cost of slightly visible per-tick jitter on fast flicks.

**`RadarFrame::yawDeg` mirrors `eyeAngles.y` exactly** — code aligning with the rotated map (e.g. `renderPlayers` adjusting per-player aim-line angle) should still read `f.yawDeg` for consistency, even though it's now equivalent to the raw value.

**Off-map corner sampling**: ImGui's DX11 backend creates its sampler with `D3D11_TEXTURE_ADDRESS_WRAP`, so a rotated quad whose UVs leave [0, 1] tiles the radar texture into the corners. To avoid this, `CreateDeviceD3D` builds a BORDER-mode sampler (`gui::g_pRadarSampler`, transparent-black border). `renderMap` swaps it in around the rotated `AddImageQuad` via the standard `ImDrawList::AddCallback` + `ImDrawCallback_ResetRenderState` pattern — never modify the vendored ImGui sampler directly. The static (non-rotating) code path uses ImGui's default sampler unchanged.

### Key namespaces (declared in `src/vgui/gui.h`)
- `gui::` — all rendering, D3D lifecycle, resource loading
- `maps::` — `mapTextures`, `mapBounds`, radar sizing constants
- `icons::` — `iconTextures`, `iconWidths`, `iconHeights`
- `settings::` — runtime-configurable render options. Per-team ESP toggles come in `*Enemies` / `*Friendlies` pairs: `showAimLines`, `showWeaponIcons`, `showHealthBars`, `showPlayerNames`. The local player counts as a friendly. Plus standalone bools `showTeamPanels`, `rotateRadar` and the float sliders `iconScale`, `aimLineLength`, `dotRadius`, `radarZoom`. All defined with defaults in `gui.cpp`, exposed in the collapsible Settings panel of `RenderControlPanel()` (per-team options live in a 2-column table; standalones below). **Default philosophy**: the radar is the only place that shows position-relative enemy info, so enemy aim lines / weapon icons / health bars all default **on**; enemy names default off because the top-left enemy panel already shows names. All friendly overlays default **off** — your own team is fully detailed in the panel and aim lines on teammates are noise (the local player counts as a friendly, so this also hides the redundant always-up arrow on your own dot in rotated mode). Team panels and rotate-radar default on. `radarZoom` is a multiplier on rotated-mode `pxPerWorld` (default `0.7` to widen the view from the 1.0 diagonal-fit baseline at the cost of small transparent corners; >1 zooms in tighter, <1 widens further); no effect in static mode. `rotateRadar` makes the radar follow the local player's yaw (player at center, eye direction up) like the in-game CS2 radar.

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
- `t_ModulePtrs` (1000ms) — reads `dwEntityList`, `dwLocalPlayerController`, `dwLocalPlayerPawn`, `dwGlobalVars`, `dwPlantedC4`, `dwGameRules`; clears `m_Local->players` and `m_EntityChunk` on either entity list **or local controller** pointer change — `dwEntityList` is a static pointer inside client.dll that does **not** change between map loads, so the controller pointer is the reliable map-change signal. Also dereferences `dwGameRules` once a second through `C_CSGameRulesProxy::m_pGameRules` and caches the result as `m_GameRulesPtr` so `t_BombState` can read round-state fields directly without re-walking the proxy chain on every 16ms tick
- `t_MapName` (100ms) — depends on `m_GlobalVarsPtr`. Watches `m_MapGeneration`: on change (map switch), zeroes `m_Local->mapName` and re-scans for the live map-name field offset on `CGlobalVarsBase` (defaults to `0x188`). Reads the map name string from the resolved offset
- `t_EntityChain` (100ms) — 4-pass entity chain: chunk → controllerBase → pawnHandle → listEntry2 → pawnBase. On every run, saves the previous `controllerBase` per slot before resetting; after Pass 1, if a slot now holds a different controller, `ctrl` and `pawn` are cleared to prevent stale `teamID` from miscoloring the new occupant until `t_PlayerCtrl` refreshes it
- `t_PlayerCtrl` (50ms) — reads all controller fields **including `localPlayer.ctrl`** (must be read explicitly here or `localPlayer.ctrl.teamID` stays 0)
- `t_PlayerPositions` / `t_LocalPlayerPos` (8ms) — hot pawn fields. `t_PlayerPositions` only writes into `players[]`; `localPlayer.pawn` is separate and must be populated explicitly here, just like `localPlayer.ctrl`. `t_LocalPlayerPos` reads `position`, `eyeAngles` (required by the rotating-radar code in `render.cpp`), and `lifeState`; a dead→alive transition (respawn) clears `m_Local->bomb` and resets `m_PlantedC4Ptr` to invalidate stale bomb state from the previous round. **Spectate-follow**: when dead (`lifeState != 0`), it also reads `m_pObserverServices->m_hObserverTarget` and resolves the handle through the entity-list chunk chain to find the spectated pawn's pointer; if it matches a `players[i].pawnBase`, that slot's `pawn.position`/`pawn.eyeAngles` are copied over `localPlayer.pawn`'s, so the radar follows the spectated player instead of stranding at the corpse. Three extra scatter executes per tick while dead (each step depends on the previous read).
- `t_PlayerWeapons` (100ms) — resolves active weapon via 4-pass handle chain (weaponServicesPtr → m_hActiveWeapon handle → entity list → weapon ptr → item def index); `m_pClippingWeapon` no longer exists in CS2
- `t_PlayerNames` (5000ms) — depends on `ctrl.nameAddr`. Reads each player's sanitized name string into `ctrl.name`. Slow tick on purpose: names rarely change and string reads are cheap to defer
- `t_CarrierScan` (100ms) — finds the C4 carrier and matches against `players[].pawnBase`. **Indirection chain matters**: `dwWeaponC4` does NOT point directly to a `C_C4` entity — it points to a small wrapper struct (probably a "current C4" cache or list head; first 8 bytes hold the C_C4 pointer, its own vtable lives at +0x10). So the resolution is `*(client_base+dwWeaponC4) → wrapper → *(wrapper) → C_C4 → +m_hOwnerEntity`. Reading `m_hOwnerEntity` directly off the wrapper yields a pointer's low 32 bits where you expected a CHandle (decodes to a bogus high entity index). Then standard handle resolution: chunk via `((h & 0x7FFF) >> 9)` → entry via `(h & 0x1FF) * 0x70`. Skipped once the bomb is planted (carrier state stays cleared from then on; we do not track the planter). `dwWeaponC4` is sigscanned in `sigscanOffsets()`.
- `t_NetworkState` (500ms) — reads `engine2_dll::dwNetworkGameClient->signOnState` into `m_Local->signOnState`. Logs transitions. Value 6 = SIGNONSTATE_FULL (in-game).
- `t_BombState` (16ms) — reads planted bomb fields plus `C_CSGameRules::m_iRoundEndWinnerTeam` (off the cached `m_GameRulesPtr`); on a `0 → non-zero` transition of the winner field, clears `m_Local->bomb` immediately so a stale "DEFUSED"/"EXPLODED" banner or a still-counting timer doesn't carry into the next round

Timers use `CTimer` (interval + lambda). `m_Local` is `std::unique_ptr<CGame>`; published to `m_Game` under `m_Mutex` at the end of each `Tick()`.

### Pointer validation
Always validate pointers as `p > 0x10000 && p < 0x7FFFFFFFFFFF` — a null check alone is insufficient. During map transitions, torn reads can produce garbage values (e.g. `0xC3C3C3C3...`) that pass a null check and cause cascading bad reads or entity list corruption.

### Bomb gotcha: site vs ticking ordering
`m_nBombSite` reads as -1 before `m_bBombTicking` goes true. Gate site validation on `newTicking`: `if (newTicking && newSite != 0 && newSite != 1) return;`. Applying that check unconditionally blocks all bomb state writes until ticking starts, preventing position and flags from ever being populated.

### Bomb tracking (`C_PlantedC4`)
`CGame::bomb` is a `C_PlantedC4`. Key fields: `entity`, `position`, `isTicking`, `isBeingDefused`, `hasExploded`, `hasDefused`, `site` (0=A, 1=B), `isCarried`, `carrierSlot`. Timer is wall-clock (`std::chrono::steady_clock`) latched when `isTicking` first goes true — `timeRemaining()` counts 40s from that point. `dwPlantedC4` is sigscan-only (no hardcoded fallback); pattern in `updater::sigscanOffsets()`.

`carrierSlot`/`isCarried` are pre-plant only. As soon as `bomb.entity` becomes non-zero (planted), `t_CarrierScan` resets both and stops scanning — the planted-bomb dot on the radar is the only post-plant indicator; we do not track or highlight the planter. There is no `planterSlot`.

**Stale bomb after round end**: when CTs win by T-elimination with the bomb still planted (or any round-end where the bomb's "DEFUSED"/"EXPLODED" banner would otherwise stick), `m_bBombTicking` and `m_bC4Activated` stay true on the entity and `dwPlantedC4` keeps pointing to it. Two layered fixes:
1. **Round-end signal** (primary, fires for everyone — alive surviving CTs included): `t_BombState` reads `C_CSGameRules::m_iRoundEndWinnerTeam` (resolved via `dwGameRules → C_CSGameRulesProxy::m_pGameRules`, cached as `m_GameRulesPtr` by `t_ModulePtrs`). When the field transitions `0 → non-zero`, the winner has been declared and the bomb state is cleared immediately.
2. **Respawn fallback** (still active): `t_LocalPlayerPos` detects the local player's dead→alive lifeState transition. Cheap belt-and-suspenders for cases where `m_iRoundEndWinnerTeam` lags or the player joined mid-round-end.

**Bomb panel** (`RenderBombPanel`) renders whenever `bomb.entity != 0`, on both teams — T players get the timer for rotation/peek calls, CT for defuse calls. Carrier info (who holds C4 pre-plant) is shown exclusively in the team panel, not the bomb panel.

### Memory reading
- `src/dma/DMA.h/.cpp` — `DMA_Connection` class. Global scatter object is `g_Scatter` (a `ScatterRead*`, set in `CS2Context::Initialize`). Use `g_Scatter->Add(addr, &val)` / `g_Scatter->Execute()` / `g_Scatter->Clear()` for all reads.
- `src/game/sdk.h` — `CGame` (class), `CPlayer`, `mapData` structs
- `src/game/sdk.cpp` — utility methods only; all DMA reads are driven by CS2Context timer methods
- `src/game/offsets.h/.cpp` — all CS2 struct offsets as `extern std::ptrdiff_t` declarations; `offsets.cpp` initialises them to the values current at build time as fallbacks (overwritten at startup by sigscan + HTTP fetch)
- `src/game/updater.cpp` — fetches `client_dll.hpp` from a2x/cs2-dumper via WinINet with ETag caching (cache files `client_dll.hpp` + `client_dll.hpp.etag` next to exe), and does signature scanning via `SigScan.cpp`

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
Player names are read as raw UTF-8 bytes from `m_sSanitizedPlayerName` (CS2's own sanitized name — preserves Unicode, strips control characters). No filtering is applied in the game layer. ImGui is initialized with Segoe UI Semibold at 16px (`segoeuisb.ttf`, falling back to `segoeui.ttf`, then ImGui's built-in bitmap font), covering Latin + Cyrillic glyph ranges. The size is tuned for glance-reading on a second-monitor DMA setup, where the user isn't sitting close to the screen — don't shrink it below ~14px. To add more script support (CJK etc.), merge additional fonts with glyph ranges in `InitImGui()` (`gui.cpp`).

### DMA initialization
`VMMDLL_Initialize` is called with `"-device", "FPGA", "-memmap", "auto", "-waitinitialize"`. `-memmap auto` lets LeechCore auto-detect the physical memory map; `-waitinitialize` blocks until VMM async init completes before the first scatter read fires.

### Log output
All log lines include a wall-clock timestamp: `HH:MM:SS.mmm [INFO] message`. Generated in `Log::Write` via `localtime_s` + `std::chrono::system_clock`. Always use `Log::Info` / `Log::Warn` / `Log::Error` — never `std::println` directly.

### WinINet is already linked
`updater.cpp` and `dx11.cpp` both `#pragma comment(lib, "wininet")`. Any new HTTP fetching can reuse the `httpGet` pattern in `updater.cpp` (`InternetOpenA` → `InternetOpenUrlA` with optional headers → `HttpQueryInfoA` for status/ETag → `InternetReadFile` loop).

## Offset update strategy
Each offset has exactly one source — never both. Two mechanisms run at startup:

1. **Sigscan** (`updater::sigscanOffsets`) resolves critical `dw*` RVA pointers (`dwEntityList`, `dwLocalPlayerController`, `dwLocalPlayerPawn`, `dwGlobalVars`, `dwPlantedC4`, `dwWeaponC4`, `dwGameRules`) directly from the live `client.dll`. Most resilient — works on any CS2 build. On sig failure, `ResolveOffset` preserves the existing target value (i.e. the compiled-in default in `offsets.cpp`) rather than zeroing it.
2. **HTTP fetch** (`updater::fetchClassOffsets` + `updater::fetchModuleOffsets`) pulls cs2-dumper's `client_dll.hpp` and `offsets.hpp` for everything not sigscanned:
   - **Class members** (e.g. `m_iHealth`, `m_hOwnerEntity`) → `kClassSpecs` table
   - **Non-sigscanned module fields** (e.g. `engine2_dll::dwBuildNumber`, `dwNetworkGameClient`) → `kModuleSpecs` table

**Don't put a sigscanned offset in `kModuleSpecs`** — keeping two sources for the same value creates a stale-data trap when one drifts.

**Conditional fetch**: each source caches body + ETag (`client_dll.hpp[.etag]`, `offsets.hpp[.etag]` next to the exe). Subsequent runs send `If-None-Match` — server returns 304 with no body when unchanged. Network failure falls back to disk cache; missing cache falls back to compiled-in values in `offsets.cpp`.

**Parser**: `parseHeader()` is a brace-matched walker + small regex. After matching `namespace X { ... }`, it advances past the OPEN brace (not close) so nested namespaces like `cs2_dumper::schemas::client_dll::C_BaseEntity` are picked up too.

**Adding a new offset**:
1. Declare in `offsets.h` (`extern std::ptrdiff_t`).
2. Define a build-time fallback in `offsets.cpp`.
3. Add a `(namespace, field, &target)` row to the appropriate spec table in `updater.cpp` (`kClassSpecs` for `client_dll.hpp` fields, `kModuleSpecs` for `offsets.hpp` fields).

**Build number sanity check**: `CS2Context::Initialize` reads `[engine2_base + dwBuildNumber]` once and logs it. Compare against the timestamp in cs2-dumper's header comment if you suspect offset drift.

Log strings use ASCII hyphens (`-`), not em dashes (`—`), to avoid CP1252/UTF-8 garbling in the Windows console.
