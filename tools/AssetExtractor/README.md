# AssetExtractor

Extracts CS2 radar images and weapon icons directly from the game's VPK files and outputs PNGs ready for use alongside GrimApostles CS2.

## Requirements

### 1. Python 3.10+
Must be on PATH. `run.bat` handles the rest (creates a venv, installs Python dependencies).

### 2. VRF CLI
Used to decompile Source 2 compiled assets (`.vtex_c` textures and `.vsvg_c` SVGs) from the VPK.

1. Go to https://github.com/ValveResourceFormat/ValveResourceFormat/releases
2. Download **`cli-windows-x64.zip`** (self-contained, no .NET install required)
3. Extract the zip contents into:
   ```
   tools/AssetExtractor/vrf/
   ```
   The script will find whichever `.exe` is in that folder automatically.

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

Place the `textures/` folder alongside `GrimApostles CS2.exe` at runtime.

## How it works

1. Opens `game/csgo/pak01_dir.vpk` using the `vpk` library
2. Extracts `.vtex_c` (compiled textures) and `.vsvg_c` (compiled SVGs) to a temporary directory
3. Runs `Decompiler.exe` on the temp directory — outputs `.png` for textures, `.svg` for icons
4. Renames radar PNGs to the naming convention expected by `resources.cpp`
5. Converts weapon icon SVGs to PNGs using `svglib`

## Adding to the Visual Studio solution

Right-click the solution → **Add** → **New Solution Folder** → name it `tools`. Then right-click that folder → **Add Existing Item** and select all files here. The `vrf/` and `.venv/` folders are local-only and not committed.
