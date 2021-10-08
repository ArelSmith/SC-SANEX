#include "../main.h"
#include "game.h"
#include "net/netgame.h"
#include "net/localplayer.h"
#include "util/armhook.h"
#include "chatwindow.h"
#include "vehicle.h"

extern CGame* pGame;
extern CNetGame *pNetGame;
extern CLocalPlayer *pLocalPlayer;
extern CChatWindow *pChatWindow;

extern bool bUsedPlayerSlots[];

CPlayerPed::CPlayerPed()
{
	m_dwGTAId = 1;
	m_pPed = (PED_TYPE*)GamePool_FindPlayerPed();
	m_pEntity = (ENTITY_TYPE*)GamePool_FindPlayerPed();

	m_bytePlayerNumber = 0;
	SetPlayerPedPtrRecord(m_bytePlayerNumber, (uintptr_t)m_pPed);
	ScriptCommand(&set_actor_weapon_droppable, m_dwGTAId, 1);
	ScriptCommand(&set_actor_can_be_decapitated, m_dwGTAId, 0);

	for(int i = 0; i < MAX_PLAYER_ATTACHED; i++)
	{
		m_bObjectSlotUsed[i] = false;
		memset(&m_AttachedObjectInfo[i], 0, sizeof(ATTACHED_OBJECT));
		m_pAttachedObjects[i] = nullptr;
	}

	m_dwArrow = 0;
	m_bHaveBulletData = false;
	memset(&m_bulletData, 0, sizeof(BULLET_DATA));
	m_iJetpackState = 0;
	//m_iDancingStyle = 0;
	m_iCellPhoneEnabled = 0;
	m_iHandsUp = 0;
	m_bGoggleState = false;
	m_iParachuteButton = false;
	m_iParachuteAnim = 0;
	m_iParachuteState = 0;
	m_dwParachuteObject = 0;
}

CPlayerPed::CPlayerPed(uint8_t bytePlayerNumber, int iSkin, float fX, float fY, float fZ, float fRotation)
{
	uint32_t dwPlayerActorID = 0;
	int iPlayerNum = bytePlayerNumber;

	m_pPed = nullptr;
	m_pEntity = nullptr;
	m_dwGTAId = 0;

	ScriptCommand(&create_player, &iPlayerNum, fX, fY, fZ, &dwPlayerActorID);
	ScriptCommand(&create_actor_from_player, &iPlayerNum, &dwPlayerActorID);

	m_dwGTAId = dwPlayerActorID;
	m_pPed = (PED_TYPE*)GamePool_Ped_GetAt(m_dwGTAId);
	m_pEntity = (ENTITY_TYPE*)GamePool_Ped_GetAt(m_dwGTAId);

	m_bytePlayerNumber = bytePlayerNumber;
	SetPlayerPedPtrRecord(m_bytePlayerNumber, (uintptr_t)m_pPed);
	ScriptCommand(&set_actor_weapon_droppable, m_dwGTAId, 1);
	ScriptCommand(&set_actor_immunities, m_dwGTAId, 0, 0, 1, 0, 0);
	ScriptCommand(&set_actor_can_be_decapitated, m_dwGTAId, 0);


	if(pNetGame) SetMoney(pNetGame->m_iDeathDropMoney);

	SetModelIndex(iSkin);
	ForceTargetRotation(fRotation);

	MATRIX4X4 mat;
	GetMatrix(&mat);
	mat.pos.X = fX;
	mat.pos.Y = fY;
	mat.pos.Z = fZ + 0.15f;
	SetMatrix(mat);

	m_dwArrow = 0;
	m_bHaveBulletData = false;
	memset(&m_bulletData, 0, sizeof(BULLET_DATA));
	memset(&RemotePlayerKeys[m_bytePlayerNumber], 0, sizeof(PAD_KEYS));

	for(int i = 0; i < MAX_PLAYER_ATTACHED; i++)
	{
		m_bObjectSlotUsed[i] = false;
		memset(&m_AttachedObjectInfo[i], 0, sizeof(ATTACHED_OBJECT));
		m_pAttachedObjects[i] = nullptr;
	}

	m_iDanceState = 0;
	m_iCellPhoneEnabled = 0;
	m_iHandsUp = 0;
	m_bGoggleState = false;
}

CPlayerPed::~CPlayerPed()
{
	Destroy();
}

void CPlayerPed::Destroy()
{
	memset(&RemotePlayerKeys[m_bytePlayerNumber], 0, sizeof(PAD_KEYS));
	SetPlayerPedPtrRecord(m_bytePlayerNumber, 0);
	bUsedPlayerSlots[m_bytePlayerNumber] = false;

	if(!m_pPed || !GamePool_Ped_GetAt(m_dwGTAId) || m_pPed->entity.vtable == 0x5C7358)
	{
		Log("CPlayerPed::Destroy: invalid pointer/vtable");
		m_pPed = nullptr;
		m_pEntity = nullptr;
		m_dwGTAId = 0;
		return;
	}

	Log("Removing player attached object..");
	if(IsHaveAttachedObject())
		RemoveAllAttachedObjects();

	Log("Removing from vehicle..");
	RemoveFromVehicleAndPutAt(100.0f, 100.0f, 10.0f);

	// playerfix
	Log("Setting flag state..");
	uintptr_t dwPedPtr = (uintptr_t)m_pPed;
	*(uint32_t*)(*(uintptr_t*)(dwPedPtr + 1088) + 76) = 0;

	// CWorld::Remove
	/*Log("Calling destructor..");
	WorldRemoveEntity((uintptr_t)m_pEntity);*/

	// CPopulation::RemovePed
	Log("Calling destructor..");
	((void (*)(uintptr_t))(g_libGTASA + 0x45D82C + 1))((uintptr_t)m_pEntity);
	
	// CPlayerPed::Destructor
	/*Log("Calling destructor..");
	(( void (*)(PED_TYPE*))(*(void**)(m_pPed->entity.vtable+0x4)))(m_pPed);*/

	m_pPed = nullptr;
	m_pEntity = nullptr;
	m_dwGTAId = 0;
}

// 0.3.7
bool CPlayerPed::IsInVehicle()
{
	if(!m_pPed) return false;

	if(IN_VEHICLE(m_pPed))
		return true;

	return false;
}

// 0.3.7
bool CPlayerPed::IsAPassenger()
{
	if(!m_pPed) return false;

	if(m_pPed->pVehicle && IN_VEHICLE(m_pPed))
	{
		VEHICLE_TYPE *pVehicle = (VEHICLE_TYPE *)m_pPed->pVehicle;

		if(	pVehicle->pDriver != m_pPed ||
			pVehicle->entity.nModelIndex == TRAIN_PASSENGER ||
			pVehicle->entity.nModelIndex == TRAIN_FREIGHT )
			return true;
	}

	return false;
}

// 0.3.7
VEHICLE_TYPE* CPlayerPed::GetGtaVehicle()
{
	return (VEHICLE_TYPE*)m_pPed->pVehicle;
}

// 0.3.7
void CPlayerPed::RemoveFromVehicleAndPutAt(float fX, float fY, float fZ)
{
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;
	if(m_pPed && IN_VEHICLE(m_pPed))
		ScriptCommand(&remove_actor_from_car_and_put_at, m_dwGTAId, fX, fY, fZ);
}

// 0.3.7
void CPlayerPed::SetInitialState()
{
	if(!m_pPed) return;

	(( void (*)(PED_TYPE*))(g_libGTASA+0x458D1C+1))(m_pPed);
}

void CPlayerPed::ToggleCellphone(int iOn)
{
	if(!m_pPed) return;
	
	m_iCellPhoneEnabled = iOn;
	ScriptCommand(&toggle_actor_cellphone, m_dwGTAId, iOn);
}

int CPlayerPed::IsCellphoneEnabled()
{
	if(!m_pPed) return 0;
	
    return m_iCellPhoneEnabled;
}

// 0.3.7
bool CPlayerPed::IsInJetpackMode()
{
	if(!m_pPed) return false;

	return ScriptCommand(&is_player_using_jetpack, m_bytePlayerNumber);
}

// 0.3.7
void CPlayerPed::StartJetpack()
{
	if(!m_pPed) return;

	ScriptCommand(&task_jetpack, m_dwGTAId);
}

// 0.3.7
void CPlayerPed::StopJetpack()
{
	if(!m_pPed) return;

	ScriptCommand(&clear_char_tasks, m_dwGTAId);
	ScriptCommand(&clear_char_tasks_immediately, m_dwGTAId);
}

// 0.3.7
int CPlayerPed::HasHandsUp()
{
	if(!m_pPed) return 0;

	return m_iHandsUp;
}

// 0.3.7
void CPlayerPed::HandsUp()
{
	if(!m_pPed) return;

	m_iHandsUp = true;
	ScriptCommand(&task_hands_up, m_dwGTAId, 15000);
}

void CPlayerPed::PlayDance(int danceId)
{
	if(!m_pPed) return;

	switch(danceId)
	{
		case 1:
			pGame->FindPlayerPed()->ApplyAnimation("DANCE_LOOP", "WOP", 4.1, 1, 0, 0, 1, 1);
		break;

		case 2:
			pGame->FindPlayerPed()->ApplyAnimation("DANCE_LOOP", "GFUNK", 4.1, 1, 0, 0, 1, 1);
		break;

		case 3:
			pGame->FindPlayerPed()->ApplyAnimation("DANCE_LOOP", "RUNNINGMAN", 4.1, 1, 0, 0, 1, 1);
		break;

		case 4:
			pGame->FindPlayerPed()->ApplyAnimation("STR_LOOP_A", "STRIP", 4.1, 1, 0, 0, 1, 1);
		break;
	}

	m_iDanceState = 1;
	m_iDanceStyle = danceId;
}

void CPlayerPed::StopDancing()
{
	m_iHandsUp = false;
	m_iDanceState = 0;
	MATRIX4X4 mat;
	GetMatrix(&mat);
	TeleportTo(mat.pos.X,mat.pos.Y,mat.pos.Z);
}

bool CPlayerPed::IsDancing()
{
	if(m_iDanceState) return true;
	return false;
}

// 0.3.7
void CPlayerPed::SetHealth(float fHealth)
{
	if(!m_pPed) return;
	m_pPed->fHealth = fHealth;
	m_pPed->fMaxHealth = fHealth;
}

// 0.3.7
float CPlayerPed::GetHealth()
{
	if(!m_pPed) return 0.0f;
	return m_pPed->fHealth;
}

// 0.3.7
void CPlayerPed::SetArmour(float fArmour)
{
	if(!m_pPed) return;
	m_pPed->fArmour = fArmour;
}

float CPlayerPed::GetArmour()
{
	if(!m_pPed) return 0.0f;
	return m_pPed->fArmour;
}

void CPlayerPed::SetInterior(uint8_t byteID)
{
	if(!m_pPed) return;

	ScriptCommand(&select_interior, byteID);
	ScriptCommand(&link_actor_to_interior, m_dwGTAId, byteID);

	MATRIX4X4 mat;
	GetMatrix(&mat);
	ScriptCommand(&refresh_streaming_at, mat.pos.X, mat.pos.Y);
}

void CPlayerPed::PutDirectlyInVehicle(int iVehicleID, int iSeat)
{
	if(!m_pPed) return;
	if(!GamePool_Vehicle_GetAt(iVehicleID)) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	
	if(GetCurrentWeapon() == 46) {
		SetArmedWeapon(0);
	}

	VEHICLE_TYPE *pVehicle = GamePool_Vehicle_GetAt(iVehicleID);

	if(pVehicle->fHealth == 0.0f) return;
	// check is CPlaceable
	if (pVehicle->entity.vtable == g_libGTASA+0x5C7358) return;
	// check seatid (äîïèëèòü)

	if(iSeat == 0)
	{
		if(pVehicle->pDriver && IN_VEHICLE(pVehicle->pDriver)) return;
		ScriptCommand(&put_actor_in_car, m_dwGTAId, iVehicleID);
	}
	else
	{
		iSeat--;
		ScriptCommand(&put_actor_in_car2, m_dwGTAId, iVehicleID, iSeat);
	}

	if(m_pPed == GamePool_FindPlayerPed() && IN_VEHICLE(m_pPed))
		pGame->GetCamera()->SetBehindPlayer();

	if(pNetGame)
	{
		// äîïèëèòü (òðåéëåðû)
	}
}

WEAPON_SLOT_TYPE* CPlayerPed::GetCurrentWeaponSlot()
{
	if(!m_pPed || !GamePool_Ped_GetAt(m_dwGTAId))
		return nullptr;

	return &m_pPed->WeaponSlots[m_pPed->byteCurWeaponSlot];
}

void CPlayerPed::GiveWeapon(int iWeaponID, int iAmmo)
{
	if(!m_pPed) return;

	int uiWeaponModel = GameGetWeaponModelIDFromWeaponID(iWeaponID);
	if(uiWeaponModel != -1)
	{
		if(!pGame->IsModelLoaded(uiWeaponModel))
		{
			pGame->RequestModel(uiWeaponModel);
			pGame->LoadRequestedModels(false);
			while(!pGame->IsModelLoaded(uiWeaponModel)) 
				usleep(500);
		}

		// CPed::GiveWeapon
		((void (*)(PED_TYPE *, int, int, bool))(g_libGTASA+0x43429C+1))(m_pPed, iWeaponID, iAmmo, true);

		SetArmedWeapon(iWeaponID);
	}
}

WEAPON_SLOT_TYPE* CPlayerPed::FindWeaponSlot(int iWeapon) 
{
	if(!m_pPed) return nullptr;

	for(int i = 0; i < 13; i++) 
	{
		if(m_pPed->WeaponSlots[i].dwType == iWeapon)
			return &m_pPed->WeaponSlots[i];
	}
	return nullptr;
}

void CPlayerPed::SetAmmo(int iWeapon, int iAmmo) 
{
	WEAPON_SLOT_TYPE *pWeaponSlot = FindWeaponSlot(iWeapon);
	if(pWeaponSlot)
		pWeaponSlot->dwAmmo = iAmmo;
}

void CPlayerPed::SetArmedWeapon(int iWeaponType)
{
	if(!m_pPed) return;

	ScriptCommand(&set_current_player_weapon, m_dwGTAId, iWeaponType);
}

uint8_t CPlayerPed::GetCurrentWeapon() 
{
	if(!m_pPed) return 0;

	int dwRetVal = 0;
	ScriptCommand(&get_current_char_weapon, m_dwGTAId, &dwRetVal);

	return dwRetVal;
}

void CPlayerPed::ClearAllWeapons()
{
	if(!m_pPed) return;

	// CPed::ClearWeapons
	((void (*)(uintptr_t))(g_libGTASA+0x4345AC+1))((uintptr_t)m_pPed);
}

void CPlayerPed::EnterVehicle(int iVehicleID, bool bPassenger)
{
	if(!m_pPed) return;
	VEHICLE_TYPE* ThisVehicleType;
	if((ThisVehicleType = GamePool_Vehicle_GetAt(iVehicleID)) == 0) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	if(bPassenger)
	{
		if(ThisVehicleType->entity.nModelIndex == TRAIN_PASSENGER &&
			(m_pPed == GamePool_FindPlayerPed()))
		{
			ScriptCommand(&put_actor_in_car2, m_dwGTAId, iVehicleID, -1);
		}
		else
		{
			ScriptCommand(&send_actor_to_car_passenger,m_dwGTAId,iVehicleID, 3000, -1);
		}
	}
	else ScriptCommand(&send_actor_to_car_driverseat, m_dwGTAId, iVehicleID, 3000);
}

// 0.3.7
void CPlayerPed::ExitCurrentVehicle()
{
	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	VEHICLE_TYPE* ThisVehicleType = 0;

	if(IN_VEHICLE(m_pPed))
	{
		if(GamePool_Vehicle_GetIndex((VEHICLE_TYPE*)m_pPed->pVehicle))
		{
			int index = GamePool_Vehicle_GetIndex((VEHICLE_TYPE*)m_pPed->pVehicle);
			ThisVehicleType = GamePool_Vehicle_GetAt(index);
			if(ThisVehicleType)
			{
				if(	ThisVehicleType->entity.nModelIndex != TRAIN_PASSENGER &&
					ThisVehicleType->entity.nModelIndex != TRAIN_PASSENGER_LOCO)
				{
					ScriptCommand(&make_actor_leave_car, m_dwGTAId, GetCurrentVehicleID());
				}
			}
		}
	}
}

// 0.3.7
int CPlayerPed::GetCurrentVehicleID()
{
	if(!m_pPed) return 0;

	VEHICLE_TYPE *pVehicle = (VEHICLE_TYPE *)m_pPed->pVehicle;
	return GamePool_Vehicle_GetIndex(pVehicle);
}

int CPlayerPed::GetVehicleSeatID()
{
	if(!m_pPed) return (-1);

	VEHICLE_TYPE *pVehicle;

	if( GetActionTrigger() == ACTION_INCAR && (pVehicle = (VEHICLE_TYPE *)m_pPed->pVehicle) != 0 ) 
	{
		if(pVehicle->pDriver == m_pPed) return 0;
		if(pVehicle->pPassengers[0] == m_pPed) return 1;
		if(pVehicle->pPassengers[1] == m_pPed) return 2;
		if(pVehicle->pPassengers[2] == m_pPed) return 3;
		if(pVehicle->pPassengers[3] == m_pPed) return 4;
		if(pVehicle->pPassengers[4] == m_pPed) return 5;
		if(pVehicle->pPassengers[5] == m_pPed) return 6;
		if(pVehicle->pPassengers[6] == m_pPed) return 7;
	}

	return (-1);
}


void CPlayerPed::ShakeCam(int time){
	if(!m_pPed) return;

	ScriptCommand(&shake_cam, time);
}

// 0.3.7
void CPlayerPed::TogglePlayerControllable(bool bToggle)
{
	if(!m_pPed) return;

	MATRIX4X4 mat;

	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	if(!bToggle)
	{
		ScriptCommand(&toggle_player_controllable, m_bytePlayerNumber, 0);
		ScriptCommand(&lock_actor, m_dwGTAId, 1);
	}
	else
	{
		ScriptCommand(&toggle_player_controllable, m_bytePlayerNumber, 1);
		ScriptCommand(&lock_actor, m_dwGTAId, 0);
		if(!IsInVehicle()) 
		{
			GetMatrix(&mat);
			TeleportTo(mat.pos.X, mat.pos.Y, mat.pos.Z);
		}
	}
}

void CPlayerPed::TogglePlayerControllableWithoutLock(bool bToggle)
{
	if(!m_pPed) return;

	MATRIX4X4 mat;

	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	if(!bToggle)
	{
		ScriptCommand(&toggle_player_controllable, m_bytePlayerNumber, 0);
	}
	else
	{
		ScriptCommand(&toggle_player_controllable, m_bytePlayerNumber, 1);
		if(!IsInVehicle()) 
		{
			GetMatrix(&mat);
			TeleportTo(mat.pos.X, mat.pos.Y, mat.pos.Z);
		}
	}
}

// 0.3.7
void CPlayerPed::SetModelIndex(unsigned int uiModel)
{
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;
	if(!IsPedModel(uiModel))
		uiModel = 208; //0

	if(m_pPed)
	{
		// CClothes::RebuildPlayer nulled
		WriteMemory(g_libGTASA+0x3F1030, (uintptr_t)"\x70\x47", 2);
		DestroyFollowPedTask();
		CEntity::SetModelIndex(uiModel);

		// reset the Ped Audio Attributes
		(( void (*)(uintptr_t, uintptr_t))(g_libGTASA+0x34B2A8+1))(((uintptr_t)m_pPed+660), (uintptr_t)m_pPed);
	}
}

// äîïèëèòü
void CPlayerPed::DestroyFollowPedTask()
{

}

// äîïèëèòü
void CPlayerPed::ResetDamageEntity()
{
	if(m_pPed) 
	{
		m_pPed->pdwDamageEntity = 0;
		m_pPed->dwWeaponUsed = 255;
	}
}



// 0.3.7
void CPlayerPed::RestartIfWastedAt(VECTOR *vecRestart, float fRotation)
{	
	if(!m_pPed) return;

	ScriptCommand(&restart_if_wasted_at, vecRestart->X, vecRestart->Y, vecRestart->Z, fRotation, 0);
}

// 0.3.7
void CPlayerPed::ForceTargetRotation(float fRotation)
{
	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	m_pPed->fRotation1 = DegToRad(fRotation);
	m_pPed->fRotation2 = DegToRad(fRotation);

	ScriptCommand(&set_actor_z_angle,m_dwGTAId,fRotation);
}

void CPlayerPed::GetTargetRotation()
{
	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;
}

void CPlayerPed::SetRotation(float fRotation)
{
	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	m_pPed->fRotation1 = DegToRad(fRotation);
	m_pPed->fRotation2 = DegToRad(fRotation);
}

// 0.3.7
uint8_t CPlayerPed::GetActionTrigger()
{
	return (uint8_t)m_pPed->dwAction;
}

// 0.3.7
bool CPlayerPed::IsDead()
{
	if(!m_pPed) return true;
	if(m_pPed->fHealth > 0.0f) return false;

	return true;
}

void CPlayerPed::SetMoney(int iAmount)
{
	if(!m_pPed) return;
	ScriptCommand(&set_actor_money, m_dwGTAId, 0);
	ScriptCommand(&set_actor_money, m_dwGTAId, iAmount);
}

// 0.3.7
void CPlayerPed::ShowMarker(uint32_t iMarkerColorID)
{
	if(!m_pPed) return;
	if(m_dwArrow) HideMarker();

	ScriptCommand(&create_arrow_above_actor, m_dwGTAId, &m_dwArrow);
	ScriptCommand(&set_marker_color, m_dwArrow, iMarkerColorID);
	ScriptCommand(&show_on_radar2, m_dwArrow, 2);
}

// 0.3.7
void CPlayerPed::HideMarker()
{
	if(!m_pPed) return;

	if(m_dwArrow) ScriptCommand(&disable_marker, m_dwArrow);
	m_dwArrow = 0;
}

// 0.3.7
void CPlayerPed::SetFightingStyle(int iStyle)
{
	if(!m_pPed) return;
	ScriptCommand(&set_fighting_style, m_dwGTAId, iStyle, 6);
}

// 0.3.7
void CPlayerPed::ApplyAnimation(char *szAnimName, char *szAnimFile, float fT, int opt1, int opt2, int opt3, int opt4, int iUnk)
{
	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;
	if(!strcmp(szAnimFile, "SAMP")) return;

	int iWaitAnimLoad;
	if(!pGame->IsAnimationLoaded(szAnimFile))
	{
		pGame->RequestAnimation(szAnimFile);
		while(!pGame->IsAnimationLoaded(szAnimFile))
		{
			usleep(500);
			iWaitAnimLoad++;
			if(iWaitAnimLoad >= 50) 
				return;
		}
	}

	ScriptCommand(&apply_animation, m_dwGTAId, szAnimName, szAnimFile, fT, opt1, opt2, opt3, opt4, iUnk);
}

uint8_t CPlayerPed::FindDeathReasonAndResponsiblePlayer(PLAYERID *nPlayer)
{
	uint8_t byteDeathReason;
	PLAYERID PlayerIDWhoKilled;
	CVehiclePool *pVehiclePool;
	CPlayerPool *pPlayerPool;
	
	if(pNetGame) 
	{
		pVehiclePool = pNetGame->GetVehiclePool();
		pPlayerPool = pNetGame->GetPlayerPool();
	}
	else 
	{ // just leave if there's no netgame.
		*nPlayer = INVALID_PLAYER_ID;
		return 0xFF;
	}

	if(m_pPed)
	{
		byteDeathReason = (uint8_t)m_pPed->dwWeaponUsed;
		if(byteDeathReason < WEAPON_CAMERA || byteDeathReason == WEAPON_HELIBLADES || byteDeathReason == WEAPON_EXPLOSION) { // It's a weapon of some sort.

			if(m_pPed->pdwDamageEntity) { // check for a player pointer.

				PlayerIDWhoKilled = pPlayerPool->FindRemotePlayerIDFromGtaPtr((PED_TYPE *)m_pPed->pdwDamageEntity);
				if(PlayerIDWhoKilled != INVALID_PLAYER_ID) 
				{
					// killed by another player with a weapon, this is all easy.
					*nPlayer = PlayerIDWhoKilled;
					return byteDeathReason;
				}
				else
				{
					if(pVehiclePool->FindIDFromGtaPtr((VEHICLE_TYPE *)m_pPed->pdwDamageEntity) != INVALID_VEHICLE_ID) 
					{
						VEHICLE_TYPE *pGtaVehicle = (VEHICLE_TYPE *)m_pPed->pdwDamageEntity;
						PlayerIDWhoKilled = pPlayerPool->FindRemotePlayerIDFromGtaPtr((PED_TYPE *)pGtaVehicle->pDriver);
													
						if(PlayerIDWhoKilled != INVALID_PLAYER_ID) 
						{
							*nPlayer = PlayerIDWhoKilled;
							return byteDeathReason;
						}
					}
				}
			}
			//else { // weapon was used but who_killed is 0 (?)
			*nPlayer = INVALID_PLAYER_ID;
			return 0xFF;
			//}
		}
	}
	else if(byteDeathReason == WEAPON_DROWN) {
		*nPlayer = INVALID_PLAYER_ID;
		return WEAPON_DROWN;
	}
	else if(byteDeathReason == WEAPON_VEHICLE) {

		if(m_pPed->pdwDamageEntity) {
			// now, if we can find the vehicle
			// we can probably derive the responsible player.
			// Look in the vehicle pool for this vehicle.
			if(pVehiclePool->FindIDFromGtaPtr((VEHICLE_TYPE *)m_pPed->pdwDamageEntity) != INVALID_VEHICLE_ID)
			{
				VEHICLE_TYPE *pGtaVehicle = (VEHICLE_TYPE *)m_pPed->pdwDamageEntity;

				PlayerIDWhoKilled = pPlayerPool->
					FindRemotePlayerIDFromGtaPtr((PED_TYPE *)pGtaVehicle->pDriver);
										
				if(PlayerIDWhoKilled != INVALID_PLAYER_ID) {
					*nPlayer = PlayerIDWhoKilled;
					return WEAPON_VEHICLE;
				}
			}									
		}
	}
	else if(byteDeathReason == WEAPON_COLLISION) {

		if(m_pPed->pdwDamageEntity) {
			if(pVehiclePool->FindIDFromGtaPtr((VEHICLE_TYPE *)m_pPed->pdwDamageEntity) != INVALID_VEHICLE_ID)
			{
				VEHICLE_TYPE *pGtaVehicle = (VEHICLE_TYPE *)m_pPed->pdwDamageEntity;
									
				PlayerIDWhoKilled = pPlayerPool->
					FindRemotePlayerIDFromGtaPtr((PED_TYPE *)pGtaVehicle->pDriver);
					
				if(PlayerIDWhoKilled != INVALID_PLAYER_ID) {
					*nPlayer = PlayerIDWhoKilled;
					return WEAPON_COLLISION;
				}
			}
			else {
				*nPlayer = INVALID_PLAYER_ID;
				return WEAPON_COLLISION;
			}
		}
	}

	// Unhandled death type.
	*nPlayer = INVALID_PLAYER_ID;
	return 0xFF;
}

// 0.3.7
void CPlayerPed::GetBonePosition(int iBoneID, VECTOR* vecOut)
{
	if(!m_pPed) return;
	if(m_pEntity->vtable == g_libGTASA+0x5C7358) return;

	(( void (*)(PED_TYPE*, VECTOR*, int, int))(g_libGTASA+0x436590+1))(m_pPed, vecOut, iBoneID, 0);
}

ENTITY_TYPE* CPlayerPed::GetEntityUnderPlayer()
{
	uintptr_t entity;
	VECTOR vecStart;
	VECTOR vecEnd;
	char buf[100];

	if(!m_pPed) return nullptr;
	if( IN_VEHICLE(m_pPed) || !GamePool_Ped_GetAt(m_dwGTAId))
		return 0;

	vecStart.X = m_pPed->entity.mat->pos.X;
	vecStart.Y = m_pPed->entity.mat->pos.Y;
	vecStart.Z = m_pPed->entity.mat->pos.Z - 0.25f;

	vecEnd.X = m_pPed->entity.mat->pos.X;
	vecEnd.Y = m_pPed->entity.mat->pos.Y;
	vecEnd.Z = vecStart.Z - 1.75f;

	LineOfSight(&vecStart, &vecEnd, (void*)buf, (uintptr_t)&entity,
		0, 1, 0, 1, 0, 0, 0, 0);

	return (ENTITY_TYPE*)entity;
}

// äîïèëèòü
uint16_t CPlayerPed::GetKeys(uint16_t *lrAnalog, uint16_t *udAnalog, uint8_t *additionalKey) {
	*lrAnalog = LocalPlayerKeys.wKeyLR;
	*udAnalog = LocalPlayerKeys.wKeyUD;

	uint16_t wRet = 0;

	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_YES]) {
		*additionalKey = 0b01;
	}

	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_NO]) {
		*additionalKey = 0b10;
	}
	
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_CTRL_BACK]) {
		*additionalKey = 0b11;
	}
	
	// KEY_ANALOG_RIGHT
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_ANALOG_RIGHT]) wRet |= 1;
	wRet <<= 1;
	// KEY_ANALOG_LEFT
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_ANALOG_LEFT]) wRet |= 1;
	wRet <<= 1;
	// KEY_ANALOG_DOWN
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_ANALOG_DOWN]) wRet |= 1;
	wRet <<= 1;
	// KEY_ANALOG_UP
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_ANALOG_UP]) wRet |= 1;
	wRet <<= 1;
	// KEY_WALK
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_WALK]) wRet |= 1;
	wRet <<= 1;
	// KEY_SUBMISSION
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_SUBMISSION]) wRet |= 1;
	wRet <<= 1;
	// KEY_WALK
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_WALK]) wRet |= 1;
	wRet <<= 1;
	// KEY_SUBMISSION
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_SUBMISSION]) wRet |= 1;
	wRet <<= 1;
	// KEY_LOOK_LEFT
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_LOOK_LEFT]) wRet |= 1;
	wRet <<= 1;
	// KEY_HANDBRAKE
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE]) wRet |= 1;
	wRet <<= 1;
	// KEY_LOOK_RIGHT
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_LOOK_RIGHT]) wRet |= 1;
	wRet <<= 1;
	// KEY_JUMP
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP]) wRet |= 1;
	wRet <<= 1;
	// KEY_SECONDARY_ATTACK
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK]) wRet |= 1;
	wRet <<= 1;
	// KEY_SPRINT
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_SPRINT]) wRet |= 1;
	wRet <<= 1;
	// KEY_FIRE
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE]) wRet |= 1;
	wRet <<= 1;
	// KEY_CROUCH
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_CROUCH]) wRet |= 1;
	wRet <<= 1;
	// KEY_ACTION
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_ACTION]) wRet |= 1;

	memset(LocalPlayerKeys.bKeys, 0, ePadKeys::SIZE);

	return wRet;
}

uint8_t CPlayerPed::GetExtendedKeys()
{
	uint8_t result = 0;
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_YES])
	{
		LocalPlayerKeys.bKeys[ePadKeys::KEY_YES] = false;
		result = 1;
	}
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_NO])
	{
		LocalPlayerKeys.bKeys[ePadKeys::KEY_NO] = false;
		result = 2;
	}
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_CTRL_BACK])
	{
		LocalPlayerKeys.bKeys[ePadKeys::KEY_CTRL_BACK] = false;
		result = 3;
	}
	
	return result;
}

void CPlayerPed::SetKeys(uint16_t wKeys, uint16_t lrAnalog, uint16_t udAnalog)
{
	PAD_KEYS *pad = &RemotePlayerKeys[m_bytePlayerNumber];

	// LEFT/RIGHT
	pad->wKeyLR = lrAnalog;
	// UP/DOWN
	pad->wKeyUD = udAnalog;

	// KEY_ACTION
	pad->bKeys[ePadKeys::KEY_ACTION] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_CROUCH
	pad->bKeys[ePadKeys::KEY_CROUCH] = (wKeys & 1);
	if(!pad->bKeys[ePadKeys::KEY_CROUCH]) 
	{
		if(pad->bIgnoreHydraulicJump)
			pad->bIgnoreHydraulicJump = false;
	}
	wKeys >>= 1;

	// KEY_FIRE
	pad->bKeys[ePadKeys::KEY_FIRE] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_SPRINT
	pad->bKeys[ePadKeys::KEY_SPRINT] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_SECONDARY_ATTACK
	pad->bKeys[ePadKeys::KEY_SECONDARY_ATTACK] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_JUMP
	pad->bKeys[ePadKeys::KEY_JUMP] = (wKeys & 1);
	if(!pad->bKeys[ePadKeys::KEY_JUMP]) pad->bIgnoreJump = false;
	wKeys >>= 1;
	// KEY_LOOK_RIGHT
	pad->bKeys[ePadKeys::KEY_LOOK_RIGHT] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_HANDBRAKE
	pad->bKeys[ePadKeys::KEY_HANDBRAKE] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_LOOK_LEFT
	pad->bKeys[ePadKeys::KEY_LOOK_LEFT] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_SUBMISSION
	pad->bKeys[ePadKeys::KEY_SUBMISSION] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_WALK
	pad->bKeys[ePadKeys::KEY_WALK] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_ANALOG_UP
	pad->bKeys[ePadKeys::KEY_ANALOG_UP] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_ANALOG_DOWN
	pad->bKeys[ePadKeys::KEY_ANALOG_DOWN] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_ANALOG_LEFT
	pad->bKeys[ePadKeys::KEY_ANALOG_LEFT] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_ANALOG_RIGHT
	pad->bKeys[ePadKeys::KEY_ANALOG_RIGHT] = (wKeys & 1);
	wKeys >>= 1;

	// KEY_ACTION
	pad->bKeys[ePadKeys::KEY_ACTION] = (wKeys & 1);
	if(!pad->bKeys[ePadKeys::KEY_ACTION]) 
	{
		if(pad->bIgnoreNitroFired)
			pad->bIgnoreNitroFired = false;
	}
	wKeys >>= 1;

	return;
}

void CPlayerPed::ClumpUpdateAnimations(float step, int flag)
{
	if(!m_pPed || !GamePool_Ped_GetAt(m_dwGTAId))
		return;

	uintptr_t pRwObj = GetRWObject();
	if(pRwObj)
		((void (*)(uintptr_t, float, int))(g_libGTASA+0x33D6E4+1))(pRwObj, step, flag);
}

void CPlayerPed::SetPlayerAimState()
{
	if(!m_pPed) return;

	uintptr_t ped = (uintptr_t)m_pPed;
	uint8_t old = *(uint8_t*)(g_libGTASA + 0x008E864C); // CWorld::PlayerInFocus - 0x008E864C
	*(uint8_t*)(g_libGTASA + 0x008E864C) = m_bytePlayerNumber;

	((uint32_t(*)(uintptr_t, int, int, int))(g_libGTASA + 0x00454A6C + 1))(ped, 1, 1, 1); // CPlayerPed::ClearWeaponTarget
	*(uint8_t *)(*(uint32_t *)(ped + 1088) + 52) = *(uint8_t *)(*(uint32_t *)(ped + 1088) + 52) & 0xF7 | 8 * (1 & 1); // magic 

	*(uint8_t*)(g_libGTASA + 0x008E864C) = old;
}

void CPlayerPed::ClearPlayerAimState()
{
	if(!m_pPed) return;

	uintptr_t ped = (uintptr_t)m_pPed;
	uint8_t old = *(uint8_t*)(g_libGTASA + 0x008E864C);	// CWorld::PlayerInFocus - 0x008E864C
	*(uint8_t*)(g_libGTASA + 0x008E864C) = m_bytePlayerNumber;

	*(uint32_t *)(ped + 1432) = 0;	// unk
	((uint32_t(*)(uintptr_t, int, int, int))(g_libGTASA + 0x00454A6C + 1))(ped, 0, 0, 0);	// CPlayerPed::ClearWeaponTarget
	*(uint8_t *)(*(uint32_t *)(ped + 1088) + 52) = *(uint8_t *)(*(uint32_t *)(ped + 1088) + 52) & 0xF7 | 8 * (0 & 1);	// magic...

	*(uint8_t*)(g_libGTASA + 0x008E864C) = old;
}

void CPlayerPed::GetBoneMatrix(MATRIX4X4 *matOut, int iBoneID)
{
	if(!m_pPed) return;
	if(m_pPed->entity.vtable == g_libGTASA+0x5C7358) return;
	if(!m_pPed->entity.m_pRwObject) return;

	uintptr_t m_pRwObject = m_pPed->entity.m_pRwObject;
	uintptr_t pAnimHierarchy = (( uintptr_t (*)(uintptr_t))(g_libGTASA+0x559338+1))(m_pRwObject);
	int iAnimIndex = (( uintptr_t (*)(uintptr_t, uintptr_t))(g_libGTASA+0x19A448+1))(pAnimHierarchy, iBoneID) << 6;
	MATRIX4X4 *mat = (MATRIX4X4*)(iAnimIndex + *(uintptr_t*)(pAnimHierarchy+8));
	memcpy(matOut, mat, sizeof(MATRIX4X4));
}

void CPlayerPed::UpdateAttachedObject(uint32_t index, uint32_t model, uint32_t bone, VECTOR vecOffset, VECTOR vecRotation, VECTOR vecScale)
{
	if(!m_pPed || m_pPed->entity.vtable == g_libGTASA+0x5C7358 || !m_pPed->entity.m_pRwObject) 
		return;

	if(!IsTrueModelID(model))
		return;

	int waitPreloaded;
	if(!pGame->IsModelLoaded(model))
	{
		pGame->RequestModel(model);
		pGame->LoadRequestedModels(false);
		while(!pGame->IsModelLoaded(model)) 
		{
			usleep(500);
			waitPreloaded++;
			if(waitPreloaded >= 50)
			{
				pChatWindow->AddDebugMessage("Warning: attached object %u wouldn't load in time.", model);
				model = OBJECT_NOMODELFILE;
			}
		}
	}
	
	DeleteAttachedObjects(index);

	ATTACHED_OBJECT stAttachedObject;
	stAttachedObject.iModel = model;
	stAttachedObject.iBoneID = bone;
	stAttachedObject.vecOffset = vecOffset;
	stAttachedObject.vecRotation = vecRotation;
	stAttachedObject.vecScale = vecScale;
	//stAttachedObject.dwMaterialColor1 = materialcolor1;
	//stAttachedObject.dwMaterialColor2 = materialcolor2;

	MATRIX4X4 matrix;
	GetMatrix(&matrix);
	memcpy(&m_AttachedObjectInfo[index], &stAttachedObject, sizeof(ATTACHED_OBJECT));
	CObject *pNewObject = new CObject(model, 
		matrix.pos.X, 
		matrix.pos.Y, 
		matrix.pos.Z, 
		vecRotation, 150.0f);

	m_pAttachedObjects[index] = pNewObject;
	m_bObjectSlotUsed[index] = true;

	pNewObject->SetCollision(false);
	pNewObject->SetCollisionChecking(false);
}

bool CPlayerPed::GetObjectSlotState(int iObjectIndex)
{
	if(iObjectIndex < 0 || iObjectIndex >= 10)
		return false;

	return m_bObjectSlotUsed[iObjectIndex];
}

void CPlayerPed::ProcessAttachedObjects()
{
	if(!m_pPed || !GamePool_Ped_GetAt(m_dwGTAId))
		return;

	bool bAnimUpdated = false;
	MATRIX4X4 boneMatrix;
	VECTOR vecProj;

	for(int i = 0; i < MAX_PLAYER_ATTACHED; i++)
	{
		if(m_bObjectSlotUsed[i] && m_pAttachedObjects[i])
		{
			if(!m_pAttachedObjects[i]->m_pEntity)
				continue;

			CObject *pObject = m_pAttachedObjects[i];
			if(!pObject)
				continue;

			if(IsAdded())
			{
				if(!bAnimUpdated)
				{
					(( void (*)(ENTITY_TYPE*))(g_libGTASA+0x391968+1))(m_pEntity);
					bAnimUpdated = true;
				}
				
				((void (*) (ENTITY_TYPE*))(*(void**)(m_pEntity->vtable+16)))(pObject->m_pEntity);

				GetBoneMatrix(&boneMatrix, m_pPed->aPedBones[m_AttachedObjectInfo[i].iBoneID]->iNodeId);
				ProjectMatrix(&vecProj, &boneMatrix, &m_AttachedObjectInfo[i].vecOffset);

				boneMatrix.pos = vecProj;

				VECTOR *vecRot = &m_AttachedObjectInfo[i].vecRotation;
				if(vecRot->X != 0.0f) {
					RwMatrixRotate(&boneMatrix, 0, vecRot->X);
				}

				if(vecRot->Y != 0.0f) {
					RwMatrixRotate(&boneMatrix, 1, vecRot->Y);
				}

				if(vecRot->Z != 0.0f) {
					RwMatrixRotate(&boneMatrix, 2, vecRot->Z);
				}

				VECTOR *vecScale = &m_AttachedObjectInfo[i].vecScale;
				if(!vecScale) {
					continue;
				}

				RwMatrixScale(&boneMatrix, vecScale);

				pObject->SetMatrix(boneMatrix);
				pObject->UpdateRwMatrixAndFrame();

				((void (*) (ENTITY_TYPE*))(*(void**)(m_pEntity->vtable+8)))(pObject->m_pEntity);

				pObject->SetCollision(false);
				pObject->SetCollisionChecking(false);
			}
			else
			{
				pObject->TeleportTo(0.0f, 0.0f, 0.0f);
			}
		}
	}
}

bool CPlayerPed::IsHaveAttachedObject()
{
	for(int i = 0; i < MAX_PLAYER_ATTACHED; i++)
	{
		if(GetObjectSlotState(i))
			return true;
	}
	return false;
}

void CPlayerPed::DeleteAttachedObjects(int index)
{
	if(GetObjectSlotState(index))
	{
		CObject *pObject = m_pAttachedObjects[index];
		if(pObject)
		{
			delete pObject;
			m_pAttachedObjects[index] = nullptr;
		}

		memset(&m_AttachedObjectInfo[index], 0, sizeof(ATTACHED_OBJECT));
		m_bObjectSlotUsed[index] = false;
	}
}

void CPlayerPed::RemoveAllAttachedObjects()
{
	for(int index = 0; index < MAX_PLAYER_ATTACHED; index++)
	{
		if(GetObjectSlotState(index))
		{
			CObject *pObject = m_pAttachedObjects[index];
			if(pObject)
			{
				delete pObject;
				m_pAttachedObjects[index] = nullptr;
			}

			memset(&m_AttachedObjectInfo[index], 0, sizeof(ATTACHED_OBJECT));
			m_bObjectSlotUsed[index] = false;
		}
	}
}

void CPlayerPed::SetDead()
{
	if(m_dwGTAId && m_pPed) 
	{
		/*if(IsInJetpackMode()) StopJetpack();

		try {
			ExtinguishFire();
		} catch(...) {}*/

		MATRIX4X4 mat;
		GetMatrix(&mat);
		// will reset the tasks
		TeleportTo(mat.pos.X, mat.pos.Y, mat.pos.Z);
        m_pPed->fHealth = 0.0f;
		*(uint8_t*)(g_libGTASA + 0x008E864C) = m_bytePlayerNumber;
		ScriptCommand(&kill_actor, m_dwGTAId);
		*(uint8_t*)(g_libGTASA + 0x008E864C) = 0;
	}
}

void CPlayerPed::ExtinguishFire()
{
	if(m_pPed)
	{
		// Remove ped's fire if any
		uint32_t pFireObject = m_pPed->pFireObject;
		if(pFireObject != NULL)
		{
			((void(*) (uintptr_t))(g_libGTASA+0x39670C+1))(pFireObject);
		}

		// Remove ped's vehicle's fire if any
		if(IN_VEHICLE(m_pPed))
		{
			VEHICLE_TYPE *pVeh = (VEHICLE_TYPE *)m_pPed->pVehicle;
			pFireObject = pVeh->pFireObject;
			if(pFireObject)
			{
				((void(*) (uintptr_t))(g_libGTASA+0x39670C+1))(pFireObject);
			}
		}
	}
}

CPlayerPed *g_pCurrentFiredPed = nullptr;
BULLET_DATA *g_pCurrentBulletData = nullptr;
extern uint32_t (*CWeapon_FireInstantHit)(WEAPON_SLOT_TYPE* thiz, PED_TYPE* pFiringEntity, VECTOR* vecOrigin, VECTOR* muzzlePos, ENTITY_TYPE* targetEntity, VECTOR *target, VECTOR* originForDriveBy, int arg6, int muzzle);
void CPlayerPed::FireInstant()
{
	uint8_t byteSavedCameraMode = 0;
	uint16_t wSavedCameraMode2 = 0;
	
	if(!m_pPed) return;
	if(m_bytePlayerNumber != 0)
	{
		byteSavedCameraMode = *pbyteCameraMode;
		*pbyteCameraMode = GameGetPlayerCameraMode(m_bytePlayerNumber);
		
		wSavedCameraMode2 = *wCameraMode2;
		*wCameraMode2 = GameGetPlayerCameraMode(m_bytePlayerNumber);
		if(*wCameraMode2 == 4) *wCameraMode2 = 0;

		GameStoreLocalPlayerCameraExtZoomAndAspect();
		GameSetRemotePlayerCameraExtZoomAndAspect(m_bytePlayerNumber);

		GameStoreLocalPlayerAim();
		GameSetRemotePlayerAim(m_bytePlayerNumber);
		GameStoreLocalPlayerSkills();
		GameSetRemotePlayerSkills(m_bytePlayerNumber);
	}

	g_pCurrentFiredPed = this;

	if(m_bHaveBulletData)
	{
		g_pCurrentBulletData = &m_bulletData;
	}
	else
	{
		g_pCurrentBulletData = nullptr;
	}

	WEAPON_SLOT_TYPE *pSlot = GetCurrentWeaponSlot();
	if(pSlot)
	{
		if(GetCurrentWeapon() != 0)
		{
			VECTOR vecBone;
			VECTOR vecOut;

			GetWeaponInfoForFire(false, &vecBone, &vecOut);
			CWeapon_FireInstantHit(pSlot, m_pPed, &vecBone, &vecOut, nullptr, nullptr, nullptr, 0, 1);
		}
	}

	g_pCurrentFiredPed = nullptr;
	g_pCurrentBulletData = nullptr;

	if(m_bytePlayerNumber)
	{
		*pbyteCameraMode = byteSavedCameraMode;
		*wCameraMode2 = wSavedCameraMode2;

		GameSetLocalPlayerCameraExtZoomAndAspect();
		GameSetLocalPlayerAim();
		GameSetLocalPlayerSkills();
	}
}

void CPlayerPed::GetWeaponInfoForFire(int bLeft, VECTOR *vecBone, VECTOR *vecOut) 
{
	if(!IsValidGamePed(m_pPed) || !GamePool_Ped_GetAt(m_dwGTAId))
		return;
	
	if(IsGameEntityArePlaceable(&m_pPed->entity))
		return;
	
	VECTOR *pFireOffset = GetCurrentWeaponFireOffset();
	if(pFireOffset && vecBone && vecOut) 
	{
		vecOut->X = pFireOffset->X;
		vecOut->Y = pFireOffset->Y;
		vecOut->Z = pFireOffset->Z;

		int bone_id = 24;
		if(bLeft) {
			bone_id = 34;
		}
	
		// CPed::GetBonePosition
		((void (*)(PED_TYPE *, VECTOR *, int, bool))(g_libGTASA + 0x00436590 + 1))(m_pPed, vecBone, bone_id, false);

		vecBone->Z += pFireOffset->Z + 0.15f;

		// CPed::GetTransformedBonePosition
		((void (*)(PED_TYPE *, VECTOR *, int, bool))(g_libGTASA + 0x004383C0 + 1))(m_pPed, vecOut, bone_id, false);
	}
}

VECTOR* CPlayerPed::GetCurrentWeaponFireOffset() 
{
	if(!IsValidGamePed(m_pPed) || !GamePool_Ped_GetAt(m_dwGTAId))
		return nullptr;

	WEAPON_SLOT_TYPE *pSlot = GetCurrentWeaponSlot();
	if(pSlot)
		return (VECTOR *)(GetWeaponInfo(pSlot->dwType, 1) + 0x24);

	return nullptr;
}

void CPlayerPed::ProcessBulletData(BULLET_DATA *btData)
{
	if(!m_pPed) return;
	
	BULLET_SYNC_DATA bulletSyncData;

	if(btData)
	{
		m_bHaveBulletData = true;
		m_bulletData.pEntity = btData->pEntity;
		m_bulletData.vecOrigin.X = btData->vecOrigin.X;
		m_bulletData.vecOrigin.Y = btData->vecOrigin.Y;
		m_bulletData.vecOrigin.Z = btData->vecOrigin.Z;

		m_bulletData.vecPos.X = btData->vecPos.X;
		m_bulletData.vecPos.Y = btData->vecPos.Y;
		m_bulletData.vecPos.Z = btData->vecPos.Z;

		m_bulletData.vecOffset.X = btData->vecOffset.X;
		m_bulletData.vecOffset.Y = btData->vecOffset.Y;
		m_bulletData.vecOffset.Z = btData->vecOffset.Z;

		uint8_t byteHitType = 0;
		unsigned short InstanceID = 0xFFFF;

		if(m_bytePlayerNumber == 0)
		{
			if(pNetGame)
			{
				CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
				if(pPlayerPool)
				{
					CPlayerPed *pPlayerPed = pPlayerPool->GetLocalPlayer()->GetPlayerPed();
					if(pPlayerPed)
					{
						memset(&bulletSyncData, 0, sizeof(BULLET_SYNC_DATA));
						if(pPlayerPed->GetCurrentWeapon() != WEAPON_SNIPER || btData->pEntity)
						{
							if(btData->pEntity)
							{
								CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
								CObjectPool *pObjectPool = pNetGame->GetObjectPool();

								uint16_t PlayerID;
								uint16_t VehicleID;
								uint16_t ObjectID;

								if(pVehiclePool && pObjectPool)
								{
									PlayerID = pPlayerPool->FindRemotePlayerIDFromGtaPtr((PED_TYPE*)btData->pEntity);
									if(PlayerID == INVALID_PLAYER_ID)
									{
										VehicleID = pVehiclePool->FindIDFromGtaPtr((VEHICLE_TYPE*)btData->pEntity);
										if(VehicleID == INVALID_VEHICLE_ID)
										{
											ObjectID = pObjectPool->FindIDFromGtaPtr(btData->pEntity);
											if(ObjectID == INVALID_OBJECT_ID)
											{
												VECTOR vecOut;
												vecOut.X = 0.0f;
												vecOut.Y = 0.0f;
												vecOut.Z = 0.0f;
												
												if(btData->pEntity->mat)
												{
													ProjectMatrix(&vecOut, btData->pEntity->mat, &btData->vecOffset);
													btData->vecOffset.X = vecOut.X;
													btData->vecOffset.Y = vecOut.Y;
													btData->vecOffset.Z = vecOut.Z;
												}
												else
												{
													btData->vecOffset.X = btData->pEntity->mat->pos.X + btData->vecOffset.X;
													btData->vecOffset.Y = btData->pEntity->mat->pos.Y + btData->vecOffset.Y;
													btData->vecOffset.Z = btData->pEntity->mat->pos.Z + btData->vecOffset.Z;
												}
											}
											else
											{
												// object
												byteHitType = 3;
												InstanceID = ObjectID;
											}
										}
										else
										{
											// vehicle
											byteHitType = 2;
											InstanceID = VehicleID;
										}
									}
									else
									{
										// player
										byteHitType = 1;
										InstanceID = PlayerID;
									}
								}
							}

							bulletSyncData.vecOrigin.X = btData->vecOrigin.X;
							bulletSyncData.vecOrigin.Y = btData->vecOrigin.Y;
							bulletSyncData.vecOrigin.Z = btData->vecOrigin.Z;

							bulletSyncData.vecPos.X = btData->vecPos.X;
							bulletSyncData.vecPos.Y = btData->vecPos.Y;
							bulletSyncData.vecPos.Z = btData->vecPos.Z;

							bulletSyncData.vecOffset.X = btData->vecOffset.X;
							bulletSyncData.vecOffset.Y = btData->vecOffset.Y;
							bulletSyncData.vecOffset.Z = btData->vecOffset.Z;

							bulletSyncData.byteHitType = byteHitType;
							bulletSyncData.PlayerID = InstanceID;
							bulletSyncData.byteWeaponID = pPlayerPed->GetCurrentWeapon();

							RakNet::BitStream bsBullet;
							bsBullet.Write((char)ID_BULLET_SYNC);
							bsBullet.Write((char*)&bulletSyncData, sizeof(BULLET_SYNC_DATA));
							pNetGame->GetRakClient()->Send(&bsBullet, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
						}
					}
				}
			}
		}
	}
	else
	{
		m_bHaveBulletData = false;
		memset(&m_bulletData, 0, sizeof(BULLET_DATA));
	}
}

void CPlayerPed::ApplyCrouch()
{
	if(!IsValidGamePed(m_pPed) || !GamePool_Ped_GetAt(m_dwGTAId))
		return;

	uintptr_t ped = (uintptr_t)m_pPed;
	reinterpret_cast<int(*)(uintptr_t, uint16_t)>(g_libGTASA+0x44E0F4+1)(*((uintptr_t*)ped + 271), 0);
}

void CPlayerPed::ResetCrouch()
{
	if(!IsValidGamePed(m_pPed) || !GamePool_Ped_GetAt(m_dwGTAId))
		return;

	m_pPed->dwStateFlags &= 0xFBFFFFFF;
}

bool CPlayerPed::IsCrouching()
{
	if(!IsValidGamePed(m_pPed) || !GamePool_Ped_GetAt(m_dwGTAId))
		return false;

	return IS_CROUCHING(m_pPed);
}

bool CPlayerPed::StartPassengerDriveByMode(bool bDriveBy)
{
	if(m_pPed) {
		if(bDriveBy)
		{
		if(!IN_VEHICLE(m_pPed) || !m_pPed->pVehicle) return false;

		int iWeapon = GetCurrentWeapon();
		
		// Don't allow them to enter driveby with a para
		if(iWeapon == WEAPON_PARACHUTE) 
		{
			SetArmedWeapon(0);
			return false;
		}

		// Check for an uzi type weapon.
		if((iWeapon != WEAPON_UZI) && (iWeapon != WEAPON_MP5) && (iWeapon != WEAPON_TEC9) && 
			(iWeapon != WEAPON_SAWEDOFF))
			return false;

		SetArmedWeapon(iWeapon);
	
		/* We should replace them in their current seat.
		int iVehicleID = GamePool_Vehicle_GetIndex((VEHICLE_TYPE *)m_pPed->pVehicle);
		int iSeatID = GetVehicleSeatID();
		PutDirectlyInVehicle(iVehicleID,iSeatID);*/	

		ScriptCommand(&enter_passenger_driveby,
			m_dwGTAId,-1,-1,0.0f,0.0f,0.0f,300.0f,8,1,100);
		} else {
		int iVehicleID = GamePool_Vehicle_GetIndex((VEHICLE_TYPE *)m_pPed->pVehicle);
		EnterVehicle(iVehicleID, false);
		}

		//SetWeaponModelIndex(iWeapon);

		return true;
	}
	return false;
}

CAMERA_AIM * CPlayerPed::GetCurrentAim()
{
	return GameGetInternalAim();
}

void CPlayerPed::SetCurrentAim(CAMERA_AIM * pAim)
{
	GameStoreRemotePlayerAim(m_bytePlayerNumber, pAim);
}

float CPlayerPed::GetAimZ()
{
	return *(float*)(*((uintptr_t*)m_pPed + 272) + 84);
}

void CPlayerPed::SetAimZ(float fAimZ)
{
	*(float*)(*((uintptr_t*)m_pPed + 272) + 84) = fAimZ;
	//m_pPed + 272 - dwPlayerInfo
}

uint16_t CPlayerPed::GetCameraMode()
{
	return GameGetLocalPlayerCameraMode();
}

void CPlayerPed::SetCameraMode(uint16_t byteCamMode)
{
	GameSetPlayerCameraMode(byteCamMode, m_bytePlayerNumber);
}

float CPlayerPed::GetCameraExtendedZoom()
{
	return GameGetLocalPlayerCameraExtZoom();
}

void CPlayerPed::SetCameraExtendedZoom(float fZoom)
{
	GameSetPlayerCameraMode(m_bytePlayerNumber, fZoom);
}

void CPlayerPed::SetCameraZoomAndAspect(float fZoom, float fAspectRatio)
{
	GameSetPlayerCameraExtZoomAndAspect(m_bytePlayerNumber, fZoom, fAspectRatio);
}

ENTITY_TYPE* CPlayerPed::GetGtaContactEntity()
{
	if (!m_pPed->pContactEntity) return NULL;
	return (ENTITY_TYPE*)m_pPed->pContactEntity;
}

VEHICLE_TYPE* CPlayerPed::GetGtaContactVehicle()
{
	return (VEHICLE_TYPE*)m_pPed->pContactVehicle;
}

bool CPlayerPed::IsOnGround()
{
	if(m_pPed) 
	{
		if(m_pPed->dwStateFlags & 3)
			return true;
	}

	return false;
}

uint32_t CPlayerPed::GetStateFlags()
{	
	if(!m_pPed) return 0;
	return m_pPed->dwStateFlags;
}

void CPlayerPed::SetStateFlags(uint32_t dwState)
{	
	if(!m_pPed) return;
	m_pPed->dwStateFlags = dwState;
}	
void CPlayerPed::CheckVehicleParachute()
{
	if(m_dwParachuteObject)
	{
		ScriptCommand(&disassociate_object,m_dwParachuteObject,0.0f,0.0f,0.0f,0);
		ScriptCommand(&destroy_object,m_dwParachuteObject);
		m_dwParachuteObject = 0;
	}
}

//-----------------------------------------------------------

void CPlayerPed::ProcessParachutes()
{
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;
	if(!IsAdded()) return;

	if(m_iParachuteState == 0) {

		if(m_dwParachuteObject) {
			ScriptCommand(&disassociate_object,m_dwParachuteObject,0.0f,0.0f,0.0f,0);
			ScriptCommand(&destroy_object_with_fade,m_dwParachuteObject);
			m_dwParachuteObject = 0;
		}

		// See if we should enter the initial parachuting state.
		if(GetCurrentWeapon() == WEAPON_PARACHUTE) { // armed with para
			if(ScriptCommand(&is_actor_falling_think,m_dwGTAId)) { // is falling

				float fDistanceFromGround;

				ScriptCommand(&get_actor_distance_from_ground,m_dwGTAId,&fDistanceFromGround);
				if(fDistanceFromGround > 20.0f) {
					//ScriptCommand(&actor_set_collision,m_dwGTAId,0);
					m_iParachuteState = 1;
					m_iParachuteAnim = 0;
				}
			}
		}
		return;
	}
	
	if( (GetCurrentWeapon() != WEAPON_PARACHUTE) || 
		ScriptCommand(&is_actor_in_the_water,m_dwGTAId) ) {
		// A parachuting state is active, but they no longer have the parachute
		// or they've ended up in the water.
		if(m_dwParachuteObject) {
			MATRIX4X4 mat;
			ScriptCommand(&disassociate_object,m_dwParachuteObject,0.0f,0.0f,0.0f,0);
			ScriptCommand(&destroy_object_with_fade,m_dwParachuteObject);
			//ScriptCommand(&actor_set_collision,m_dwGTAId,1);
			GetMatrix(&mat);
			TeleportTo(mat.pos.X,mat.pos.Y,mat.pos.Z);						
			m_dwParachuteObject = 0;			
		}
		m_iParachuteState = 0;
		m_iParachuteAnim = 0;
	}		

	if(m_iParachuteState == 1) {
		//m_iParachuteButton = true;
		ProcessParachuteSkydiving();
		return;
	}
	
	if(m_iParachuteState == 2) {
		ProcessParachuting();
		return;
	}	
}

//-----------------------------------------------------------

void CPlayerPed::ProcessParachuteSkydiving()
{
	PAD_KEYS *pad = &RemotePlayerKeys[m_bytePlayerNumber];
	short sUpDown = (short)pad->bKeys[1];
	MATRIX4X4 mat;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;
	
	// if the parachute object isn't created, do it now.
	if(!m_dwParachuteObject) {
		GetMatrix(&mat);
		ScriptCommand(&create_object,OBJECT_PARACHUTE,mat.pos.X,mat.pos.Y,mat.pos.Z,&m_dwParachuteObject);
		
		if(!GamePool_Object_GetAt(m_dwParachuteObject)) {
			m_dwParachuteObject = 0;
			return;
		}

		ScriptCommand(&attach_object_to_actor,m_dwParachuteObject,m_dwGTAId,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f);
		ScriptCommand(&set_object_visible,m_dwParachuteObject,0);
	}

	if((sUpDown > 0) && (m_iParachuteAnim != PARA_DECEL)) {
		ApplyAnimation("PARA_DECEL","PARACHUTE",1.0f,1,0,0,1,-2);
		ScriptCommand(&apply_object_animation,m_dwParachuteObject,"PARA_DECEL_O","PARACHUTE",1.0f,1,1);
		ScriptCommand(&apply_object_animation,m_dwParachuteObject,"PARA_OPEN_O","PARACHUTE",1000.0f,0,1);
		ScriptCommand(&set_object_visible,m_dwParachuteObject,1);
		ScriptCommand(&set_object_scaling,m_dwParachuteObject,1.0f);
		m_iParachuteAnim = PARA_DECEL;
	}
	
	if(!GamePool_Object_GetAt(m_dwParachuteObject)) {
		m_dwParachuteObject = 0;
		return;
	}

	// process parachute opening event
	if(pad->bKeys[17]) {
		ApplyAnimation("PARA_OPEN","PARACHUTE",8.0f,0,0,0,1,-2);
		ScriptCommand(&apply_object_animation,m_dwParachuteObject,"PARA_OPEN_O","PARACHUTE",1000.0f,0,1);
		ScriptCommand(&set_object_visible,m_dwParachuteObject,1);
		ScriptCommand(&set_object_scaling,m_dwParachuteObject,1.0f);
		m_iParachuteState = 1;
		m_iParachuteAnim = 0;
		//m_iParachuteButton = true;
	}
}

//-----------------------------------------------------------

void CPlayerPed::ProcessParachuting()
{	
	PAD_KEYS *pad = &RemotePlayerKeys[m_bytePlayerNumber];
	short sUpDown = (short)pad->bKeys[1];

	if((sUpDown > 0) && (m_iParachuteAnim != PARA_DECEL)) {
		ApplyAnimation("PARA_DECEL","PARACHUTE",1.0f,1,0,0,1,-2);
		ScriptCommand(&apply_object_animation,m_dwParachuteObject,"PARA_DECEL_O","PARACHUTE",1.0f,1,1);
		m_iParachuteAnim = PARA_DECEL;
	}
	else if((sUpDown <= 0) && (m_iParachuteAnim != PARA_FLOAT)) {
		ApplyAnimation("PARA_FLOAT","PARACHUTE",1.0f,1,0,0,1,-2);
		ScriptCommand(&apply_object_animation,m_dwParachuteObject,"PARA_FLOAT_O","PARACHUTE",1.0f,1,1);
		m_iParachuteAnim = PARA_FLOAT;
	}
}
