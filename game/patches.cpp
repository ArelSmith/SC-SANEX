#include "../main.h"
#include "../util/armhook.h"
#include "common.h"

char* PLAYERS_REALLOC = nullptr;

struct _ATOMIC_MODEL
{
	uintptr_t func_tbl;
	char data[56];
} *ATOMIC_MODELS = nullptr;

void ApplyCrashFixPatches()
{
	// reallocate CPools::ms_pEntryInfoNodePool
	WriteMemory(g_libGTASA+0x3AF27A, (uintptr_t)"\x4f\xf4\x20\x40", 4); // MOV.W	R0, #0xA000 | size = 0x14

	WriteMemory(g_libGTASA+0x3AF284, (uintptr_t)"\x4f\xf4\x00\x60", 4); // MOV.W R0, #0x800
	WriteMemory(g_libGTASA+0x3AF28C, (uintptr_t)"\x4f\xf4\x00\x62", 4); // MOV.W R2, #0x800
	WriteMemory(g_libGTASA+0x3AF2BA, (uintptr_t)"\xb3\xf5\x00\x6f", 4); // CMP.W R3, #0x800

	WriteMemory(g_libGTASA+0x3AF284, (uintptr_t)"\x4f\xf4\x90\x50", 4); // MOV.W R0, #0x1200
	WriteMemory(g_libGTASA+0x3AF28C, (uintptr_t)"\x4f\xf4\x90\x52", 4); // MOV.W R2, #0x1200
	WriteMemory(g_libGTASA+0x3AF2BA, (uintptr_t)"\xb3\xf5\x90\x5f", 4); // CMP.W R3, #0x1200

	// reallocate CPools::ms_pPtrNodeDoubleLinkPool
	WriteMemory(g_libGTASA+0x3AF21C, (uintptr_t)"\x4F\xF4\x00\x30", 4); // MOV.W R0, #0x20000
}

void ApplyETCpatches()
{
    // ---> Remove 'Language' in settings
    NOP(g_libGTASA + 0x00261A50, 2);

    // ---> Delete 'Sing in to Social Club' in 'Options->Game'
    NOP(g_libGTASA + 0x002665EE, 2);

	// 'Shadows' in options
	NOP(g_libGTASA + 0x2661DA, 2);

	// jan sebar
	NOP(g_libGTASA + 0x454950, 17);
        NOP(g_libGTASA+0x454A88, 2);
	RET(g_libGTASA + 0x4DD00C);
	RET(g_libGTASA + 0x4DD038);
	RET(g_libGTASA + 0x3E17F0); 
	RET(g_libGTASA + 0x3E42E0); 
	RET(g_libGTASA + 0x3E4490); 
	RET(g_libGTASA + 0x3E3F38);
	WriteMemory(g_libGTASA + 0x1A7EF2, (uintptr_t)"\x4F\xF4\x00\x10\x4F\xF4\x80\x00", 8);
	WriteMemory(g_libGTASA + 0x1A7ECE, (uintptr_t)"\x4F\xF0\x01\x00\x00\x46", 6);


	// TODO FIX REMOVE BUILDING //
}

void ApplyRadarPatches()
{
	// CRadar::AddBlipToLegendList
    RET(g_libGTASA+0x3DBB30);

    // CRadar::DrawLegend
    RET(g_libGTASA+0x3DA500);
}

void ApplyPatches_level0()
{
	PLAYERS_REALLOC = (( char* (*)(int))(g_libGTASA+0x179B40))(404*MAX_PLAYERS);
	UnFuck(g_libGTASA+0x5D021C);
	*(char**)(g_libGTASA+0x5D021C) = PLAYERS_REALLOC;
	Log("CWorld::Player new address = 0x%X", PLAYERS_REALLOC);

	// reallocate Pads[]
	UnFuck(g_libGTASA+0x5CF914);
	*(int8_t **)(g_libGTASA+0x5CF914) = new int8_t[(PLAYER_PED_SLOTS + 1) * 340];

	// ---> Crash "fix" creator
	// WriteMemory(g_libGTASA + 0x003AAF3C, (uintptr_t)"\xFF\xF4\x00\x34", 4);

	// --->  Apply crash fix patches
	ApplyCrashFixPatches();
	ApplyETCpatches();
	ApplyRadarPatches();

	// CdStreamInit(6);
	WriteMemory(g_libGTASA+0x3981EC, (uintptr_t)"\x06\x20", 2);

	// ---> Skip button "OFFLINE" -------------
	uintptr_t g_libSCAnd = FindLibrary("libSCAnd.so");
	if(g_libSCAnd)
	{
		UnFuck(g_libSCAnd+0x20C670);
		*(bool*)(g_libSCAnd+0x20C670) = true;

		UnFuck(g_libSCAnd+0x1E1738);
		UnFuck(g_libSCAnd+0x1E16DC);
		UnFuck(g_libSCAnd+0x1E080C);

		memcpy((void*)(g_libSCAnd+0x1E16DC), "com/rockstargames/hal/andViewManager", 37);
		memcpy((void*)(g_libSCAnd+0x1E1738), "staticExitSocialClub", 21);

		*(uint32_t*)(g_libSCAnd+0x1E080C) = 0x562928;
	}
}

void ApplyPatches()
{
	Log("Installing patches..");

	ATOMIC_MODELS = new _ATOMIC_MODEL[20001];
	for (int i = 0; i < 20000; i++) 
	{
		// CBaseModelInfo::CBaseModelInfo
		((void(*)(_ATOMIC_MODEL*))(g_libGTASA + 0x33559C + 1))(&ATOMIC_MODELS[i]);
		// vtable
		ATOMIC_MODELS[i].func_tbl = g_libGTASA + 0x5C6C68;
		memset(ATOMIC_MODELS[i].data, 0, sizeof(ATOMIC_MODELS->data));
	}

	// CAudioEngine::StartLoadingTune
	NOP(g_libGTASA+0x56C150, 2);

	*(uint32_t*)(g_libGTASA + 0x008E87E4) = 1;
	WriteMemory(g_libGTASA + 0x003C5B58, (uintptr_t)"\x02\x21", 2);

	// DefaultPCSaveFileName
	char* DefaultPCSaveFileName = (char*)(g_libGTASA+0x60EAE8);
	memcpy((char*)DefaultPCSaveFileName, "GTASAMP", 8);

	// CWidgetRegionSteeringSelection::Draw
	WriteMemory(g_libGTASA+0x284BB8, (uintptr_t)"\xF7\x46", 2);

	// fix bg viewmodel
	WriteMemory(g_libGTASA+0x1859FC, (uintptr_t)"\x01\x22", 2);

	// CHud::SetHelpMessage
	WriteMemory(g_libGTASA+0x3D4244, (uintptr_t)"\xF7\x46", 2);
	// CHud::SetHelpMessageStatUpdate
	WriteMemory(g_libGTASA+0x3D42A8, (uintptr_t)"\xF7\x46", 2);
	// CVehicleRecording::Load
	WriteMemory(g_libGTASA+0x2DC8E0, (uintptr_t)"\xF7\x46", 2);

	// nop calling CRealTimeShadowManager::ReturnRealTimeShadow from ~CPhysical
	NOP(g_libGTASA+0x3A019C, 2);

	// ---> TESTING -------------
    // // VehicleStruct increase (0x32C*0x50 = 0xFDC0)
    // WriteMemory(g_libGTASA + 0x00405338, (uintptr_t)"\x4F\xF6\xC0\x50", 4);	// MOV  R0, #0xFDC0
    // WriteMemory(g_libGTASA + 0x00405342, (uintptr_t)"\x50\x20", 2);			// MOVS R0, #0x50
    // WriteMemory(g_libGTASA + 0x00405348, (uintptr_t)"\x50\x22", 2);			// MOVS R2, #0x50
    // WriteMemory(g_libGTASA + 0x00405374, (uintptr_t)"\x50\x2B", 2);			// CMP  R3, #0x50

    // // CPed pool (old: 140, new: 210)
    // // 	MOVW R0, #0x5EC8
    // //  MOVT R0, #6 
    // WriteMemory(g_libGTASA + 0x003AF2D0, (uintptr_t)"\x45\xF6\xC8\x60\xC0\xF2\x06\x00", 8); // MOV  R0, #0x65EC8 | size=0x7C4 (old: 0x43F30)
    // WriteMemory(g_libGTASA + 0x003AF2DE, (uintptr_t)"\xD2\x20", 2); // MOVS R0, #0xD2
    // WriteMemory(g_libGTASA + 0x003AF2E4, (uintptr_t)"\xD2\x22", 2); // MOVS R2, #0xD2
    // WriteMemory(g_libGTASA + 0x003AF310, (uintptr_t)"\xD2\x2B", 2); // CMP  R3, #0xD2

    // // CPedIntelligence pool (old: 140, new: 210)
    // // movw r0, #0x20B0
    // // movt r0, #2
    // // nop
    // WriteMemory(g_libGTASA + 0x003AF7E6, (uintptr_t)"\x42\xF2\xB0\x00\xC0\xF2\x02\x00\x00\x46", 10); // MOVS R0, #0x220B0 | size=0x298 (old: 0x16B20)
    // WriteMemory(g_libGTASA + 0x003AF7F6, (uintptr_t)"\xD2\x20", 2); // MOVS R0, #0xD2
    // WriteMemory(g_libGTASA + 0x003AF7FC, (uintptr_t)"\xD2\x22", 2); // MOVS R2, #0xD2
    // WriteMemory(g_libGTASA + 0x003AF824, (uintptr_t)"\xD2\x2B", 2); // CMP  R3, 0xD2

    // // Task pool (old: 500, new: 1524 (1536))
    // WriteMemory(g_libGTASA + 0x003AF4EA, (uintptr_t)"\x4F\xF4\x40\x30", 4); // MOV.W R0, #30000 | size = 0x80 (old: 0xFA00)
    // WriteMemory(g_libGTASA + 0x003AF4F4, (uintptr_t)"\x4F\xF4\xC0\x60", 4); // MOV.W R0, #0x600
    // WriteMemory(g_libGTASA + 0x003AF4FC, (uintptr_t)"\x4F\xF4\xC0\x62", 4); // MOV.W R2, #0x600
    // WriteMemory(g_libGTASA + 0x003AF52A, (uintptr_t)"\xB3\xF5\xC0\x6F", 4); // CMP.W R3, #0x600

    // // ColModel pool (old:10150, new: 32511)
    // // mov r0, #0xCFD0
    // // movt r0, #0x17
    // WriteMemory(g_libGTASA + 0x003AF48E, (uintptr_t)"\x4C\xF6\xD0\x70\xC0\xF2\x17\x00", 8); // MOV R0, #0x17CFD0 | size=0x30 (old: 0x76F20)
    // WriteMemory(g_libGTASA + 0x003AF49C, (uintptr_t)"\x47\xF6\xFF\x60", 4); // MOVW R0, #0x7EFF
    // WriteMemory(g_libGTASA + 0x003AF4A4, (uintptr_t)"\x47\xF6\xFF\x62", 4); // MOVW R2, #0x7EFF
    // ---> TESTING -------------

    // Increase matrix count in CPlaceable::InitMatrixArray
 	WriteMemory(g_libGTASA+0x3ABB0A, (uintptr_t)"\x4F\xF4\x7A\x61", 4); // MOV.W R1, #4000
}

void ApplyInGamePatches()
{
	Log("Installing patches (ingame)..");

	// CTheZones::ZonesVisited[100]
	memset((void *)(g_libGTASA+0x008EA7B0), 1, 100);
	
	// CTheZones::ZonesRevealed
	*(uint32_t *)(g_libGTASA+0x8EA7A8) = 100;

	// CTaskSimplePlayerOnFoot::PlayIdleAnimations 
	WriteMemory(g_libGTASA+0x4BDB18, (uintptr_t)"\x70\x47", 2);

	// CarCtl::GenerateRandomCars nulled from CGame::Process
	UnFuck(g_libGTASA+0x398A3A);
	NOP(g_libGTASA+0x398A3A, 2);

	// CTheCarGenerators::Process nulled from CGame::Process
	UnFuck(g_libGTASA+0x398A34);
	NOP(g_libGTASA+0x398A34, 2);

	// ìíîæèòåëü äëÿ MaxHealth
	UnFuck(g_libGTASA+0x3BAC68);
	*(float*)(g_libGTASA+0x3BAC68) = 176.0f; //176

	// ìíîæèòåëü äëÿ Armour
	UnFuck(g_libGTASA+0x27D884);
	*(float*)(g_libGTASA+0x27D884) = 176.0f; //176

	// CEntryExit::GenerateAmbientPeds nulled from CEntryExit::TransitionFinished
	UnFuck(g_libGTASA+0x2C2C22);
	NOP(g_libGTASA+0x2C2C22, 4);

	// CPlayerPed::CPlayerPed task fix
	WriteMemory(g_libGTASA+0x458ED1, (uintptr_t)"\xE0", 1);

	// ReapplyPlayerAnimation
	NOP(g_libGTASA+0x45477E, 5);

	// radar draw blips
    UnFuck(g_libGTASA+0x3DCA90);
    NOP(g_libGTASA+0x3DCA90, 2);
    UnFuck(g_libGTASA+0x3DD4A4);
    NOP(g_libGTASA+0x3DD4A4, 2);
    
    // CCamera::CamShake from CExplosion::AddExplosion
    NOP(g_libGTASA+0x55EFB8, 2);
    NOP(g_libGTASA+0x55F8F8, 2);

    // Engine lights patch
	NOP(g_libGTASA+0x408AAA, 2);

    // camera_on_actor path
    UnFuck(g_libGTASA+0x2F7B68);
    *(uint8_t*)(g_libGTASA+0x2F7B6B) = 0xBE;

    // CPed::RemoveWeaponWhenEnteringVehicle (GetPlayerInfoForThisPlayerPed)
   	UnFuck(g_libGTASA+0x434D94);
   	NOP(g_libGTASA+0x434D94, 6);

    // CBike::ProcessAI
    UnFuck(g_libGTASA+0x4EE200);
    *(uint8_t*)(g_libGTASA+0x4EE200) = 0x9B;

    // CWidgetPlayerInfo::DrawWanted
    WriteMemory(g_libGTASA+0x27D8D0, (uintptr_t)"\x4F\xF0\x00\x08", 4);

    // no vehicle audio processing (cause mass lags when many vehicles in stream)
    UnFuck(g_libGTASA+0x4E31A6);
    NOP(g_libGTASA+0x4E31A6, 2);
    UnFuck(g_libGTASA+0x4EE7D2);
    NOP(g_libGTASA+0x4EE7D2, 2);
    UnFuck(g_libGTASA+0x4F741E);
    NOP(g_libGTASA+0x4F741E, 2);
    UnFuck(g_libGTASA+0x50AB4A);
    NOP(g_libGTASA+0x50AB4A, 2);

	// hud patch
	RET(g_libGTASA+0x3D541C); // CHud::DrawVehicleName
	RET(g_libGTASA+0x3D62FC); // CHud::DrawBustedWastedMessage


	WriteMemory(g_libGTASA+0x56C1F6, (uintptr_t)"\x5A", 1);
	WriteMemory(g_libGTASA+0x5E4990, (uintptr_t)"\x5A", 1);

    // display FPS
	*(uint8_t*)(g_libGTASA+0x8ED875) = 1;
}