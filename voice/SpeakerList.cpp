#include "../main.h"
#include "../gui/gui.h"
#include "../game/common.h"
#include "../game/util.h"
#include "../game/game.h"
#include "../game/playerped.h"
#include "../net/netgame.h"

#include "SpeakerList.h"

#include "PluginConfig.h"

extern CGUI *pGUI;
extern CNetGame *pNetGame;

bool SpeakerList::Init() noexcept
{
    if(SpeakerList::initStatus)
        return false;

    try
    {
        SpeakerList::tSpeakerIcon = (RwTexture*)LoadTextureFromDB("samp", "speaker");
    }
    catch (const std::exception& exception)
    {
        LogVoice("[sv:err:speakerlist:init] : failed to create speaker icon");
        SpeakerList::tSpeakerIcon = nullptr;
        return false;
    }

    if(!PluginConfig::IsSpeakerLoaded())
    {
        PluginConfig::SetSpeakerLoaded(true);
    }

    SpeakerList::initStatus = true;

    return true;
}

void SpeakerList::Free() noexcept
{
    if(!SpeakerList::initStatus)
        return;

    SpeakerList::tSpeakerIcon = nullptr;

    SpeakerList::initStatus = false;
}

void SpeakerList::Show() noexcept
{
    SpeakerList::showStatus = true;
}

bool SpeakerList::IsShowed() noexcept
{
    return SpeakerList::showStatus;
}

void SpeakerList::Hide() noexcept
{
    SpeakerList::showStatus = false;
}

void SpeakerList::Render()
{
    if(!SpeakerList::initStatus || !SpeakerList::IsShowed())
        return;

    if(!pNetGame) return;

    CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
    if(!pPlayerPool) return;

    int curTextLine;
    char szText[64], szText2[64];
    ImVec2 textPos = ImVec2(pGUI->ScaleX(24), pGUI->ScaleY(480));

    for(PLAYERID playerId { 0 }; playerId < MAX_PLAYERS; ++playerId)
    {
        CRemotePlayer* pPlayer = pPlayerPool->GetAt(playerId);
        if(pPlayer && pPlayer->IsActive())
        {
            CPlayerPed* pPlayerPed = pPlayer->GetPlayerPed();
            if(pPlayerPed)
            {
                if(const auto playerName = pPlayerPool->GetPlayerName(playerId); playerName)
                {
                    if(!SpeakerList::playerStreams[playerId].empty())
                    {
                        for(const auto& playerStream : SpeakerList::playerStreams[playerId])
                        {
                            if(playerStream.second.GetType() == StreamType::LocalStreamAtPlayer)
                            {
                                VECTOR VecPos;

                                if(!pPlayerPed->IsAdded()) continue;
                                VecPos.X = 0.0f;
                                VecPos.Y = 0.0f;
                                VecPos.Z = 0.0f;
                                pPlayerPed->GetBonePosition(8, &VecPos);

                                if(pPlayerPed->GetDistanceFromLocalPlayerPed() < 40.0f)
                                    SpeakerList::Draw(&VecPos, pPlayerPed->GetDistanceFromCamera());
                            }
                        }

                        if(curTextLine < 8)
                        {
                            ImVec2 a = ImVec2(textPos.x, textPos.y);
                            ImVec2 b = ImVec2(textPos.x + pGUI->GetFontSize(), textPos.y + pGUI->GetFontSize());
                            ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)SpeakerList::tSpeakerIcon->raster, a, b);

                            float fSpaceFromIconToText = pGUI->GetFontSize() + ImGui::GetStyle().ItemSpacing.x;

                            textPos.x += fSpaceFromIconToText;
                            sprintf(szText, "%s (%d) ", playerName, playerId);
                            pGUI->RenderText(textPos, 0xFFFFFFFF, false, szText);

                            for(const auto& streamInfo : SpeakerList::playerStreams[playerId])
                            {
                                if(streamInfo.second.GetColor() == NULL)
                                    continue;

                                textPos.x += ImGui::CalcTextSize(szText).x;
                                sprintf(szText2, "[%s]", streamInfo.second.GetName().c_str());
                                pGUI->RenderText(textPos, streamInfo.second.GetColor(), false, szText2);
                            }

                            textPos.x -= fSpaceFromIconToText;
                            textPos.x -= ImGui::CalcTextSize(szText).x;
                            textPos.y += pGUI->GetFontSize();

                            curTextLine++;
                        }
                    }
                }
            }
        }
    }
}

void SpeakerList::Draw(VECTOR* vec, float fDist)
{
	VECTOR TagPos;

	TagPos.X = vec->X;
	TagPos.Y = vec->Y;
	TagPos.Z = vec->Z;
	TagPos.Z += 0.25f + (fDist * 0.0475f);

	VECTOR Out;
	// CSprite::CalcScreenCoors
	(( void (*)(VECTOR*, VECTOR*, float*, float*, bool, bool))(g_libGTASA+0x54EEC0+1))(&TagPos, &Out, 0, 0, 0, 0);

	if(Out.Z < 1.0f)
		return;

    ImVec2 pos = ImVec2(Out.X, Out.Y);
	pos.x -= PluginConfig::kDefValSpeakerIconSize / 2;
    pos.y -= pGUI->GetFontSize();

    ImVec2 a = ImVec2(pos.x, pos.y);
    ImVec2 b = ImVec2(pos.x + PluginConfig::kDefValSpeakerIconSize, pos.y + PluginConfig::kDefValSpeakerIconSize);
    ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)SpeakerList::tSpeakerIcon->raster, a, b);
}

void SpeakerList::OnSpeakerPlay(const Stream& stream, const uint16_t speaker) noexcept
{
    uint16_t wSpeaker = speaker;
    if(speaker < 0) wSpeaker = 0;
    else if(speaker > MAX_PLAYERS - 1) wSpeaker = MAX_PLAYERS - 1;
    if(speaker != wSpeaker) return;

    SpeakerList::playerStreams[speaker][(Stream*)(&stream)] = stream.GetInfo();
}

void SpeakerList::OnSpeakerStop(const Stream& stream, const uint16_t speaker) noexcept
{
    uint16_t wSpeaker = speaker;
    if(speaker < 0) wSpeaker = 0;
    else if(speaker > MAX_PLAYERS - 1) wSpeaker = MAX_PLAYERS - 1;
    if(speaker != wSpeaker) return;

    SpeakerList::playerStreams[speaker].erase((Stream*)(&stream));
}

std::array<std::unordered_map<Stream*, StreamInfo>, MAX_PLAYERS> SpeakerList::playerStreams;

bool SpeakerList::initStatus { false };
bool SpeakerList::showStatus { false };

RwTexture* SpeakerList::tSpeakerIcon { nullptr };