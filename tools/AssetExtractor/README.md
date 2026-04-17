# AssetExtractor

Extracts CS2 radar images and weapon icons directly from the game's VPK files and outputs PNGs ready for use alongside GrimApostles CS2.

## Requirements

### 1. Python 3.10+
Must be on PATH. `run.bat` handles the rest (creates a venv, installs Python dependencies).

### 2. VRF Source2Viewer CLI
Used to decompile Source 2 compiled assets (`.vtex_c` textures and `.vsvg_c` SVGs) from the VPK.

1. Go to https://github.com/ValveResourceFormat/ValveResourceFormat/releases
2. Download **`cli-windows-x64.zip`** (self-contained, no .NET install required)
3. Extract the zip contents into:
   ```
   tools/AssetExtractor/vrf/
   ```
   The folder must contain `Source2Viewer-CLI.exe`. The script finds whichever `.exe` is present automatically.

The `vrf/` folder is gitignored — it is never committed to the repo.

## Usage

```
run.bat
```

On first run, `run.bat` creates a local `.venv\` and installs Python dependencies. Subsequent runs skip setup.

CS2 is located automatically via the Steam registry (including libraries on secondary drives).

**Pass arguments after `run.bat` the same as you would to the script:**

```
run.bat --out C:\path\to\bin\Release\textures --icon-size 256 -v
```

**Options:**

| Flag | Default | Description |
|---|---|---|
| `--cs2 <path>` | auto | CS2 install directory (the `Counter-Strike Global Offensive` folder) |
| `--out <path>` | `./textures` | Output root — produces `maps/` and `icons/` subfolders |
| `--icon-size <px>` | `128` | Weapon icon height in pixels |
| `--no-radars` | — | Skip radar image extraction |
| `--no-icons` | — | Skip weapon icon extraction |
| `-v` / `--verbose` | — | Print each file as it is extracted |
| `--list [filter]` | — | Dump VPK file paths (optional substring filter) then exit |

## Output structure

```
textures/
  maps/
    de_dust2_radar.png
    de_nuke_radar.png
    de_nuke_lower_radar.png
    ...
  icons/
    ak47.png
    awp.png
    knife_karambit.png
    ...
```

Place the `textures/` folder alongside `GrimApostles CS2.exe` at runtime. The radar overlay loads every `.png` in `textures/icons/` whose filename matches a known weapon — new icons are picked up automatically without any code changes.

## How it works

1. Passes `game/csgo/pak01_dir.vpk` directly to VRF CLI, which reads and decompiles assets in one step. (The `vpk` Python library is only used for `--list` mode.)
2. Extracts all `.vtex_c` radar textures from `panorama/images/overheadmaps/`; VRF outputs `.png` files.
3. Normalises filenames to `<mapname>_radar.png` (e.g. `de_dust2_radar_psd.png` → `de_dust2_radar.png`). All map variants are included — arena maps, night versions, etc.
4. Extracts `.vsvg_c` weapon icon SVGs from the equipment icons path and converts them to PNGs using `svglib`: renders white silhouettes onto black, then promotes luminance to alpha so icons are white-on-transparent.

## Adding to the Visual Studio solution

Right-click the solution → **Add** → **New Solution Folder** → name it `tools`. Then right-click that folder → **Add Existing Item** and select all files here. The `vrf/` and `.venv/` folders are local-only and not committed.
