#include "../main.h"
#include "../game/game.h"
#include "netgame.h"
#include "../spawnscreen.h"
#include "../extrakeyboard.h"
#include "../util/armhook.h"
#include "../chatwindow.h"

// voice
#include "../voice/MicroIcon.h"
#include "../voice/SpeakerList.h"

extern CGame *pGame;
extern CNetGame *pNetGame;
extern CSpawnScreen *pSpawnScreen;
extern CExtraKeyBoard *pExtraKeyBoard;
extern CChatWindow *pChatWindow;

bool bFirstSpawn = true;

extern int iNetModeNormalOnfootSendRate;
extern int iNetModeNormalInCarSendRate;
extern int iNetModeFiringSendRate;
extern bool bUsedPlayerSlots[];
extern int g_iLagCompensation;
#define IS_TARGETING(x) (x & 128)
#define IS_FIRING(x) (x & 4)
#define NETMODE_HEADSYNC_SENDRATE		1000
#define NETMODE_AIM_SENDRATE			100
#define NETMODE_FIRING_SENDRATE			30


CLocalPlayer::CLocalPlayer()
{
	m_pPlayerPed = pGame->FindPlayerPed();
	m_bIsActive = false;
	m_bIsWasted = false;

	m_iSelectedClass = 0;
	m_bHasSpawnInfo = false;
	m_bWaitingForSpawnRequestReply = false;
	m_bWantsAnotherClass = false;
	m_bInRCMode = false;

	memset(&m_OnFootData, 0, sizeof(ONFOOT_SYNC_DATA));

	m_dwLastSendTick = GetTickCount();
	m_dwLastSendAimTick = GetTickCount();
	m_dwLastSendBulletTick = GetTickCount();
	m_dwLastSendSpecTick = GetTickCount();
	m_dwLastUpdateOnFootData = GetTickCount();
	m_dwLastUpdateInCarData = GetTickCount();
	m_dwLastUpdatePassengerData = GetTickCount();
	m_dwPassengerEnterExit = GetTickCount();

	m_CurrentVehicle = 0;
	m_bSurfingMode = false;
	ResetAllSyncAttributes();

	m_bIsSpectating = false;
	m_byteSpectateType = SPECTATE_TYPE_NONE;
	m_SpectateID = 0xFFFFFFFF;

	for(unsigned char i = 0; i < 13; i++)
	{
		m_byteLastWeapon[i] = 0;
		m_dwLastAmmo[i] = 0;
	}
}

CLocalPlayer::~CLocalPlayer()
{
	// ~
}

void CLocalPlayer::ResetAllSyncAttributes()
{
	m_byteCurInterior = 0;
	m_LastVehicle = INVALID_VEHICLE_ID;
	m_bInRCMode = false;
}

bool CLocalPlayer::Process()
{
	uint32_t dwThisTick = GetTickCount();

	if(m_bIsActive && m_pPlayerPed)
	{
		// handle dead
		if(!m_bIsWasted && (m_pPlayerPed->GetActionTrigger() == ACTION_DEATH || m_pPlayerPed->IsDead()))
		{
			ToggleSpectating(false); // Player shouldn't die while spectating, but scripts may mess with that

			if(m_pPlayerPed->IsDancing() || m_pPlayerPed->HasHandsUp()) {
				m_pPlayerPed->StopDancing(); // there's no need to dance when you're dead
			}

			if(m_pPlayerPed->IsCellphoneEnabled()) {
				m_pPlayerPed->ToggleCellphone(0);
			}

			// reset tasks/anims
			m_pPlayerPed->TogglePlayerControllable(true);

			if(m_pPlayerPed->IsInVehicle() && !m_pPlayerPed->IsAPassenger())
			{
				SendInCarFullSyncData(); // One last time - for explosions
				m_LastVehicle = pNetGame->GetVehiclePool()->FindIDFromGtaPtr(m_pPlayerPed->GetGtaVehicle());
			}

			m_pPlayerPed->ExtinguishFire();

			SendWastedNotification();

			m_bIsActive = false;
			m_bIsWasted = true;

			// Disable zone names till they respawn (looks silly in request spawn)
			pGame->EnableZoneNames(0);

			// Clear all attached object
			if(m_pPlayerPed->IsHaveAttachedObject())
				m_pPlayerPed->RemoveAllAttachedObjects();

			return true;
		}

		if(m_pPlayerPed->IsInVehicle() && (m_pPlayerPed->IsDancing() || m_pPlayerPed->HasHandsUp())) 
			m_pPlayerPed->StopDancing(); // can't dance in vehicle

		// server checkpoints update
		pGame->UpdateCheckpoints();

		// check weapons
		CheckWeapons();

		// current weapon update
		uint8_t m_bCharWeapon = GetPlayerPed()->GetCurrentWeapon();
		m_pPlayerPed->SetArmedWeapon(m_bCharWeapon);

		// handle interior changing
		uint8_t byteInterior = pGame->GetActiveInterior();
		if(byteInterior != m_byteCurInterior)
			UpdateRemoteInterior(byteInterior);

		// Disabled weapons
		if((byteInterior != 0) && (!pNetGame->m_bAllowWeapons))
			m_pPlayerPed->SetArmedWeapon(0);

		// The regime for adjusting sendrates is based on the number
		// of players that will be effected by this update. The more players
		// there are within a small radius, the more we must scale back
		// the number of sends.
		int iNumberOfPlayersInLocalRange = DetermineNumberOfPlayersInLocalRange();
		if(!iNumberOfPlayersInLocalRange) iNumberOfPlayersInLocalRange = 20;

		// SPECTATING
		if(m_bIsSpectating)
		{
			ProcessSpectating();
			m_bPassengerDriveByMode = false;
		}

		// DRIVER
		else if(m_pPlayerPed->IsInVehicle() && !m_pPlayerPed->IsAPassenger())
		{
			CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
			CVehicle *pVehicle;
			if(pVehiclePool)
				m_CurrentVehicle = pVehiclePool->FindIDFromGtaPtr(m_pPlayerPed->GetGtaVehicle());

			pVehicle = pVehiclePool->GetAt(m_CurrentVehicle);

			if((dwThisTick - m_dwLastSendTick) > (unsigned int)GetOptimumInCarSendRate())
			{
				m_dwLastSendTick = GetTickCount();
				SendInCarFullSyncData();
			}

			ProcessVehicleDamageUpdates(m_CurrentVehicle);
			m_bPassengerDriveByMode = false;
		}

		// ONFOOT
		else if(m_pPlayerPed->GetActionTrigger() == ACTION_NORMAL || m_pPlayerPed->GetActionTrigger() == ACTION_SCOPE)
		{
			UpdateSurfing();

			if(m_CurrentVehicle != INVALID_VEHICLE_ID)
			{
				m_LastVehicle = m_CurrentVehicle;
				m_CurrentVehicle = INVALID_VEHICLE_ID;
			}

			if((dwThisTick - m_dwLastSendSyncTick) > (unsigned int)GetOptimumOnFootSendRate())
			{
				m_dwLastSendSyncTick = GetTickCount();
				SendOnFootFullSyncData();
			}

			uint8_t exKeys = 0;
			uint16_t lrAnalog, udAnalog;
			uint16_t wKeys = m_pPlayerPed->GetKeys(&lrAnalog, &udAnalog, &exKeys);
			
			if((dwThisTick - m_dwLastSendSyncTick) < NETMODE_HEADSYNC_SENDRATE)
			{
				if(IS_TARGETING(wKeys) && IS_FIRING(wKeys))
				{
					if(g_iLagCompensation == 2)
					{
						if((dwThisTick - m_dwLastSendSyncTick) > iNetModeFiringSendRate)
						{
							SendAimSyncData();
							m_dwLastSendSyncTick = GetTickCount();
						}
					}
					else if((dwThisTick - m_dwLastSendSyncTick) > NETMODE_AIM_SENDRATE)
					{
						SendAimSyncData();
						m_dwLastSendSyncTick = GetTickCount();
					}
				}
				else
				{
					if((dwThisTick - m_dwLastSendSyncTick) > NETMODE_HEADSYNC_SENDRATE)
					{
						SendAimSyncData();
						m_dwLastSendSyncTick = GetTickCount();
					}
				}
			}
		}

		// PASSENGER
		else if(m_pPlayerPed->IsInVehicle() && m_pPlayerPed->IsAPassenger())
		{
			if((dwThisTick - m_dwLastSendTick) > (unsigned int)GetOptimumInCarSendRate())
			{
				m_dwLastSendTick = GetTickCount();
				SendPassengerFullSyncData();
			}
		
			if(LocalPlayerKeys.bKeys[KEY_CROUCH])
			{
				int iWeapon = m_pPlayerPed->GetCurrentWeapon();
				if(iWeapon == WEAPON_UZI || iWeapon == WEAPON_MP5 || iWeapon == WEAPON_TEC9 || 
					iWeapon == WEAPON_SAWEDOFF) 
				{
					// NOT IN DRIVEBY MODE AND HORN HELD
					if(!m_bPassengerDriveByMode) 
					{
						// FOR ENTERING PASSENGER DRIVEBY MODE
						if(m_pPlayerPed->StartPassengerDriveByMode(true))
							m_bPassengerDriveByMode = true;
					}
					// IN DRIVEBY MODE AND HORN HELD
					else
					{
						// FOR LEAVING PASSENGER DRIVEBY MODE
						m_pPlayerPed->ClearPlayerAimState();
						m_bPassengerDriveByMode = false;
					}
				}
			}
		}
	}

	// handle !IsActive spectating
	if(m_bIsSpectating && !m_bIsActive)
	{
		ProcessSpectating();
		return true;
	}

	// handle needs to respawn
	if(m_bIsWasted && (m_pPlayerPed->GetActionTrigger() != ACTION_WASTED) && 
		(m_pPlayerPed->GetActionTrigger() != ACTION_DEATH) )
	{
		if( m_bClearedToSpawn && !m_bWantsAnotherClass &&
			pNetGame->GetGameState() == GAMESTATE_CONNECTED)
		{
			if(m_pPlayerPed->GetHealth() > 0.0f)
				Spawn();
		}
		else
		{
			m_bIsWasted = false;
			HandleClassSelection();
			m_bWantsAnotherClass = false;
		}

		return true;
	}

	return true;
}

void CLocalPlayer::SendWastedNotification()
{
	RakNet::BitStream bsPlayerDeath;
	uint8_t byteDeathReason = 0;
	PLAYERID WhoWasResponsible = INVALID_PLAYER_ID;
	
	//byteDeathReason = m_pPlayerPed->FindDeathReasonAndResponsiblePlayer(&WhoWasResponsible);

	bsPlayerDeath.Write(byteDeathReason);
	bsPlayerDeath.Write(WhoWasResponsible);
	pNetGame->GetRakClient()->RPC(&RPC_Death, &bsPlayerDeath, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
}

void CLocalPlayer::HandleClassSelection()
{
	m_bClearedToSpawn = false;

	if(m_pPlayerPed)
	{
		m_pPlayerPed->SetInitialState();
		m_pPlayerPed->SetHealth(100.0f);
		m_pPlayerPed->TogglePlayerControllable(0);
	}
	
	RequestClass(m_iSelectedClass);
	pSpawnScreen->Show(true);

	return;
}

// 0.3.7
void CLocalPlayer::HandleClassSelectionOutcome()
{
	if(m_pPlayerPed)
	{
		m_pPlayerPed->ClearAllWeapons();
		m_pPlayerPed->SetModelIndex(m_SpawnInfo.iSkin);
	}

	m_bClearedToSpawn = true;
}

void CLocalPlayer::SendNextClass()
{
	MATRIX4X4 matPlayer;
	m_pPlayerPed->GetMatrix(&matPlayer);

	if(m_iSelectedClass == (pNetGame->m_iSpawnsAvailable - 1)) m_iSelectedClass = 0;
		else m_iSelectedClass++;

	pGame->PlaySound(1052, matPlayer.pos.X, matPlayer.pos.Y, matPlayer.pos.Z);
	RequestClass(m_iSelectedClass);
}

void CLocalPlayer::SendPrevClass()
{
	MATRIX4X4 matPlayer;
	m_pPlayerPed->GetMatrix(&matPlayer);
	
	if(m_iSelectedClass == 0) m_iSelectedClass = (pNetGame->m_iSpawnsAvailable - 1);
		else m_iSelectedClass--;		

	pGame->PlaySound(1053, matPlayer.pos.X, matPlayer.pos.Y, matPlayer.pos.Z);
	RequestClass(m_iSelectedClass);
}

void CLocalPlayer::SendSpawn()
{
	RequestSpawn();
	m_bWaitingForSpawnRequestReply = true;
}

void CLocalPlayer::RequestClass(int iClass)
{
	RakNet::BitStream bsSpawnRequest;
	bsSpawnRequest.Write(iClass);
	pNetGame->GetRakClient()->RPC(&RPC_RequestClass, &bsSpawnRequest, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, 0);
}

void CLocalPlayer::RequestSpawn()
{
	RakNet::BitStream bsSpawnRequest;
	pNetGame->GetRakClient()->RPC(&RPC_RequestSpawn, &bsSpawnRequest, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, 0);
}

bool CLocalPlayer::HandlePassengerEntry()
{
	if(GetTickCount() - m_dwPassengerEnterExit < 1000 )
		return true;

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	// CTouchInterface::IsHoldDown
    int isHoldDown = (( int (*)(int, int, int))(g_libGTASA+0x270818+1))(0, 1, 1);

	if(isHoldDown)
	{
		VEHICLEID ClosetVehicleID = pVehiclePool->FindNearestToLocalPlayerPed();
		if(ClosetVehicleID < MAX_VEHICLES && pVehiclePool->GetSlotState(ClosetVehicleID))
		{
			CVehicle* pVehicle = pVehiclePool->GetAt(ClosetVehicleID);
			if(pVehicle->GetDistanceFromLocalPlayerPed() < 4.0f)
			{
				m_pPlayerPed->EnterVehicle(pVehicle->m_dwGTAId, true);
				SendEnterVehicleNotification(ClosetVehicleID, true);
				m_dwPassengerEnterExit = GetTickCount();
				return true;
			}
		}
	}

	return false;
}

bool CLocalPlayer::HandlePassengerEntryByCommand()
{
	if(GetTickCount() - m_dwPassengerEnterExit < 1000 )
		return true;

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	VEHICLEID ClosetVehicleID = pVehiclePool->FindNearestToLocalPlayerPed();
	if(ClosetVehicleID < MAX_VEHICLES && pVehiclePool->GetSlotState(ClosetVehicleID))
	{
		CVehicle* pVehicle = pVehiclePool->GetAt(ClosetVehicleID);

		if(pVehicle->GetDistanceFromLocalPlayerPed() < 4.0f)
		{
			m_pPlayerPed->EnterVehicle(pVehicle->m_dwGTAId, true);
			SendEnterVehicleNotification(ClosetVehicleID, true);
			m_dwPassengerEnterExit = GetTickCount();
			return true;
		}
	}

	return false;
}


void CLocalPlayer::UpdateSurfing() 
{
	VEHICLE_TYPE *Contact = m_pPlayerPed->GetGtaContactVehicle();

	if(!Contact) {
		m_bSurfingMode = FALSE;
		m_vecLockedSurfingOffsets.X = 0.0f;
		m_vecLockedSurfingOffsets.Y = 0.0f;
		m_vecLockedSurfingOffsets.Z = 0.0f;
		m_SurfingID = INVALID_VEHICLE_ID;
		return;
	}

	VEHICLEID vehID = pNetGame->GetVehiclePool()->FindIDFromGtaPtr(Contact);

	if(vehID && vehID != INVALID_VEHICLE_ID) {

		CVehicle *pVehicle = pNetGame->GetVehiclePool()->GetAt(vehID);

		if( pVehicle && pVehicle->IsOccupied() && 
			pVehicle->GetDistanceFromLocalPlayerPed() < 5.0f ) {

			VECTOR vecSpeed;
			VECTOR vecTurn;
			MATRIX4X4 matPlayer;
			MATRIX4X4 matVehicle;
			VECTOR vecVehiclePlane = {0.0f,0.0f,0.0f};

			pVehicle->GetMatrix(&matVehicle);
			m_pPlayerPed->GetMatrix(&matPlayer);

			//_Multiply3x3(&vecVehiclePlane,Contact->entity.mat,&vecVehiclePlane);
			/* if(m_bSurfingMode == TRUE && (lr || ud)) {
				// allow them to update their surfing offsets while
				// analog keys/stick is non-0
				m_vecLockedSurfingOffsets.X = matPlayer.pos.X - (matVehicle.pos.X + vecVehiclePlane.X);
				m_vecLockedSurfingOffsets.Y = matPlayer.pos.Y - (matVehicle.pos.Y + vecVehiclePlane.Y);
				m_vecLockedSurfingOffsets.Z = matPlayer.pos.Z - (matVehicle.pos.Z + vecVehiclePlane.Z);
				return;
			}*/

			m_bSurfingMode = true;
			m_SurfingID = vehID;

			m_vecLockedSurfingOffsets.X = matPlayer.pos.X - matVehicle.pos.X;
			m_vecLockedSurfingOffsets.Y = matPlayer.pos.Y - matVehicle.pos.Y;
			m_vecLockedSurfingOffsets.Z = matPlayer.pos.Z - matVehicle.pos.Z;

			vecSpeed.X = Contact->entity.vecMoveSpeed.X;
			vecSpeed.Y = Contact->entity.vecMoveSpeed.Y;
			vecSpeed.Z = Contact->entity.vecMoveSpeed.Z;
			vecTurn.X = Contact->entity.vecTurnSpeed.X;
			vecTurn.Y = Contact->entity.vecTurnSpeed.Y;
			vecTurn.Z = Contact->entity.vecTurnSpeed.Z;

			//m_pPlayerPed->SetMoveSpeedVector(vecSpeed);
			//m_pPlayerPed->SetTurnSpeedVector(vecTurn);
			return;
		}
	}
	m_bSurfingMode = false;
	m_vecLockedSurfingOffsets.X = 0.0f;
	m_vecLockedSurfingOffsets.Y = 0.0f;
	m_vecLockedSurfingOffsets.Z = 0.0f;
	m_SurfingID = INVALID_VEHICLE_ID;
}

void CLocalPlayer::SendEnterVehicleNotification(VEHICLEID VehicleID, bool bPassenger)
{
	RakNet::BitStream bsSend;
	bsSend.Write(VehicleID);
	bsSend.Write(bPassenger);

	pNetGame->GetRakClient()->RPC(&RPC_EnterVehicle, &bsSend, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, false, UNASSIGNED_NETWORK_ID, nullptr);

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	CVehicle* pVehicle = pVehiclePool->GetAt(VehicleID);

	if (pVehicle && pVehicle->IsATrainPart()) {
		uint32_t dwVehicle = pVehicle->m_dwGTAId;
		ScriptCommand(&camera_on_vehicle, dwVehicle, 3, 2);
	}
}

void CLocalPlayer::SendExitVehicleNotification(VEHICLEID VehicleID)
{
	RakNet::BitStream bsSend;

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	CVehicle* pVehicle = pVehiclePool->GetAt(VehicleID);

	if(pVehicle)
	{ 
		if (!m_pPlayerPed->IsAPassenger()) 
			m_LastVehicle = VehicleID;

		if ( pVehicle->IsATrainPart() )	{
			pGame->GetCamera()->SetBehindPlayer();
		}
		
		bsSend.Write(VehicleID);
		pNetGame->GetRakClient()->RPC(&RPC_ExitVehicle, &bsSend, HIGH_PRIORITY,RELIABLE_SEQUENCED, 0, false, UNASSIGNED_NETWORK_ID, NULL);
	}
}

void CLocalPlayer::UpdateRemoteInterior(uint8_t byteInterior)
{
	m_byteCurInterior = byteInterior;
	RakNet::BitStream bsUpdateInterior;
	bsUpdateInterior.Write(byteInterior);
	pNetGame->GetRakClient()->RPC(&RPC_SetInteriorId, &bsUpdateInterior, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}

void CLocalPlayer::SetSpawnInfo(PLAYER_SPAWN_INFO *pSpawn)
{
	memcpy(&m_SpawnInfo, pSpawn, sizeof(PLAYER_SPAWN_INFO));
	m_bHasSpawnInfo = true;
}

bool CLocalPlayer::Spawn()
{
	if(!m_bHasSpawnInfo) return false;
	
	// voice
	SpeakerList::Show();
    MicroIcon::Show();

	pSpawnScreen->Show(false);
	pExtraKeyBoard->Show(true);

	CCamera *pGameCamera;
	pGameCamera = pGame->GetCamera();
	pGameCamera->Restore();
	pGameCamera->SetBehindPlayer();
	pGame->DisplayWidgets(true);
	pGame->DisplayHUD(true);
	m_pPlayerPed->TogglePlayerControllable(true);

	if(!bFirstSpawn)
		m_pPlayerPed->SetInitialState();
	else
		bFirstSpawn = false;

	pGame->RefreshStreamingAt(m_SpawnInfo.vecPos.X,m_SpawnInfo.vecPos.Y);

	m_pPlayerPed->RestartIfWastedAt(&m_SpawnInfo.vecPos, m_SpawnInfo.fRotation);
	m_pPlayerPed->SetModelIndex(m_SpawnInfo.iSkin);
	m_pPlayerPed->ClearAllWeapons();
	m_pPlayerPed->ResetDamageEntity();

	pGame->DisableTrainTraffic();

	// CCamera::Fade
	WriteMemory(g_libGTASA+0x36EA2C, (uintptr_t)"\x70\x47", 2); // bx lr

	m_pPlayerPed->TeleportTo(m_SpawnInfo.vecPos.X,
		m_SpawnInfo.vecPos.Y, (m_SpawnInfo.vecPos.Z + 0.5f));

	m_pPlayerPed->ForceTargetRotation(m_SpawnInfo.fRotation);

	m_bIsWasted = false;
	m_bIsActive = true;
	m_bWaitingForSpawnRequestReply = false;

	RakNet::BitStream bsSendSpawn;
	pNetGame->GetRakClient()->RPC(&RPC_Spawn, &bsSendSpawn, HIGH_PRIORITY, 
		RELIABLE_SEQUENCED, 0, false, UNASSIGNED_NETWORK_ID, NULL);
	
	return true;
}

uint32_t CLocalPlayer::GetPlayerColor()
{
	return TranslateColorCodeToRGBA(pNetGame->GetPlayerPool()->GetLocalPlayerID());
}

uint32_t CLocalPlayer::GetPlayerColorAsARGB()
{
	return (TranslateColorCodeToRGBA(pNetGame->GetPlayerPool()->GetLocalPlayerID()) >> 8) | 0xFF000000;	
}

void CLocalPlayer::SetPlayerColor(uint32_t dwColor)
{
	SetRadarColor(pNetGame->GetPlayerPool()->GetLocalPlayerID(), dwColor);
}

void CLocalPlayer::ApplySpecialAction(uint8_t byteSpecialAction)
{
	switch(byteSpecialAction)
	{
		default:
		case SPECIAL_ACTION_NONE:
			// ~
		break;

		case SPECIAL_ACTION_USECELLPHONE:
			if(!m_pPlayerPed->IsInVehicle()) m_pPlayerPed->ToggleCellphone(1);
		break;

		case SPECIAL_ACTION_STOPUSECELLPHONE:
			if(m_pPlayerPed->IsCellphoneEnabled()) m_pPlayerPed->ToggleCellphone(0);
		break;

		case SPECIAL_ACTION_USEJETPACK:
			if(!m_pPlayerPed->IsInJetpackMode()) m_pPlayerPed->StartJetpack();
		break;
		
	/*	case SPECIAL_ACTION_SMOKE_CIGGY:
			if(!m_pPlayerPed->ProcessParachuteSkydiving()) m_pPlayerPed->ProcessParachutes();
		break;*/
		
		case SPECIAL_ACTION_HANDSUP:
			if(!m_pPlayerPed->HasHandsUp()) m_pPlayerPed->HandsUp();
		break;

		case SPECIAL_ACTION_DANCE1:
			m_pPlayerPed->PlayDance(1);
		break;

		case SPECIAL_ACTION_DANCE2:
			m_pPlayerPed->PlayDance(2);
		break;

		case SPECIAL_ACTION_DANCE3:
			m_pPlayerPed->PlayDance(3);
		break;

		case SPECIAL_ACTION_DANCE4:
			m_pPlayerPed->PlayDance(4);
		break;
	}
}

int CLocalPlayer::GetOptimumOnFootSendRate()
{
	if(!m_pPlayerPed) return 1000;

	return (iNetModeNormalOnfootSendRate + DetermineNumberOfPlayersInLocalRange());
}

int CLocalPlayer::GetOptimumInCarSendRate()
{
	if(!m_pPlayerPed) return 1000;

	return (iNetModeNormalInCarSendRate + DetermineNumberOfPlayersInLocalRange());
}

uint8_t CLocalPlayer::DetermineNumberOfPlayersInLocalRange()
{
	int iNumPlayersInRange = 0;
	for(int i=2; i < PLAYER_PED_SLOTS; i++)
		if(bUsedPlayerSlots[i]) iNumPlayersInRange++;

	return iNumPlayersInRange;
}

void CLocalPlayer::SendOnFootFullSyncData()
{
	RakNet::BitStream bsPlayerSync;
	MATRIX4X4 matPlayer;
	VECTOR vecMoveSpeed;
	uint16_t lrAnalog, udAnalog;
	uint8_t additionalKey = 0;
	uint16_t wKeys = m_pPlayerPed->GetKeys(&lrAnalog, &udAnalog, &additionalKey);

	ONFOOT_SYNC_DATA ofSync;

	m_pPlayerPed->GetMatrix(&matPlayer);
	m_pPlayerPed->GetMoveSpeedVector(&vecMoveSpeed);

	ofSync.lrAnalog = lrAnalog;
	ofSync.udAnalog = udAnalog;
	ofSync.wKeys = wKeys;
	ofSync.vecPos.X = matPlayer.pos.X;
	ofSync.vecPos.Y = matPlayer.pos.Y;
	ofSync.vecPos.Z = matPlayer.pos.Z;

	ofSync.quat.SetFromMatrix(matPlayer);
	ofSync.quat.Normalize();

	if( FloatOffset(ofSync.quat.w, m_OnFootData.quat.w) < 0.00001 &&
		FloatOffset(ofSync.quat.x, m_OnFootData.quat.x) < 0.00001 &&
		FloatOffset(ofSync.quat.y, m_OnFootData.quat.y) < 0.00001 &&
		FloatOffset(ofSync.quat.z, m_OnFootData.quat.z) < 0.00001)
	{
		ofSync.quat.Set(m_OnFootData.quat);
	}

	ofSync.byteHealth = (uint8_t)m_pPlayerPed->GetHealth();
	ofSync.byteArmour = (uint8_t)m_pPlayerPed->GetArmour();

	uint8_t exKeys = GetPlayerPed()->GetExtendedKeys();
	ofSync.byteCurrentWeapon = (additionalKey << 6) | ofSync.byteCurrentWeapon & 0x3F;
	ofSync.byteCurrentWeapon ^= (ofSync.byteCurrentWeapon ^ GetPlayerPed()->GetCurrentWeapon()) & 0x3F;
	ofSync.byteSpecialAction = GetSpecialAction();

	ofSync.vecMoveSpeed.X = vecMoveSpeed.X;
	ofSync.vecMoveSpeed.Y = vecMoveSpeed.Y;
	ofSync.vecMoveSpeed.Z = vecMoveSpeed.Z;

	if ( m_bSurfingMode ) {
		ofSync.vecSurfOffsets.X = m_vecLockedSurfingOffsets.X;
		ofSync.vecSurfOffsets.Y = m_vecLockedSurfingOffsets.Y;
		ofSync.vecSurfOffsets.Z = m_vecLockedSurfingOffsets.Z;
		ofSync.wSurfInfo = m_SurfingID;
	} else {
		ofSync.vecSurfOffsets.X = 0.0f;
		ofSync.vecSurfOffsets.Y = 0.0f;
		ofSync.vecSurfOffsets.Z = 0.0f;
		ofSync.wSurfInfo = 0;
	}

	ofSync.wAnimation = 0;
	ofSync.wAnimationFlags = 0;

	if((GetTickCount() - m_dwLastUpdateOnFootData) > 500 || memcmp(&m_OnFootData, &ofSync, sizeof(ONFOOT_SYNC_DATA)))
	{
		m_dwLastUpdateOnFootData = GetTickCount();

		bsPlayerSync.Write((uint8_t)ID_PLAYER_SYNC);
		bsPlayerSync.Write((char*)&ofSync, sizeof(ONFOOT_SYNC_DATA));
		pNetGame->GetRakClient()->Send(&bsPlayerSync, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);

		memcpy(&m_OnFootData, &ofSync, sizeof(ONFOOT_SYNC_DATA));
	}
}

void CLocalPlayer::SendInCarFullSyncData()
{
	RakNet::BitStream bsVehicleSync;
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(!pVehiclePool) return;

	MATRIX4X4 matPlayer;
	VECTOR vecMoveSpeed;

	uint16_t lrAnalog, udAnalog;
	uint8_t additionalKey = 0;
	uint16_t wKeys = m_pPlayerPed->GetKeys(&lrAnalog, &udAnalog, &additionalKey);
	CVehicle *pVehicle;

	INCAR_SYNC_DATA icSync;
	memset(&icSync, 0, sizeof(INCAR_SYNC_DATA));

	if(m_pPlayerPed)
	{
		icSync.VehicleID = pVehiclePool->FindIDFromGtaPtr(m_pPlayerPed->GetGtaVehicle());

		if(icSync.VehicleID == INVALID_VEHICLE_ID) return;

		icSync.lrAnalog = lrAnalog;
		icSync.udAnalog = udAnalog;
		icSync.wKeys = wKeys;

		pVehicle = pVehiclePool->GetAt(icSync.VehicleID);
		if(!pVehicle) return;

		pVehicle->GetMatrix(&matPlayer);
		pVehicle->GetMoveSpeedVector(&vecMoveSpeed);

		icSync.quat.SetFromMatrix(matPlayer);
		icSync.quat.Normalize();

		if(	FloatOffset(icSync.quat.w, m_InCarData.quat.w) < 0.00001 &&
			FloatOffset(icSync.quat.x, m_InCarData.quat.x) < 0.00001 &&
			FloatOffset(icSync.quat.y, m_InCarData.quat.y) < 0.00001 &&
			FloatOffset(icSync.quat.z, m_InCarData.quat.z) < 0.00001)
		{
			icSync.quat.Set(m_InCarData.quat);
		}

		// pos
		icSync.vecPos.X = matPlayer.pos.X;
		icSync.vecPos.Y = matPlayer.pos.Y;
		icSync.vecPos.Z = matPlayer.pos.Z;

		// move speed
		icSync.vecMoveSpeed.X = vecMoveSpeed.X;
		icSync.vecMoveSpeed.Y = vecMoveSpeed.Y;
		icSync.vecMoveSpeed.Z = vecMoveSpeed.Z;

		icSync.fCarHealth = pVehicle->GetHealth();
		icSync.bytePlayerHealth = (uint8_t)m_pPlayerPed->GetHealth();
		icSync.bytePlayerArmour = (uint8_t)m_pPlayerPed->GetArmour();

		if( pVehicle->GetModelIndex() == TRAIN_PASSENGER_LOCO ||
			pVehicle->GetModelIndex() == TRAIN_FREIGHT_LOCO ||
			pVehicle->GetModelIndex() == TRAIN_TRAM) {
			icSync.fTrainSpeed = pVehicle->GetTrainSpeed();
		} else {
			icSync.fTrainSpeed = 0.0f;
		}

		icSync.TrailerID = 0;
		VEHICLE_TYPE* vehTrailer = (VEHICLE_TYPE*)pVehicle->m_pVehicle->dwTrailer;
		if(vehTrailer != NULL)	
		{
			if(ScriptCommand(&is_trailer_on_cab, 
				pVehiclePool->FindIDFromGtaPtr(vehTrailer), 
				pVehicle->m_dwGTAId))
			{
				icSync.TrailerID = pVehiclePool->FindIDFromGtaPtr(vehTrailer);
			} else {
				icSync.TrailerID = 0;
			}
		}

		uint8_t exKeys = GetPlayerPed()->GetExtendedKeys();
		icSync.byteCurrentWeapon = (exKeys << 6) | icSync.byteCurrentWeapon & 0x3F;
		icSync.byteCurrentWeapon ^= (icSync.byteCurrentWeapon ^ GetPlayerPed()->GetCurrentWeapon()) & 0x3F;

		icSync.byteSirenOn = pVehicle->IsSirenOn();
		//icSync.byteLandingGearState = pVehicle->GetLandingGearState() != 0;

		// send
		if( (GetTickCount() - m_dwLastUpdateInCarData) > 500 || memcmp(&m_InCarData, &icSync, sizeof(INCAR_SYNC_DATA)))
		{
			m_dwLastUpdateInCarData = GetTickCount();

			bsVehicleSync.Write((uint8_t)ID_VEHICLE_SYNC);
			bsVehicleSync.Write((char*)&icSync, sizeof(INCAR_SYNC_DATA));
			pNetGame->GetRakClient()->Send(&bsVehicleSync, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);

			memcpy(&m_InCarData, &icSync, sizeof(INCAR_SYNC_DATA));
		}

		if(icSync.TrailerID && icSync.TrailerID < MAX_VEHICLES)
		{
			MATRIX4X4 matTrailer;
			TRAILER_SYNC_DATA trSync;
			CVehicle* pTrailer = pVehiclePool->GetAt(icSync.TrailerID);
			if(pTrailer)
			{
				pTrailer->GetMatrix(&matTrailer);

				trSync.quat.SetFromMatrix(matTrailer);
				trSync.quat.Normalize();

				if(	FloatOffset(trSync.quat.w, m_TrailerData.quat.w) < 0.00001 &&
					FloatOffset(trSync.quat.x, m_TrailerData.quat.x) < 0.00001 &&
					FloatOffset(trSync.quat.y, m_TrailerData.quat.y) < 0.00001 &&
					FloatOffset(trSync.quat.z, m_TrailerData.quat.z) < 0.00001)
				{
					trSync.quat.Set(m_TrailerData.quat);
				}
				
				trSync.vecPos.X = matTrailer.pos.X;
				trSync.vecPos.Y = matTrailer.pos.Y;
				trSync.vecPos.Z = matTrailer.pos.Z;
				
				pTrailer->GetMoveSpeedVector(&trSync.vecMoveSpeed);
				pTrailer->GetTurnSpeedVector(&trSync.vecTurnSpeed);

				RakNet::BitStream bsTrailerSync;
				bsTrailerSync.Write((uint8_t)ID_TRAILER_SYNC);
				bsTrailerSync.Write((char*)&trSync, sizeof (TRAILER_SYNC_DATA));
				pNetGame->GetRakClient()->Send(&bsTrailerSync,HIGH_PRIORITY,UNRELIABLE_SEQUENCED,0);

				memcpy(&m_TrailerData, &trSync, sizeof(TRAILER_SYNC_DATA));
			}
		}
	}
}

void CLocalPlayer::SendPassengerFullSyncData()
{
	RakNet::BitStream bsPassengerSync;
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	uint16_t lrAnalog, udAnalog;
	uint8_t additionalKey = 0;
	uint16_t wKeys = m_pPlayerPed->GetKeys(&lrAnalog, &udAnalog, &additionalKey);
	PASSENGER_SYNC_DATA psSync;
	MATRIX4X4 mat;

	psSync.VehicleID = pVehiclePool->FindIDFromGtaPtr(m_pPlayerPed->GetGtaVehicle());

	if(psSync.VehicleID == INVALID_VEHICLE_ID) return;

	psSync.lrAnalog = lrAnalog;
	psSync.udAnalog = udAnalog;
	psSync.wKeys = wKeys;
	psSync.bytePlayerHealth = (uint8_t)m_pPlayerPed->GetHealth();
	psSync.bytePlayerArmour = (uint8_t)m_pPlayerPed->GetArmour();

	psSync.byteSeatFlags = m_pPlayerPed->GetVehicleSeatID();
	psSync.byteDriveBy = m_bPassengerDriveByMode;

	uint8_t exKeys = GetPlayerPed()->GetExtendedKeys();
	psSync.byteCurrentWeapon = (exKeys << 6) | psSync.byteCurrentWeapon & 0x3F;
	psSync.byteCurrentWeapon ^= (psSync.byteCurrentWeapon ^ GetPlayerPed()->GetCurrentWeapon()) & 0x3F;

	m_pPlayerPed->GetMatrix(&mat);
	psSync.vecPos.X = mat.pos.X;
	psSync.vecPos.Y = mat.pos.Y;
	psSync.vecPos.Z = mat.pos.Z;

	// send
	if((GetTickCount() - m_dwLastUpdatePassengerData) > 500 || memcmp(&m_PassengerData, &psSync, sizeof(PASSENGER_SYNC_DATA)))
	{
		m_dwLastUpdatePassengerData = GetTickCount();

		bsPassengerSync.Write((uint8_t)ID_PASSENGER_SYNC);
		bsPassengerSync.Write((char*)&psSync, sizeof(PASSENGER_SYNC_DATA));
		pNetGame->GetRakClient()->Send(&bsPassengerSync, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);

		memcpy(&m_PassengerData, &psSync, sizeof(PASSENGER_SYNC_DATA));
	}
	
	if(m_bPassengerDriveByMode)	SendAimSyncData();
}

void CLocalPlayer::SendAimSyncData()
{
	AIM_SYNC_DATA aimSync;
	CAMERA_AIM * caAim = m_pPlayerPed->GetCurrentAim();

	aimSync.byteCamMode = (uint8_t)m_pPlayerPed->GetCameraMode();
	aimSync.vecAimf1.X = caAim->f1x;
	aimSync.vecAimf1.Y = caAim->f1y;
	aimSync.vecAimf1.Z = caAim->f1z;
	aimSync.vecAimPos.X = caAim->pos1x;
	aimSync.vecAimPos.Y = caAim->pos1y;
	aimSync.vecAimPos.Z = caAim->pos1z;

	aimSync.fAimZ = m_pPlayerPed->GetAimZ();

	aimSync.byteCamExtZoom = (uint8_t)(m_pPlayerPed->GetCameraExtendedZoom() * 63.0f);

	WEAPON_SLOT_TYPE* pwstWeapon = m_pPlayerPed->GetCurrentWeaponSlot();
	if(pwstWeapon->dwState == 2)
		aimSync.byteWeaponState = WS_RELOADING;
	else
		aimSync.byteWeaponState = (pwstWeapon->dwAmmoInClip > 1) ? WS_MORE_BULLETS : pwstWeapon->dwAmmoInClip;

	if ((GetTickCount() - m_dwLastSendTick) > 500 || memcmp(&m_AimData, &aimSync, sizeof(AIM_SYNC_DATA)))
	{
		RakNet::BitStream bsAimSync;
		bsAimSync.Write((uint8_t)ID_AIM_SYNC);
		bsAimSync.Write((char*)&aimSync, sizeof(AIM_SYNC_DATA));
		pNetGame->GetRakClient()->Send(&bsAimSync, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
	}
}

void CLocalPlayer::ProcessSpectating()
{
	RakNet::BitStream bsSpectatorSync;
	SPECTATOR_SYNC_DATA spSync;
	MATRIX4X4 matPos;

	uint16_t lrAnalog, udAnalog;
	uint8_t additionalKey = 0;
	uint16_t wKeys = m_pPlayerPed->GetKeys(&lrAnalog, &udAnalog, &additionalKey);
	pGame->GetCamera()->GetMatrix(&matPos);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	if(!pPlayerPool || !pVehiclePool) return;

	spSync.vecPos.X = matPos.pos.X;
	spSync.vecPos.Y = matPos.pos.Y;
	spSync.vecPos.Z = matPos.pos.Z;
	spSync.lrAnalog = lrAnalog;
	spSync.udAnalog = udAnalog;
	spSync.wKeys = wKeys;

	if((GetTickCount() - m_dwLastSendSpecTick) > GetOptimumOnFootSendRate())
	{
		m_dwLastSendSpecTick = GetTickCount();
		bsSpectatorSync.Write((uint8_t)ID_SPECTATOR_SYNC);
		bsSpectatorSync.Write((char*)&spSync, sizeof(SPECTATOR_SYNC_DATA));
		pNetGame->GetRakClient()->Send(&bsSpectatorSync, HIGH_PRIORITY, UNRELIABLE, 0);

		if((GetTickCount() - m_dwLastSendAimTick) > (GetOptimumOnFootSendRate() * 2))
		{
			m_dwLastSendAimTick = GetTickCount();
			SendAimSyncData();
		}
	}

	pGame->DisplayHUD(false);

	m_pPlayerPed->SetHealth(100.0f);
	GetPlayerPed()->TeleportTo(spSync.vecPos.X, spSync.vecPos.Y, spSync.vecPos.Z + 20.0f);

	// handle spectate player left the server
	if(m_byteSpectateType == SPECTATE_TYPE_PLAYER &&
		!pPlayerPool->GetSlotState(m_SpectateID))
	{
		m_byteSpectateType = SPECTATE_TYPE_NONE;
		m_bSpectateProcessed = false;
	}

	// handle spectate player is no longer active (ie Died)
	if(m_byteSpectateType == SPECTATE_TYPE_PLAYER &&
		pPlayerPool->GetSlotState(m_SpectateID) &&
		(!pPlayerPool->GetAt(m_SpectateID)->IsActive() ||
		pPlayerPool->GetAt(m_SpectateID)->GetState() == PLAYER_STATE_WASTED))
	{
		m_byteSpectateType = SPECTATE_TYPE_NONE;
		m_bSpectateProcessed = false;
	}

	if(m_bSpectateProcessed) return;

	if(m_byteSpectateType == SPECTATE_TYPE_NONE)
	{
		GetPlayerPed()->RemoveFromVehicleAndPutAt(0.0f, 0.0f, 10.0f);
		pGame->GetCamera()->SetPosition(50.0f, 50.0f, 50.0f, 0.0f, 0.0f, 0.0f);
		pGame->GetCamera()->LookAtPoint(60.0f, 60.0f, 50.0f, 2);
		m_bSpectateProcessed = true;
	}
	else if(m_byteSpectateType == SPECTATE_TYPE_PLAYER)
	{
		uint32_t dwGTAId = 0;
		CPlayerPed *pPlayerPed = 0;

		if(pPlayerPool->GetSlotState(m_SpectateID))
		{
			pPlayerPed = pPlayerPool->GetAt(m_SpectateID)->GetPlayerPed();
			if(pPlayerPed)
			{
				dwGTAId = pPlayerPed->m_dwGTAId;
				ScriptCommand(&camera_on_actor, dwGTAId, m_byteSpectateMode, 2);
				m_bSpectateProcessed = true;
			}
		}
	}
	else if(m_byteSpectateType == SPECTATE_TYPE_VEHICLE)
	{
		CVehicle *pVehicle = nullptr;
		uint32_t dwGTAId = 0;

		if (pVehiclePool->GetSlotState((VEHICLEID)m_SpectateID)) 
		{
			pVehicle = pVehiclePool->GetAt((VEHICLEID)m_SpectateID);
			if(pVehicle) 
			{
				dwGTAId = pVehicle->m_dwGTAId;
				ScriptCommand(&camera_on_vehicle, dwGTAId, m_byteSpectateMode, 2);
				m_bSpectateProcessed = true;
			}
		}
	}	
}

void CLocalPlayer::ToggleSpectating(bool bToggle)
{
	if(m_bIsSpectating && !bToggle)
		Spawn();

	m_bIsSpectating = bToggle;
	m_byteSpectateType = SPECTATE_TYPE_NONE;
	m_SpectateID = 0xFFFFFFFF;
	m_bSpectateProcessed = false;
}

void CLocalPlayer::SpectatePlayer(PLAYERID playerId)
{
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	if(pPlayerPool && pPlayerPool->GetSlotState(playerId))
	{
		if(pPlayerPool->GetAt(playerId)->GetState() != PLAYER_STATE_NONE &&
			pPlayerPool->GetAt(playerId)->GetState() != PLAYER_STATE_WASTED)
		{
			m_byteSpectateType = SPECTATE_TYPE_PLAYER;
			m_SpectateID = playerId;
			m_bSpectateProcessed = false;
		}
	}
}

void CLocalPlayer::SpectateVehicle(VEHICLEID VehicleID)
{
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	if (pVehiclePool && pVehiclePool->GetSlotState(VehicleID)) 
	{
		m_byteSpectateType = SPECTATE_TYPE_VEHICLE;
		m_SpectateID = VehicleID;
		m_bSpectateProcessed = false;
	}
}

void CLocalPlayer::SendGiveDamageEvent(uint16_t wPlayerID, float damage_amount, uint32_t weapon_id, uint32_t bodypart)
{
	RakNet::BitStream bsSend;
	
	bsSend.Write0();
	bsSend.Write(wPlayerID);
	bsSend.Write(damage_amount);
	bsSend.Write(weapon_id);
	bsSend.Write(bodypart);
	
	pNetGame->GetRakClient()->RPC(&RPC_PlayerGiveTakeDamage, &bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}

void CLocalPlayer::SendTakeDamageEvent(uint16_t wPlayerID, float damage_amount, uint32_t weapon_id, uint32_t bodypart)
{
	RakNet::BitStream bsSend;
	
	bsSend.Write1();
	bsSend.Write(wPlayerID);
	bsSend.Write(damage_amount);
	bsSend.Write(weapon_id);
	bsSend.Write(bodypart);
	
	pNetGame->GetRakClient()->RPC(&RPC_PlayerGiveTakeDamage, &bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}

void CLocalPlayer::SendGiveDamageActorEvent(uint16_t wPlayerID, float damage_amount, uint32_t weapon_id, uint32_t bodypart)
{
	RakNet::BitStream bsSend;
	
	bsSend.Write0();
	bsSend.Write((uint16_t)wPlayerID);
	bsSend.Write((float)damage_amount);
	bsSend.Write((uint32_t)weapon_id);
	bsSend.Write((uint32_t)bodypart);
	
	pNetGame->GetRakClient()->RPC(&RPC_GiveDamageActor, &bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}

void CLocalPlayer::ProcessVehicleDamageUpdates(uint16_t CurrentVehicle)
{
	CVehicle *pVehicle = pNetGame->GetVehiclePool()->GetAt(CurrentVehicle);
	if(!pVehicle) 
		return;
	
	// If this isn't the vehicle we were last monitoring for damage changes
	// update our stored data and return.
	if(CurrentVehicle != m_DamageVehicleUpdating) 
	{
        m_dwLastPanelDamageStatus = pVehicle->GetPanelDamageStatus();
		m_dwLastDoorDamageStatus = pVehicle->GetDoorDamageStatus();
		m_byteLastLightsDamageStatus = pVehicle->GetLightDamageStatus();
		m_byteLastTireDamageStatus = pVehicle->GetWheelPoppedStatus();
		m_DamageVehicleUpdating = CurrentVehicle;
		return;
	}

	if(m_dwLastPanelDamageStatus != pVehicle->GetPanelDamageStatus() ||
		m_dwLastDoorDamageStatus != pVehicle->GetDoorDamageStatus() ||
		m_byteLastLightsDamageStatus != pVehicle->GetLightDamageStatus() |
		m_byteLastTireDamageStatus != pVehicle->GetWheelPoppedStatus()) 
	{			
			m_dwLastPanelDamageStatus = pVehicle->GetPanelDamageStatus();
			m_dwLastDoorDamageStatus = pVehicle->GetDoorDamageStatus();
			m_byteLastLightsDamageStatus = pVehicle->GetLightDamageStatus();
			m_byteLastTireDamageStatus = pVehicle->GetWheelPoppedStatus();

		// We need to update the server that the vehicle we're driving
		// has had its damage model modified.
		RakNet::BitStream bsData;
		bsData.Write(m_DamageVehicleUpdating);
		bsData.Write(m_dwLastPanelDamageStatus);
		bsData.Write(m_dwLastDoorDamageStatus);
        bsData.Write(m_byteLastLightsDamageStatus);
        bsData.Write(m_byteLastTireDamageStatus);
		pNetGame->GetRakClient()->RPC(&RPC_DamageVehicle, &bsData, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, NULL);
	}  
}

void CLocalPlayer::SendStatsUpdate() 
{
	RakNet::BitStream bsStats;
	int32_t iMoney = pGame->GetLocalMoney();
	int32_t iDrunkLevel = 0 /* where can i find it? */;

	bsStats.Write((uint8_t)ID_STATS_UPDATE);
	bsStats.Write(iMoney);
	bsStats.Write(iDrunkLevel);
	pNetGame->GetRakClient()->Send(&bsStats, HIGH_PRIORITY, UNRELIABLE, 0);
}

void CLocalPlayer::CheckWeapons() 
{
	if(m_pPlayerPed->IsInVehicle())
		return;
	
	bool bSend = false;
	for (uint8_t i = 0; i <= 12; i++) 
	{
		if(m_byteLastWeapon[i] != m_pPlayerPed->m_pPed->WeaponSlots[i].dwType) 
		{
			m_byteLastWeapon[i] = m_pPlayerPed->m_pPed->WeaponSlots[i].dwType;
			bSend = true;
		}

		if(m_dwLastAmmo[i] != m_pPlayerPed->m_pPed->WeaponSlots[i].dwAmmo) 
		{
			m_dwLastAmmo[i] = m_pPlayerPed->m_pPed->WeaponSlots[i].dwAmmo;
			bSend = true;
		}
	}
	
	if(bSend) 
	{
		RakNet::BitStream bsWeapons;
		bsWeapons.Write((uint8_t)ID_WEAPONS_UPDATE);
		for(uint8_t i = 0; i <= 12; ++i) 
		{
			bsWeapons.Write((uint8_t)i);
			bsWeapons.Write((uint8_t)m_byteLastWeapon[i]);
			bsWeapons.Write((uint16_t)m_dwLastAmmo[i]);
		}
		
		pNetGame->GetRakClient()->Send(&bsWeapons, HIGH_PRIORITY, UNRELIABLE, 0);
	}
}

uint8_t CLocalPlayer::GetSpecialAction()
{
	if(m_pPlayerPed->IsCrouching())
		return SPECIAL_ACTION_DUCK;

	if(m_pPlayerPed->IsInJetpackMode())
		return SPECIAL_ACTION_USEJETPACK;
		
/*	if(m_pPlayerPed->ProcessParachuteSkydiving())
		return SPECIAL_ACTION_SMOKE_CIGGY;*/

	if(m_pPlayerPed->IsDancing()) 
	{
		switch(m_pPlayerPed->m_iDanceStyle) 
		{
			case 0:
				return SPECIAL_ACTION_DANCE1;
			case 1:
				return SPECIAL_ACTION_DANCE2;
			case 2:
				return SPECIAL_ACTION_DANCE3;
			case 3:
				return SPECIAL_ACTION_DANCE4;
		}
	}

	if(m_pPlayerPed->HasHandsUp())
		return SPECIAL_ACTION_HANDSUP;

	if(m_pPlayerPed->IsCellphoneEnabled())
		return SPECIAL_ACTION_USECELLPHONE;

	return SPECIAL_ACTION_NONE;
}
