#include "pch.h"
#include "CS2Context.h"
#include "GameModules.h"
#include "updater.h"

using namespace std::chrono_literals;

// ── Globals (set in Initialize, used by entity Read() methods) ────────────────
ScatterRead*      g_Scatter    = nullptr;
uintptr_t         g_ClientBase = 0;
std::atomic<bool> g_Connected{ false };

// ── CS2Context lifecycle ───────────────────────────────────────────────────────

CS2Context::CS2Context(CGame& game, std::mutex& gameMutex)
	: m_Game(game), m_Mutex(gameMutex) {}

CS2Context::~CS2Context()
{
	g_Connected.store(false);
	g_Scatter    = nullptr;
	g_ClientBase = 0;
	delete m_Scatter;
}

bool CS2Context::Initialize(DMA_Connection* conn)
{
	if (!m_Process.GetProcessInfo(GameModules::ProcessName, GameModules::ModuleList(), conn))
		return false;

	g_ClientBase = m_Process.GetModuleBase(GameModules::ClientDll);
	if (!g_ClientBase) {
		Log::Error("[CS2Context]: {} base not found", GameModules::ClientDll);
		return false;
	}
	Log::Info("[CS2Context]: {} at 0x{:X}", GameModules::ClientDll, g_ClientBase);

	m_Scatter = new ScatterRead(conn->GetHandle(), m_Process.GetPID());
	g_Scatter  = m_Scatter;
	m_Local    = std::make_unique<CGame>();

	updater::sigscanOffsets(conn, &m_Process);

	// Register timers in dependency order.
	// CTimer::m_LastExecutionTime defaults to epoch, so every timer fires on
	// the very first tick — all state is populated before the render thread
	// takes its first snapshot.
	m_Timers.emplace_back(5000ms, [this]{ t_ModulePtrs();      });  // seeds entity list + ptrs
	m_Timers.emplace_back(1000ms, [this]{ t_MapName();          });  // depends on m_GlobalVarsPtr
	m_Timers.emplace_back( 500ms, [this]{ t_EntityChain();      });  // depends on entityList
	m_Timers.emplace_back(   8ms, [this]{ t_LocalPlayerPos();   });  // depends on localPlayer.pawnBase
	m_Timers.emplace_back(   8ms, [this]{ t_PlayerPositions();  });  // depends on pawnBase
	m_Timers.emplace_back( 150ms, [this]{ t_PlayerCtrl();       });  // depends on controllerBase
	m_Timers.emplace_back( 100ms, [this]{ t_PlayerWeapons();    });  // depends on activeWeapon
	m_Timers.emplace_back(5000ms, [this]{ t_PlayerNames();      });  // depends on ctrl.nameAddr
	m_Timers.emplace_back( 100ms, [this]{ t_CarrierScan();      });  // depends on weaponServicesPtr
	m_Timers.emplace_back(  16ms, [this]{ t_BombState();        });  // depends on m_PlantedC4Ptr

	g_Connected.store(true, std::memory_order_release);
	Log::Info("[CS2Context]: Initialized - {} timers registered", m_Timers.size());
	return true;
}

void CS2Context::Tick(DMA_Connection* /*conn*/,
                      std::chrono::steady_clock::time_point now)
{
	for (auto& t : m_Timers)
		t.Tick(now);

	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Game = *m_Local;
}

// ── t_ModulePtrs — 5000 ms ────────────────────────────────────────────────────
// Reads module-level global pointers from client.dll. These are static addresses
// that rarely move mid-session; 5 s refresh is indistinguishable from immediate.

void CS2Context::t_ModulePtrs()
{
	uint64_t base = g_ClientBase;
	uint64_t newEntityList = 0, newController = 0, newPawn = 0,
	         newGlobalVars = 0, newC4Ptr = 0;

	g_Scatter->Add(base + client_dll::dwGlobalVars,            &newGlobalVars);
	g_Scatter->Add(base + client_dll::dwLocalPlayerController, &newController);
	g_Scatter->Add(base + client_dll::dwLocalPlayerPawn,       &newPawn);
	g_Scatter->Add(base + client_dll::dwEntityList,            &newEntityList);
	g_Scatter->Add(base + client_dll::dwPlantedC4,             &newC4Ptr);
	g_Scatter->Execute();
	g_Scatter->Clear();

	if (newGlobalVars) m_GlobalVarsPtr = newGlobalVars;
	if (newC4Ptr)      m_PlantedC4Ptr  = newC4Ptr;

	static uint64_t prevEL = 0, prevCtrl = 0;

	if (newEntityList) {
		if (newEntityList != prevEL) {
			Log::Info("[SDK]: Entity list -> 0x{:X}", newEntityList);
			prevEL = newEntityList;
		}
		m_Local->entityList = newEntityList;
	}
	if (newController) {
		if (newController != prevCtrl) {
			Log::Info("[SDK]: Local controller -> 0x{:X}", newController);
			prevCtrl = newController;
		}
		m_Local->localPlayer.controllerBase = newController;
	}
	if (newPawn)
		m_Local->localPlayer.pawnBase = newPawn;
}

// ── t_MapName — 1000 ms ───────────────────────────────────────────────────────
// Reads the map name string via globalVars → mapPtr → 32-byte string.
// Changes only between rounds.

void CS2Context::t_MapName()
{
	if (!m_GlobalVarsPtr) return;

	uint64_t mapPtr = 0;
	g_Scatter->Add(m_GlobalVarsPtr + 0x0188, &mapPtr);
	g_Scatter->Execute();
	g_Scatter->Clear();

	if (!mapPtr) return;

	char buf[32] = {};
	g_Scatter->AddRaw(mapPtr, sizeof(buf), buf);
	g_Scatter->Execute();
	g_Scatter->Clear();

	buf[sizeof(buf) - 1] = '\0';
	bool valid = (buf[0] != '\0');
	for (size_t i = 0; valid && i < sizeof(buf) && buf[i]; i++) {
		unsigned char c = (unsigned char)buf[i];
		if (c < 0x20 || c > 0x7E) valid = false;
	}
	if (valid && strncmp(m_Local->mapName, buf, sizeof(m_Local->mapName)) != 0) {
		Log::Info("[SDK]: Map -> {}", buf);
		memcpy(m_Local->mapName, buf, sizeof(m_Local->mapName));
	}
}
