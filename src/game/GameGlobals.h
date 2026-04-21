#pragma once

// Globals set by CS2Context::Initialize before the update loop starts.
// Accessible from sdk.cpp and entity files via this header.
extern ScatterRead*      g_Scatter;
extern uintptr_t         g_ClientBase;
extern std::atomic<bool> g_Connected;
