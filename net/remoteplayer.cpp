#include "../main.h"
#include "../game/game.h"
#include "netgame.h"
#include "../chatwindow.h"
#include "util/armhook.h"

extern CGame *pGame;
extern CNetGame *pNetGame;
extern CChatWindow *pChatWindow;

extern bool bUsedPlayerSlots[];

CRemotePlayer::CRemotePlayer()
{
	m_PlayerID = INVALID_PLAYER_ID;
	m_VehicleID = INVALID_VEHICLE_ID;
	m_pPlayerPed = nullptr;
	m_pCurrentVehicle = nullptr;
	m_bIsNPC = false;
	m_bIsAFK = true;
	m_dwMarkerID = 0;
	m_dwGlobalMarkerID = 0;
	m_byteState = PLAYER_STATE_NONE;
	m_byteUpdateFromNetwork = UPDATE_TYPE_NONE;
	m_bShowNameTag = true;

	m_dwLastRecvTick = 0;
	m_dwUnkTime = 0;

	m_byteCurrentWeapon = 0;
	m_byteSpecialAction = 0;
	m_byteSeatID = 0;

	m_fReportedHealth = 0.0f;
	m_fReportedArmour = 0.0f;

	m_bJetpack = false;
}

CRemotePlayer::~CRemotePlayer()
{
	Remove();
}

void CRemotePlayer::Process()
{
	MATRIX4X4 matPlayer, matVehicle;
	VECTOR vecMoveSpeed;

	if(IsActive())
	{
		// ---- ONFOOT NETWORK PROCESSING ----
		if(GetState() == PLAYER_STATE_ONFOOT && 
			m_byteUpdateFromNetwork == UPDATE_TYPE_ONFOOT && !m_pPlayerPed->IsInVehicle())
		{
			// If the user has sent X, Y or Z offsets we need to 'attach' him to the car using these offsets
			if ( !IsSurfingOrTurretMode() )
			{
				// If the user hasn't sent these, they're obviously not on the car, so we just sync
				// their normal movement/speed/etc.
				UpdateOnFootPositionAndSpeed(&m_ofSync.vecPos,&m_ofSync.vecMoveSpeed);
				UpdateOnFootTargetPosition();
			}


			// UPDATE CURRENT WEAPON
			uint8_t byteCurrentWeapon = m_ofSync.byteCurrentWeapon & 0x3F;
			if(m_pPlayerPed->IsAdded() && m_pPlayerPed->GetCurrentWeapon() != byteCurrentWeapon) 
			{
				m_pPlayerPed->GiveWeapon(byteCurrentWeapon, 9999);
				m_pPlayerPed->SetArmedWeapon(byteCurrentWeapon);

				// double check
				if(m_pPlayerPed->GetCurrentWeapon() != byteCurrentWeapon) 
				{
					m_pPlayerPed->GiveWeapon(byteCurrentWeapon, 9999);
					m_pPlayerPed->SetArmedWeapon(byteCurrentWeapon);
				}
			}
		}
		else if(GetState() == PLAYER_STATE_DRIVER && 
			m_byteUpdateFromNetwork == UPDATE_TYPE_INCAR && m_pPlayerPed->IsInVehicle())
		{
			if(!m_pCurrentVehicle || !GamePool_Vehicle_GetAt(m_pCurrentVehicle->m_dwGTAId))
				return;

			m_icSync.quat.Normalize();
			m_icSync.quat.GetMatrix(&matVehicle);
			matVehicle.pos.X = m_icSync.vecPos.X;
			matVehicle.pos.Y = m_icSync.vecPos.Y;
			matVehicle.pos.Z = m_icSync.vecPos.Z;

			if(m_pCurrentVehicle->GetModelIndex() == TRAIN_PASSENGER_LOCO ||
				m_pCurrentVehicle->GetModelIndex() == TRAIN_FREIGHT_LOCO ||
				m_pCurrentVehicle->GetModelIndex() == TRAIN_TRAM)
			{
				UpdateTrainDriverMatrixAndSpeed(&matVehicle, &vecMoveSpeed, m_icSync.fTrainSpeed);
			}
			else
			{
				UpdateInCarMatrixAndSpeed(&matVehicle, &m_icSync.vecPos, &m_icSync.vecMoveSpeed);
				UpdateInCarTargetPosition();
			}

			m_pCurrentVehicle->SetSirenState(m_icSync.byteSirenOn);

			m_pCurrentVehicle->SetHealth(m_icSync.fCarHealth);

			// TRAILER AUTOMATIC ATTACHMENT AS THEY MOVE INTO IT
			if(m_pCurrentVehicle->GetDistanceFromLocalPlayerPed() < 200.0f)
			{
				if(m_icSync.TrailerID && m_icSync.TrailerID < MAX_VEHICLES)
				{
					CVehicle* pRealTrailer = m_pCurrentVehicle->GetTrailer();
					if(!pRealTrailer) 
					{
						CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
						if(pVehiclePool)
						{
							CVehicle *pTrailerVehicle = pVehiclePool->GetAt(m_icSync.TrailerID);
							if(pTrailerVehicle) 
							{
								m_pCurrentVehicle->SetTrailer(pTrailerVehicle);
								m_pCurrentVehicle->AttachTrailer();
							}
						}
					}
				} 
				else
				{
					if(m_pCurrentVehicle->GetTrailer()) 
					{
						m_pCurrentVehicle->DetachTrailer();
						m_pCurrentVehicle->SetTrailer(NULL);
					}
				}
			}
			else m_pCurrentVehicle->SetTrailer(NULL);
		}
		else if(GetState() == PLAYER_STATE_PASSENGER && 
			m_byteUpdateFromNetwork == UPDATE_TYPE_PASSENGER)
		{
			if(!m_pCurrentVehicle) return;

			// UPDATE CURRENT WEAPON
			uint8_t byteCurrentWeapon = m_ofSync.byteCurrentWeapon & 0x3F;
			if(m_pPlayerPed->IsAdded() && m_pPlayerPed->GetCurrentWeapon() != byteCurrentWeapon) 
			{
				m_pPlayerPed->GiveWeapon(byteCurrentWeapon, 9999);
				m_pPlayerPed->SetArmedWeapon(byteCurrentWeapon);

				// double check
				if(m_pPlayerPed->GetCurrentWeapon() != byteCurrentWeapon) 
				{
					m_pPlayerPed->GiveWeapon(byteCurrentWeapon, 9999);
					m_pPlayerPed->SetArmedWeapon(byteCurrentWeapon);
				}
			}

			// FOR INITIALISING PASSENGER DRIVEBY
			if(!m_bPassengerDriveByMode && m_pPlayerPed->GetActionTrigger() == ACTION_INCAR) 
			{
				if(m_psSync.byteDriveBy) 
				{
					if(m_pPlayerPed->StartPassengerDriveByMode(true))
						m_bPassengerDriveByMode = true;
				}
			}
		}

		m_byteUpdateFromNetwork = UPDATE_TYPE_NONE;

		// ------ PROCESSED FOR ALL FRAMES ----- 
		if(GetState() == PLAYER_STATE_ONFOOT && !m_pPlayerPed->IsInVehicle())
		{
			m_bPassengerDriveByMode = false;
			SlerpRotation();
			ProcessSpecialActions(m_ofSync.byteSpecialAction);
			
			m_pPlayerPed->SetKeys(m_ofSync.wKeys, m_ofSync.lrAnalog, m_ofSync.udAnalog);

			if(RemotePlayerKeys[GetPlayerPed()->m_bytePlayerNumber].bKeys[ePadKeys::KEY_HANDBRAKE])
				m_pPlayerPed->SetPlayerAimState();
			else m_pPlayerPed->ClearPlayerAimState();
			
			if(m_ofSync.vecMoveSpeed.X == 0.0f &&
				m_ofSync.vecMoveSpeed.Y == 0.0f &&
				m_ofSync.vecMoveSpeed.Z == 0.0f)
			{
				m_pPlayerPed->SetMoveSpeedVector(m_ofSync.vecMoveSpeed);
			}

			if(IsSurfingOrTurretMode()) {
				UpdateSurfing();
				m_pPlayerPed->SetGravityProcessing(0);
				m_pPlayerPed->SetCollisionChecking(0);
			} else {
				m_pPlayerPed->SetGravityProcessing(1);
				m_pPlayerPed->SetCollisionChecking(1);
			}

			if((GetTickCount() - m_dwLastRecvTick) > 1500)
				m_bIsAFK = true;

			if(m_bIsAFK && ((GetTickCount() - m_dwLastRecvTick) > 3000))
			{
				m_ofSync.lrAnalog = 0;
				m_ofSync.udAnalog = 0;
				
				vecMoveSpeed.X = 0.0f;
				vecMoveSpeed.Y = 0.0f;
				vecMoveSpeed.Z = 0.0f;
				m_pPlayerPed->SetMoveSpeedVector(vecMoveSpeed);
				
				m_pPlayerPed->GetMatrix(&matPlayer);
				matPlayer.pos.X = m_ofSync.vecPos.X;
				matPlayer.pos.Y = m_ofSync.vecPos.Y;
				matPlayer.pos.Z = m_ofSync.vecPos.Z;
				m_pPlayerPed->SetMatrix(matPlayer);
			}
		}
		else if(GetState() == PLAYER_STATE_DRIVER && m_pPlayerPed->IsInVehicle())
		{
			/*if(m_pCurrentVehicle->GetModelIndex() != TRAIN_PASSENGER_LOCO &&
				m_pCurrentVehicle->GetModelIndex() != TRAIN_FREIGHT_LOCO &&
				m_pCurrentVehicle->GetModelIndex() != TRAIN_TRAM)
			{
				UpdateVehicleRotation();
			}*/
			UpdateVehicleRotation();

			if(m_icSync.vecMoveSpeed.X == 0.0f &&
				m_icSync.vecMoveSpeed.Y == 0.0f &&
				m_icSync.vecMoveSpeed.Z == 0.0f)
			{
				m_pCurrentVehicle->SetMoveSpeedVector(m_icSync.vecMoveSpeed);
			}

			m_pPlayerPed->SetKeys(m_icSync.wKeys, m_icSync.lrAnalog, m_icSync.udAnalog);

			if((GetTickCount() - m_dwLastRecvTick) > 1500)
				m_bIsAFK = true;

		}
		else if(GetState() == PLAYER_STATE_PASSENGER)
		{
			m_bPassengerDriveByMode = false;
			if((GetTickCount() - m_dwLastRecvTick) >= 3000)
				m_bIsAFK = true;
		}
		else
		{
			m_pPlayerPed->SetKeys(0, 0, 0);
			vecMoveSpeed.X = 0.0f;
			vecMoveSpeed.Y = 0.0f;
			vecMoveSpeed.Z = 0.0f;
			m_pPlayerPed->SetMoveSpeedVector(vecMoveSpeed);
		}

		if(m_byteState != PLAYER_STATE_WASTED)
			m_pPlayerPed->SetHealth(1000.0f);

		if((GetTickCount() - m_dwLastRecvTick) < 1500)
			m_bIsAFK = false;
	}
	else
	{
		if(m_pPlayerPed)
		{
			ResetAllSyncAttributes();
			pGame->RemovePlayer(m_pPlayerPed);
			m_pPlayerPed = nullptr;
		}
	}
}

void CRemotePlayer::SlerpRotation()
{
	MATRIX4X4 mat;
	CQuaternion quatPlayer, quatResult;

	if(m_pPlayerPed)
	{
		m_pPlayerPed->GetMatrix(&mat);

		quatPlayer.SetFromMatrix(mat);

		quatResult.Slerp(&m_ofSync.quat, &quatPlayer, 0.75f);
		quatResult.GetMatrix(&mat);
		m_pPlayerPed->SetMatrix(mat);

		float fZ = atan2(-mat.up.X, mat.up.Y) * 57.295776; /* rad to deg */
		if(fZ > 360.0f) fZ -= 360.0f;
		if(fZ < 0.0f) fZ += 360.0f;
		m_pPlayerPed->SetRotation(fZ);
	}
}

void CRemotePlayer::UpdateInCarMatrixAndSpeed(MATRIX4X4* mat, VECTOR* pos, VECTOR* speed)
{
	m_InCarQuaternion.SetFromMatrix(*mat);

	m_vecInCarTargetPos.X = pos->X;
	m_vecInCarTargetPos.Y = pos->Y;
	m_vecInCarTargetPos.Z = pos->Z;

	m_vecInCarTargetSpeed.X = speed->X;
	m_vecInCarTargetSpeed.Y = speed->Y;
	m_vecInCarTargetSpeed.Z = speed->Z;

	m_pCurrentVehicle->SetMoveSpeedVector(*speed);
}

void CRemotePlayer::UpdateInCarTargetPosition()
{
	MATRIX4X4 matEnt;
	VECTOR vec = { 0.0f, 0.0f, 0.0f };

	float delta = 0.0f;

	if(!m_pCurrentVehicle) return;

	m_pCurrentVehicle->GetMatrix(&matEnt);

	if(m_pCurrentVehicle->IsAdded())
	{
		m_vecPosOffset.X = FloatOffset(m_vecInCarTargetPos.X, matEnt.pos.X);
		m_vecPosOffset.Y = FloatOffset(m_vecInCarTargetPos.Y, matEnt.pos.Y);
		m_vecPosOffset.Z = FloatOffset(m_vecInCarTargetPos.Z, matEnt.pos.Z);

		if(m_vecPosOffset.X > 0.05f || m_vecPosOffset.Y > 0.05f || m_vecPosOffset.Z > 0.05f)
		{
			delta = 0.5f;
			if( m_pCurrentVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_BOAT ||
				m_pCurrentVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_PLANE ||
				m_pCurrentVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_HELI)
			{
				delta = 2.0f;
			}

			if(m_vecPosOffset.X > 8.0f || m_vecPosOffset.Y > 8.0f || m_vecPosOffset.Z > delta)
			{
				matEnt.pos.X = m_vecInCarTargetPos.X;
				matEnt.pos.Y = m_vecInCarTargetPos.Y;
				matEnt.pos.Z = m_vecInCarTargetPos.Z;
				m_pCurrentVehicle->SetMatrix(matEnt);
				m_pCurrentVehicle->SetMoveSpeedVector(m_vecInCarTargetSpeed);
			}
			else
			{
				m_pCurrentVehicle->GetMoveSpeedVector(&vec);
				if(m_vecPosOffset.X > 0.05f)
					vec.X += (m_vecInCarTargetPos.X - matEnt.pos.X) * 0.06f;
				if(m_vecPosOffset.Y > 0.05f)
					vec.Y += (m_vecInCarTargetPos.Y - matEnt.pos.Y) * 0.06f;
				if(m_vecPosOffset.Z > 0.05f)
					vec.Z += (m_vecInCarTargetPos.Z - matEnt.pos.Z) * 0.06f;

				if( FloatOffset(vec.X, 0.0f) > 0.01f ||
					FloatOffset(vec.Y, 0.0f) > 0.01f ||
					FloatOffset(vec.Z, 0.0f) > 0.01f)
				{
					m_pCurrentVehicle->SetMoveSpeedVector(vec);
				}
			}
		}
	}
	else
	{
		matEnt.pos.X = m_vecInCarTargetPos.X;
		matEnt.pos.Y = m_vecInCarTargetPos.Y;
		matEnt.pos.Z = m_vecInCarTargetPos.Z;
		m_pCurrentVehicle->SetMatrix(matEnt);
	}
}

void CRemotePlayer::UpdateVehicleRotation()
{
	CQuaternion quat, qresult;
	MATRIX4X4 matEnt;
	VECTOR vec = { 0.0f, 0.0f, 0.0f };

	if(!m_pCurrentVehicle) return;

	m_pCurrentVehicle->GetTurnSpeedVector(&vec);
	if(vec.X <= 0.02f)
	{
		if(vec.X < -0.02f) vec.X = -0.02f;
	}
	else vec.X = 0.02f;

	if(vec.Y <= 0.02f)
	{
		if(vec.Y < -0.02f) vec.Y = -0.02f;
	}
	else vec.Y = 0.02f;

	if(vec.Z <= 0.02f)
	{
		if(vec.Z < -0.02f) vec.Z = -0.02f;
	}
	else vec.Z = 0.02f;

	m_pCurrentVehicle->SetTurnSpeedVector(vec);

	m_pCurrentVehicle->GetMatrix(&matEnt);
	quat.SetFromMatrix(matEnt);
	qresult.Slerp(&m_InCarQuaternion, &quat, 0.75f);
	qresult.Normalize();
	qresult.GetMatrix(&matEnt);
	m_pCurrentVehicle->SetMatrix(matEnt);
}

void CRemotePlayer::StoreTrailerFullSyncData(TRAILER_SYNC_DATA *trSync)
{
	VEHICLEID TrailerID = m_icSync.TrailerID;
	if(!TrailerID) return;

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(pVehiclePool) 
	{
		MATRIX4X4 matTrailer;

		CVehicle *pTrailer = pVehiclePool->GetAt(TrailerID);
		if(pTrailer)
		{
			trSync->quat.Normalize();
			trSync->quat.GetMatrix(&matTrailer);
			matTrailer.pos.X = trSync->vecPos.X;
			matTrailer.pos.Y = trSync->vecPos.Y;
			matTrailer.pos.Z = trSync->vecPos.Z;

			pTrailer->SetMatrix(matTrailer);
			pTrailer->SetMoveSpeedVector(trSync->vecMoveSpeed);
			pTrailer->SetTurnSpeedVector(trSync->vecTurnSpeed);
		}
	}
}

bool CRemotePlayer::Spawn(uint8_t byteTeam, unsigned int iSkin, VECTOR *vecPos, float fRotation, 
	uint32_t dwColor, uint8_t byteFightingStyle, bool bVisible)
{
	if(m_pPlayerPed)
	{
		pGame->RemovePlayer(m_pPlayerPed);
	}

	CPlayerPed *pPlayerPed = pGame->NewPlayer(iSkin, vecPos->X, vecPos->Y, vecPos->Z, fRotation);

	if(pPlayerPed)
	{
		if(dwColor != 0) SetPlayerColor(dwColor);

		if(m_dwMarkerID)
		{
			pGame->DisableMarker(m_dwMarkerID);
			m_dwMarkerID = 0;
		}

		if(pNetGame->m_iShowPlayerMarkers) 
			pPlayerPed->ShowMarker(m_PlayerID);

		m_pPlayerPed = pPlayerPed;
		m_fReportedHealth = 100.0f;
		if(byteFightingStyle != 4)
			m_pPlayerPed->SetFightingStyle(byteFightingStyle);

		SetState(PLAYER_STATE_SPAWNED);
		return true;
	}

	SetState(PLAYER_STATE_NONE);
	return false;
}

void CRemotePlayer::Remove()
{
	if(m_dwMarkerID)
	{
		pGame->DisableMarker(m_dwMarkerID);
		m_dwMarkerID = 0;
	}

	if(m_dwGlobalMarkerID)
	{
		pGame->DisableMarker(m_dwGlobalMarkerID);
		m_dwGlobalMarkerID = 0;
	}

	if(m_pPlayerPed && bUsedPlayerSlots[m_pPlayerPed->m_bytePlayerNumber])
	{
		pGame->RemovePlayer(m_pPlayerPed);
		m_pPlayerPed = nullptr;
	}
	
	Deactivate();
}

void CRemotePlayer::HandleVehicleEntryExit()
{
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	if(!m_pPlayerPed) return;

	if(!m_pPlayerPed->IsInVehicle())
	{
		if(pVehiclePool->GetAt(m_VehicleID))
		{
			int iCarID = pVehiclePool->FindGtaIDFromID(m_VehicleID);
			m_pPlayerPed->PutDirectlyInVehicle(iCarID, m_byteSeatID);
		}
	}
}

void CRemotePlayer::EnterVehicle(VEHICLEID VehicleID, bool bPassenger)
{
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	if(bPassenger) {
		SetState(PLAYER_STATE_ENTER_VEHICLE_PASSENGER);
	} else {
		SetState(PLAYER_STATE_ENTER_VEHICLE_DRIVER);
	}
	
	if( m_pPlayerPed &&
		pVehiclePool->GetAt(VehicleID) &&
		!m_pPlayerPed->IsInVehicle())
	{
		int iGtaVehicleID = pVehiclePool->FindGtaIDFromID(VehicleID);
		if(iGtaVehicleID && iGtaVehicleID != INVALID_VEHICLE_ID)
		{
			m_pPlayerPed->SetKeys(0, 0, 0);
			m_pPlayerPed->EnterVehicle(iGtaVehicleID, bPassenger);
		}
	}
}

void CRemotePlayer::ExitVehicle()
{
	if(m_pPlayerPed && m_pPlayerPed->IsInVehicle())
	{
		m_pPlayerPed->SetKeys(0, 0, 0);
		m_pPlayerPed->ExitCurrentVehicle();
	}
}

void CRemotePlayer::UpdateOnFootPositionAndSpeed(VECTOR *vecPos, VECTOR *vecMove)
{
	m_vecOnFootTargetPos.X = vecPos->X;
	m_vecOnFootTargetPos.Y = vecPos->Y;
	m_vecOnFootTargetPos.Z = vecPos->Z;
	m_vecOnFootTargetSpeed.X = vecMove->X;
	m_vecOnFootTargetSpeed.Y = vecMove->Y;
	m_vecOnFootTargetSpeed.Z = vecMove->Z;

	m_pPlayerPed->SetMoveSpeedVector(m_vecOnFootTargetSpeed);
}

void CRemotePlayer::UpdateOnFootTargetPosition()
{
	MATRIX4X4 mat;
	VECTOR vec;

	if(!m_pPlayerPed) return;
	m_pPlayerPed->GetMatrix(&mat);

	if(!m_pPlayerPed->IsAdded())
	{
		mat.pos.X = m_vecOnFootTargetPos.X;
		mat.pos.Y = m_vecOnFootTargetPos.Y;
		mat.pos.Z = m_vecOnFootTargetPos.Z;

		m_pPlayerPed->SetMatrix(mat);
		return;
	}

	m_vecPosOffset.X = FloatOffset(m_vecOnFootTargetPos.X, mat.pos.X);
	m_vecPosOffset.Y = FloatOffset(m_vecOnFootTargetPos.Y, mat.pos.Y);
	m_vecPosOffset.Z = FloatOffset(m_vecOnFootTargetPos.Z, mat.pos.Z);

	if(m_vecPosOffset.X > 0.00001f || m_vecPosOffset.Y > 0.00001f || m_vecPosOffset.Z > 0.00001f)
	{
		if(m_vecPosOffset.X > 2.0f || m_vecPosOffset.Y > 2.0f || m_vecPosOffset.Z > 1.0f)
		{
			mat.pos.X = m_vecOnFootTargetPos.X;
			mat.pos.Y = m_vecOnFootTargetPos.Y;
			mat.pos.Z = m_vecOnFootTargetPos.Z;
			m_pPlayerPed->SetMatrix(mat);
			return;
		}

		m_pPlayerPed->GetMoveSpeedVector(&vec);
		if(m_vecPosOffset.X > 0.00001f)
			vec.X += (m_vecOnFootTargetPos.X - mat.pos.X) * 0.1f;
		if(m_vecPosOffset.Y > 0.00001f)
			vec.Y += (m_vecOnFootTargetPos.Y - mat.pos.Y) * 0.1f;
		if(m_vecPosOffset.Z > 0.00001f)
			vec.Z += (m_vecOnFootTargetPos.Z - mat.pos.Z) * 0.1f;

		m_pPlayerPed->SetMoveSpeedVector(vec);
	}
}

void CRemotePlayer::SetPlayerColor(uint32_t dwColor)
{
	if(m_PlayerID != INVALID_PLAYER_ID)
		SetRadarColor(m_PlayerID, dwColor);
}

void CRemotePlayer::Say(unsigned char* szText)
{
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	if (pPlayerPool) 
	{
		char * szPlayerName = pPlayerPool->GetPlayerName(m_PlayerID);
		pChatWindow->AddChatMessage(szPlayerName,GetPlayerColor(), (char*)szText);
	}
}

void CRemotePlayer::StoreOnFootFullSyncData(ONFOOT_SYNC_DATA *pofSync, uint32_t dwTime)
{
	if( !dwTime || (dwTime - m_dwUnkTime) >= 0 )
	{
		m_dwUnkTime = dwTime;

		m_dwLastRecvTick = GetTickCount();
		memcpy(&m_ofSync, pofSync, sizeof(ONFOOT_SYNC_DATA));
		m_fReportedHealth = (float)pofSync->byteHealth;
		m_fReportedArmour = (float)pofSync->byteArmour;
		m_byteSpecialAction = pofSync->byteSpecialAction;
		m_byteUpdateFromNetwork = UPDATE_TYPE_ONFOOT;

		if(m_pPlayerPed)
		{
			if(m_pPlayerPed->IsInVehicle())
			{
				if( m_byteSpecialAction != SPECIAL_ACTION_ENTER_VEHICLE && 
				m_byteSpecialAction != SPECIAL_ACTION_EXIT_VEHICLE /*&& !sub_100A6F00()*/)
					RemoveFromVehicle();
			}
			else SetSpecialAction(m_byteSpecialAction);
		}

		SetState(PLAYER_STATE_ONFOOT);
	}
}

void CRemotePlayer::StoreAimFullSyncData(AIM_SYNC_DATA * pAimSync)
{
	if(!m_pPlayerPed) return;
	m_pPlayerPed->SetCameraMode(pAimSync->byteCamMode);

	CAMERA_AIM Aim;

	Aim.f1x = pAimSync->vecAimf1.X;
	Aim.f1y = pAimSync->vecAimf1.Y;
	Aim.f1z = pAimSync->vecAimf1.Z;
	Aim.f2x = pAimSync->vecAimf1.X;
	Aim.f2y = pAimSync->vecAimf1.Y;
	Aim.f2z = pAimSync->vecAimf1.Z;
	Aim.pos1x = pAimSync->vecAimPos.X;
	Aim.pos1y = pAimSync->vecAimPos.Y;
	Aim.pos1z = pAimSync->vecAimPos.Z;
	Aim.pos2x = pAimSync->vecAimPos.X;
	Aim.pos2y = pAimSync->vecAimPos.Y;
	Aim.pos2z = pAimSync->vecAimPos.Z;

	m_pPlayerPed->SetCurrentAim(&Aim);
	m_pPlayerPed->SetAimZ(pAimSync->fAimZ);

	float fExtZoom = (float)(pAimSync->byteCamExtZoom)/63.0f;
	m_pPlayerPed->SetCameraExtendedZoom(fExtZoom);
	
	WEAPON_SLOT_TYPE* pwstWeapon = m_pPlayerPed->GetCurrentWeaponSlot();
	if(pAimSync->byteWeaponState == WS_RELOADING)
		pwstWeapon->dwState = 2;		// Reloading
	else
	{
		if (pAimSync->byteWeaponState != WS_MORE_BULLETS) 
			pwstWeapon->dwAmmoInClip = (uint32_t)pAimSync->byteWeaponState;
		else
		{
			if(pwstWeapon->dwAmmoInClip < 2)
				pwstWeapon->dwAmmoInClip = 2;
		}
	}
}

void CRemotePlayer::StoreBulletFullSyncData(BULLET_SYNC_DATA* pBulletSync)
{
	if(!m_pPlayerPed || !m_pPlayerPed->IsAdded()) return;

	BULLET_DATA btData;
	memset(&btData, 0, sizeof(BULLET_DATA));

	btData.vecOrigin.X = pBulletSync->vecOrigin.X;
	btData.vecOrigin.Y = pBulletSync->vecOrigin.Y;
	btData.vecOrigin.Z = pBulletSync->vecOrigin.Z;

	btData.vecPos.X = pBulletSync->vecPos.X;
	btData.vecPos.Y = pBulletSync->vecPos.Y;
	btData.vecPos.Z = pBulletSync->vecPos.Z;

	btData.vecOffset.X = pBulletSync->vecOffset.X;
	btData.vecOffset.Y = pBulletSync->vecOffset.Y;
	btData.vecOffset.Z = pBulletSync->vecOffset.Z;

	if(pBulletSync->byteHitType != 0)
	{
		if(	btData.vecOffset.X > 300.0f 	||
			btData.vecOffset.X < -300.0f 	||
			btData.vecOffset.Y > 300.0f 	||
			btData.vecOffset.Y < -300.0f 	||
			btData.vecOffset.Z > 300.0f 	||
			btData.vecOffset.Z < -300.0f
			)
		{
			return;
		}

		if(pBulletSync->byteHitType == 1)
		{
			CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
			if(pPlayerPool)
			{
				if(pBulletSync->PlayerID == pPlayerPool->GetLocalPlayerID())
				{
					btData.pEntity = &pGame->FindPlayerPed()->m_pPed->entity;
				}
				else if(pBulletSync->PlayerID == m_PlayerID) return;
				else
				{
					CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(pBulletSync->PlayerID);
					if(pRemotePlayer)
					{
						CPlayerPed *pPlayerPed = pRemotePlayer->GetPlayerPed();
						if(m_pPlayerPed)
							btData.pEntity = &m_pPlayerPed->m_pPed->entity;
					}
				}
			}
		}
		else if(pBulletSync->byteHitType == 2)
		{
			CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
			if(pVehiclePool)
			{
				CVehicle *pVehicle = pVehiclePool->GetAt(pBulletSync->PlayerID);
				if(pVehicle)
				{
					btData.pEntity = &pVehicle->m_pVehicle->entity;
				}
			}
		}
	}

	if(m_pPlayerPed->IsAdded())
	{
		uint8_t byteWeapon = pBulletSync->byteWeaponID;
		if(m_pPlayerPed->GetCurrentWeapon() != byteWeapon)
		{
			m_pPlayerPed->SetArmedWeapon(byteWeapon);
			if(m_pPlayerPed->GetCurrentWeapon() != byteWeapon)
			{
				m_pPlayerPed->GiveWeapon(byteWeapon, 9999);
				m_pPlayerPed->SetArmedWeapon(byteWeapon);
			}
		}
	}

	m_pPlayerPed->ProcessBulletData(&btData);
	m_pPlayerPed->FireInstant();
}

void CRemotePlayer::StoreInCarFullSyncData(INCAR_SYNC_DATA *picSync, uint32_t dwTime)
{
	if(!dwTime || (dwTime - m_dwUnkTime >= 0))
	{
		m_dwUnkTime = dwTime;

		CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

		memcpy(&m_icSync, picSync, sizeof(INCAR_SYNC_DATA));
		m_VehicleID = picSync->VehicleID;
		if(pVehiclePool) m_pCurrentVehicle = pVehiclePool->GetAt(m_VehicleID);

		m_byteSeatID = 0;
		m_fReportedHealth = (float)picSync->bytePlayerHealth;
		m_fReportedArmour = (float)picSync->bytePlayerArmour;
		m_byteUpdateFromNetwork = UPDATE_TYPE_INCAR;
		m_dwLastRecvTick = GetTickCount();

		m_byteSpecialAction = 0;

		if(m_pPlayerPed && !m_pPlayerPed->IsInVehicle())
			HandleVehicleEntryExit();

		SetState(PLAYER_STATE_DRIVER);
	}
}

void CRemotePlayer::StorePassengerFullSyncData(PASSENGER_SYNC_DATA *ppsSync)
{
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	memcpy(&m_psSync, ppsSync, sizeof(PASSENGER_SYNC_DATA));
	m_VehicleID = ppsSync->VehicleID;
	m_byteSeatID = ppsSync->byteSeatFlags & 127;
	if(pVehiclePool) m_pCurrentVehicle = pVehiclePool->GetAt(m_VehicleID);
	m_fReportedHealth = (float)ppsSync->bytePlayerHealth;
	m_fReportedArmour = (float)ppsSync->bytePlayerArmour;
	m_byteUpdateFromNetwork = UPDATE_TYPE_PASSENGER;
	m_dwLastRecvTick = GetTickCount();

	if(m_pPlayerPed && !m_pPlayerPed->IsInVehicle())
		HandleVehicleEntryExit();

	SetState(PLAYER_STATE_PASSENGER);
}

void CRemotePlayer::RemoveFromVehicle()
{
	MATRIX4X4 mat;

	if(m_pPlayerPed)
	{
		if(m_pPlayerPed->IsInVehicle())
		{
			m_pPlayerPed->GetMatrix(&mat);
			m_pPlayerPed->RemoveFromVehicleAndPutAt(mat.pos.X, mat.pos.Y, mat.pos.Z);
		}
	}
}

uint32_t CRemotePlayer::GetPlayerColor()
{
	return TranslateColorCodeToRGBA(m_PlayerID);
}

uint32_t CRemotePlayer::GetPlayerColorAsARGB()
{
	return (TranslateColorCodeToRGBA(m_PlayerID) >> 8) | 0xFF000000;	
}

void CRemotePlayer::ShowGlobalMarker(short sX, short sY, short sZ)
{
	if(m_dwGlobalMarkerID)
	{
		pGame->DisableMarker(m_dwGlobalMarkerID);
		m_dwGlobalMarkerID = 0;
	}

	if(!m_pPlayerPed)
	{
		m_dwGlobalMarkerID = pGame->CreateRadarMarkerIcon(0, (float)sX, (float)sY, (float)sZ, m_PlayerID, 0);
	}
}

void CRemotePlayer::HideGlobalMarker()
{
	if(m_dwGlobalMarkerID)
	{
		pGame->DisableMarker(m_dwGlobalMarkerID);
		m_dwGlobalMarkerID = 0;
	}
}

void CRemotePlayer::StateChange(uint8_t byteNewState, uint8_t byteOldState)
{
	if(byteNewState == PLAYER_STATE_DRIVER && byteOldState == PLAYER_STATE_ONFOOT)
	{
		// If their new vehicle is the one the local player
		// is driving, we'll have to kick the local player out
		CPlayerPed *pLocalPlayerPed = pGame->FindPlayerPed();
		CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
		VEHICLEID LocalVehicle=0xFFFF;
		MATRIX4X4 mat;

		if(pLocalPlayerPed && pLocalPlayerPed->IsInVehicle() && !pLocalPlayerPed->IsAPassenger() && pVehiclePool)
		{
			LocalVehicle = pVehiclePool->FindIDFromGtaPtr(pLocalPlayerPed->GetGtaVehicle());
			if(LocalVehicle == m_VehicleID) 
			{
				pLocalPlayerPed->GetMatrix(&mat);
				pLocalPlayerPed->RemoveFromVehicleAndPutAt(mat.pos.X,mat.pos.Y,mat.pos.Z + 1.0f);	
				pGame->DisplayGameText("~r~Car Jacked~w~!",1000,5);
			}
		}
	}
}

void CRemotePlayer::ResetAllSyncAttributes()
{	
	m_VehicleID = 0;
	memset(&m_ofSync,0,sizeof(ONFOOT_SYNC_DATA));
	memset(&m_icSync,0,sizeof(INCAR_SYNC_DATA));
	memset(&m_psSync,0,sizeof(PASSENGER_SYNC_DATA));
	memset(&m_aimSync,0,sizeof(AIM_SYNC_DATA));
	memset(&m_trSync,0,sizeof(TRAILER_SYNC_DATA));
	m_fReportedHealth = 0.0f;
	m_fReportedArmour = 0.0f;
	m_pCurrentVehicle = NULL;
	m_byteSeatID = 0;
	m_bPassengerDriveByMode = false;
	m_iIsInAModShop = 0;
}

void CRemotePlayer::HandleDeath()
{
	if(!m_pPlayerPed || !m_pPlayerPed->IsAdded()) 
		return;

	m_pPlayerPed->SetKeys(0, 0, 0);
	m_pPlayerPed->SetDead();

	SetState(PLAYER_STATE_WASTED);
	ResetAllSyncAttributes();
}

void CRemotePlayer::ProcessSpecialActions(uint8_t byteSpecialAction)
{
	if(!m_pPlayerPed || !m_pPlayerPed->IsAdded()) return;

	if(GetState() != PLAYER_STATE_ONFOOT) 
	{
		byteSpecialAction = SPECIAL_ACTION_NONE;
		m_ofSync.byteSpecialAction = SPECIAL_ACTION_NONE;
	}

	// cellphone:start
	if(byteSpecialAction == SPECIAL_ACTION_USECELLPHONE && !m_pPlayerPed->IsCellphoneEnabled()) 
	{
		m_pPlayerPed->ToggleCellphone(1);
		return;
	}

	// cellphone:stop
	if(byteSpecialAction != SPECIAL_ACTION_USECELLPHONE && m_pPlayerPed->IsCellphoneEnabled()) 
	{
		m_pPlayerPed->ToggleCellphone(0);
		return;
	}

	// jetpack:start
	if(byteSpecialAction == SPECIAL_ACTION_USEJETPACK) 
	{
		if(!m_pPlayerPed->IsInJetpackMode()) 
		{
			m_bJetpack = true;
			m_pPlayerPed->StartJetpack();
			return;
		}
	}

	// jetpack:stop
	if(byteSpecialAction != SPECIAL_ACTION_USEJETPACK) 
	{
		if(m_pPlayerPed->IsInJetpackMode()) 
		{
			m_bJetpack = false;
			m_pPlayerPed->StopJetpack();
			return;
		}
	}

	// handsup:start
	if(byteSpecialAction == SPECIAL_ACTION_HANDSUP && !m_pPlayerPed->HasHandsUp())
		m_pPlayerPed->HandsUp();

	// handsup:stop
	if(byteSpecialAction != SPECIAL_ACTION_HANDSUP && m_pPlayerPed->HasHandsUp())
		m_pPlayerPed->StopDancing(); // has the same effect

	// dancing:start
	if(!m_pPlayerPed->IsDancing())
	{
		if(byteSpecialAction == SPECIAL_ACTION_DANCE1)
			m_pPlayerPed->PlayDance(0);
		if(byteSpecialAction == SPECIAL_ACTION_DANCE2)
			m_pPlayerPed->PlayDance(1);
		if(byteSpecialAction == SPECIAL_ACTION_DANCE3)
			m_pPlayerPed->PlayDance(2);
		if(byteSpecialAction == SPECIAL_ACTION_DANCE4)
			m_pPlayerPed->PlayDance(3);
	}

	// dancing:stop
	if(m_pPlayerPed->IsDancing())
	{
		if(byteSpecialAction != SPECIAL_ACTION_DANCE1 && 
			byteSpecialAction != SPECIAL_ACTION_DANCE2 && 
			byteSpecialAction != SPECIAL_ACTION_DANCE3 && 
			byteSpecialAction != SPECIAL_ACTION_DANCE4)
			m_pPlayerPed->StopDancing();
	}

	// parachutes:we don't have any network indicators for this yet
	//m_pPlayerPed->ProcessParachutes();
}

void CRemotePlayer::SetSpecialAction(uint8_t special_action)
{
	if(!m_pPlayerPed || !m_pPlayerPed->IsAdded()) return;

	if(special_action == SPECIAL_ACTION_DUCK && GetPlayerPed()->IsCrouching() == false)
		GetPlayerPed()->ApplyCrouch();
	else if(special_action == SPECIAL_ACTION_NONE)
	{
		if(GetPlayerPed()->IsCrouching())
			GetPlayerPed()->ResetCrouch();
	}
}

void CRemotePlayer::UpdateTrainDriverMatrixAndSpeed(MATRIX4X4 *matWorld,VECTOR *vecMoveSpeed, float fTrainSpeed)
{
	MATRIX4X4 matVehicle;
	VECTOR vecInternalMoveSpeed;
	bool bTeleport;
	float fDif;

	if(!m_pPlayerPed || !m_pCurrentVehicle) return;

	m_pCurrentVehicle->GetMatrix(&matVehicle);

	if(matWorld->pos.X >= matVehicle.pos.X) {
		fDif = matWorld->pos.X - matVehicle.pos.X;
	} else {
		fDif = matVehicle.pos.X - matWorld->pos.X;
	}
	if(fDif > 10.0f) bTeleport = true;

	if(matWorld->pos.Y >= matVehicle.pos.Y) {
		fDif = matWorld->pos.Y - matVehicle.pos.Y;
	} else {
		fDif = matVehicle.pos.Y - matWorld->pos.Y;
	}
	if(fDif > 10.0f) bTeleport = true;

	if(bTeleport) m_pCurrentVehicle->TeleportTo(matWorld->pos.X,matWorld->pos.Y,matWorld->pos.Z);
	
	m_pCurrentVehicle->GetMoveSpeedVector(&vecInternalMoveSpeed);
	vecInternalMoveSpeed.X = vecMoveSpeed->X;
	vecInternalMoveSpeed.Y = vecMoveSpeed->Y;
	vecInternalMoveSpeed.Z = vecMoveSpeed->Z;
	m_pCurrentVehicle->SetMoveSpeedVector(vecInternalMoveSpeed);
	m_pCurrentVehicle->SetTrainSpeed(fTrainSpeed);
}

bool CRemotePlayer::IsSurfingOrTurretMode()
{
	if(GetState() == PLAYER_STATE_ONFOOT) {
		if( m_ofSync.wSurfInfo != 0 && 
			m_ofSync.wSurfInfo != INVALID_VEHICLE_ID &&
			m_ofSync.wSurfInfo != INVALID_OBJECT_ID ) {
				return true;
			}
	}
	return false;
}

void CRemotePlayer::UpdateSurfing()
{
	if ( m_ofSync.wSurfInfo > MAX_VEHICLES ) { // its an object

			m_ofSync.wSurfInfo -= MAX_VEHICLES; // derive proper object id
			CObjectPool* pObjectPool = pNetGame->GetObjectPool();
			CObject* pObject = pObjectPool->GetAt((uint8_t)m_ofSync.wSurfInfo);

			if (pObject) {
				MATRIX4X4 objMat;
				pObject->GetMatrix(&objMat);
				objMat.pos.X += m_ofSync.vecSurfOffsets.X;
				objMat.pos.Y += m_ofSync.vecSurfOffsets.Y;
				objMat.pos.Z += m_ofSync.vecSurfOffsets.Z;
				m_pPlayerPed->SetMatrix(objMat);
			}

	} else { // must be a vehicle

		CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
		CVehicle* pVehicle = pVehiclePool->GetAt(m_ofSync.wSurfInfo);

		if (pVehicle) {
			MATRIX4X4 matPlayer, matVehicle;
			VECTOR vecMoveSpeed,vecTurnSpeed;

			pVehicle->GetMatrix(&matVehicle);
			pVehicle->GetMoveSpeedVector(&vecMoveSpeed);
			pVehicle->GetTurnSpeedVector(&vecTurnSpeed);

			m_pPlayerPed->GetMatrix(&matPlayer);
		
			matPlayer.pos.X = matVehicle.pos.X + m_ofSync.vecSurfOffsets.X;
			matPlayer.pos.Y = matVehicle.pos.Y + m_ofSync.vecSurfOffsets.Y;
			matPlayer.pos.Z = matVehicle.pos.Z + m_ofSync.vecSurfOffsets.Z;

			m_pPlayerPed->SetMatrix(matPlayer);
			m_pPlayerPed->SetMoveSpeedVector(vecMoveSpeed);
			m_pPlayerPed->SetTurnSpeedVector(vecTurnSpeed);
		}
	}
}
