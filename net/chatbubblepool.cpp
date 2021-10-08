#include "../main.h"
#include "../game/game.h"
#include "netgame.h"
#include "chatbubblepool.h" 

extern CGame *pGame;
extern CNetGame *pNetGame;
extern CGUI *pGUI;

CChatBubblePool::CChatBubblePool() 
{
    for(size_t i = 0; i < MAX_PLAYERS; i++)
        m_ChatBubbleArray[i].bInited = false;
}

CChatBubblePool::~CChatBubblePool() 
{ 
    for(size_t i = 0; i < MAX_PLAYERS; i++)
        m_ChatBubbleArray[i].bInited = false;
}

void CChatBubblePool::AddChatBubble(PLAYERID playerid, CHATBUBBLE_DATA bubble)
{
    m_ChatBubbleArray[playerid] = bubble;
    m_ChatBubbleArray[playerid].bInited = true;
    m_ChatBubbleArray[playerid].iFirstTick = GetTickCount();
}

void CChatBubblePool::Render()
{
    VECTOR VecPos;
	MATRIX4X4 matLocal, matPlayer;
	int dwHitEntity;

	if(pNetGame)
	{
		CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
		pGame->FindPlayerPed()->GetMatrix(&matLocal);
		size_t tmpcount = 0;
		for(size_t playerId = 0; playerId < MAX_PLAYERS; playerId++)
		{
            if(m_ChatBubbleArray[playerId].bInited == false) continue;
            if(GetTickCount() - m_ChatBubbleArray[playerId].iFirstTick > m_ChatBubbleArray[playerId].iElapseTime) 
            {
                m_ChatBubbleArray[playerId].bInited = false;
                continue;
            }
			if(pPlayerPool->GetSlotState(playerId) == true)
			{
				CRemotePlayer* pPlayer = pPlayerPool->GetAt(playerId);
				if(pPlayer && pPlayer->IsActive())
				{
					CPlayerPed* pPlayerPed = pPlayer->GetPlayerPed();

					if(pPlayerPed->GetDistanceFromCamera() <= m_ChatBubbleArray[playerId].fDrawDistance)
					{
                        if(pPlayerPed->IsAdded())
                        {
	                        VecPos.X = 0.0f;
	                        VecPos.Y = 0.0f;
	                        VecPos.Z = 0.0f;
	                        pPlayerPed->GetBonePosition(8, &VecPos);
	                        
							Draw(playerId, pPlayerPed->GetDistanceFromCamera(), &VecPos);
						}
					}
				}
			}
		}
	}
}

void CChatBubblePool::Draw(size_t id, float dist, VECTOR *vec)
{
	VECTOR TagPos;

	TagPos.X = vec->X;
	TagPos.Y = vec->Y;
	TagPos.Z = vec->Z;
	TagPos.Z += (0.25f + (dist * 0.0475f)) + 0.25f;

	VECTOR Out;

	// CSprite::CalcScreenCoors
	(( void (*)(VECTOR*, VECTOR*, float*, float*, bool, bool))(g_libGTASA+0x54EEC0+1))(&TagPos, &Out, 0, 0, 0, 0); // CSprite::CalcScreenCoors
	if(Out.Z < 1.0f) return;
	ImVec2 pos = ImVec2(Out.X, Out.Y);
	// removed piece
	Render3DLabel(pos, m_ChatBubbleArray[id].szText, m_ChatBubbleArray[id].dwColor);
}
