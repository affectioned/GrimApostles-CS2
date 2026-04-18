# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

A DMA-based radar overlay for Counter-Strike 2. It reads game memory via FPGA hardware (PCILeech/MemProcFS — read-only, no writes) and renders a live radar using ImGui + DirectX 11. Written in C++ targeting Windows x64.

## Building

Open `GrimApostles CS2.sln` in **Visual Studio 2022 (v143)**. Set configuration to **Release x64** and build. Output goes to `bin/Release/`.

There is no test suite and no script-based build — MSBuild via Visual Studio is the only build path. The CI (`release.yml`) uses:
```
msbuild "GrimApostles CS2.sln" /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /m /nologo
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
4. `updater::fetchClassOffsets()` — background thread fetches `client_dll.hpp` from a2x/cs2-dumper, falls back to hardcoded defaults in `src/game/offsets.cpp`
5. `gui::RunLoop()` — main render loop (blocks until exit)

### Source layout
Follows Valve Source 2 folder naming conventions:
- `src/tier0/` — platform layer: `dma.h/.cpp` (MemProcFS wrapper), `sigscan.h/.cpp`
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
- `renderBomb()` draws: yellow carrier halo on C4 holder, yellow dot for planted bomb, cyan defuse ring
- `src/vgui/gui.cpp`: `RenderTeamPanels()` (enemy list, top-left), `RenderBombPanel()` (bottom-left: carrier/planted/timer/defusing states)
- `worldToRadar()` converts CS2 world coordinates to radar pixel space using the per-map bounds/scale from `mapData`
- Textures are loaded via `DirectX::CreateWICTextureFromFile` (WIC, from DirectXTK)

### Key namespaces (declared in `src/vgui/gui.h`)
- `gui::` — all rendering, D3D lifecycle, resource loading
- `maps::` — `mapTextures`, `mapBounds`, radar sizing constants
- `icons::` — `iconTextures`, `iconWidths`, `iconHeights`
- `settings::` — runtime-configurable render options (`showWeaponIcons`, `showPlayerNames`, `showHealthBars`, `showAimLines`, `showTeamPanels`, `iconScale`, `aimLineLength`, `dotRadius`); defined with defaults in `gui.cpp`, exposed in the collapsible Settings panel of `RenderControlPanel()`. Radar-only overlays (weapon icons, names, health bars) default **off** since the enemy panel covers them; aim lines and team panels default on.

### CPlayer structure
`CPlayer` composes two sub-structs matching CS2's class hierarchy:
- `p.ctrl` — `CCSPlayerController`: `teamID`, `color`, `ping`, `armor`, `hasDefuser`, `hasHelmet`, `name`, `nameAddr`
- `p.pawn` — `C_CSPlayerPawn`: `health`, `lifeState`, `position`, `eyeAngles`, `activeWeapon`, `activeWeaponID`, `isDefusing`, `lastPlaceName`
- `p.controllerBase`, `p.pawnBase` — DMA chain pointers

Valid player filter: `p.controllerBase && p.pawn.lifeState == 0 && p.ctrl.teamID >= 2 && p.pawn.health > 0`

### Bomb tracking (`C_PlantedC4`)
`CGame::bomb` is a `C_PlantedC4`. Key fields: `entity`, `position`, `isTicking`, `isBeingDefused`, `hasExploded`, `hasDefused`, `site` (0=A, 1=B), `isCarried`, `carrierSlot`. Timer is wall-clock (`std::chrono::steady_clock`) latched when `isTicking` first goes true — `timeRemaining()` counts 40s from that point. `dwPlantedC4` is sigscan-only (no hardcoded fallback); pattern in `updater::sigscanOffsets()`.

### Memory reading
- `src/tier0/dma.h/.cpp` — `DMADevice` class wrapping MemProcFS; global instance is `g_DMA` (defined in `dma.cpp`). Use `g_DMA.PrepareEX(addr, &val, size)` — no `hScatter` parameter (it's a member). Access state via `g_DMA.bConnected`, `g_DMA.moduleBase`, etc. Static constants (`kProcess`, `kModule`) are accessed as `DMADevice::kProcess`.
- `src/game/sdk.h` — `CGame` (class), `CPlayer`, `mapData` structs
- `src/game/sdk.cpp` — `CGame::update()` drives all per-frame memory reads; also contains `getPlayerData()` and `getWeapons()`
- `src/game/offsets.h/.cpp` — all CS2 struct offsets; auto-updated at startup, hardcoded defaults as fallback
- `src/game/updater.cpp` — fetches `client_dll.hpp` from a2x/cs2-dumper via WinINet (`fetchURL`), also does signature scanning via `sigscan.cpp`

### DMA reliability and read batching
The FPGA hardware runs at ~200MB/s; PCIe round-trip latency matters more than bandwidth. Guidelines:
- **Never use individual `MemReadPtr`/`MemRead` calls in `update()`** — always batch into `g_DMA.PrepareEX(...)` + `g_DMA.ExecuteRead()` + `g_DMA.Clear()` scatter passes.
- Entity class `Read(base)` methods only *queue* scatter reads — they call `g_DMA.PrepareEX` but do NOT call `ExecuteRead`. The caller (`sdk.cpp`) batches multiple entities then executes once.
- `update()` is structured as three scatter batches before the entity chain: **Scatter A** (5 module-level offsets including `dwPlantedC4`), **Scatter B** (mapPtr + local player fields + 64 entity chunk pointers), **Scatter C** (mapName + local player name strings).
- Entity chain passes are **guarded** (`if (players[i].listEntry)` etc.) so empty slots don't generate reads.
- `VMMDLL_FLAG_ZEROPAD_ON_FAIL` handles complete read failures (returns zeros). `sdk.cpp` has `isValidPtr()` and `isValidAscii()` helpers for validation.
- **Valid player filter**: `p.pawn.lifeState == 0 && p.ctrl.teamID >= 2 && p.pawn.health > 0`. Do not filter by `ping` (too unreliable mid-scatter).

### Texture loading
`LoadImageTexture()` in `src/vgui/dx11.cpp` — two overloads, one optionally outputs pixel dimensions. Used exclusively in `src/vgui/resources.cpp`.

### Dynamic texture loading system
Both maps and icons are loaded by scanning their folders at startup via `std::filesystem::directory_iterator` — no hardcoded load calls.
- **Maps**: scans `textures/maps/` for `*_radar.png` (textures) and `*_radar.txt` (bounds). The txt files are CS2's overview KeyValues files extracted from the VPK by AssetExtractor (`resource/overviews/*.txt`) and renamed `<mapname>_radar.txt`. `loadMapBounds()` parses `pos_x`/`pos_y`/`scale` from each — no hardcoded bounds. Multi-level variants (`de_nuke_lower`, `de_vertigo_lower`) inherit the base map's bounds after the scan. Note: the overviews folder does **not** exist on the filesystem — files are inside `pak01_dir.vpk` and extracted via the `vpk` Python library.
- **Icons**: scans `textures/icons/` for `.png` files whose stem matches a key in the static `kWeaponIDs` table in `resources.cpp`. To add a new weapon, add its name→ID entry there — do not add `loadDim()` calls.
- `<filesystem>` is included in `pch.h`.

### Settings persistence
Settings are saved/loaded via ImGui's built-in `.ini` system using a registered `ImGuiSettingsHandler` (in `InitImGui()`, `gui.cpp`). File is `GrimApostles.ini` next to the `.exe`. Loads automatically on first `NewFrame()`, saves on `DestroyContext()`. To add a new setting: add a line to both `ReadLineFn` and `WriteAllFn` — no new files or explicit save calls needed. `ImGuiSettingsHandler` and `ImHashStr` come from `imgui_internal.h` (already included).

### WinINet is already linked
`updater.cpp` and `dx11.cpp` both `#pragma comment(lib, "wininet")`. Any new HTTP fetching can reuse the existing `fetchURL()` static helper in `updater.cpp`.

## Offset update strategy
- Module-level offsets (`dwEntityList`, etc.) are resolved via signature scanning (`updater::sigscanOffsets()`)
- Class member offsets are fetched from `https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.hpp` and parsed with regex
- Hardcoded fallback values live in `src/game/offsets.cpp`
