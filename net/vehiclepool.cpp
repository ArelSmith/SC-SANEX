#include "../main.h"
#include "../game/game.h"
#include "netgame.h"
#include "../chatwindow.h"

extern CGame *pGame;
extern CNetGame *pNetGame;
extern CChatWindow *pChatWindow;

void ProcessIncomingEvent(uint16_t bytePlayerID, int iEventType, uint32_t dwParam1, uint32_t dwParam2, uint32_t dwParam3);

CVehiclePool::CVehiclePool()
{
	for(VEHICLEID VehicleID = 0; VehicleID < MAX_VEHICLES; VehicleID++)
	{
		m_bIsVehicleRespawn[VehicleID] = false;
		m_bVehicleSlotState[VehicleID] = false;
		m_pVehicles[VehicleID] = nullptr;
		m_pGTAVehicles[VehicleID] = nullptr;
	}
}

CVehiclePool::~CVehiclePool()
{
	for(VEHICLEID VehicleID = 0; VehicleID < MAX_VEHICLES; VehicleID++)
		Delete(VehicleID);
}

void CVehiclePool::Process()
{
	CVehicle *pVehicle;
	for(VEHICLEID x = 0; x < MAX_VEHICLES; x++)
	{
		if(GetSlotState(x))
		{
			pVehicle = m_pVehicles[x];
			if(m_bIsActive[x])
			{
				// android object slow render than pc
				// possible make the car teleport to nearest road
				if(pVehicle->GetDistanceFromLocalPlayerPed() < 150.0f)
				{
					if(!m_bIsVehicleRespawn[x])
					{
						m_bIsVehicleRespawn[x] = true;
						pVehicle->TeleportTo(m_vecSpawnPos[x].X, m_vecSpawnPos[x].Y, m_vecSpawnPos[x].Z + 0.1f);
						ScriptCommand(&set_car_z_angle, pVehicle->m_dwGTAId, m_fSpawnRotation[x]);
					}
				} 

				if(pVehicle->IsDriverLocalPlayer()) 
				{
					// check if the m_bIsEngineOn is off but car engine still on
					if(!pVehicle->m_bIsEngineOn && ScriptCommand(&is_car_engine_on, pVehicle->m_dwGTAId))
						pVehicle->SetEngineState(0);

					pVehicle->SetInvulnerable(false);
				}
				else pVehicle->SetInvulnerable(true);

				if(pVehicle->GetHealth() <= 0.0f && pVehicle->GetVehicleSubtype() != VEHICLE_SUBTYPE_BOAT && pVehicle->GetDistanceFromLocalPlayerPed() < 200.0f && pVehicle->HasSunk())
				{
					NotifyVehicleDeath(x);
					continue;
				}

				if(pVehicle->m_pVehicle != m_pGTAVehicles[x])
					m_pGTAVehicles[x] = pVehicle->m_pVehicle;

				pVehicle->ProcessMarkers();
			}
		}
	}
}

bool CVehiclePool::New(NEW_VEHICLE *pNewVehicle)
{
	if(pNewVehicle->VehicleID == INVALID_VEHICLE_ID) return false;
	
	if(m_pVehicles[pNewVehicle->VehicleID] != nullptr)
	{
		pChatWindow->AddDebugMessage("Warning: vehicle %u was not deleted", pNewVehicle->VehicleID);
		Delete(pNewVehicle->VehicleID);
	}

	m_pVehicles[pNewVehicle->VehicleID] = pGame->NewVehicle(pNewVehicle->iVehicleType, pNewVehicle->vehPosX, pNewVehicle->vehPosY, pNewVehicle->vehPosZ, pNewVehicle->fRotation, pNewVehicle->byteAddSiren);

	if(m_pVehicles[pNewVehicle->VehicleID])
	{
		// colors
		if(pNewVehicle->aColor1 != -1 || pNewVehicle->aColor2 != -1)
		{
			m_pVehicles[pNewVehicle->VehicleID]->SetColor(
				pNewVehicle->aColor1, pNewVehicle->aColor2);
		}

		// health
		m_pVehicles[pNewVehicle->VehicleID]->SetHealth(pNewVehicle->fHealth);

		// gta handle
		m_pGTAVehicles[pNewVehicle->VehicleID] = m_pVehicles[pNewVehicle->VehicleID]->m_pVehicle;
		m_bVehicleSlotState[pNewVehicle->VehicleID] = true;

		// interior
		if(pNewVehicle->byteInterior > 0)
			LinkToInterior(pNewVehicle->VehicleID, pNewVehicle->byteInterior);

		// damage status
		if(pNewVehicle->dwPanelDamageStatus || pNewVehicle->dwDoorDamageStatus || pNewVehicle->byteLightDamageStatus || pNewVehicle->byteTireDamageStatus)
			m_pVehicles[pNewVehicle->VehicleID]->UpdateDamageStatus(pNewVehicle->dwPanelDamageStatus, pNewVehicle->dwDoorDamageStatus, pNewVehicle->byteLightDamageStatus, pNewVehicle->byteTireDamageStatus);

		// manual engine and Light
		if(pNetGame->m_bManualVehicleEngineAndLight)
		{
			m_pVehicles[pNewVehicle->VehicleID]->SetEngineState(0);
			m_pVehicles[pNewVehicle->VehicleID]->SetLightsState(0);
		}
		/*
		// tune
		if(pNewVehicle->bytePaintjob > 0)
			ProcessIncomingEvent(-1, EVENT_TYPE_PAINTJOB, pNewVehicle->VehicleID, pNewVehicle->bytePaintjob, 0);
		
		for(int i = 0; i < 14; i++)
			ProcessIncomingEvent(-1, EVENT_TYPE_CARCOMPONENT, pNewVehicle->VehicleID, pNewVehicle->byteModSlots[i], 0);
		*/
		// 
		m_vecSpawnPos[pNewVehicle->VehicleID].X = pNewVehicle->vehPosX;
		m_vecSpawnPos[pNewVehicle->VehicleID].Y = pNewVehicle->vehPosY;
		m_vecSpawnPos[pNewVehicle->VehicleID].Z = pNewVehicle->vehPosZ;
		m_fSpawnRotation[pNewVehicle->VehicleID] = pNewVehicle->fRotation;

		// 
		m_bIsActive[pNewVehicle->VehicleID] = true;
		m_bIsWasted[pNewVehicle->VehicleID] = false;
		m_bIsVehicleRespawn[pNewVehicle->VehicleID] = false;

		return true;
	}

	return false;
}

bool CVehiclePool::Delete(VEHICLEID VehicleID)
{
	if(!GetSlotState(VehicleID) || !m_pVehicles[VehicleID])
		return false;

	m_bVehicleSlotState[VehicleID] = false;
	m_bIsActive[VehicleID] = false;
	delete m_pVehicles[VehicleID];
	m_pVehicles[VehicleID] = nullptr;
	m_pGTAVehicles[VehicleID] = nullptr;

	return true;
}

VEHICLEID CVehiclePool::FindIDFromGtaPtr(VEHICLE_TYPE *pGtaVehicle)
{
	int x=1;

	while(x != MAX_VEHICLES) 
	{
		if(pGtaVehicle == m_pGTAVehicles[x]) return x;
		x++;
	}

	return INVALID_VEHICLE_ID;
}

int CVehiclePool::FindGtaIDFromID(int iID)
{
	if(m_pGTAVehicles[iID])
		return GamePool_Vehicle_GetIndex(m_pGTAVehicles[iID]);
	else return INVALID_VEHICLE_ID;
}

int CVehiclePool::FindNearestToLocalPlayerPed()
{
	float fLeastDistance = 10000.0f;
	float fThisDistance = 0.0f;
	VEHICLEID ClosetSoFar = INVALID_VEHICLE_ID;

	VEHICLEID x = 0;
	while(x < MAX_VEHICLES)
	{
		if(GetSlotState(x) && m_bIsActive[x])
		{
			fThisDistance = m_pVehicles[x]->GetDistanceFromLocalPlayerPed();
			if(fThisDistance < fLeastDistance)
			{
				fLeastDistance = fThisDistance;
				ClosetSoFar = x;
			}
		}

		x++;
	}

	return ClosetSoFar;
}

void CVehiclePool::LinkToInterior(VEHICLEID VehicleID, int iInterior)
{
	if(m_bVehicleSlotState[VehicleID])
		m_pVehicles[VehicleID]->LinkToInterior(iInterior);
}

void CVehiclePool::NotifyVehicleDeath(VEHICLEID VehicleID)
{
	if(pNetGame->GetPlayerPool()->GetLocalPlayer()->m_LastVehicle != VehicleID) return;
	
	RakNet::BitStream bsDeath;
	bsDeath.Write(VehicleID);
	pNetGame->GetRakClient()->RPC(&RPC_VehicleDestroyed, &bsDeath, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, false, UNASSIGNED_NETWORK_ID, NULL);
	pNetGame->GetPlayerPool()->GetLocalPlayer()->m_LastVehicle = INVALID_VEHICLE_ID;
}

void CVehiclePool::AssignSpecialParamsToVehicle(VEHICLEID VehicleID, uint8_t byteObjective, uint8_t byteDoorsLocked)
{
	if(!GetSlotState(VehicleID)) return;
	CVehicle *pVehicle = m_pVehicles[VehicleID];

	if(pVehicle && m_bIsActive[VehicleID])
	{
		pVehicle->SetObjState(byteObjective);
		pVehicle->SetDoorState(byteDoorsLocked);
	}
}

void CVehiclePool::AttachObjectToVehicle(uint16_t objectId, VEHICLEID vehId, VECTOR offset, VECTOR rotation, bool syncRot)
{
	if(objectId < 0 || objectId > MAX_OBJECTS) return;
	
	CObjectPool* pObjectPool = pNetGame->GetObjectPool();
	if(pObjectPool) 
	{
		CObject* pObject = pObjectPool->GetAt(objectId);
		if(pObject)
		{
			CVehicle *pVehicle = GetAt(vehId);
			if(pVehicle)
				ScriptCommand(&attach_object_to_car, pObject->m_dwGTAId, pVehicle->m_dwGTAId, offset.X, offset.Y, offset.Z, rotation.X, rotation.Y, rotation.Z);
		}
	}
}