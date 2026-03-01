GrimApostles CS2 Radar
---
A DMA-based radar for Counter-Strike 2 built with ImGui and DirectX 11, written in C++. Uses PCILeech/MemProcFS to read game memory via FPGA hardware (DMA device). **Read-only — no writes to game memory.**

<img width="1919" height="1079" alt="image" src="https://github.com/user-attachments/assets/daaf4ba4-f194-4cdb-a968-9b16c0140280" />

**If you have any questions, comments, or feedback, feel free to reach out on Discord @grimapostles**

---

> **Educational Disclaimer**
>
> This project is provided for **educational and research purposes only**. It demonstrates techniques such as DMA memory reading via FPGA hardware, DirectX 11 rendering with ImGui, and runtime offset resolution from public dumper output.
>
> The authors do not endorse or encourage use of this software in any online multiplayer environment. Usage in online games may violate the game's Terms of Service and could result in account penalties. **You are solely responsible for how you use this software.**
>
> No game memory is written. All data is read-only.

---

Building
---
Clone the repository and open the `.sln` file in **Visual Studio 2022 (v143)**. Set the configuration to **Release x64** and build. The output `.exe` will be in `bin/Release/`.

**PCILeech runtime dependencies** — download from the [PCILeech releases page](https://github.com/ufrisk/pcileech/releases) (Windows) and place alongside the `.exe`:
- `vmm.dll`
- `leechcore.dll`
- `FTD3XX.dll`

**Textures** — place the `textures/` folder (maps and weapon icons) alongside the `.exe`. You can find the source assets in the [assets](https://github.com/GrimApostles/GrimApostles-CS2/tree/main/GrimApostles%20CS2/assets) directory of this repo.

Example directory layout:

<img width="1059" height="292" alt="Screenshot 2026-02-25 005002" src="https://github.com/user-attachments/assets/3bc84ec6-995e-4dce-be40-df87005dc85b" />

Updating
---
Offsets are fetched automatically at startup from the a2x CS2 dumper output:

- [`offsets.hpp`](https://github.com/a2x/cs2-dumper/blob/main/output/offsets.hpp) — module-level pointer offsets (`dwEntityList`, `dwLocalPlayerController`, etc.)
- [`client_dll.hpp`](https://github.com/a2x/cs2-dumper/blob/main/output/client_dll.hpp) — class member offsets (`m_iHealth`, `m_angEyeAngles`, etc.)

If the fetch fails (no internet connection or the dumper is temporarily unavailable), the radar falls back to the last known good values hardcoded in `src/offsets.cpp`. To update those defaults manually, replace the values in that file with the current ones from the dumper.
