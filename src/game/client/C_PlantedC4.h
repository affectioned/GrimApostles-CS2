#pragma once
#include "vec.h"
#include <chrono>
#include <cstdint>

struct C_PlantedC4 {
	uint64_t entity    = 0;
	uint64_t sceneNode = 0;
	Vector3  position  = {};

	bool    isTicking      = false;
	bool    isBeingDefused = false;
	bool    hasExploded    = false;
	bool    hasDefused     = false;
	int32_t site           = -1;

	bool isCarried   = false;
	int  carrierSlot = -1;
	int  planterSlot = -1;

	float c4Blow = 0.0f; // m_flC4Blow: absolute game time of detonation; used as bomb identity

	using Clock = std::chrono::steady_clock;
	Clock::time_point plantTime    = {};
	bool              plantTimeSet = false;

	float timeRemaining() const {
		if (!isTicking || !plantTimeSet) return 40.0f;
		float elapsed = std::chrono::duration<float>(Clock::now() - plantTime).count();
		return std::max(0.0f, 40.0f - elapsed);
	}
};
