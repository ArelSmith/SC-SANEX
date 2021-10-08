#include "../main.h"
#include "../game/game.h"
#include "netgame.h"
#include "../chatwindow.h"

extern CGame *pGame;
extern CChatWindow *pChatWindow;

int iExceptPlayerMessageDisplayed = 0;

CPlayerPool::CPlayerPool()
{
	m_pLocalPlayer = new CLocalPlayer();

	for(PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++)
	{
		m_bPlayerSlotState[playerId] = false;
		m_pPlayers[playerId] = nullptr;
	}
}

CPlayerPool::~CPlayerPool()
{
	delete m_pLocalPlayer;
	m_pLocalPlayer = nullptr;

	for(PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++)
		Delete(playerId, 0);
}

bool CPlayerPool::Process()
{
	for(PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++)
	{
		if(m_bPlayerSlotState[playerId])
		{
			try {
				m_pPlayers[playerId]->Process();
			} catch(...) {
				if(!iExceptPlayerMessageDisplayed) {
					pChatWindow->AddDebugMessage("Warning: Error Processing Player(%u)",playerId);
					//Delete(playerId,0);
					iExceptPlayerMessageDisplayed++;
				}
			}
		}
	}

	try {
		m_pLocalPlayer->Process();
	} catch(...) {
		if(!iExceptPlayerMessageDisplayed) {
			pChatWindow->AddDebugMessage("Warning: Error Processing Player");
			iExceptPlayerMessageDisplayed++;
		}
	}
	return true;
}

bool CPlayerPool::New(PLAYERID playerId, char *szPlayerName, bool IsNPC)
{
	m_pPlayers[playerId] = new CRemotePlayer();

	if(m_pPlayers[playerId])
	{
		strcpy(m_szPlayerNames[playerId], szPlayerName);
		m_pPlayers[playerId]->SetID(playerId);
		m_pPlayers[playerId]->SetNPC(IsNPC);
		m_bPlayerSlotState[playerId] = true;

		return true;
	}

	return false;
}

uint8_t CPlayerPool::GetPlayerTeam(PLAYERID playerId)
{
	return 255;
}

bool CPlayerPool::Delete(PLAYERID playerId, uint8_t byteReason)
{
	if(!GetSlotState(playerId) || !m_pPlayers[playerId])
		return false;

	if(GetLocalPlayer()->IsSpectating() && GetLocalPlayer()->m_SpectateID == playerId)
		GetLocalPlayer()->ToggleSpectating(false);

	m_bPlayerSlotState[playerId] = false;
	delete m_pPlayers[playerId];
	m_pPlayers[playerId] = nullptr;

	return true;
}

PLAYERID CPlayerPool::FindRemotePlayerIDFromGtaPtr(PED_TYPE * pActor)
{
	CPlayerPed *pPlayerPed;

	for(PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++)
	{
		if(m_bPlayerSlotState[playerId])
		{
			pPlayerPed = m_pPlayers[playerId]->GetPlayerPed();

			if(pPlayerPed) {
				PED_TYPE *pTestActor = pPlayerPed->GetGtaActor();
				if((pTestActor != NULL) && (pActor == pTestActor)) // found it
					return m_pPlayers[playerId]->GetID();
			}
		}
	}

	return INVALID_PLAYER_ID;	
}

void CPlayerPool::ApplyCollisionChecking()
{
	for(int i = 0; i < MAX_PLAYERS; i++) 
	{
		CRemotePlayer *pPlayer = GetAt(i);
		if(pPlayer) 
		{
			CPlayerPed *pPlayerPed = pPlayer->GetPlayerPed();
			if(pPlayerPed) 
			{
				if(!pPlayerPed->IsInVehicle()) 
				{
					m_bCollisionChecking[i] = pPlayerPed->GetCollisionChecking();
					pPlayerPed->SetCollisionChecking(true);
				}
			}
		}
	}
}

void CPlayerPool::ResetCollisionChecking()
{
	for(int i = 0; i < MAX_PLAYERS; i++) 
	{
		CRemotePlayer *pPlayer = GetAt(i);
		if(pPlayer) 
		{
			CPlayerPed *pPlayerPed = pPlayer->GetPlayerPed();
			if(pPlayerPed) 
			{
				if(!pPlayerPed->IsInVehicle())
					pPlayerPed->SetCollisionChecking(m_bCollisionChecking[i]);
			}
		}
	}
}

PLAYERID CPlayerPool::GetCount(bool bWithNpc)
{
	PLAYERID byteCount = 0;
	if(bWithNpc)
	{
		for(PLAYERID p = 0; p < MAX_PLAYERS; p++)
		{
			if(GetSlotState(p)) 
				byteCount++;
		}
	}
	else
	{
		for(PLAYERID p = 0; p < MAX_PLAYERS; p++)
		{
			if(GetSlotState(p))
			{
				CRemotePlayer* pPlayer = GetAt(p);
				if(pPlayer && !pPlayer->IsNPC()) 
					byteCount++;
			}
		}
	}
	return byteCount;
}

void CPlayerPool::UpdateScore(PLAYERID playerId, int iScore)
{
	if(playerId == m_LocalPlayerID)
		m_iLocalPlayerScore = iScore;
	else
	{
		if(playerId > MAX_PLAYERS-1)
			return;

		m_iPlayerScores[playerId] = iScore;
	}
}

void CPlayerPool::UpdatePing(PLAYERID playerId, uint32_t dwPing)
{
	if(playerId == m_LocalPlayerID)
		m_dwLocalPlayerPing = dwPing;
	else
	{
		if(playerId > MAX_PLAYERS)
			return;

		m_dwPlayerPings[playerId] = dwPing;
	}
}