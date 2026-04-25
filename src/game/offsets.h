#pragma once
#include <cstddef>

// ─── offsets.hpp  (cs2_dumper::offsets::*) ────────────────────────────────────
// Module-level pointer offsets. Fetched at startup by updater::fetchOffsets().

namespace client_dll {
	extern std::ptrdiff_t dwEntityList;
	extern std::ptrdiff_t dwLocalPlayerController;
	extern std::ptrdiff_t dwLocalPlayerPawn;
	extern std::ptrdiff_t dwGlobalVars;
	extern std::ptrdiff_t dwPlantedC4;
}

// ─── client_dll.hpp  (cs2_dumper::schemas::client_dll::*) ────────────────────
// Class member offsets. Fetched at startup by updater::fetchClassOffsets().

namespace client_dll {
	// Parent: None
	// Field count: 83
	namespace C_BaseEntity {
		constexpr std::ptrdiff_t m_CBodyComponent = 0x30; // CBodyComponent*
		constexpr std::ptrdiff_t m_NetworkTransmitComponent = 0x38; // CNetworkTransmitComponent
		constexpr std::ptrdiff_t m_nLastThinkTick = 0x328; // GameTick_t
		constexpr std::ptrdiff_t m_pGameSceneNode = 0x330; // CGameSceneNode*
		constexpr std::ptrdiff_t m_pRenderComponent = 0x338; // CRenderComponent*
		constexpr std::ptrdiff_t m_pCollision = 0x340; // CCollisionProperty*
		constexpr std::ptrdiff_t m_iMaxHealth = 0x348; // int32
		constexpr std::ptrdiff_t m_iHealth = 0x34C; // int32
		constexpr std::ptrdiff_t m_flDamageAccumulator = 0x350; // float32
		constexpr std::ptrdiff_t m_lifeState = 0x354; // uint8
		constexpr std::ptrdiff_t m_bTakesDamage = 0x355; // bool
		constexpr std::ptrdiff_t m_nTakeDamageFlags = 0x358; // TakeDamageFlags_t
		constexpr std::ptrdiff_t m_nPlatformType = 0x360; // EntityPlatformTypes_t
		constexpr std::ptrdiff_t m_ubInterpolationFrame = 0x361; // uint8
		constexpr std::ptrdiff_t m_hSceneObjectController = 0x364; // CHandle<C_BaseEntity>
		constexpr std::ptrdiff_t m_nNoInterpolationTick = 0x368; // int32
		constexpr std::ptrdiff_t m_nVisibilityNoInterpolationTick = 0x36C; // int32
		constexpr std::ptrdiff_t m_flProxyRandomValue = 0x370; // float32
		constexpr std::ptrdiff_t m_iEFlags = 0x374; // int32
		constexpr std::ptrdiff_t m_nWaterType = 0x378; // uint8
		constexpr std::ptrdiff_t m_bInterpolateEvenWithNoModel = 0x379; // bool
		constexpr std::ptrdiff_t m_bPredictionEligible = 0x37A; // bool
		constexpr std::ptrdiff_t m_bApplyLayerMatchIDToModel = 0x37B; // bool
		constexpr std::ptrdiff_t m_tokLayerMatchID = 0x37C; // CUtlStringToken
		constexpr std::ptrdiff_t m_nSubclassID = 0x380; // CUtlStringToken
		constexpr std::ptrdiff_t m_nSimulationTick = 0x390; // int32
		constexpr std::ptrdiff_t m_iCurrentThinkContext = 0x394; // int32
		constexpr std::ptrdiff_t m_aThinkFunctions = 0x398; // CUtlVector<thinkfunc_t>
		constexpr std::ptrdiff_t m_bDisabledContextThinks = 0x3B0; // bool
		constexpr std::ptrdiff_t m_flAnimTime = 0x3B4; // float32
		constexpr std::ptrdiff_t m_flSimulationTime = 0x3B8; // float32
		constexpr std::ptrdiff_t m_nSceneObjectOverrideFlags = 0x3BC; // uint8
		constexpr std::ptrdiff_t m_bHasSuccessfullyInterpolated = 0x3BD; // bool
		constexpr std::ptrdiff_t m_bHasAddedVarsToInterpolation = 0x3BE; // bool
		constexpr std::ptrdiff_t m_bRenderEvenWhenNotSuccessfullyInterpolated = 0x3BF; // bool
		constexpr std::ptrdiff_t m_nInterpolationLatchDirtyFlags = 0x3C0; // int32[2]
		constexpr std::ptrdiff_t m_ListEntry = 0x3C8; // uint16[11]
		constexpr std::ptrdiff_t m_flCreateTime = 0x3E0; // GameTime_t
		constexpr std::ptrdiff_t m_flSpeed = 0x3E4; // float32
		constexpr std::ptrdiff_t m_EntClientFlags = 0x3E8; // uint16
		constexpr std::ptrdiff_t m_bClientSideRagdoll = 0x3EA; // bool
		constexpr std::ptrdiff_t m_iTeamNum = 0x3EB; // uint8
		constexpr std::ptrdiff_t m_spawnflags = 0x3EC; // uint32
		constexpr std::ptrdiff_t m_nNextThinkTick = 0x3F0; // GameTick_t
		constexpr std::ptrdiff_t m_fFlags = 0x3F8; // uint32
		constexpr std::ptrdiff_t m_vecAbsVelocity = 0x3FC; // Vector
		constexpr std::ptrdiff_t m_vecServerVelocity = 0x408; // CNetworkVelocityVector
		constexpr std::ptrdiff_t m_vecVelocity = 0x430; // CNetworkVelocityVector
		constexpr std::ptrdiff_t m_vecBaseVelocity = 0x510; // Vector
		constexpr std::ptrdiff_t m_hEffectEntity = 0x51C; // CHandle<C_BaseEntity>
		constexpr std::ptrdiff_t m_hOwnerEntity = 0x520; // CHandle<C_BaseEntity>
		constexpr std::ptrdiff_t m_MoveCollide = 0x524; // MoveCollide_t
		constexpr std::ptrdiff_t m_MoveType = 0x525; // MoveType_t
		constexpr std::ptrdiff_t m_nActualMoveType = 0x526; // MoveType_t
		constexpr std::ptrdiff_t m_flWaterLevel = 0x528; // float32
		constexpr std::ptrdiff_t m_fEffects = 0x52C; // uint32
		constexpr std::ptrdiff_t m_hGroundEntity = 0x530; // CHandle<C_BaseEntity>
		constexpr std::ptrdiff_t m_nGroundBodyIndex = 0x534; // int32
		constexpr std::ptrdiff_t m_flFriction = 0x538; // float32
		constexpr std::ptrdiff_t m_flElasticity = 0x53C; // float32
		constexpr std::ptrdiff_t m_flGravityScale = 0x540; // float32
		constexpr std::ptrdiff_t m_flTimeScale = 0x544; // float32
		constexpr std::ptrdiff_t m_bAnimatedEveryTick = 0x548; // bool
		constexpr std::ptrdiff_t m_bGravityDisabled = 0x549; // bool
		constexpr std::ptrdiff_t m_flNavIgnoreUntilTime = 0x54C; // GameTime_t
		constexpr std::ptrdiff_t m_hThink = 0x550; // uint16
		constexpr std::ptrdiff_t m_fBBoxVisFlags = 0x560; // uint8
		constexpr std::ptrdiff_t m_flActualGravityScale = 0x564; // float32
		constexpr std::ptrdiff_t m_bGravityActuallyDisabled = 0x568; // bool
		constexpr std::ptrdiff_t m_bPredictable = 0x569; // bool
		constexpr std::ptrdiff_t m_bRenderWithViewModels = 0x56A; // bool
		constexpr std::ptrdiff_t m_nFirstPredictableCommand = 0x56C; // int32
		constexpr std::ptrdiff_t m_nLastPredictableCommand = 0x570; // int32
		constexpr std::ptrdiff_t m_hOldMoveParent = 0x574; // CHandle<C_BaseEntity>
		constexpr std::ptrdiff_t m_Particles = 0x578; // CParticleProperty
		constexpr std::ptrdiff_t m_vecAngVelocity = 0x5A8; // QAngle
		constexpr std::ptrdiff_t m_DataChangeEventRef = 0x5B4; // int32
		constexpr std::ptrdiff_t m_dependencies = 0x5B8; // CUtlVector<CEntityHandle>
		constexpr std::ptrdiff_t m_nCreationTick = 0x5D0; // int32
		constexpr std::ptrdiff_t m_bAnimTimeChanged = 0x5E1; // bool
		constexpr std::ptrdiff_t m_bSimulationTimeChanged = 0x5E2; // bool
		constexpr std::ptrdiff_t m_sUniqueHammerID = 0x5F0; // CUtlString
		constexpr std::ptrdiff_t m_nBloodType = 0x5F8; // BloodType
	}

	// Parent: None
	// Field count: 35
	namespace CGameSceneNode {
		constexpr std::ptrdiff_t m_nodeToWorld = 0x10; // CTransformWS
		constexpr std::ptrdiff_t m_pOwner = 0x30; // CEntityInstance*
		constexpr std::ptrdiff_t m_pParent = 0x38; // CGameSceneNode*
		constexpr std::ptrdiff_t m_pChild = 0x40; // CGameSceneNode*
		constexpr std::ptrdiff_t m_pNextSibling = 0x48; // CGameSceneNode*
		constexpr std::ptrdiff_t m_hParent = 0x70; // CGameSceneNodeHandle
		constexpr std::ptrdiff_t m_vecOrigin = 0x80; // CNetworkOriginCellCoordQuantizedVector
		constexpr std::ptrdiff_t m_angRotation = 0xB8; // QAngle
		constexpr std::ptrdiff_t m_flScale = 0xC4; // float32
		constexpr std::ptrdiff_t m_vecAbsOrigin = 0xC8; // VectorWS
		constexpr std::ptrdiff_t m_angAbsRotation = 0xD4; // QAngle
		constexpr std::ptrdiff_t m_flAbsScale = 0xE0; // float32
		constexpr std::ptrdiff_t m_vecWrappedLocalOrigin = 0xE4; // Vector
		constexpr std::ptrdiff_t m_angWrappedLocalRotation = 0xF0; // QAngle
		constexpr std::ptrdiff_t m_flWrappedScale = 0xFC; // float32
		constexpr std::ptrdiff_t m_nParentAttachmentOrBone = 0x100; // int16
		constexpr std::ptrdiff_t m_bDebugAbsOriginChanges = 0x102; // bool
		constexpr std::ptrdiff_t m_bDormant = 0x103; // bool
		constexpr std::ptrdiff_t m_bForceParentToBeNetworked = 0x104; // bool
		constexpr std::ptrdiff_t m_bDirtyHierarchy = 0x0; // bitfield:1
		constexpr std::ptrdiff_t m_bDirtyBoneMergeInfo = 0x0; // bitfield:1
		constexpr std::ptrdiff_t m_bNetworkedPositionChanged = 0x0; // bitfield:1
		constexpr std::ptrdiff_t m_bNetworkedAnglesChanged = 0x0; // bitfield:1
		constexpr std::ptrdiff_t m_bNetworkedScaleChanged = 0x0; // bitfield:1
		constexpr std::ptrdiff_t m_bWillBeCallingPostDataUpdate = 0x0; // bitfield:1
		constexpr std::ptrdiff_t m_bBoneMergeFlex = 0x0; // bitfield:1
		constexpr std::ptrdiff_t m_nLatchAbsOrigin = 0x0; // bitfield:2
		constexpr std::ptrdiff_t m_bDirtyBoneMergeBoneToRoot = 0x0; // bitfield:1
		constexpr std::ptrdiff_t m_nHierarchicalDepth = 0x107; // uint8
		constexpr std::ptrdiff_t m_nHierarchyType = 0x108; // uint8
		constexpr std::ptrdiff_t m_nDoNotSetAnimTimeInInvalidatePhysicsCount = 0x109; // uint8
		constexpr std::ptrdiff_t m_name = 0x10C; // CUtlStringToken
		constexpr std::ptrdiff_t m_hierarchyAttachName = 0x120; // CUtlStringToken
		constexpr std::ptrdiff_t m_flClientLocalScale = 0x124; // float32
		constexpr std::ptrdiff_t m_vRenderOrigin = 0x128; // Vector
	}

	// Parent: CBaseAnimGraph
	// Field count: 29
	namespace C_PlantedC4 {
		constexpr std::ptrdiff_t m_bBombTicking = 0x1160; // bool
		constexpr std::ptrdiff_t m_nBombSite = 0x1164; // int32
		constexpr std::ptrdiff_t m_nSourceSoundscapeHash = 0x1168; // int32
		constexpr std::ptrdiff_t m_entitySpottedState = 0x1170; // EntitySpottedState_t
		constexpr std::ptrdiff_t m_flNextGlow = 0x1188; // GameTime_t
		constexpr std::ptrdiff_t m_flNextBeep = 0x118C; // GameTime_t
		constexpr std::ptrdiff_t m_flC4Blow = 0x1190; // GameTime_t
		constexpr std::ptrdiff_t m_bCannotBeDefused = 0x1194; // bool
		constexpr std::ptrdiff_t m_bHasExploded = 0x1195; // bool
		constexpr std::ptrdiff_t m_flTimerLength = 0x1198; // float32
		constexpr std::ptrdiff_t m_bBeingDefused = 0x119C; // bool
		constexpr std::ptrdiff_t m_bTriggerWarning = 0x11A0; // float32
		constexpr std::ptrdiff_t m_bExplodeWarning = 0x11A4; // float32
		constexpr std::ptrdiff_t m_bC4Activated = 0x11A8; // bool
		constexpr std::ptrdiff_t m_bTenSecWarning = 0x11A9; // bool
		constexpr std::ptrdiff_t m_flDefuseLength = 0x11AC; // float32
		constexpr std::ptrdiff_t m_flDefuseCountDown = 0x11B0; // GameTime_t
		constexpr std::ptrdiff_t m_bBombDefused = 0x11B4; // bool
		constexpr std::ptrdiff_t m_hBombDefuser = 0x11B8; // CHandle<C_CSPlayerPawn>
		constexpr std::ptrdiff_t m_AttributeManager = 0x11C0; // C_AttributeContainer
		constexpr std::ptrdiff_t m_hDefuserMultimeter = 0x1690; // CHandle<C_Multimeter>
		constexpr std::ptrdiff_t m_flNextRadarFlashTime = 0x1694; // GameTime_t
		constexpr std::ptrdiff_t m_bRadarFlash = 0x1698; // bool
		constexpr std::ptrdiff_t m_pBombDefuser = 0x169C; // CHandle<C_CSPlayerPawn>
		constexpr std::ptrdiff_t m_fLastDefuseTime = 0x16A0; // GameTime_t
		constexpr std::ptrdiff_t m_pPredictionOwner = 0x16A8; // CBasePlayerController*
		constexpr std::ptrdiff_t m_vecC4ExplodeSpectatePos = 0x16B0; // Vector
		constexpr std::ptrdiff_t m_vecC4ExplodeSpectateAng = 0x16BC; // QAngle
		constexpr std::ptrdiff_t m_flC4ExplodeSpectateDuration = 0x16C8; // float32
	}

	// Parent: C_BaseCombatCharacter
	// Field count: 28
	namespace C_BasePlayerPawn {
		constexpr std::ptrdiff_t m_pWeaponServices = 0x11E0; // CPlayer_WeaponServices*
		constexpr std::ptrdiff_t m_pItemServices = 0x11E8; // CPlayer_ItemServices*
		constexpr std::ptrdiff_t m_pAutoaimServices = 0x11F0; // CPlayer_AutoaimServices*
		constexpr std::ptrdiff_t m_pObserverServices = 0x11F8; // CPlayer_ObserverServices*
		constexpr std::ptrdiff_t m_pWaterServices = 0x1200; // CPlayer_WaterServices*
		constexpr std::ptrdiff_t m_pUseServices = 0x1208; // CPlayer_UseServices*
		constexpr std::ptrdiff_t m_pFlashlightServices = 0x1210; // CPlayer_FlashlightServices*
		constexpr std::ptrdiff_t m_pCameraServices = 0x1218; // CPlayer_CameraServices*
		constexpr std::ptrdiff_t m_pMovementServices = 0x1220; // CPlayer_MovementServices*
		constexpr std::ptrdiff_t m_ServerViewAngleChanges = 0x1230; // C_UtlVectorEmbeddedNetworkVar<ViewAngleServerChange_t>
		constexpr std::ptrdiff_t v_angle = 0x1298; // QAngle
		constexpr std::ptrdiff_t v_anglePrevious = 0x12A4; // QAngle
		constexpr std::ptrdiff_t m_iHideHUD = 0x12B0; // uint32
		constexpr std::ptrdiff_t m_skybox3d = 0x12B8; // sky3dparams_t
		constexpr std::ptrdiff_t m_flDeathTime = 0x1348; // GameTime_t
		constexpr std::ptrdiff_t m_vecPredictionError = 0x134C; // Vector
		constexpr std::ptrdiff_t m_flPredictionErrorTime = 0x1358; // GameTime_t
		constexpr std::ptrdiff_t m_vecLastCameraSetupLocalOrigin = 0x1378; // Vector
		constexpr std::ptrdiff_t m_flLastCameraSetupTime = 0x1384; // GameTime_t
		constexpr std::ptrdiff_t m_flFOVSensitivityAdjust = 0x1388; // float32
		constexpr std::ptrdiff_t m_flMouseSensitivity = 0x138C; // float32
		constexpr std::ptrdiff_t m_vOldOrigin = 0x1390; // Vector
		constexpr std::ptrdiff_t m_flOldSimulationTime = 0x139C; // float32
		constexpr std::ptrdiff_t m_nLastExecutedCommandNumber = 0x13A0; // int32
		constexpr std::ptrdiff_t m_nLastExecutedCommandTick = 0x13A4; // int32
		constexpr std::ptrdiff_t m_hController = 0x13A8; // CHandle<CBasePlayerController>
		constexpr std::ptrdiff_t m_hDefaultController = 0x13AC; // CHandle<CBasePlayerController>
		constexpr std::ptrdiff_t m_bIsSwappingToPredictableController = 0x13B0; // bool
	}

	// Parent: CBasePlayerController
	// Field count: 68
	namespace CCSPlayerController {
		constexpr std::ptrdiff_t m_pInGameMoneyServices = 0x800; // CCSPlayerController_InGameMoneyServices*
		constexpr std::ptrdiff_t m_pInventoryServices = 0x808; // CCSPlayerController_InventoryServices*
		constexpr std::ptrdiff_t m_pActionTrackingServices = 0x810; // CCSPlayerController_ActionTrackingServices*
		constexpr std::ptrdiff_t m_pDamageServices = 0x818; // CCSPlayerController_DamageServices*
		constexpr std::ptrdiff_t m_iPing = 0x820; // uint32
		constexpr std::ptrdiff_t m_bHasCommunicationAbuseMute = 0x824; // bool
		constexpr std::ptrdiff_t m_uiCommunicationMuteFlags = 0x828; // uint32
		constexpr std::ptrdiff_t m_szCrosshairCodes = 0x830; // CUtlSymbolLarge
		constexpr std::ptrdiff_t m_iPendingTeamNum = 0x838; // uint8
		constexpr std::ptrdiff_t m_flForceTeamTime = 0x83C; // GameTime_t
		constexpr std::ptrdiff_t m_iCompTeammateColor = 0x840; // int32
		constexpr std::ptrdiff_t m_bEverPlayedOnTeam = 0x844; // bool
		constexpr std::ptrdiff_t m_flPreviousForceJoinTeamTime = 0x848; // GameTime_t
		constexpr std::ptrdiff_t m_szClan = 0x850; // CUtlSymbolLarge
		constexpr std::ptrdiff_t m_sSanitizedPlayerName = 0x858; // CUtlString
		constexpr std::ptrdiff_t m_iCoachingTeam = 0x860; // int32
		constexpr std::ptrdiff_t m_nPlayerDominated = 0x868; // uint64
		constexpr std::ptrdiff_t m_nPlayerDominatingMe = 0x870; // uint64
		constexpr std::ptrdiff_t m_iCompetitiveRanking = 0x878; // int32
		constexpr std::ptrdiff_t m_iCompetitiveWins = 0x87C; // int32
		constexpr std::ptrdiff_t m_iCompetitiveRankType = 0x880; // int8
		constexpr std::ptrdiff_t m_iCompetitiveRankingPredicted_Win = 0x884; // int32
		constexpr std::ptrdiff_t m_iCompetitiveRankingPredicted_Loss = 0x888; // int32
		constexpr std::ptrdiff_t m_iCompetitiveRankingPredicted_Tie = 0x88C; // int32
		constexpr std::ptrdiff_t m_nEndMatchNextMapVote = 0x890; // int32
		constexpr std::ptrdiff_t m_unActiveQuestId = 0x894; // uint16
		constexpr std::ptrdiff_t m_rtActiveMissionPeriod = 0x898; // uint32
		constexpr std::ptrdiff_t m_nQuestProgressReason = 0x89C; // QuestProgress::Reason
		constexpr std::ptrdiff_t m_unPlayerTvControlFlags = 0x8A0; // uint32
		constexpr std::ptrdiff_t m_iDraftIndex = 0x8D0; // int32
		constexpr std::ptrdiff_t m_msQueuedModeDisconnectionTimestamp = 0x8D4; // uint32
		constexpr std::ptrdiff_t m_uiAbandonRecordedReason = 0x8D8; // uint32
		constexpr std::ptrdiff_t m_eNetworkDisconnectionReason = 0x8DC; // uint32
		constexpr std::ptrdiff_t m_bCannotBeKicked = 0x8E0; // bool
		constexpr std::ptrdiff_t m_bEverFullyConnected = 0x8E1; // bool
		constexpr std::ptrdiff_t m_bAbandonAllowsSurrender = 0x8E2; // bool
		constexpr std::ptrdiff_t m_bAbandonOffersInstantSurrender = 0x8E3; // bool
		constexpr std::ptrdiff_t m_bDisconnection1MinWarningPrinted = 0x8E4; // bool
		constexpr std::ptrdiff_t m_bScoreReported = 0x8E5; // bool
		constexpr std::ptrdiff_t m_nDisconnectionTick = 0x8E8; // int32
		constexpr std::ptrdiff_t m_bControllingBot = 0x8F8; // bool
		constexpr std::ptrdiff_t m_bHasControlledBotThisRound = 0x8F9; // bool
		constexpr std::ptrdiff_t m_bHasBeenControlledByPlayerThisRound = 0x8FA; // bool
		constexpr std::ptrdiff_t m_nBotsControlledThisRound = 0x8FC; // int32
		constexpr std::ptrdiff_t m_bCanControlObservedBot = 0x900; // bool
		constexpr std::ptrdiff_t m_hPlayerPawn = 0x904; // CHandle<C_CSPlayerPawn>
		constexpr std::ptrdiff_t m_hObserverPawn = 0x908; // CHandle<C_CSObserverPawn>
		constexpr std::ptrdiff_t m_bPawnIsAlive = 0x90C; // bool
		constexpr std::ptrdiff_t m_iPawnHealth = 0x910; // uint32
		constexpr std::ptrdiff_t m_iPawnArmor = 0x914; // int32
		constexpr std::ptrdiff_t m_bPawnHasDefuser = 0x918; // bool
		constexpr std::ptrdiff_t m_bPawnHasHelmet = 0x919; // bool
		constexpr std::ptrdiff_t m_nPawnCharacterDefIndex = 0x91A; // uint16
		constexpr std::ptrdiff_t m_iPawnLifetimeStart = 0x91C; // int32
		constexpr std::ptrdiff_t m_iPawnLifetimeEnd = 0x920; // int32
		constexpr std::ptrdiff_t m_iPawnBotDifficulty = 0x924; // int32
		constexpr std::ptrdiff_t m_hOriginalControllerOfCurrentPawn = 0x928; // CHandle<CCSPlayerController>
		constexpr std::ptrdiff_t m_iScore = 0x92C; // int32
		constexpr std::ptrdiff_t m_recentKillQueue = 0x930; // uint8[8]
		constexpr std::ptrdiff_t m_nFirstKill = 0x938; // uint8
		constexpr std::ptrdiff_t m_nKillCount = 0x939; // uint8
		constexpr std::ptrdiff_t m_bMvpNoMusic = 0x93A; // bool
		constexpr std::ptrdiff_t m_eMvpReason = 0x93C; // int32
		constexpr std::ptrdiff_t m_iMusicKitID = 0x940; // int32
		constexpr std::ptrdiff_t m_iMusicKitMVPs = 0x944; // int32
		constexpr std::ptrdiff_t m_iMVPs = 0x948; // int32
		constexpr std::ptrdiff_t m_bIsPlayerNameDirty = 0x94C; // bool
		constexpr std::ptrdiff_t m_bFireBulletsSeedSynchronized = 0x94D; // bool
	}

	// Parent: CPlayerPawnComponent
	// Field count: 2
	namespace CPlayer_WeaponServices {
		constexpr std::ptrdiff_t m_hMyWeapons = 0x48; // C_NetworkUtlVectorBase<CHandle<C_BasePlayerWeapon>>
		constexpr std::ptrdiff_t m_hActiveWeapon = 0x60; // CHandle<C_BasePlayerWeapon>
		constexpr std::ptrdiff_t m_hLastWeapon = 0x64; // CHandle<C_BasePlayerWeapon>
		constexpr std::ptrdiff_t m_iAmmo = 0x68; // uint16[32]
	}

	// Parent: CPlayer_WeaponServices
	// Field count: 5
	namespace CCSPlayer_WeaponServices {
		constexpr std::ptrdiff_t m_flNextAttack = 0xD0; // GameTime_t
		constexpr std::ptrdiff_t m_nOldTotalShootPositionHistoryCount = 0xD4; // uint32
		constexpr std::ptrdiff_t m_nOldTotalInputHistoryCount = 0x370; // uint32
		constexpr std::ptrdiff_t m_networkAnimTiming = 0x1588; // C_NetworkUtlVectorBase<uint8>
		constexpr std::ptrdiff_t m_bBlockInspectUntilNextGraphUpdate = 0x15A0; // bool
	}

	// Parent: C_CSPlayerPawnBase
	// Field count: 106
	namespace C_CSPlayerPawn {
		constexpr std::ptrdiff_t m_pBulletServices = 0x1468; // CCSPlayer_BulletServices*
		constexpr std::ptrdiff_t m_pHostageServices = 0x1470; // CCSPlayer_HostageServices*
		constexpr std::ptrdiff_t m_pBuyServices = 0x1478; // CCSPlayer_BuyServices*
		constexpr std::ptrdiff_t m_pGlowServices = 0x1480; // CCSPlayer_GlowServices*
		constexpr std::ptrdiff_t m_pActionTrackingServices = 0x1488; // CCSPlayer_ActionTrackingServices*
		constexpr std::ptrdiff_t m_pAimPunchServices = 0x1490; // CCSPlayer_AimPunchServices*
		constexpr std::ptrdiff_t m_pDamageReactServices = 0x1498; // CCSPlayer_DamageReactServices*
		constexpr std::ptrdiff_t m_flHealthShotBoostExpirationTime = 0x14A0; // GameTime_t
		constexpr std::ptrdiff_t m_flLastFiredWeaponTime = 0x14A4; // GameTime_t
		constexpr std::ptrdiff_t m_bHasFemaleVoice = 0x14A8; // bool
		constexpr std::ptrdiff_t m_flLandingTimeSeconds = 0x14AC; // float32
		constexpr std::ptrdiff_t m_flOldFallVelocity = 0x14B0; // float32
		constexpr std::ptrdiff_t m_szLastPlaceName = 0x14B4; // char[18]
		constexpr std::ptrdiff_t m_bPrevDefuser = 0x14C6; // bool
		constexpr std::ptrdiff_t m_bPrevHelmet = 0x14C7; // bool
		constexpr std::ptrdiff_t m_nPrevArmorVal = 0x14C8; // int32
		constexpr std::ptrdiff_t m_nPrevGrenadeAmmoCount = 0x14CC; // int32
		constexpr std::ptrdiff_t m_unPreviousWeaponHash = 0x14D0; // uint32
		constexpr std::ptrdiff_t m_unWeaponHash = 0x14D4; // uint32
		constexpr std::ptrdiff_t m_bInBuyZone = 0x14D8; // bool
		constexpr std::ptrdiff_t m_bPreviouslyInBuyZone = 0x14D9; // bool
		constexpr std::ptrdiff_t m_bInLanding = 0x14DA; // bool
		constexpr std::ptrdiff_t m_flLandingStartTime = 0x14DC; // float32
		constexpr std::ptrdiff_t m_bInHostageRescueZone = 0x14E0; // bool
		constexpr std::ptrdiff_t m_bInBombZone = 0x14E1; // bool
		constexpr std::ptrdiff_t m_bIsBuyMenuOpen = 0x14E2; // bool
		constexpr std::ptrdiff_t m_flTimeOfLastInjury = 0x14E4; // GameTime_t
		constexpr std::ptrdiff_t m_flNextSprayDecalTime = 0x14E8; // GameTime_t
		constexpr std::ptrdiff_t m_iRetakesOffering = 0x1640; // int32
		constexpr std::ptrdiff_t m_iRetakesOfferingCard = 0x1644; // int32
		constexpr std::ptrdiff_t m_bRetakesHasDefuseKit = 0x1648; // bool
		constexpr std::ptrdiff_t m_bRetakesMVPLastRound = 0x1649; // bool
		constexpr std::ptrdiff_t m_iRetakesMVPBoostItem = 0x164C; // int32
		constexpr std::ptrdiff_t m_RetakesMVPBoostExtraUtility = 0x1650; // loadout_slot_t
		constexpr std::ptrdiff_t m_bNeedToReApplyGloves = 0x1655; // bool
		constexpr std::ptrdiff_t m_EconGloves = 0x1658; // C_EconItemView
		constexpr std::ptrdiff_t m_nEconGlovesChanged = 0x1AC8; // uint8
		constexpr std::ptrdiff_t m_bMustSyncRagdollState = 0x1AC9; // bool
		constexpr std::ptrdiff_t m_nRagdollDamageBone = 0x1ACC; // int32
		constexpr std::ptrdiff_t m_vRagdollDamageForce = 0x1AD0; // Vector
		constexpr std::ptrdiff_t m_vRagdollDamagePosition = 0x1ADC; // Vector
		constexpr std::ptrdiff_t m_szRagdollDamageWeaponName = 0x1AE8; // char[64]
		constexpr std::ptrdiff_t m_bRagdollDamageHeadshot = 0x1B28; // bool
		constexpr std::ptrdiff_t m_vRagdollServerOrigin = 0x1B2C; // Vector
		constexpr std::ptrdiff_t m_lastLandTime = 0x1B38; // GameTime_t
		constexpr std::ptrdiff_t m_bOnGroundLastTick = 0x1B3C; // bool
		constexpr std::ptrdiff_t m_hHudModelArms = 0x1B58; // CHandle<C_CS2HudModelArms>
		constexpr std::ptrdiff_t m_qDeathEyeAngles = 0x1B5C; // QAngle
		constexpr std::ptrdiff_t m_bLeftHanded = 0x1B68; // bool
		constexpr std::ptrdiff_t m_fSwitchedHandednessTime = 0x1B6C; // GameTime_t
		constexpr std::ptrdiff_t m_flViewmodelOffsetX = 0x1B70; // float32
		constexpr std::ptrdiff_t m_flViewmodelOffsetY = 0x1B74; // float32
		constexpr std::ptrdiff_t m_flViewmodelOffsetZ = 0x1B78; // float32
		constexpr std::ptrdiff_t m_flViewmodelFOV = 0x1B7C; // float32
		constexpr std::ptrdiff_t m_vecPlayerPatchEconIndices = 0x1B80; // uint32[5]
		constexpr std::ptrdiff_t m_GunGameImmunityColor = 0x1BC0; // Color
		constexpr std::ptrdiff_t m_vecBulletHitModels = 0x1C10; // CUtlVector<C_BulletHitModel*>
		constexpr std::ptrdiff_t m_bIsWalking = 0x1C28; // bool
		constexpr std::ptrdiff_t m_entitySpottedState = 0x1C30; // EntitySpottedState_t
		constexpr std::ptrdiff_t m_bIsScoped = 0x1C48; // bool
		constexpr std::ptrdiff_t m_bResumeZoom = 0x1C49; // bool
		constexpr std::ptrdiff_t m_bIsDefusing = 0x1C4A; // bool
		constexpr std::ptrdiff_t m_bIsGrabbingHostage = 0x1C4B; // bool
		constexpr std::ptrdiff_t m_iBlockingUseActionInProgress = 0x1C4C; // CSPlayerBlockingUseAction_t
		constexpr std::ptrdiff_t m_flEmitSoundTime = 0x1C50; // GameTime_t
		constexpr std::ptrdiff_t m_bInNoDefuseArea = 0x1C54; // bool
		constexpr std::ptrdiff_t m_nWhichBombZone = 0x1C58; // int32
		constexpr std::ptrdiff_t m_iShotsFired = 0x1C5C; // int32
		constexpr std::ptrdiff_t m_flFlinchStack = 0x1C60; // float32
		constexpr std::ptrdiff_t m_flVelocityModifier = 0x1C64; // float32
		constexpr std::ptrdiff_t m_bWaitForNoAttack = 0x1C68; // bool
		constexpr std::ptrdiff_t m_ignoreLadderJumpTime = 0x1C6C; // float32
		constexpr std::ptrdiff_t m_bKilledByHeadshot = 0x1C71; // bool
		constexpr std::ptrdiff_t m_ArmorValue = 0x1C74; // int32
		constexpr std::ptrdiff_t m_unCurrentEquipmentValue = 0x1C78; // uint16
		constexpr std::ptrdiff_t m_unRoundStartEquipmentValue = 0x1C7A; // uint16
		constexpr std::ptrdiff_t m_unFreezetimeEndEquipmentValue = 0x1C7C; // uint16
		constexpr std::ptrdiff_t m_nLastKillerIndex = 0x1C80; // CEntityIndex
		constexpr std::ptrdiff_t m_bOldIsScoped = 0x1C84; // bool
		constexpr std::ptrdiff_t m_bHasDeathInfo = 0x1C85; // bool
		constexpr std::ptrdiff_t m_flDeathInfoTime = 0x1C88; // float32
		constexpr std::ptrdiff_t m_vecDeathInfoOrigin = 0x1C8C; // Vector
		constexpr std::ptrdiff_t m_grenadeParameterStashTime = 0x1CC8; // GameTime_t
		constexpr std::ptrdiff_t m_bGrenadeParametersStashed = 0x1CCC; // bool
		constexpr std::ptrdiff_t m_angStashedShootAngles = 0x1CD0; // QAngle
		constexpr std::ptrdiff_t m_vecStashedGrenadeThrowPosition = 0x1CDC; // Vector
		constexpr std::ptrdiff_t m_vecStashedVelocity = 0x1CE8; // Vector
		constexpr std::ptrdiff_t m_angShootAngleHistory = 0x1CF4; // QAngle[2]
		constexpr std::ptrdiff_t m_vecThrowPositionHistory = 0x1D0C; // Vector[2]
		constexpr std::ptrdiff_t m_vecVelocityHistory = 0x1D24; // Vector[2]
		constexpr std::ptrdiff_t m_bShouldAutobuyDMWeapons = 0x3270; // bool
		constexpr std::ptrdiff_t m_fImmuneToGunGameDamageTime = 0x3274; // GameTime_t
		constexpr std::ptrdiff_t m_bGunGameImmunity = 0x3278; // bool
		constexpr std::ptrdiff_t m_fImmuneToGunGameDamageTimeLast = 0x327C; // GameTime_t
		constexpr std::ptrdiff_t m_fMolotovDamageTime = 0x3280; // float32
		constexpr std::ptrdiff_t m_bThirdpersonActiveWeaponCanSafelyOcclude = 0x3288; // bool
		constexpr std::ptrdiff_t m_nPlayerInfernoBodyFx = 0x328C; // ParticleIndex_t
		constexpr std::ptrdiff_t m_angEyeAngles = 0x3300; // QAngle
		constexpr std::ptrdiff_t m_arrOldEyeAnglesTimes = 0x3390; // GameTime_t[4]
		constexpr std::ptrdiff_t m_arrOldEyeAngles = 0x33A0; // QAngle[4]
		constexpr std::ptrdiff_t m_angEyeAnglesVelocity = 0x33D0; // QAngle
		constexpr std::ptrdiff_t m_iIDEntIndex = 0x33DC; // CEntityIndex
		constexpr std::ptrdiff_t m_delayTargetIDTimer = 0x33E0; // CountdownTimer
		constexpr std::ptrdiff_t m_iTargetItemEntIdx = 0x33F8; // CEntityIndex
		constexpr std::ptrdiff_t m_iOldIDEntIndex = 0x33FC; // CEntityIndex
		constexpr std::ptrdiff_t m_holdTargetIDTimer = 0x3400; // CountdownTimer
	}

	// Parent: CBaseAnimGraph
	// Field count: 1
	namespace C_EconEntity {
		constexpr std::ptrdiff_t m_flFlexDelayTime = 0x1168; // float32
		constexpr std::ptrdiff_t m_flFlexDelayedWeight = 0x1170; // float32*
		constexpr std::ptrdiff_t m_bAttributesInitialized = 0x1178; // bool
		constexpr std::ptrdiff_t m_AttributeManager = 0x1180; // C_AttributeContainer
		constexpr std::ptrdiff_t m_OriginalOwnerXuidLow = 0x1650; // uint32
		constexpr std::ptrdiff_t m_OriginalOwnerXuidHigh = 0x1654; // uint32
		constexpr std::ptrdiff_t m_nFallbackPaintKit = 0x1658; // int32
		constexpr std::ptrdiff_t m_nFallbackSeed = 0x165C; // int32
		constexpr std::ptrdiff_t m_flFallbackWear = 0x1660; // float32
		constexpr std::ptrdiff_t m_nFallbackStatTrak = 0x1664; // int32
		constexpr std::ptrdiff_t m_bClientside = 0x1668; // bool
		constexpr std::ptrdiff_t m_bParticleSystemsCreated = 0x1669; // bool
		constexpr std::ptrdiff_t m_vecAttachedParticles = 0x1670; // CUtlVector<int32>
		constexpr std::ptrdiff_t m_hViewmodelAttachment = 0x1688; // CHandle<CBaseAnimGraph>
		constexpr std::ptrdiff_t m_iOldTeam = 0x168C; // int32
		constexpr std::ptrdiff_t m_bAttachmentDirty = 0x1690; // bool
		constexpr std::ptrdiff_t m_nUnloadedModelIndex = 0x1694; // int32
		constexpr std::ptrdiff_t m_iNumOwnerValidationRetries = 0x1698; // int32
		constexpr std::ptrdiff_t m_hOldProvidee = 0x16A8; // CHandle<C_BaseEntity>
		constexpr std::ptrdiff_t m_vecAttachedModels = 0x16B0; // CUtlVector<C_EconEntity::AttachedModelData_t>
	}

	// Parent: CAttributeManager
	// Field count: 3
	namespace C_AttributeContainer {
		constexpr std::ptrdiff_t m_Item = 0x50; // C_EconItemView
		constexpr std::ptrdiff_t m_iExternalItemProviderRegisteredToken = 0x4C0; // int32
		constexpr std::ptrdiff_t m_ullRegisteredAsItemID = 0x4C8; // uint64
	}

	// Parent: None
	// Field count: 29
	namespace C_EconItemView {
		constexpr std::ptrdiff_t m_bInventoryImageRgbaRequested = 0x60; // bool
		constexpr std::ptrdiff_t m_bInventoryImageTriedCache = 0x61; // bool
		constexpr std::ptrdiff_t m_nInventoryImageRgbaWidth = 0x80; // int32
		constexpr std::ptrdiff_t m_nInventoryImageRgbaHeight = 0x84; // int32
		constexpr std::ptrdiff_t m_szCurrentLoadCachedFileName = 0x88; // char[260]
		constexpr std::ptrdiff_t m_bRestoreCustomMaterialAfterPrecache = 0x1B8; // bool
		constexpr std::ptrdiff_t m_iItemDefinitionIndex = 0x1BA; // uint16
		constexpr std::ptrdiff_t m_iEntityQuality = 0x1BC; // int32
		constexpr std::ptrdiff_t m_iEntityLevel = 0x1C0; // uint32
		constexpr std::ptrdiff_t m_iItemID = 0x1C8; // uint64
		constexpr std::ptrdiff_t m_iItemIDHigh = 0x1D0; // uint32
		constexpr std::ptrdiff_t m_iItemIDLow = 0x1D4; // uint32
		constexpr std::ptrdiff_t m_iAccountID = 0x1D8; // uint32
		constexpr std::ptrdiff_t m_iInventoryPosition = 0x1DC; // uint32
		constexpr std::ptrdiff_t m_bInitialized = 0x1E8; // bool
		constexpr std::ptrdiff_t m_bDisallowSOC = 0x1E9; // bool
		constexpr std::ptrdiff_t m_bIsStoreItem = 0x1EA; // bool
		constexpr std::ptrdiff_t m_bIsTradeItem = 0x1EB; // bool
		constexpr std::ptrdiff_t m_iEntityQuantity = 0x1EC; // int32
		constexpr std::ptrdiff_t m_iRarityOverride = 0x1F0; // int32
		constexpr std::ptrdiff_t m_iQualityOverride = 0x1F4; // int32
		constexpr std::ptrdiff_t m_iOriginOverride = 0x1F8; // int32
		constexpr std::ptrdiff_t m_ubStyleOverride = 0x1FC; // uint8
		constexpr std::ptrdiff_t m_unClientFlags = 0x1FD; // uint8
		constexpr std::ptrdiff_t m_AttributeList = 0x208; // CAttributeList
		constexpr std::ptrdiff_t m_NetworkedDynamicAttributes = 0x280; // CAttributeList
		constexpr std::ptrdiff_t m_szCustomName = 0x2F8; // char[161]
		constexpr std::ptrdiff_t m_szCustomNameOverride = 0x399; // char[161]
		constexpr std::ptrdiff_t m_bInitializedTags = 0x468; // bool
	}
}
