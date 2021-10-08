#include "main.h"
#include "gui/gui.h"
#include "game/game.h"
#include "net/netgame.h"
#include "PassengerButton.h"

extern CGUI * pGUI;
extern CGame* pGame;
extern CNetGame* pNetGame;

CPassengerButton::CPassengerButton() { }
CPassengerButton::~CPassengerButton() { }

void CPassengerButton::Render()
{
	passengerButton = (RwTexture*)LoadTextureFromDB("samp", "g");
	if(!pNetGame)
		return;
	
	ImGuiIO &io = ImGui::GetIO();
	
	ImGui::PushStyleColor(
		ImGuiCol_Button,
		(ImVec4)ImColor(0x00, 0x00,
		0x00, 0x00).Value
	);
	ImGui::PushStyleColor(
		ImGuiCol_ButtonHovered,
		(ImVec4)ImColor(0x00, 0x00,
		0x00, 0x00).Value
	);
	ImGui::PushStyleColor(
		ImGuiCol_ButtonActive,
		(ImVec4)ImColor(0x00, 0x00,
		0x00, 0x00).Value
	);
	
	ImGuiStyle style;
	style.FrameBorderSize = ImGui::GetStyle().FrameBorderSize;
	ImGui::GetStyle().FrameBorderSize = 0.0f;
	
	ImGui::Begin("Passenger", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoSavedSettings
	);
	
	ImVec2 vecButSize_g = ImVec2(
		ImGui::GetFontSize() * 5+5.0f,
		ImGui::GetFontSize() * 5);
	
	CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
	if(!pPlayerPed->IsInVehicle() && !pPlayerPed->IsAPassenger())
	{
		CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
		if(pVehiclePool)
		{
			uint16_t sNearestVehicleID = pVehiclePool->FindNearestToLocalPlayerPed();

			CVehicle *pVehicle = pVehiclePool->GetAt(sNearestVehicleID);
			if(pVehicle)
			{
				if(pVehicle->GetDistanceFromLocalPlayerPed() < 4.0f)
				{
					CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
					if(pPlayerPool)
					{
						CLocalPlayer *pLocalPlayer = pPlayerPool->GetLocalPlayer();
						if(pLocalPlayer)
						{
							if(!pLocalPlayer->IsSpectating())
							{
								if(ImGui::ImageButton((ImTextureID)passengerButton->raster, vecButSize_g))
								{
									pPlayerPed->EnterVehicle(pVehicle->m_dwGTAId, true);
									pLocalPlayer->SendEnterVehicleNotification(sNearestVehicleID, true);
								}
							}
						}
					}
				}
			}
		}
	}
	
	ImGui::SetWindowSize(ImVec2(-1, -1));
	
	ImGui::SetWindowPos(ImVec2(pGUI->ScaleX(1400), pGUI->ScaleY(365)));
	ImGui::End();
	
	ImGui::PopStyleColor(3);
	ImGui::GetStyle().FrameBorderSize = style.FrameBorderSize;
}
