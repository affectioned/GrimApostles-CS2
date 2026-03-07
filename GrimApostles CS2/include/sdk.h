#pragma once
#include "DMADevice.h"
#include "vec.h"
#include "offsets.h"
#include <unordered_map>

class mapData {
public:
	float xBound;
	float yBound;
	float scale;

	
	mapData(float x, float y, float s) : xBound(x), yBound(y), scale(s) {}

	mapData() : xBound(0.0f), yBound(0.0f), scale(0.0f) {}
};

class CWeapon {
public:
	uint32_t weaponHandle;
	uint64_t weaponEntry;
	uint64_t weaponController;
	uint16_t weaponID;
};

class CPlayer {
	public:
		uint64_t controller;
		uint64_t listEntry, listEntry2;
		uint32_t pawnAddr;
		uint64_t nameAddr;
		uint64_t pawn;
		char name[36];
		uint8_t teamID;
		uint32_t health;
		DWORD color;
		Vector2 eyeAngles;
		Vector3 position;
		//Active Weapon
		uint64_t activeWeapon;
		uint16_t activeWeaponID;
		//WeaponServices
		uint64_t weaponServices;
		int32_t weaponCount;
		uint64_t weaponData;
		CWeapon weapons[9];
};

class CGame {
	public:
		CPlayer localPlayer;
		char map[36];
		uintptr_t entityList;
		CPlayer players[64];
		void update();

		void getMap();
		void getLocalPlayer();
		void getLocalPawn();
		void getLocalTeam();
		void getLocalPos();
		void getLocalName();

		void getEntityList();
		void getPlayers();
		void getPlayerData();

		void getWeapons();
};
