#include "../main.h"
#include "game.h"
#include "../util/armhook.h"
#include "../chatwindow.h"

extern CChatWindow *pChatWindow;

void ApplyPatches();
void ApplyInGamePatches();
void InstallHooks();
void LoadSplashTexture();
void InitScripting();

uint16_t *wszTextDrawGXT = nullptr;
uint16_t *szGameTextMessage;
bool bUsedPlayerSlots[PLAYER_PED_SLOTS];

extern char* PLAYERS_REALLOC;

CGame::CGame()
{
	m_pGameCamera = new CCamera();
	m_pGamePlayer = nullptr;

	m_bClockEnabled = true;
	m_bCheckpointsEnabled = false;
	m_dwCheckpointMarker = 0;

	m_bRaceCheckpointsEnabled = 0;
	m_dwRaceCheckpointHandle = 0;
	m_dwRaceCheckpointMarker = 0;

	memset(&bUsedPlayerSlots[0], 0, PLAYER_PED_SLOTS);
}


void CGame::SetDrunkBlur(float level)
{
	//UnFuck(g_libGTASA+0x28E500);
	//*(float*)(g_libGTASA+0x28E500) = level;
}

bool CGame::IsKeyPressed(int iKeyIdentifier)
{
	GTA_CONTROLSET *pControlSet; 

	if(pControlSet->wKeys1[iKeyIdentifier]) return true;
		return false;
}

/*int CGame::GetScreenWidth() 
{
	int retn_RwEngineGetCurrentVideoMode = ((int (*)(void))(g_libGTASA + 0x001AE184 + 1))();

	RwVideoMode mode;
	((void (*)(RwVideoMode*, int))(g_libGTASA + 0x001AE154 + 1))(&mode, retn_RwEngineGetCurrentVideoMode);

	return mode.width;
}

int CGame::GetScreenHeight() 
{
	int retn_RwEngineGetCurrentVideoMode = ((int (*)(void))(g_libGTASA + 0x001AE184 + 1))();

	RwVideoMode mode;
	((void (*)(RwVideoMode*, int))(g_libGTASA + 0x001AE154 + 1))(&mode, retn_RwEngineGetCurrentVideoMode);

	return mode.height;
}*/


// 0.3.7
uint8_t CGame::FindFirstFreePlayerPedSlot()
{
	uint8_t x = 2;
	while(x != PLAYER_PED_SLOTS)
	{
		if(!bUsedPlayerSlots[x]) return x;
		x++;
	}

	return 0;
}
void CGame::DisableAutoAim()
{
     RET(g_libGTASA+0x438DB4);
}
// 0.3.7
CPlayerPed* CGame::NewPlayer(int iSkin, float fX, float fY, float fZ, float fRotation, uint8_t byteCreateMarker)
{
	uint8_t bytePlayerNum = FindFirstFreePlayerPedSlot();
	if(!bytePlayerNum) return nullptr;

	CPlayerPed* pPlayerNew = new CPlayerPed(bytePlayerNum, iSkin, fX, fY, fZ, fRotation);
	if(pPlayerNew && pPlayerNew->m_pPed)
		bUsedPlayerSlots[bytePlayerNum] = true;

	//if(byteCreateMarker) (no xrefs ;( )
	return pPlayerNew;
}

// 0.3.7
void CGame::RemovePlayer(CPlayerPed* pPlayer)
{
	if(pPlayer)
	{
		delete pPlayer;
	}
}

CActorPed *CGame::NewActor(int iSkin, VECTOR vecPosition, float fRotation, float fHealth, bool bInvulnerable)
{
	CActorPed *pActorNew = new CActorPed(iSkin, vecPosition, fRotation, fHealth, bInvulnerable);
	return pActorNew;
}

// 0.3.7
CVehicle* CGame::NewVehicle(int iType, float fPosX, float fPosY, float fPosZ, float fRotation, bool bAddSiren)
{
	CVehicle *pVehicleNew = new	CVehicle(iType, fPosX, fPosY, fPosZ, fRotation, bAddSiren);
	Log("Fine");
	return pVehicleNew;
}

CObject *CGame::NewObject(int iModel, float fPosX, float fPosY, float fPosZ, VECTOR vecRot, float fDrawDistance)
{
	#ifdef DEBUG_MODE
	Log("NewObject(%d, %f, %f, %f, %f, %f, %f, %f)", iModel, fPosX, fPosY, fPosZ, vecRot.X, vecRot.Y, vecRot.Z, fDrawDistance);
	#endif

	if(!IsTrueModelID(iModel))
		iModel = OBJECT_NOMODELFILE;

	int waitPreloaded;
	if(!IsModelLoaded(iModel))
	{
		RequestModel(iModel);
		LoadRequestedModels(false);
		while(!IsModelLoaded(iModel)) 
		{
			usleep(500);
			waitPreloaded++;
			if(waitPreloaded >= 50)
			{
				pChatWindow->AddDebugMessage("Warning: object %u wouldn't load in time.", iModel);
				iModel = OBJECT_NOMODELFILE;
			}
		}
	}

	CObject *pObjectNew = new CObject(iModel, fPosX, fPosY, fPosZ, vecRot, fDrawDistance);

	#ifdef DEBUG_MODE
	Log("Object %d created", iModel);
	#endif

	return pObjectNew;
}

uint32_t CGame::CreatePickup(int iModel, int iType, float fX, float fY, float fZ, int* unk)
{
	#ifdef DEBUG_MODE
	Log("CreatePickup(%d, %d, %f, %f, %f)", iModel, iType, fX, fY, fZ);
	#endif
	
	if(!IsTrueModelID(iModel))
		iModel = OBJECT_NOMODELFILE;

	int waitPreloaded;
	if(!IsModelLoaded(iModel))
	{
		RequestModel(iModel);
		LoadRequestedModels(false);
		while(!IsModelLoaded(iModel)) 
		{
			usleep(500);
			waitPreloaded++;
			if(waitPreloaded >= 50)
			{
				pChatWindow->AddDebugMessage("Warning: pickup %u wouldn't load in time.", iModel);
				iModel = OBJECT_NOMODELFILE;
			}
		}
	}

	uint32_t hnd;

	ScriptCommand(&create_pickup, iModel, iType, fX, fY, fZ, &hnd);

	/*int lol = 32 * (uint16_t)hnd;
	if(lol) lol /= 32;
	if(unk) *unk = lol;*/

	int lol = 32 * hnd & 0x1FFFE0;
	if(lol) lol >>= 5;
	*unk = lol;

	#ifdef DEBUG_MODE
	Log("Pickup %d created", iModel);
	#endif

	return hnd;
}

void CGame::InitInMenu()
{
	Log("CGame: InitInMenu");
	ApplyPatches();
	InstallHooks();
	GameAimSyncInit();
	LoadSplashTexture();

	wszTextDrawGXT = new uint16_t[MAX_TEXT_DRAW_LINE];
	szGameTextMessage = new uint16_t[0xFF];
}

void CGame::InitInGame()
{
	Log("CGame: InitInGame");
	ApplyInGamePatches();
	InitScripting();

	GameResetRadarColors();
}

float CGame::FindGroundZForCoord(float x, float y, float z)
{
	float fGroundZ;
	ScriptCommand(&get_ground_z, x, y, z, &fGroundZ);
	return fGroundZ;
}

// 0.3.7
void CGame::SetCheckpointInformation(VECTOR *pos, VECTOR *extent)
{
	memcpy(&m_vecCheckpointPos,pos,sizeof(VECTOR));
	memcpy(&m_vecCheckpointExtent,extent,sizeof(VECTOR));

	if(m_dwCheckpointMarker) 
	{
		DisableMarker(m_dwCheckpointMarker);
		m_dwCheckpointMarker = 0;

		m_dwCheckpointMarker = CreateRadarMarkerIcon(0, m_vecCheckpointPos.X,
			m_vecCheckpointPos.Y, m_vecCheckpointPos.Z, 1005, 0);
	}
}

// 0.3.7
void CGame::SetRaceCheckpointInformation(uint8_t byteType, VECTOR *pos, VECTOR *next, float fSize)
{
	memcpy(&m_vecRaceCheckpointPos,pos,sizeof(VECTOR));
	memcpy(&m_vecRaceCheckpointNext,next,sizeof(VECTOR));
	m_fRaceCheckpointSize = fSize;
	m_byteRaceType = byteType;

	if(m_dwRaceCheckpointMarker)
	{
		DisableMarker(m_dwRaceCheckpointMarker);

		m_dwRaceCheckpointMarker = NULL;

		m_dwRaceCheckpointMarker = CreateRadarMarkerIcon(0, m_vecRaceCheckpointPos.X,
			m_vecRaceCheckpointPos.Y, m_vecRaceCheckpointPos.Z, 1005, 0);
	}

	MakeRaceCheckpoint();
}

// 0.3.7
void CGame::MakeRaceCheckpoint()
{
	DisableRaceCheckpoint();

	ScriptCommand(&create_racing_checkpoint, (int)m_byteRaceType,
				m_vecRaceCheckpointPos.X, m_vecRaceCheckpointPos.Y, m_vecRaceCheckpointPos.Z,
				m_vecRaceCheckpointNext.X, m_vecRaceCheckpointNext.Y, m_vecRaceCheckpointNext.Z,
				m_fRaceCheckpointSize, &m_dwRaceCheckpointHandle);

	m_bRaceCheckpointsEnabled = true;
}

// 0.3.7
void CGame::DisableRaceCheckpoint()
{
	if (m_dwRaceCheckpointHandle)
	{
		ScriptCommand(&destroy_racing_checkpoint, m_dwRaceCheckpointHandle);
		m_dwRaceCheckpointHandle = NULL;
	}
	
	m_bRaceCheckpointsEnabled = false;
}

// 0.3.7
void CGame::UpdateCheckpoints()
{
	if(m_bCheckpointsEnabled) 
	{
		CPlayerPed *pPlayerPed = this->FindPlayerPed();
		if(pPlayerPed) 
		{
			ScriptCommand(&is_actor_near_point_3d,pPlayerPed->m_dwGTAId,
				m_vecCheckpointPos.X,m_vecCheckpointPos.Y,m_vecCheckpointPos.Z,
				m_vecCheckpointExtent.X,m_vecCheckpointExtent.Y,m_vecCheckpointExtent.Z,1);
			
			if (!m_dwCheckpointMarker)
			{
				m_dwCheckpointMarker = CreateRadarMarkerIcon(0, m_vecCheckpointPos.X,
					m_vecCheckpointPos.Y, m_vecCheckpointPos.Z, 1005, 0);
			}
		}
	}
	else if(m_dwCheckpointMarker) 
	{
		DisableMarker(m_dwCheckpointMarker);
		m_dwCheckpointMarker = 0;
	}
	
	if(m_bRaceCheckpointsEnabled) 
	{
		CPlayerPed *pPlayerPed = this->FindPlayerPed();
		if(pPlayerPed)
		{
			if (!m_dwRaceCheckpointMarker)
			{
				m_dwRaceCheckpointMarker = CreateRadarMarkerIcon(0, m_vecRaceCheckpointPos.X,
					m_vecRaceCheckpointPos.Y, m_vecRaceCheckpointPos.Z, 1005, 0);
			}
		}
	}
	else if(m_dwRaceCheckpointMarker) 
	{
		DisableMarker(m_dwRaceCheckpointMarker);
		DisableRaceCheckpoint();
		m_dwRaceCheckpointMarker = 0;
	}
}


// 0.3.7
uint32_t CGame::CreateRadarMarkerIcon(int iMarkerType, float fX, float fY, float fZ, int iColor, int iStyle)
{
	uint32_t dwMarkerID = 0;

	if(iStyle == 1) 
		ScriptCommand(&create_marker_icon, fX, fY, fZ, iMarkerType, &dwMarkerID);
	else if(iStyle == 2) 
		ScriptCommand(&create_radar_marker_icon, fX, fY, fZ, iMarkerType, &dwMarkerID);
	else if(iStyle == 3) 
		ScriptCommand(&create_icon_marker_sphere, fX, fY, fZ, iMarkerType, &dwMarkerID);
	else 
		ScriptCommand(&create_radar_marker_without_sphere, fX, fY, fZ, iMarkerType, &dwMarkerID);

	if(iMarkerType == 0)
	{
		if(iColor >= 1004)
		{
			ScriptCommand(&set_marker_color, dwMarkerID, iColor);
			ScriptCommand(&show_on_radar, dwMarkerID, 3);
		}
		else
		{
			ScriptCommand(&set_marker_color, dwMarkerID, iColor);
			ScriptCommand(&show_on_radar, dwMarkerID, 2);
		}
	}

	return dwMarkerID;
}

// 0.3.7
uint8_t CGame::GetActiveInterior()
{
	uint32_t dwRet;
	ScriptCommand(&get_active_interior, &dwRet);
	return (uint8_t)dwRet;
}

// 0.3.7
void CGame::SetWorldTime(int iHour, int iMinute)
{
	*(uint8_t*)(g_libGTASA+0x8B18A4) = (uint8_t)iMinute;
	*(uint8_t*)(g_libGTASA+0x8B18A5) = (uint8_t)iHour;
	ScriptCommand(&set_current_time, iHour, iMinute);
}

// 0.3.7
void CGame::SetWorldWeather(unsigned char byteWeatherID)
{
	*(unsigned char*)(g_libGTASA+0x9DB98E) = byteWeatherID;

	if(!m_bClockEnabled)
	{
		*(uint16_t*)(g_libGTASA+0x9DB990) = byteWeatherID;
		*(uint16_t*)(g_libGTASA+0x9DB992) = byteWeatherID;
	}
}

void CGame::ToggleThePassingOfTime(bool bOnOff)
{
	if(bOnOff)
		WriteMemory(g_libGTASA+0x38C154, (uintptr_t)"\x2D\xE9", 2);
	else
		WriteMemory(g_libGTASA+0x38C154, (uintptr_t)"\xF7\x46", 2);
}

// 0.3.7
void CGame::EnableClock(bool bEnable)
{
	char byteClockData[] = { '%', '0', '2', 'd', ':', '%', '0', '2', 'd', 0 };
	UnFuck(g_libGTASA+0x599504);

	if(bEnable)
	{
		ToggleThePassingOfTime(true);
		m_bClockEnabled = true;
		memcpy((void*)(g_libGTASA+0x599504), byteClockData, 10);
	}
	else
	{
		ToggleThePassingOfTime(false);
		m_bClockEnabled = false;
		memset((void*)(g_libGTASA+0x599504), 0, 10);
	}
}

// 0.3.7
void CGame::EnableZoneNames(bool bEnable)
{
	ScriptCommand(&enable_zone_names, bEnable);
}

void CGame::DisplayWidgets(bool bDisp)
{
	if(bDisp)
		*(uint16_t*)(g_libGTASA+0x8B82A0+0x10C) = 0;
	else
		*(uint16_t*)(g_libGTASA+0x8B82A0+0x10C) = 1;
}

// ��������
void CGame::PlaySound(int iSound, float fX, float fY, float fZ)
{
	ScriptCommand(&play_sound, fX, fY, fZ, iSound);
}

void CGame::ToggleRadar(bool iToggle)
{
	*(uint8_t*)(g_libGTASA+0x8EF36B) = (uint8_t)!iToggle;
}

void CGame::DisplayHUD(bool bDisp)
{
	if(bDisp)
	{	
		// CTheScripts11bDisplayHud
		*(uint8_t*)(g_libGTASA+0x7165E8) = 1;
		ToggleRadar(1);
	} else {
		*(uint8_t*)(g_libGTASA+0x7165E8) = 0;
		ToggleRadar(0);
	}
}

// 0.3.7
void CGame::RequestModel(unsigned int iModelID, int iLoadingStream)
{
	ScriptCommand(&request_model, iModelID);
}

// 0.3.7
void CGame::RequestVehicleUpgrade(uint16_t iModelId, uint8_t iLoadingStream) 
{
	if(iModelId < 0 || iModelId > 20000)
		return;
	
	//CStreaming::RequestVehicleUpgrade
	((void (*)(int32_t, int32_t))(g_libGTASA + 0x0028F2F8 + 1))(iModelId, iLoadingStream);
}

// 0.3.7
void CGame::LoadRequestedModels(bool bOnlyPriorityRequests)
{
	//CStreaming::LoadAllRequestedModels
	((void (*)(bool))(g_libGTASA + 0x00294CB4 + 1))(bOnlyPriorityRequests);
	//ScriptCommand(&load_requested_models);
}

// 0.3.7
uint8_t CGame::IsModelLoaded(unsigned int iModelID)
{
	return ScriptCommand(&is_model_available, iModelID);
}

// 0.3.7
void CGame::RefreshStreamingAt(float x, float y)
{
	ScriptCommand(&refresh_streaming_at, x, y);
}

// 0.3.7
void CGame::DisableTrainTraffic()
{
	ScriptCommand(&enable_train_traffic,0);
}

// 0.3.7
void CGame::SetMaxStats()
{
	// CCheat::VehicleSkillsCheat
	(( int (*)())(g_libGTASA+0x2BAED0+1))();

	// CCheat::WeaponSkillsCheat
	//(( int (*)())(g_libGTASA+0x2BAE68+1))();

	// CStats::SetStatValue nop
	WriteMemory(g_libGTASA+0x3B9074, (uintptr_t)"\xF7\x46", 2);
}

void CGame::SetWantedLevel(uint8_t byteLevel)
{
	WriteMemory(g_libGTASA+0x27D8D2, (uintptr_t)&byteLevel, 1);
}

bool CGame::IsAnimationLoaded(char *szAnimFile)
{
	return ScriptCommand(&is_animation_loaded, szAnimFile);
}

void CGame::RequestAnimation(char *szAnimFile)
{
	ScriptCommand(&request_animation, szAnimFile);
}

// 0.3.7
void CGame::DisplayGameText(char* szStr, int iTime, int iType)
{
	ScriptCommand(&text_clear_all);
	CFont::AsciiToGxtChar(szStr, szGameTextMessage);

	// CMessages::AddBigMesssage
	(( void (*)(uint16_t*, int, int))(g_libGTASA+0x4D18C0+1))(szGameTextMessage, iTime, iType);
}

// 0.3.7
void CGame::SetGravity(float fGravity)
{
	UnFuck(g_libGTASA+0x3A0B64);
	*(float*)(g_libGTASA+0x3A0B64) = fGravity;
}

float CGame::GetGravity()
{
	UnFuck(g_libGTASA+0x3A0B64);
	return *(float*)(g_libGTASA+0x3A0B64);
}

void CGame::ToggleCJWalk(bool bUseCJWalk)
{
	if(bUseCJWalk)
		WriteMemory(g_libGTASA+0x45477E, (uintptr_t)"\xC4\xF8\xDC\x64", 4);
	else
		NOP(g_libGTASA+0x45477E, 2);
}

void CGame::DisableMarker(uint32_t dwMarkerID)
{
	ScriptCommand(&disable_marker, dwMarkerID);
}

// 0.3.7
int CGame::GetLocalMoney()
{
	return *(int*)(PLAYERS_REALLOC+0xB8);
}

// 0.3.7
void CGame::AddToLocalMoney(int iAmmount)
{
	ScriptCommand(&add_to_player_money, 0, iAmmount);
}

// 0.3.7
void CGame::ResetLocalMoney()
{
	int iMoney = GetLocalMoney();
	if(!iMoney) return;

	if(iMoney < 0)
		AddToLocalMoney(abs(iMoney));
	else
		AddToLocalMoney(-(iMoney));
}

// 0.3.7
void CGame::DisableInteriorEnterExits()
{
	uintptr_t addr = *(uintptr_t*)(g_libGTASA+0x700120);
	int count = *(uint32_t*)(addr+8);
	Log("Count = %d", count);

	addr = *(uintptr_t*)addr;

	for(int i=0; i<count; i++)
	{
		*(uint16_t*)(addr+0x30) = 0;
		addr += 0x3C;
	}
}

extern uint8_t bGZ;
void CGame::DrawGangZone(float fPos[], uint32_t dwColor)
{
    (( void (*)(float*, uint32_t*, uint8_t))(g_libGTASA+0x3DE7F8+1))(fPos, &dwColor, bGZ);
}

void CGame::RemoveModel(int iModel, bool bFromStreaming)
{
	if(IsModelLoaded(iModel))
	{
		if(bFromStreaming)
		{
			if(ScriptCommand(&is_model_available, iModel)) 
			{
				// CStreaming::RemoveModel
				((void(*)(int))(g_libGTASA + 0x290C4C + 1))(iModel);
			}
		}
		else 
		{
			if(ScriptCommand(&is_model_available, iModel)) 
				ScriptCommand(&release_model, iModel);
		}
	}
}

void CGame::RemoveBuilding(uint32_t dwModel, VECTOR vecPosition, float fRange, bool bVisible) 
{
	#ifdef DEBUG_MODE
	Log("RemoveBuilding(%d, %f, %f, %f, %d, %d)", dwModel, vecPosition.X, vecPosition.Y, vecPosition.Z, fRange, bVisible);
	#endif

	if(!IsValidPosition(vecPosition))
		return;

	if(!IsTrueModelID(dwModel))
		return;

	int waitPreloaded;
	if(!IsModelLoaded(dwModel))
	{
		RequestModel(dwModel);
		LoadRequestedModels(false);
		while(!IsModelLoaded(dwModel)) 
		{
			usleep(500);
			waitPreloaded++;
			if(waitPreloaded >= 50)
			{
				pChatWindow->AddDebugMessage("Warning: building %u wouldn't load in time.", dwModel);
				return;
			}
		}
	}

	OBJECT_REMOVE objectToRemove;
	objectToRemove.dwModel = dwModel;
	objectToRemove.fRange = fRange;
	objectToRemove.vecPosition = vecPosition;

	m_vecObjectToRemove.push_back(objectToRemove);
	
	//ScriptCommand(&set_visibility_of_closest_object_of_type, vecPosition.X, vecPosition.Y, vecPosition.Z, fRange, dwModel, (int)bVisible);

	#ifdef DEBUG_MODE
	Log("Building %d removed", dwModel);
	#endif
}
