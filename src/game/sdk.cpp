#include "pch.h"
#include "sdk.h"

// All DMA reads are driven by CS2Context timer methods (CS2Context.cpp).
// sdk.cpp only contains CGame utility methods that operate on its own state.

void CGame::applyCache() {
	for (int i = 0; i < MAX_ENTITIES; i++) {
		if (players[i].pawnBase) {
			playerCache[i]    = players[i];
			playerCacheAge[i] = 0;
		} else if (playerCacheAge[i] < kMaxCacheAge) {
			players[i] = playerCache[i];
			playerCacheAge[i]++;
		} else {
			playerCache[i]    = {};
			playerCacheAge[i] = 0;
		}
	}
}
