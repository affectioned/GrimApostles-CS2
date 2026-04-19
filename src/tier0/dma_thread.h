#pragma once
#include "sdk.h"
#include <mutex>
#include <atomic>

// Single DMA thread entry point — handles connection then drives the update loop.
// Passed as the body of a std::thread in main.cpp; loops until run is set to false.
void DMAThreadMain(CGame& game, std::mutex& gameMutex, const std::atomic<bool>& run);
