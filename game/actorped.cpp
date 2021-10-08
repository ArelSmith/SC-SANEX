#include "../main.h"
#include "game.h"
#include "net/netgame.h"
#include "util/armhook.h"

extern CGame *pGame;
extern CNetGame *pNetGame;

CActorPed::CActorPed(uint16_t usModel, VECTOR vecPosition, float fRotation, float m_fHealth, bool bInvulnerable) 
{
	if(!pGame->IsModelLoaded(usModel))
	{
		pGame->RequestModel(usModel, GAME_REQUIRED);
		pGame->LoadRequestedModels(false);
	}

	uint32_t actorGTAId = 0;
	ScriptCommand(&create_char, 22, usModel, vecPosition.X, vecPosition.Y, vecPosition.Z, &actorGTAId);

	m_dwGTAId = actorGTAId;
	m_pPed = GamePool_Ped_GetAt(m_dwGTAId);
	m_pEntity = (ENTITY_TYPE *)m_pPed;

	ForceTargetRotation(fRotation);
	TeleportTo(vecPosition.X, vecPosition.Y, vecPosition.Z);

	if(m_fHealth <= 0.0f)
		SetDead();
	else SetHealth(m_fHealth);

	ScriptCommand(&lock_actor, m_dwGTAId, 1);
	m_pEntity->nEntityFlags.m_bUsesCollision = 0;

	if(bInvulnerable)
		ScriptCommand(&set_actor_immunities, m_dwGTAId, 1, 1, 1, 1, 1);
	else ScriptCommand(&set_actor_immunities, m_dwGTAId, 0, 0, 0, 0, 0);
	return;
}

CActorPed::~CActorPed() 
{
	Destroy();
}

void CActorPed::Destroy() 
{
	if(!m_pPed || !GamePool_Ped_GetAt(m_dwGTAId) || m_pPed->entity.vtable == 0x5C7358)
	{
		Log("CActorPed::Destroy: invalid pointer/vtable");
		m_pPed = nullptr;
		m_pEntity = nullptr;
		m_dwGTAId = 0;
		return;
	}

	((void (*)(PED_TYPE *))(*(void **)(m_pPed->entity.vtable + 0x4)))(m_pPed);

	m_pPed = nullptr;
	m_pEntity = nullptr;
	m_dwGTAId = 0;
	return;
}

void CActorPed::SetHealth(float m_fHealth) 
{
	if(!m_pPed) return;

	m_pPed->fHealth = m_fHealth;
	return;
}

void CActorPed::ForceTargetRotation(float fRotation) 
{
	if(!m_pPed) return;

	m_pPed->fRotation1 = DegToRad(fRotation);
	m_pPed->fRotation2 = DegToRad(fRotation);

	ScriptCommand(&set_actor_z_angle, m_dwGTAId, fRotation);
}

void CActorPed::ApplyAnimation(char *szAnimName, char *szAnimFile, float fDelta, int bLoop, int bLockX, int bLockY, int bFreeze, int uiTime) 
{
	if(!m_pPed) return;
	if(!strcmp(szAnimFile, "SAMP")) return;

	int iWaitAnimLoad = 0;
	if(!pGame->IsAnimationLoaded(szAnimFile))
	{		
		pGame->RequestAnimation(szAnimFile);
		while(!pGame->IsAnimationLoaded(szAnimFile))
		{
			usleep(1000);
			iWaitAnimLoad++;
			if(iWaitAnimLoad > 15) return;
		}
	}

	ScriptCommand(&apply_animation, m_dwGTAId, szAnimName, szAnimFile, fDelta, bLoop, bLockX, bLockY, bFreeze, uiTime);
}

void CActorPed::SetDead() 
{
	if(m_dwGTAId && m_pPed) 
	{
		try {
			ExtinguishFire();
		} catch(...) {}

		MATRIX4X4 mat;
		GetMatrix(&mat);
		// will reset the tasks
		TeleportTo(mat.pos.X, mat.pos.Y, mat.pos.Z);
        m_pPed->fHealth = 0.0f;
		ScriptCommand(&kill_actor, m_dwGTAId);
	}
}

void CActorPed::ExtinguishFire()
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
