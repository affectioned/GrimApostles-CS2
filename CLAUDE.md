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
- `textures/maps/` — radar PNG images per map (e.g. `de_dust2_radar.png`)
- `textures/icons/` — weapon icon PNGs keyed by weapon name (e.g. `ak47.png`)

## Architecture

### Startup sequence (`src/main.cpp`)
1. Create Win32 window → init D3D11 → show window → init ImGui
2. `gui::loadMapBounds()` — populates `maps::mapBounds` with world-space extents per map name
3. `gui::loadTextures()` — loads all PNGs from disk into DX11 `ID3D11ShaderResourceView*`, stored in `maps::mapTextures` and `icons::iconTextures`
4. `updater::fetchClassOffsets()` — background thread fetches `client_dll.hpp` from a2x/cs2-dumper, falls back to hardcoded defaults in `src/game/offsets.cpp`
5. `gui::RunLoop()` — main render loop (blocks until exit)

### Rendering pipeline
- `render.cpp`: `gameLoop()` → `renderMap()` (draws map texture fullscreen) → `renderPlayers()` (dots, aim lines, weapon icons, health bars)
- `worldToRadar()` converts CS2 world coordinates to radar pixel space using the per-map bounds/scale from `mapData`
- Textures are loaded via `DirectX::CreateWICTextureFromFile` (WIC, from DirectXTK)

### Memory reading
- `src/memory/dma.h/.cpp` — wraps MemProcFS (`vmmdll.h`) for all game memory reads
- `src/game/sdk.h` — `CGame`, `CPlayer`, `CWeapon`, `mapData` structs
- `src/game/sdk.cpp` — `CGame::update()` drives all per-frame memory reads
- `src/game/offsets.h/.cpp` — all CS2 struct offsets; auto-updated at startup, hardcoded defaults as fallback
- `src/game/updater.cpp` — fetches `client_dll.hpp` from a2x/cs2-dumper via WinINet (`fetchURL`), also does signature scanning via `sigscan.cpp`

### Key namespaces (declared in `gui.h`)
- `gui::` — all rendering, D3D lifecycle, resource loading
- `maps::` — `mapTextures`, `mapBounds`, radar sizing constants
- `icons::` — `iconTextures`, `iconWidths`, `iconHeights`, `scale`

### Texture loading
`LoadImageTexture()` in `dx11.cpp` — two overloads, one optionally outputs pixel dimensions. Used exclusively in `resources.cpp`.

### WinINet is already linked
`updater.cpp` and `dx11.cpp` both `#pragma comment(lib, "wininet")`. Any new HTTP fetching can reuse the existing `fetchURL()` static helper in `updater.cpp`.

## Offset update strategy
- Module-level offsets (`dwEntityList`, etc.) are resolved via signature scanning (`updater::sigscanOffsets()`)
- Class member offsets are fetched from `https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.hpp` and parsed with regex
- Hardcoded fallback values live in `src/game/offsets.cpp`
