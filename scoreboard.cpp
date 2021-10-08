#include "scoreboard.h"
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "gui/gui.h"
#include "vendor/imgui/imgui_internal.h"
#include "util/util.h"
#include "settings.h"

extern CNetGame *pNetGame;
extern CGame *pGame;
extern CGUI *pGUI;
extern CSettings *pSettings;

CScoreBoard::CScoreBoard()
{
    m_iOffset = 0;
    m_bSorted = false;
    m_bToggle = false;
	
	m_fScoreBoardSizeX = pGUI->ScaleX(pSettings->Get().fScoreBoardSizeX);
	m_fScoreBoardSizeY = pGUI->ScaleY(pSettings->Get().fScoreBoardSizeY);

    m_pPlayers = 0;
    m_pPlayerCount = 0;
    Log("Scoreboard initialized.");
}

CScoreBoard::~CScoreBoard(){}

void SwapPlayerInfo(PLAYER_SCORE_INFO* psi1, PLAYER_SCORE_INFO* psi2)
{
	PLAYER_SCORE_INFO plrinf;
	memcpy(&plrinf, psi1, sizeof(PLAYER_SCORE_INFO));
	memcpy(psi1, psi2, sizeof(PLAYER_SCORE_INFO));
	memcpy(psi2, &plrinf, sizeof(PLAYER_SCORE_INFO));
}

void CScoreBoard::Render()
{
    ProcessUpdating();

    if(!m_bToggle) return;
    if(!m_pPlayers) return;

    PLAYERID endplayer = m_pPlayerCount;
    char szPlayerCount[4], szPlayerId[11], szScore[11], szPing[11], szServerAddress[512];
    unsigned char RGBcolors[3];

    cp1251_to_utf8(szServerAddress, pNetGame->m_szHostName);
    sprintf(szPlayerCount, "%d", m_pPlayerCount);
    uint16_t nigga = strlen(szPlayerCount);

    ImGuiIO &io = ImGui::GetIO();
    ImVec2 size = ImGui::GetWindowSize();

    ImGui::SetNextWindowSize(ImVec2(m_fScoreBoardSizeX,m_fScoreBoardSizeY), NULL);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::Begin("ScoreboardContainer", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    ImGui::Columns(2, "###ScoreboardInfo", false);
    ImGui::Text("%s", szServerAddress); ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * (84 - nigga)); ImGui::NextColumn();
    ImGui::Text("Players: %d", m_pPlayerCount); ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * (16 + nigga));
    ImGui::EndColumns();

    ImGui::NewLine();

    ImGui::Columns(4, "###ScoreboardHeader", false);
    ImGui::TextColored(ImVec4(0.57f, 0.65f, 0.78f, 1.1f), "id"); ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * 10); ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0.57f, 0.65f, 0.78f, 1.1f), "name"); ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * 45); ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0.57f, 0.65f, 0.78f, 1.1f), "score"); ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * 20); ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0.57f, 0.65f, 0.78f, 1.1f), "ping"); ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * 25);
	ImGui::EndColumns();
	
	ImGui::BeginChild("###ScoreboardChild", ImVec2(-1, m_fScoreBoardSizeY / 100 * 81), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoSavedSettings);
	ImGui::Columns(4, "###ScoreboardBody", false);
    for(uint32_t line = m_iOffset; line < endplayer; line++)
    {
        int tmpTabId = 0;
        ImVec2 differentOffsets;
        if(tmpTabId == 0)
        {
            ImGui::PushID(tmpTabId+line);

            std::stringstream ss;
            ss << line+tmpTabId;
            std::string s = ss.str();

            std::string itemid = "##" + s;
            bool is_selected = (m_iSelectedItem == line);

            if(ImGui::Selectable(itemid.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0.0f, pGUI->GetFontSize() + ImGui::GetStyle().ItemSpacing.y)))
            {
                if(m_iLastSelectedItem == m_iSelectedItem)
                {
                    RakNet::BitStream bsSend;
                    bsSend.Write(m_pPlayers[line].dwID);
                    bsSend.Write(0); // 0 from scoreboard - 1 from ? - 2 from ?
                    pNetGame->GetRakClient()->RPC(&RPC_ClickPlayer, &bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
               
                    if(m_bToggle) m_bToggle = false;
                }

                m_iLastSelectedItem = line;
            }

            if(ImGui::IsItemHovered())
                m_iSelectedItem = line;

            if(is_selected) 
                ImGui::SetItemDefaultFocus();

            ImGui::SameLine(); 
        }

        sprintf(szPlayerId, "%d", m_pPlayers[line].dwID);
        sprintf(szScore, "%d", m_pPlayers[line].iScore);
        sprintf(szPing, "%d", m_pPlayers[line].dwPing);
        
        RGBcolors[0] = (m_pPlayers[line].dwColor - 0xFF000000) >> 16;
        RGBcolors[1] = ((m_pPlayers[line].dwColor - 0xFF000000) & 0x00ff00) >> 8;
        RGBcolors[2] = ((m_pPlayers[line].dwColor - 0xFF000000) & 0x0000ff);

        ImVec2 cursonPos;
		cursonPos = ImGui::GetCursorPos();

        ImGui::TextColored(ImColor(RGBcolors[0], RGBcolors[1], RGBcolors[2]), "%s", szPlayerId);
        ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * 10); ImGui::NextColumn();
        
        ImGui::TextColored(ImColor(RGBcolors[0], RGBcolors[1], RGBcolors[2]), "%s", m_pPlayers[line].szName);
        ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * 45); ImGui::NextColumn();
        
        ImGui::TextColored(ImColor(RGBcolors[0], RGBcolors[1], RGBcolors[2]), "%s", szScore);
        ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * 20); ImGui::NextColumn();
        
        ImGui::TextColored(ImColor(RGBcolors[0], RGBcolors[1], RGBcolors[2]), "%s", szPing);
        ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * 25); ImGui::NextColumn();
    }

    /*for(int x = m_iOffset; x < endplayer; x++)
    {
        sprintf(szPlayerId, "%d", m_pPlayers[x].dwID);
        sprintf(szScore, "%d", m_pPlayers[x].iScore);
        sprintf(szPing, "%d", m_pPlayers[x].dwPing);
        
        RGBcolors[0] = (m_pPlayers[x].dwColor - 0xFF000000) >> 16;
        RGBcolors[1] = ((m_pPlayers[x].dwColor - 0xFF000000) & 0x00ff00) >> 8;
        RGBcolors[2] = ((m_pPlayers[x].dwColor - 0xFF000000) & 0x0000ff);

        ImGui::TextColored(ImColor(RGBcolors[0], RGBcolors[1], RGBcolors[2]), "%s", szPlayerId);
        ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * 10); ImGui::NextColumn();
        
        ImGui::TextColored(ImColor(RGBcolors[0], RGBcolors[1], RGBcolors[2]), "%s", m_pPlayers[x].szName);
        ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * 45); ImGui::NextColumn();
        
        ImGui::TextColored(ImColor(RGBcolors[0], RGBcolors[1], RGBcolors[2]), "%s", szScore); 
        ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * 20); ImGui::NextColumn();
        
        ImGui::TextColored(ImColor(RGBcolors[0], RGBcolors[1], RGBcolors[2]), "%s", szPing);
        ImGui::SetColumnWidth(-1, m_fScoreBoardSizeX / 100 * 25); ImGui::NextColumn();
    }*/
    
	ImGui::EndColumns();

    ScrollWhenDraggingOnVoid();
	ImGui::EndChild();

    ImGui::End();
}

void CScoreBoard::Toggle()
{
    m_bToggle = !m_bToggle;
	if(!m_bToggle)
	{
        if(m_pPlayers)
        {
            memset(m_pPlayers, 0, m_pPlayerCount * sizeof(PLAYER_SCORE_INFO));
            free(m_pPlayers);
        }

        m_pPlayers = 0;
	}
}

void CScoreBoard::ProcessUpdating()
{
    if(pNetGame)
    {
        if((GetTickCount() - m_tickProcessingUpdate) >= 2000)
        {
            m_tickProcessingUpdate = GetTickCount();

            // Get player list
            pNetGame->UpdatePlayerScoresAndPings();

            CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
            PLAYERID playercount = pPlayerPool->GetCount(false) + 1;
            m_pPlayerCount = playercount;

            if(m_iOffset > (playercount - 20)) m_iOffset = (playercount - 20);
            if(m_iOffset < 0) m_iOffset = 0;

            m_pPlayers = (PLAYER_SCORE_INFO*)malloc(playercount * sizeof(PLAYER_SCORE_INFO));
            memset(m_pPlayers, 0, playercount * sizeof(PLAYER_SCORE_INFO));
            m_pPlayers[0].dwID = pPlayerPool->GetLocalPlayerID();
            m_pPlayers[0].szName = pPlayerPool->GetLocalPlayerName();
            m_pPlayers[0].iScore = pPlayerPool->GetLocalPlayerScore();
            m_pPlayers[0].dwPing = pPlayerPool->GetLocalPlayerPing();
            m_pPlayers[0].dwColor = pPlayerPool->GetLocalPlayer()->GetPlayerColorAsARGB();
            PLAYERID i = 1, x;
            for(x = 0; x < MAX_PLAYERS; x++)
            {
                if(!pPlayerPool->GetSlotState(x)) 
                    continue;

                CRemotePlayer* pPlayer = pPlayerPool->GetAt(i);
                if(pPlayer && pPlayer->IsNPC())
                    continue;

                m_pPlayers[i].dwID = x;
                m_pPlayers[i].szName = pPlayerPool->GetPlayerName(x);
                m_pPlayers[i].iScore = pPlayerPool->GetRemotePlayerScore(x);
                m_pPlayers[i].dwPing = pPlayerPool->GetRemotePlayerPing(x);
                m_pPlayers[i].dwColor = pPlayerPool->GetAt(x)->GetPlayerColorAsARGB();
                m_pPlayers[i].iState = (int)pPlayerPool->GetAt(x)->GetState();
                i++;
            }

            if(m_bSorted)
            {
                for(i = 0; i < playercount - 1; i++)
                {
                    for(PLAYERID j = 0; j < playercount - 1 - i; j++)
                    {
                        if(m_pPlayers[j + 1].iScore > m_pPlayers[j].iScore)
                        {
                            SwapPlayerInfo(&m_pPlayers[j], &m_pPlayers[j + 1]);
                        }
                    }
                }
            }
        }
    }
}