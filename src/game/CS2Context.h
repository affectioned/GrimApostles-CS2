#pragma once
#include "DMA/DMA Thread.h"  // IGameContext + CTimer/Timer alias
#include "sdk.h"

class CS2Context : public IGameContext
{
public:
	CS2Context(CGame& game, std::mutex& gameMutex);

	bool Initialize(DMA_Connection* conn) override;
	void Tick(DMA_Connection* conn, std::chrono::steady_clock::time_point now) override;

	~CS2Context() override;

private:
	// ── Infrastructure ────────────────────────────────────────────────────────
	Process               m_Process;
	ScatterRead*          m_Scatter = nullptr;
	CGame&                m_Game;       // shared with render thread (mutex-guarded)
	std::mutex&           m_Mutex;
	std::unique_ptr<CGame> m_Local;     // private working copy; timers write here
	std::vector<Timer>    m_Timers;     // fired in registration order every tick

	// ── Persistent DMA traversal state ────────────────────────────────────────
	// These are intermediate addresses not visible to the render thread.
	uint64_t m_GlobalVarsPtr = 0;  // latched by t_ModulePtrs (5000 ms)
	uint64_t m_PlantedC4Ptr  = 0;  // latched by t_ModulePtrs (5000 ms)
	uint64_t m_EntityChunk   = 0;  // latched by t_EntityChain (500 ms)

	// ── Timer methods ─────────────────────────────────────────────────────────
	// Registered in dependency order — called sequentially via m_Timers.
	void t_ModulePtrs();      // 5000 ms — entityList, localController, globalVars, plantedC4Ptr
	void t_MapName();         // 1000 ms — map name string
	void t_EntityChain();     //  500 ms — 4-pass entity chain resolution
	void t_LocalPlayerPos();  //    8 ms — local player position
	void t_PlayerPositions(); //    8 ms — all player positions, health, angles, defusing
	void t_PlayerCtrl();      //  150 ms — all player controller fields (team, color, armor, …)
	void t_PlayerWeapons();   //  100 ms — active weapon definition IDs
	void t_PlayerNames();     // 5000 ms — player name strings
	void t_CarrierScan();     //  100 ms — C4 carrier detection (pre-plant only)
	void t_BombState();       //   16 ms — bomb flags, site, defuse timer, position
};
