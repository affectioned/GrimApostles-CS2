#!/usr/bin/env python3
"""
CS2 Asset Extractor
Extracts radar images and weapon icons from CS2 VPK files and outputs PNGs
ready for use alongside GrimApostles CS2 (place the output textures/ folder
next to the .exe).

Requires VRF Source2Viewer-CLI.exe in tools/AssetExtractor/vrf/ — see README.md.
"""

import io
import re
import shutil
import subprocess
import sys
import tempfile
import argparse
from pathlib import Path


# ── Dependency checks ──────────────────────────────────────────────────────────

def _require(import_name, pip_name=None):
    import importlib
    try:
        return importlib.import_module(import_name)
    except ImportError:
        sys.exit(f"Missing dependency: pip install {pip_name or import_name}")

vpk      = _require("vpk")
svglib   = _require("svglib.svglib", pip_name="svglib")
renderPM = _require("reportlab.graphics.renderPM", pip_name="reportlab")
PIL      = _require("PIL", pip_name="Pillow")


# ── CS2 path detection ─────────────────────────────────────────────────────────

def _steam_library_paths(steam_root: Path) -> list[Path]:
    paths = [steam_root]
    vdf = steam_root / "steamapps" / "libraryfolders.vdf"
    if not vdf.exists():
        return paths
    text = vdf.read_text(encoding="utf-8", errors="ignore")
    for m in re.finditer(r'"path"\s+"([^"]+)"', text):
        p = Path(m.group(1).replace("\\\\", "\\"))
        if p not in paths:
            paths.append(p)
    return paths


def find_cs2() -> Path | None:
    steam_roots: list[Path] = []
    if sys.platform == "win32":
        try:
            import winreg
            key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                                 r"SOFTWARE\WOW6432Node\Valve\Steam")
            steam_roots.append(Path(winreg.QueryValueEx(key, "InstallPath")[0]))
            winreg.CloseKey(key)
        except Exception:
            pass
    steam_roots += [
        Path(r"C:\Program Files (x86)\Steam"),
        Path(r"C:\Program Files\Steam"),
        Path.home() / ".steam/steam",
    ]
    cs2_rel = Path("steamapps") / "common" / "Counter-Strike Global Offensive"
    for root in steam_roots:
        for lib in _steam_library_paths(root):
            candidate = lib / cs2_rel
            if candidate.exists():
                return candidate
    return None


# ── VRF CLI ────────────────────────────────────────────────────────────────────

def _find_cli() -> Path:
    vrf_dir = Path(__file__).parent / "vrf"
    exes = list(vrf_dir.glob("*.exe")) if vrf_dir.exists() else []
    if exes:
        return exes[0]
    sys.exit(
        f"VRF CLI not found in: {vrf_dir}\n"
        "Download cli-windows-x64.zip from:\n"
        "  https://github.com/ValveResourceFormat/ValveResourceFormat/releases\n"
        "Extract the contents into: tools/AssetExtractor/vrf/"
    )


def _vrf(cli: Path, vpk_path: Path, out_dir: Path,
         extension: str, vpk_prefix: str, verbose: bool) -> None:
    """
    Run VRF CLI to decompile files from a VPK.
    -i  input VPK
    -o  output directory
    -d  decompile/export mode
    --vpk_extensions  filter by extension inside the VPK
    --vpk_filepath    filter by path prefix inside the VPK
    """
    cmd = [str(cli), "-i", str(vpk_path), "-o", str(out_dir), "-d",
           "--vpk_extensions", extension,
           "--vpk_filepath",   vpk_prefix]
    if verbose:
        print(f"  [vrf]    {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.stdout.strip():
        print(result.stdout.rstrip())
    if result.stderr.strip():
        print(result.stderr.rstrip(), file=sys.stderr)
    if result.returncode != 0:
        raise RuntimeError(f"VRF CLI exited {result.returncode}")
    if verbose:
        all_files = list(out_dir.rglob("*"))
        print(f"  [vrf]    output dir contains {len(all_files)} file(s)")
        for f in all_files[:20]:
            print(f"           {f.relative_to(out_dir)}")


# ── SVG bytes → PNG bytes ──────────────────────────────────────────────────────
#
# CS2 weapon icons are white silhouettes on a transparent background.
# svglib/renderPM renders onto an opaque background, so we render onto
# black, then promote luminance to the alpha channel:
#   black pixel (background) → luminance 0   → fully transparent
#   white pixel (icon)       → luminance 255 → fully opaque

def _svg_to_png(svg_bytes: bytes, height: int) -> bytes:
    from PIL import Image

    drawing = svglib.svg2rlg(io.BytesIO(svg_bytes))
    if drawing is None:
        raise ValueError("svglib could not parse SVG")
    if drawing.height > 0:
        scale = height / drawing.height
        drawing.width  *= scale
        drawing.height *= scale
        drawing.transform = (scale, 0, 0, scale, 0, 0)

    raw = renderPM.drawToString(drawing, fmt="PNG", bg=0x000000)

    img = Image.open(io.BytesIO(raw)).convert("RGBA")
    img.putalpha(img.convert("L"))

    buf = io.BytesIO()
    img.save(buf, "PNG")
    return buf.getvalue()


# ── Radar extraction ───────────────────────────────────────────────────────────

_RADAR_VPK_PREFIX = "panorama/images/overheadmaps/"

def extract_radars(pak, cli: Path, vpk_path: Path, out_dir: Path, verbose: bool) -> int:
    out_dir.mkdir(parents=True, exist_ok=True)
    count = 0

    with tempfile.TemporaryDirectory() as tmp:
        tmp_out = Path(tmp) / "out"
        tmp_out.mkdir()

        _vrf(cli, vpk_path, tmp_out, "vtex_c", _RADAR_VPK_PREFIX, verbose)

        # VRF preserves VPK path structure under tmp_out, so use rglob
        candidates = list(tmp_out.rglob("*.png")) + list(tmp_out.rglob("*.tga"))
        if not candidates:
            print("  [radar]  WARNING: VRF produced no image output for radar textures",
                  file=sys.stderr)

        for img_file in candidates:
            stem = img_file.stem
            if "_radar" not in stem:
                continue
            # de_dust2_radar_psd → de_dust2_radar.png
            out_name = re.sub(r"_radar.*$", "_radar", stem) + ".png"
            shutil.copy2(img_file, out_dir / out_name)
            count += 1
            if verbose:
                print(f"  [radar]  {img_file.name}  ->  {out_name}")

    return count


# ── Weapon icon extraction ─────────────────────────────────────────────────────

_ICON_VPK_PREFIX = "panorama/images/icons/equipment/"

_NAME_OVERRIDES: dict[str, str] = {
    "p2000": "hkp2000",
}

_ALIASES: dict[str, str] = {
    "m4a1_silencer_off": "m4a1_silencer",
    "usp_silencer_off":  "usp_silencer",
}

def extract_icons(pak, cli: Path, vpk_path: Path, out_dir: Path, icon_height: int, verbose: bool) -> int:
    out_dir.mkdir(parents=True, exist_ok=True)
    extracted: dict[str, Path] = {}

    with tempfile.TemporaryDirectory() as tmp:
        tmp_out = Path(tmp) / "out"
        tmp_out.mkdir()

        _vrf(cli, vpk_path, tmp_out, "vsvg_c", _ICON_VPK_PREFIX, verbose)

        for svg in tmp_out.rglob("*.svg"):
            stem     = svg.stem
            out_stem = _NAME_OVERRIDES.get(stem, stem)
            out_path = out_dir / f"{out_stem}.png"
            try:
                png_bytes = _svg_to_png(svg.read_bytes(), icon_height)
                out_path.write_bytes(png_bytes)
                extracted[out_stem] = out_path
                if verbose:
                    print(f"  [icon]   {svg.name}  ->  {out_stem}.png")
            except Exception as e:
                print(f"  [icon]   FAILED {svg.name}: {e}", file=sys.stderr)

    for alias, source in _ALIASES.items():
        if alias not in extracted and source in extracted:
            dst = out_dir / f"{alias}.png"
            shutil.copy2(extracted[source], dst)
            extracted[alias] = dst
            if verbose:
                print(f"  [icon]   {alias}.png  (copy of {source}.png)")

    return len(extracted)


# ── Entry point ────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Extract CS2 radar images and weapon icons from game VPK files."
    )
    parser.add_argument("--cs2",       metavar="PATH", help="CS2 install directory (auto-detected if omitted)")
    parser.add_argument("--out",       metavar="PATH", default="textures", help="Output root (default: ./textures)")
    parser.add_argument("--icon-size", metavar="PX",   type=int, default=128, help="Weapon icon height in pixels (default: 128)")
    parser.add_argument("--no-radars", action="store_true", help="Skip radar extraction")
    parser.add_argument("--no-icons",  action="store_true", help="Skip weapon icon extraction")
    parser.add_argument("-v", "--verbose", action="store_true", help="Print each file as it is extracted")
    parser.add_argument("--list", metavar="FILTER", nargs="?", const="",
                        help="Dump VPK paths (optionally filtered by substring) then exit")
    args = parser.parse_args()

    cs2 = Path(args.cs2) if args.cs2 else find_cs2()
    if not cs2 or not cs2.exists():
        sys.exit("Could not find CS2. Pass --cs2 <path> pointing to the 'Counter-Strike Global Offensive' folder.")
    print(f"CS2 path : {cs2}")

    vpk_path = cs2 / "game" / "csgo" / "pak01_dir.vpk"
    if not vpk_path.exists():
        sys.exit(f"VPK not found: {vpk_path}")
    print(f"VPK      : {vpk_path.name}")

    if args.list is not None:
        pak  = vpk.open(str(vpk_path))
        filt = args.list.lower()
        for path in sorted(pak):
            if not filt or filt in path.lower():
                print(path)
        return

    cli      = _find_cli()
    out_root = Path(args.out)
    print(f"Output   : {out_root.resolve()}\n")

    if not args.no_radars:
        pak = vpk.open(str(vpk_path))
        n = extract_radars(pak, cli, vpk_path, out_root / "maps", args.verbose)
        print(f"Radars   : {n} extracted  ->  {out_root / 'maps'}")

    if not args.no_icons:
        pak = vpk.open(str(vpk_path))
        n = extract_icons(pak, cli, vpk_path, out_root / "icons", args.icon_size, args.verbose)
        print(f"Icons    : {n} extracted  ->  {out_root / 'icons'}")

    print("\nDone.")


if __name__ == "__main__":
    main()
