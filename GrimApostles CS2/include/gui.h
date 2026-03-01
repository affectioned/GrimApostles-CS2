#pragma once
#include "DMADevice.h"
#include "sdk.h"
#include "./ImGUI/imgui.h"
#include "./ImGUI/imgui_impl_win32.h"
#include "./ImGUI/imgui_impl_dx11.h"
#include "./ImGUI/imgui_internal.h"
#include "./DirectX11/WICTextureLoader.h"
#include <d3d11.h>
#pragma comment(lib, "DirectXTK")



extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace gui {
	// Data
	extern ID3D11Device* g_pd3dDevice;
	extern ID3D11DeviceContext* g_pd3dDeviceContext;
	extern IDXGISwapChain* g_pSwapChain;
	extern UINT g_ResizeWidth, g_ResizeHeight;
	extern ID3D11RenderTargetView* g_mainRenderTargetView;
	extern bool exitRequested;
	extern WNDCLASSEXW wc;
	extern HWND hwnd;

	// Lifecycle stages (called from main)
	void CreateAppWindow();
	bool InitD3D();
	void ShowAppWindow();
	void InitImGui();
	void RunLoop();
	void Cleanup();

	// D3D helpers (dx11.cpp)
	bool CreateDeviceD3D(HWND);
	void CleanupDeviceD3D();
	void CreateRenderTarget();
	void CleanupRenderTarget();
	ID3D11ShaderResourceView* LoadImageTexture(ID3D11Device* device, const wchar_t* filename);
	ID3D11ShaderResourceView* LoadImageTexture(ID3D11Device* device, const wchar_t* filename, int* outWidth, int* outHeight);
	LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);

	// UI buttons (gui.cpp)
	void ConnectButton();
	void ExitButton();

	// Game rendering (render.cpp)
	void gameLoop(CGame);
	void renderMap(ID3D11ShaderResourceView*);
	void renderPlayers(CGame);

	// Resource loading (resources.cpp)
	void loadMapBounds();
	void loadTextures();

	// Utilities (render.cpp)
	ImU32 setColor(DWORD color, float opacity);
	void worldToRadar(float& x, float& y, CGame);
	float setOpacity(float, float, CGame);
}

namespace maps {
	extern float radarSize;
	//values for knowing if player is 1st or 2nd floor on multi level maps
	extern float vertigoZBound;
	extern float nukeZBound;
	extern std::unordered_map<std::string, ID3D11ShaderResourceView*> mapTextures;
	extern std::unordered_map<std::string, mapData> mapBounds;
}

namespace icons {
	extern float scale;
	extern std::unordered_map<int, ID3D11ShaderResourceView*> iconTextures;
	extern std::unordered_map<int, int> iconWidths;
	extern std::unordered_map<int, int> iconHeights;
}
