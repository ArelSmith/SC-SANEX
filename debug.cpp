#include "main.h"
#include "game/game.h"
#include "settings.h"
#include "gui/gui.h"
#include "debug.h"
#include "util/armhook.h"

extern CGame *pGame;
extern CSettings *pSettings;

CDebug::CDebug()
{
	m_dwLastTick = GetTickCount();
}

CDebug::~CDebug() {}

void CDebug::AddMessage(char* msg)
{
	std::string str(msg);
	m_Messages.push_back(str);
}

void CDebug::Render()
{
	ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoSavedSettings);
	for(auto str : m_Messages)
	{
		ImGui::Text("%s", str.c_str());
	}

	ImGui::End();
}

void CDebug::Process() {}

void CDebug::SpawnLocalPlayer()
{
	Log("CDebug: SpawnLocalPlayer");
	CCamera *pGameCamera = pGame->GetCamera();
	pGameCamera->Restore();
	pGameCamera->SetBehindPlayer();

	pGame->SetWorldTime(12, 0);
}