#include "../main.h"
#include "game.h"
#include "../net/netgame.h"
#include "../util/armhook.h"
#include "../chatwindow.h"

extern CGame *pGame;
extern CNetGame *pNetGame;
extern CChatWindow *pChatWindow;

VEHICLE_TYPE *pLastVehicle;

CVehicle::CVehicle(int iType, float fPosX, float fPosY, float fPosZ, float fRotation, bool bSiren)
{
	MATRIX4X4 mat;
	uint32_t dwRetID = 0;

	m_pVehicle = nullptr;
	m_dwGTAId = 0;
	m_pTrailer = nullptr;

	switch(iType)
	{
		case TRAIN_PASSENGER_LOCO:
		case TRAIN_FREIGHT_LOCO:
		case TRAIN_TRAM:
		{
			if(iType == TRAIN_PASSENGER_LOCO) iType = 5;
			else if(iType == TRAIN_FREIGHT_LOCO) iType = 3;
			else if(iType == TRAIN_TRAM) iType = 9;

			int dwDirection = 0;
			if(fRotation > 180.0f) dwDirection = 1;

			pGame->RequestModel(TRAIN_PASSENGER_LOCO);
			pGame->RequestModel(TRAIN_PASSENGER);
			pGame->RequestModel(TRAIN_FREIGHT_LOCO);
			pGame->RequestModel(TRAIN_FREIGHT);
			pGame->RequestModel(TRAIN_TRAM);
			pGame->LoadRequestedModels(false);
			while(!pGame->IsModelLoaded(TRAIN_PASSENGER_LOCO)) usleep(500);
			while(!pGame->IsModelLoaded(TRAIN_PASSENGER)) usleep(500);
			while(!pGame->IsModelLoaded(TRAIN_FREIGHT_LOCO)) usleep(500);
			while(!pGame->IsModelLoaded(TRAIN_FREIGHT)) usleep(500);
			while(!pGame->IsModelLoaded(TRAIN_TRAM)) usleep(500);

			pGame->m_bIsVehiclePreloaded[iType] = true; // train always preloaded
		
			ScriptCommand(&create_train, iType, fPosX, fPosY, fPosZ, dwDirection, &dwRetID);

			m_pVehicle = (VEHICLE_TYPE*)GamePool_Vehicle_GetAt(dwRetID);
			m_pEntity = (ENTITY_TYPE*)m_pVehicle;
			m_dwGTAId = dwRetID;
			pLastVehicle = m_pVehicle;
			break;
		}

		case TRAIN_PASSENGER:
		case TRAIN_FREIGHT:
		{
			if(!pLastVehicle) {
				m_pEntity = 0;
				m_pVehicle = 0;
				pLastVehicle = 0;
				return;
			}

			//m_pVehicle = (VEHICLE_TYPE *)pLastVehicle->VehicleAttachedBottom;

			if(!m_pVehicle) 
			{
				pChatWindow->AddDebugMessage("Warning: bad train carriages");
				m_pEntity = 0;
				m_pVehicle = 0;
				pLastVehicle = 0;
				return;
			}

			dwRetID = GamePool_Vehicle_GetIndex(m_pVehicle);
			m_pEntity = (ENTITY_TYPE *)m_pVehicle;
			m_dwGTAId = dwRetID;
			pLastVehicle = m_pVehicle;
			break;
		}

		default:
		{
		// normal vehicle
		if(!pGame->IsModelLoaded(iType))
		{
			pGame->RequestModel(iType);
			pGame->LoadRequestedModels(false);
			while(!pGame->IsModelLoaded(iType)) usleep(10);
		}

			ScriptCommand(&create_car, iType, fPosX, fPosY, fPosZ + 0.1f, &dwRetID);
			ScriptCommand(&set_car_z_angle, dwRetID, fRotation);
			ScriptCommand(&car_gas_tank_explosion, dwRetID, 0);
			ScriptCommand(&set_car_hydraulics, dwRetID, 0);
			ScriptCommand(&toggle_car_tires_vulnerable, dwRetID, 0);

			m_pVehicle = (VEHICLE_TYPE*)GamePool_Vehicle_GetAt(dwRetID);
			m_pEntity = (ENTITY_TYPE*)m_pVehicle;
			m_dwGTAId = dwRetID;

			if(m_pVehicle)
			{
				m_pVehicle->dwDoorsLocked = 0;
				m_bIsLocked = false;

				m_pVehicle->nFlags.bEngineOn = 0;
				m_bIsEngineOn = false;

				ScriptCommand(&force_car_lights, m_dwGTAId, 1);
				m_bIsLightsOn = false;
				m_pVehicle->nFlags.bSirenOrAlarm = 0;
				//m_pVehicle->byteSirenOn = false;

				GetMatrix(&mat);
				mat.pos.X = fPosX;
				mat.pos.Y = fPosY;
				mat.pos.Z = fPosZ;

				switch(GetVehicleSubtype())
				{
					case VEHICLE_SUBTYPE_BIKE:
					case VEHICLE_SUBTYPE_PUSHBIKE:
					mat.pos.Z += 0.25f;
					break;
				}

				SetMatrix(mat);

				if(bSiren)
					SetSirenState(1);
				else SetSirenState(0);
			}
		}
	}

	m_byteObjectiveVehicle = 0;
	m_bSpecialMarkerEnabled = false;
	m_dwMarkerID = 0;
	m_bIsInvulnerable = false;
}

CVehicle::~CVehicle()
{
	m_pVehicle = GamePool_Vehicle_GetAt(m_dwGTAId);

	if(m_pVehicle)
	{
		if(m_dwMarkerID)
		{
			ScriptCommand(&disable_marker, m_dwMarkerID);
			m_dwMarkerID = 0;
		}

		RemoveEveryoneFromVehicle();

		if(ScriptCommand(&is_car_siren_on, m_dwGTAId)) 
			ScriptCommand(&switch_car_siren, m_dwGTAId, 0);

		if(m_pTrailer)
		{
			// допилить
		}

		if(m_pVehicle->entity.nModelIndex == TRAIN_PASSENGER_LOCO || m_pVehicle->entity.nModelIndex == TRAIN_FREIGHT_LOCO)
		{
			ScriptCommand(&destroy_train, m_dwGTAId);
		}
		else
		{
			int nModelIndex = m_pVehicle->entity.nModelIndex;
			ScriptCommand(&destroy_car, m_dwGTAId);

			if(!GetModelReferenceCount(nModelIndex) && pGame->IsModelLoaded(nModelIndex))
			{
				// CStreaming::RemoveModel
				(( void (*)(int))(g_libGTASA+0x290C4C+1))(nModelIndex);
			}
		}
	}
}

void CVehicle::LinkToInterior(int iInterior)
{
	if(GamePool_Vehicle_GetAt(m_dwGTAId)) 
		ScriptCommand(&link_vehicle_to_interior, m_dwGTAId, iInterior);
}

void CVehicle::SetColor(int iColor1, int iColor2)
{
	if(m_pVehicle)
	{
		if(GamePool_Vehicle_GetAt(m_dwGTAId))
		{
			m_pVehicle->byteColor1 = (uint8_t)iColor1;
			m_pVehicle->byteColor2 = (uint8_t)iColor2;
		}
	}

	m_byteColor1 = (uint8_t)iColor1;
	m_byteColor2 = (uint8_t)iColor2;
	m_bColorChanged = true;
}

void CVehicle::SetHealth(float fHealth)
{
	if(m_pVehicle)
		m_pVehicle->fHealth = fHealth;
	
	return;
}

float CVehicle::GetHealth()
{
	if(m_pVehicle)
		return m_pVehicle->fHealth; 

	return 0.0f;
}

// 0.3.7
void CVehicle::SetInvulnerable(bool bInv)
{
	if(!m_pVehicle) return;
	if(!GamePool_Vehicle_GetAt(m_dwGTAId)) return;
	if(m_pVehicle->entity.vtable == g_libGTASA+0x5C7358) return;

	if(bInv) 
	{
		ScriptCommand(&set_car_immunities, m_dwGTAId, 1,1,1,1,1);
		ScriptCommand(&toggle_car_tires_vulnerable, m_dwGTAId, 0);
		m_bIsInvulnerable = true;
	} 
	else 
	{ 
		ScriptCommand(&set_car_immunities, m_dwGTAId, 0,0,0,0,0);
		ScriptCommand(&toggle_car_tires_vulnerable, m_dwGTAId, 1);
		m_bIsInvulnerable = false;
	}
}

// 0.3.7
bool CVehicle::IsDriverLocalPlayer()
{
	if(m_pVehicle)
	{
		if((PED_TYPE*)m_pVehicle->pDriver == GamePool_FindPlayerPed())
			return true;
	}

	return false;
}

// 0.3.7
bool CVehicle::HasSunk()
{
	if(!m_pVehicle) return false;
	return ScriptCommand(&has_car_sunk, m_dwGTAId);
}

void CVehicle::RemoveEveryoneFromVehicle()
{
	if(!m_pVehicle) return;
	if(!GamePool_Vehicle_GetAt(m_dwGTAId)) return;

	float fPosX = m_pVehicle->entity.mat->pos.X;
	float fPosY = m_pVehicle->entity.mat->pos.Y;
	float fPosZ = m_pVehicle->entity.mat->pos.Z;

	int iPlayerID = 0;
	if(m_pVehicle->pDriver)
	{
		iPlayerID = GamePool_Ped_GetIndex( m_pVehicle->pDriver );
		ScriptCommand(&remove_actor_from_car_and_put_at, iPlayerID, fPosX, fPosY, fPosZ + 2.0f);
	}

	for(int i = 0; i < 7; i++)
	{
		if(m_pVehicle->pPassengers[i] != nullptr)
		{
			iPlayerID = GamePool_Ped_GetIndex( m_pVehicle->pPassengers[i] );
			ScriptCommand(&remove_actor_from_car_and_put_at, iPlayerID, fPosX, fPosY, fPosZ + 2.0f);
		}
	}
}

// 0.3.7
bool CVehicle::IsOccupied()
{
	if(m_pVehicle)
	{
		if(m_pVehicle->pDriver) return true;

		for(int i = 0; i < 7; i++)
			if(m_pVehicle->pPassengers[i]) return true;
	}

	return false;
}

void CVehicle::ProcessMarkers()
{
	if(!m_pVehicle) return;

	if(m_byteObjectiveVehicle)
	{
		if(!m_bSpecialMarkerEnabled)
		{
			if(m_dwMarkerID)
			{
				ScriptCommand(&disable_marker, m_dwMarkerID);
				m_dwMarkerID = 0;
			}

			ScriptCommand(&tie_marker_to_car, m_dwGTAId, 1, 3, &m_dwMarkerID);
			ScriptCommand(&set_marker_color, m_dwMarkerID, 1006);
			ScriptCommand(&show_on_radar, m_dwMarkerID, 3);
			m_bSpecialMarkerEnabled = true;
		}

		return;
	}

	if(m_byteObjectiveVehicle && m_bSpecialMarkerEnabled)
	{
		if(m_dwMarkerID)
		{
			ScriptCommand(&disable_marker, m_dwMarkerID);
			m_bSpecialMarkerEnabled = false;
			m_dwMarkerID = 0;
		}
	}

	if(GetDistanceFromLocalPlayerPed() < 150.0f && !IsOccupied())
	{
		if(!m_dwMarkerID)
		{
			ScriptCommand(&tie_marker_to_car, m_dwGTAId, 1, 2, &m_dwMarkerID);
			ScriptCommand(&set_marker_color, m_dwMarkerID, 1004);
		}
	}

	else if(IsOccupied() || GetDistanceFromLocalPlayerPed() >= 150.0f)
	{
		if(m_dwMarkerID)
		{
			ScriptCommand(&disable_marker, m_dwMarkerID);
			m_dwMarkerID = 0;
		}
	}
}

void CVehicle::SetObjState(uint8_t byteState)
{
	m_byteObjectiveVehicle = byteState == 1 ? 1 : 0;
	m_bSpecialMarkerEnabled = byteState == 1 ? false : true;
}

void CVehicle::SetDoorState(int iState)
{
	m_pVehicle->dwDoorsLocked = iState == 1 ? 2 : 0;
	m_bDoorsLocked = iState == 1 ? true : false;
}

int CVehicle::GetDoorState()
{
	return m_bDoorsLocked;
}

void CVehicle::SetDoorOpenFlag(uint8_t byteState, int iCurrentDoorId)
{
	if(byteState == 1)
		ScriptCommand(&open_car_component, m_dwGTAId, (int)iCurrentDoorId);
	else ScriptCommand(&set_car_component_rotate, m_dwGTAId, (int)iCurrentDoorId, 0.0f);
}

void CVehicle::SetEngineState(int iState)
{
	ScriptCommand(&set_car_engine_on, m_dwGTAId, iState == 1 ? 1 : 0);
	m_pVehicle->nFlags.bEngineOn = iState == 1 ? true : false;
	m_bIsEngineOn = iState == 1 ? true : false;
}

void CVehicle::SetLightsState(int iState)
{
	ScriptCommand(&force_car_lights, m_dwGTAId, iState == 1 ? 2 : 1);
	m_bIsLightsOn = iState == 1 ? true : false;
}

void CVehicle::SetAlarmState(int iState)
{
	m_pVehicle->nFlags.bSirenOrAlarm = iState == 1 ? 1 : 0;
}

void CVehicle::SetSirenState(bool iState)
{
	ScriptCommand(&switch_car_siren, m_dwGTAId, iState == 1 ? 1 : 0);
	m_pVehicle->nFlags.bSirenOrAlarm = iState == 1 ? 1 : 0;
}

bool CVehicle::IsSirenOn()
{
	return (m_pVehicle->nFlags.bSirenOrAlarm == 1);
}

void CVehicle::UpdateDamageStatus(uint32_t dwPanelDamage, uint32_t dwDoorDamage, uint8_t byteLightDamage, uint8_t byteTire)
{
	if(m_pVehicle && GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR) 
	{
		if(!dwPanelDamage && !dwDoorDamage && !byteLightDamage) 
		{
			//if(m_pVehicle->dwPanelStatus || m_pVehicle->dwDoorStatus1 || m_pVehicle->dwLightStatus) 
			//{
				// The ingame car is damaged in some way although the update
				// says the car should be repaired. So repair it and exit.
				// CAutoMobile::Fix
				uint32_t dwVehiclePtr = (uint32_t)m_pVehicle;
				((void(*) (uintptr_t))(g_libGTASA+0x4D5CA4+1))(dwVehiclePtr);
				return;
			//}
		}

		m_pVehicle->nFlags.bIsDamaged = 1;

		/*m_pVehicle->dwPanelStatus = dwPanelDamage;
		m_pVehicle->dwDoorStatus1 = dwDoorDamage;
		m_pVehicle->dwLightStatus = (uint32_t)byteLightDamage;*/

		// CAutoMobile::Render
		//uint32_t dwVehiclePtr = (uint32_t)m_pVehicle;
		//((void(*) (uintptr_t))(g_libGTASA+0x4E671C+1))(dwVehiclePtr);
	}
}
// CAutoMobile::Render = 0x4E671C
// CAutoMobile::PreRender = 0x4E87B0

uint32_t CVehicle::GetPanelDamageStatus()
{
	if(m_pVehicle && GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR) {
		//return m_pVehicle->dwPanelStatus;
	}
	return 0;
}

uint32_t CVehicle::GetDoorDamageStatus()
{
	if(m_pVehicle && GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR) {
		//return m_pVehicle->dwDoorStatus1;
	}
	return 0;
}

uint16_t CVehicle::GetLightDamageStatus()
{
	if(m_pVehicle && GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR) {
		//return (uint16_t)m_pVehicle->dwLightStatus;
	}
	return 0;
}

uint16_t CVehicle::GetWheelPoppedStatus()
{
	if(m_pVehicle && GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR) {
		//return (uint16_t)m_pVehicle->dwWheelPopped;
	}
	return 0;
}

unsigned int CVehicle::GetVehicleSubtype()
{
	if(m_pVehicle)
	{
		if(m_pVehicle->entity.vtable == g_libGTASA+0x5CC9F0) // 0x871120
		{
			return VEHICLE_SUBTYPE_CAR;
		}
		else if(m_pVehicle->entity.vtable == g_libGTASA+0x5CCD48) // 0x8721A0
		{
			return VEHICLE_SUBTYPE_BOAT;
		}
		else if(m_pVehicle->entity.vtable == g_libGTASA+0x5CCB18) // 0x871360
		{
			return VEHICLE_SUBTYPE_BIKE;
		}
		else if(m_pVehicle->entity.vtable == g_libGTASA+0x5CD0B0) // 0x871948
		{
			return VEHICLE_SUBTYPE_PLANE;
		}
		else if(m_pVehicle->entity.vtable == g_libGTASA+0x5CCE60) // 0x871680
		{
			return VEHICLE_SUBTYPE_HELI;
		}
		else if(m_pVehicle->entity.vtable == g_libGTASA+0x5CCC30) // 0x871528
		{
			return VEHICLE_SUBTYPE_PUSHBIKE;
		}
		else if(m_pVehicle->entity.vtable == g_libGTASA+0x5CD428) // 0x872370
		{
			return VEHICLE_SUBTYPE_TRAIN;
		}
	}

	return 0;
}

void CVehicle::AttachTrailer()
{
	if(m_pTrailer)
		ScriptCommand(&put_trailer_on_cab, m_pTrailer->m_dwGTAId, m_dwGTAId);
}

void CVehicle::DetachTrailer()
{
	if(m_pTrailer)
		ScriptCommand(&detach_trailer_from_cab, m_pTrailer->m_dwGTAId, m_dwGTAId);
}

void CVehicle::SetTrailer(CVehicle *pTrailer)
{
	m_pTrailer = pTrailer;
}

CVehicle* CVehicle::GetTrailer()
{
	if (!m_pVehicle) return NULL;

	// Try to find associated trailer
	uint32_t dwTrailerGTAPtr = m_pVehicle->dwTrailer;
	if(pNetGame && dwTrailerGTAPtr) 
	{
		CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
		if(pVehiclePool)
		{
			VEHICLEID TrailerID = (VEHICLEID)pVehiclePool->FindIDFromGtaPtr((VEHICLE_TYPE*)dwTrailerGTAPtr);
			if(TrailerID < MAX_VEHICLES && pVehiclePool->GetSlotState(TrailerID))
				return pVehiclePool->GetAt(TrailerID);
		}
	}

	return NULL;
}

bool CVehicle::IsATrainPart()
{
	int nModel;
	if(m_pVehicle) {
		nModel = m_pVehicle->entity.nModelIndex;
		if(nModel == TRAIN_PASSENGER_LOCO) return true;
		if(nModel == TRAIN_PASSENGER) return true;
		if(nModel == TRAIN_FREIGHT_LOCO) return true;
		if(nModel == TRAIN_FREIGHT) return true;
		if(nModel == TRAIN_TRAM) return true;
	}
	return false;
}

float CVehicle::GetTrainSpeed()
{
	return 0;//m_pVehicle->fTrainSpeed;
}

void CVehicle::SetTrainSpeed(float fSpeed)
{
	//m_pVehicle->fTrainSpeed = fSpeed;
}