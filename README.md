GrimApostles CS2 Radar
---
This is a simple DMA Radar for Counter-Strike 2 using IMGUI and DirectX11. It is written in C++. This is built using PCILeech to read memory using FPGA hardware (DMA Device). The PCILeech project is linked in the build section. This is a read-only radar - no writes. Check the Updates section for info about updating the radar yourself in case I don't have an up-to-date version in the release section.
<img width="1919" height="1079" alt="image" src="https://github.com/user-attachments/assets/59d97eab-4ca7-4bbc-bffd-ccfa8374f869" />

Building
---
To build this project, clone this repository and open the SLN file using Visual Studio. I am using the Visual Studio 2022 (v143) version. Make sure to set the output configuration to Release(x64) and build the project. The build and binary directories will be created. You will find your .exe in the "bin/release" directory.

Download the pcileech dependencies and place them along side your exe. You need: **vmm.dll, leechcore.dll, FTD3XX.dll** 

Found here [https://github.com/ufrisk/pcileech](https://github.com/ufrisk/pcileech/releases) (Windows Download)

Additonally, you will need a folder to hold the map images. You can find them in the release section, or use your own. Create a folder named "maps" and place your map images there **(.png files)**. The radar is hard coded to use the classic 1024x1024p CS2 map images for simplicity.

If you want to build using something else you will have to handle linking the dependencies.

Here's an example of what it should look like
<img width="1039" height="330" alt="image" src="https://github.com/user-attachments/assets/687b78ce-c335-4b3c-bca6-43d8b5a8a941" />


Updating
---
This project does not yet include auto updates. Use the a2x dumper to get new offsets for each update.

https://github.com/a2x/cs2-dumper

You will find [**offsets.hpp**](https://github.com/a2x/cs2-dumper/blob/main/output/offsets.hpp) and [**client_dll.hpp**](https://github.com/a2x/cs2-dumper/blob/main/output/client_dll.hpp) in the output folder. You will need to copy offsets in the **offsets::client_dll** namespace every update. The extra (Player)values you will find in the radar offset file are updated rarely and only need to be changed if an update breaks any features. You can search for these offsets in the formentioned **client_dll.hpp** file using CTRL-F
