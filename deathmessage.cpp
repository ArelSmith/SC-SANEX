#include "main.h"
#include "game/game.h"
#include "game/util.h"
#include "net/netgame.h"
#include "deathmessage.h"
#include "gui/gui.h"

extern CGUI* pGUI;
extern CGame* pGame;

CDeathMessage::CDeathMessage()
{
	m_pDeathMessage.clear();
	Log("Deathmessage initialized.");
}

CDeathMessage::~CDeathMessage()
{
	m_pDeathMessage.clear();
}

void CDeathMessage::MakeRecord(const char* playername, unsigned int playercolor, const char* killername, unsigned int killercolor, uint8_t reason)
{
	if(!playername || !strlen(playername)) return;

	DeathMessageStruct* pPlayerKill = new DeathMessageStruct;
	pPlayerKill->playerName = playername;
	pPlayerKill->playercolor = playercolor;
	pPlayerKill->killerName = killername;
	pPlayerKill->killercolor = killercolor;
	pPlayerKill->reason = reason;

	if(m_pDeathMessage.size() >= 8)
		m_pDeathMessage.pop_front();

	m_pDeathMessage.push_back(pPlayerKill);
}

void CDeathMessage::Render()
{
	if(m_pDeathMessage.empty() == false)
	{
		float fPosSprite = RsGlobal->maximumWidth / 2 + pGUI->ScaleX(580.0f);
		
		ImVec2 vecPos;
		vecPos.y = RsGlobal->maximumHeight / 2 - pGUI->ScaleY(170.0f);

		for(auto& playerkill : m_pDeathMessage)
		{
			if(playerkill)
			{
				char szText[256], playerName[24], killerName[24];
				cp1251_to_utf8(playerName, playerkill->playerName.c_str());
				cp1251_to_utf8(killerName, playerkill->killerName.c_str());

				unsigned char PlayerColor[3];
				PlayerColor[0] = (playerkill->playercolor >> 8) & 0xFF;
				PlayerColor[1] = (playerkill->playercolor >> 16) & 0xFF;
				PlayerColor[2] = (playerkill->playercolor >> 24) & 0xFF;

				unsigned char KillerColor[3];
				KillerColor[0] = (playerkill->killercolor >> 8) & 0xFF;
				KillerColor[1] = (playerkill->killercolor >> 16) & 0xFF;
				KillerColor[2] = (playerkill->killercolor >> 24) & 0xFF;

				if(!playerkill->killerName.empty() && !playerkill->playerName.empty())
				{
					vecPos.x = (fPosSprite - ImGui::CalcTextSize(playerName).x) - ImGui::GetStyle().ItemSpacing.x;
					pGUI->RenderText(vecPos, ImColor(PlayerColor[0], PlayerColor[1], PlayerColor[3]), true, playerName);
					
					vecPos.x = fPosSprite;
					pGUI->RenderText(vecPos, 0xFF000000, false, "G", nullptr, 0.0f, pGUI->GetWeaponFont());
					pGUI->RenderText(vecPos, 0xFFFFFFFF, false, SpriteIDForWeapon(playerkill->reason), nullptr, 0.0f, pGUI->GetWeaponFont());

					vecPos.x = fPosSprite + pGUI->GetFontSize() + ImGui::GetStyle().ItemSpacing.x + ImGui::GetStyle().ItemSpacing.x;
					pGUI->RenderText(vecPos, ImColor(KillerColor[0], KillerColor[1], KillerColor[3]), true, killerName);
				}
				else if(playerkill->killerName.empty() && !playerkill->playerName.empty())
				{
					vecPos.x = (fPosSprite - ImGui::CalcTextSize(playerName).x) - ImGui::GetStyle().ItemSpacing.x;
					pGUI->RenderText(vecPos, ImColor(PlayerColor[0], PlayerColor[1], PlayerColor[3]), true, playerName);
					
					uint32_t dwColor = 0xFFFFFFFF;
					if(playerkill->reason == WEAPON_SPECIAL_CONNECT) dwColor = 0xFFFF0000;
					else if(playerkill->reason == WEAPON_SPECIAL_DISCONNECT) dwColor = 0xFF0000FF;

					vecPos.x = fPosSprite;
					pGUI->RenderText(vecPos, 0xFF000000, false, "G", nullptr, 0.0f, pGUI->GetWeaponFont());
					pGUI->RenderText(vecPos, dwColor, false, SpriteIDForWeapon(playerkill->reason), nullptr, 0.0f, pGUI->GetWeaponFont());
				}

				vecPos.y += pGUI->GetFontSize() + ImGui::GetStyle().ItemSpacing.y;
			}
		}
	}
}

const char* CDeathMessage::SpriteIDForWeapon(uint8_t byteWeaponID)
{	
	switch(byteWeaponID) 
	{
		case WEAPON_FIST:
			return "%";
		case WEAPON_BRASSKNUCKLE: 
			return "B"; 
		case WEAPON_GOLFCLUB: 
			return ">"; 
		case WEAPON_NITESTICK: 
			return "("; 
		case WEAPON_KNIFE: 
			return "C";
		case WEAPON_BAT: 
			return "?"; 
		case WEAPON_SHOVEL: 
			return "&"; 
		case WEAPON_POOLSTICK: 
			return "\""; 
		case WEAPON_KATANA: 
			return "!"; 
		case WEAPON_CHAINSAW: 
			return "1"; 
		case WEAPON_DILDO: 
			return "E"; 
		case WEAPON_DILDO2: 
			return "E"; 
		case WEAPON_VIBRATOR: 
			return "E"; 
		case WEAPON_VIBRATOR2: 
			return "E"; 
		case WEAPON_FLOWER: 
			return "$"; 
		case WEAPON_CANE: 
			return "#"; 
		case WEAPON_GRENADE: 
			return "@"; 
		case WEAPON_TEARGAS: 
			return "D"; 
		case WEAPON_COLT45: 
			return "6"; 
		case WEAPON_SILENCED: 
			return "2"; 
		case WEAPON_DEAGLE: 
			return "3"; 
		case WEAPON_SHOTGUN: 
			return "="; 
		case WEAPON_SAWEDOFF: 
			return "0"; 
		case WEAPON_SHOTGSPA:
			return "+"; 
		case WEAPON_UZI: 
			return "I"; 
		case WEAPON_MP5: 
			return "8"; 
		case WEAPON_AK47: 
			return "H"; 
		case WEAPON_M4: 
			return "5";
		case WEAPON_TEC9: 
			return "7";
		case WEAPON_RIFLE: 
			return "."; 
		case WEAPON_SNIPER: 
			return "A"; 
		case WEAPON_ROCKETLAUNCHER: 
			return "4"; 
		case WEAPON_HEATSEEKER: 
			return ")"; 
		case WEAPON_FLAMETHROWER: 
			return "P"; 
		case WEAPON_MINIGUN: 
			return "F"; 
		case WEAPON_SATCHEL: 
			return "<"; 
		case WEAPON_BOMB: 
			return ";"; 
		case WEAPON_SPRAYCAN: 
			return "/"; 
		case WEAPON_FIREEXTINGUISHER: 
			return ","; 
		case WEAPON_PARACHUTE: 
			return ":";
		case WEAPON_VEHICLE: 
			return "L";
		case WEAPON_DROWN: 
			return "J";
		case WEAPON_HELIBLADES:
			return "R";
		case WEAPON_EXPLOSION:
			return "Q";
		case WEAPON_COLLISION: 
			return "K";
		case WEAPON_SPECIAL_CONNECT:
			return "N";
		case WEAPON_SPECIAL_DISCONNECT:
			return "N";
	}
	return "J";
}