#include "main.h"
#include "gui/gui.h"
#include "game/game.h"
#include "net/netgame.h"
#include "settings.h"
#include "dialog.h"
#include "spawnscreen.h"
#include "extrakeyboard.h"
#include "scoreboard.h"
//#include "settings.h"

extern CGUI *pGUI;
extern CGame *pGame;
extern CNetGame *pNetGame;
extern CSettings *pSettings;
extern CScoreBoard *pScoreBoard;
extern CDialogWindow *pDialogWindow;
extern CSpawnScreen *pSpawnScreen;
//extern CSettings *pSettings;

CExtraKeyBoard::CExtraKeyBoard() 
{
	m_bIsActive = false;
	m_bIsExtraShow = false;
/*	if(pSettings->Get().bPassengerUseTexture)
	{
		m_passengerButtonTexture[0] = (RwTexture*)LoadTextureFromDB(cryptor::create("samp").decrypt(), cryptor::create("passengertexture").decrypt()); // passenger button
		m_passengerButtonTexture[1] = (RwTexture*)LoadTextureFromDB(cryptor::create("samp").decrypt(), cryptor::create("passengertexturehover").decrypt()); // passenger button hover
		m_dwLastTickPassengerHover = GetTickCount();
	}*/
	Log("Extrakeyboard initialized.");
}

CExtraKeyBoard::~CExtraKeyBoard() 
{ 
/*	m_bIsActive = false;
	m_bIsExtraShow = false;
	if(pSettings->Get().bPassengerUseTexture)
	{
		for(int i = 0; i < 2; i++)
			m_passengerButtonTexture[i] = nullptr;

		m_dwLastTickPassengerHover = GetTickCount();
	}*/
}

void CExtraKeyBoard::Show(bool bShow) 
{
	m_bIsActive = bShow;
}

void CExtraKeyBoard::Render() 
{
	if(!m_bIsActive || pSpawnScreen->m_bEnabled)
		return;
	
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.5f, 6.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

	bool bDontExpand = false;
	if(pDialogWindow->m_bIsActive)
		bDontExpand = true;
	else if(pScoreBoard->m_bToggle)
		bDontExpand = true;

	CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
	if(pPlayerPed) 
	{
		ImGuiIO &io = ImGui::GetIO();

		ImGui::Begin("Extrakeyboard", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);

		float m_fButWidth = ImGui::CalcTextSize("QWERTY").x;
		float m_fButHeight = ImGui::CalcTextSize("QWE").x;

		ImGui::SameLine(0, 30);
		if(ImGui::Button(m_bIsExtraShow ? "HIDE" : "SHOW", ImVec2(m_fButWidth, m_fButHeight)))
		{
			if(!pScoreBoard->m_bToggle)
				m_bIsExtraShow = !m_bIsExtraShow;
		}
		if(m_bIsExtraShow) 
		{
			ImGui::SameLine(0, 25);
			if(ImGui::Button("TAB", ImVec2(m_fButWidth, m_fButHeight)))
			{
					//Close dialog, scoreboard, textdraw
				pScoreBoard->Toggle();
			}
			ImGui::SameLine(0, 25);
			if(pPlayerPed->IsInVehicle()) 
			{
				if(ImGui::Button("ALT", ImVec2(m_fButWidth, m_fButHeight)))
					LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE] = true;	
			} 
			else 
			{
				if(ImGui::Button("ALT", ImVec2(m_fButWidth, m_fButHeight)))
					LocalPlayerKeys.bKeys[ePadKeys::KEY_WALK] = true;
			}
			ImGui::SameLine(0, 25);
			{
				if(ImGui::Button("H", ImVec2(m_fButWidth, m_fButHeight)))
					LocalPlayerKeys.bKeys[ePadKeys::KEY_CTRL_BACK] = true;
			}
		    ImGui::SameLine(0, 25);
			{
				if(ImGui::Button("SPACE", ImVec2(m_fButWidth, m_fButHeight)))
				   LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = true;
				else
				LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP] = true;
			}
			ImGui::SameLine(0, 25);
			{
			if(ImGui::Button("F", ImVec2(m_fButWidth, m_fButHeight)))
				LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] = true;
			}
			ImGui::SameLine(0, 25);
			{
				if(ImGui::Button("Y", ImVec2(m_fButWidth, m_fButHeight)))
					LocalPlayerKeys.bKeys[ePadKeys::KEY_YES] = true;
			}
			ImGui::SameLine(0, 25);
			{
				if(ImGui::Button("N", ImVec2(m_fButWidth, m_fButHeight)))
					LocalPlayerKeys.bKeys[ePadKeys::KEY_NO] = true;
			}
		}
		ImGui::SetWindowSize(ImVec2(-1, -1));
		ImVec2 size = ImGui::GetWindowSize();
		ImGui::SetWindowPos(ImVec2(pGUI->ScaleX(10), pGUI->ScaleY(365)));
		ImGui::End();
			
		ImGui::PopStyleVar(2);
	
	}
	return;
}
