#include "../main.h"
#include "gui.h"
#include "../game/game.h"
#include "../net/netgame.h"
#include "../game/RW/RenderWare.h"
#include "../chatwindow.h"
#include "../spawnscreen.h"
#include "../playertags.h"
#include "../dialog.h"
#include "../extrakeyboard.h"
#include "../keyboard.h"
#include "../debug.h"
#include "../settings.h"
#include "../scoreboard.h"
#include "../deathmessage.h"
#include "../passengerbutton.h"

// voice
#include "../voice/MicroIcon.h"
#include "../voice/SpeakerList.h"
#include "../voice/include/util/Render.h"

#include <time.h>
#include <ctime>
#include <stdio.h>
#include <string.h>

extern CExtraKeyBoard *pExtraKeyBoard;
extern CSpawnScreen *pSpawnScreen;
extern CPlayerTags *pPlayerTags;
extern CDialogWindow *pDialogWindow;
extern CDebug *pDebug;
extern CChatWindow *pChatWindow;
extern CSettings *pSettings;
extern CKeyBoard *pKeyBoard;
extern CNetGame *pNetGame;
extern CGame *pGame;
extern CScoreBoard *pScoreBoard;
extern CDeathMessage *pDeathMessage;
extern CPassengerButton *pPassengerButton;

/* imgui_impl_renderware.h */
void ImGui_ImplRenderWare_RenderDrawData(ImDrawData* draw_data);
bool ImGui_ImplRenderWare_Init();
void ImGui_ImplRenderWare_NewFrame();

/*
	??? ?????????? GUI-????????? ????????
	???????????? ?????????? 1920x1080
*/
#define MULT_X					0.00052083333f	// 1/1920
#define MULT_Y					0.00092592592f 	// 1/1080

//Yellow 						ImVec4(0.96f, 0.56f, 0.19f, 1.0f)
//Dark Red 						ImVec4(0.7f, 0.12f, 0.12f, 1.0f)

#define PRIMARY_COLOR 			ImVec4(0.96f, 0.56f, 0.19f, 1.0f)
#define SECONDARY_COLOR 		ImVec4(0.47f, 0.22f, 0.22f, 0.65f)


CGUI::CGUI()
{
	// setup ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();

	ImGui_ImplRenderWare_Init();

	// scale
	m_vecScale.x = io.DisplaySize.x * MULT_X;
	m_vecScale.y = io.DisplaySize.y * MULT_Y;

	// font Size
	m_fFontSize = ScaleY(pSettings->Get().fFontSize);

	// mouse/touch
	m_bMousePressed = false;
	m_vecMousePos = ImVec2(0, 0);

	Log("GUI | Scale factor: %f, %f Font size: %f", m_vecScale.x, m_vecScale.y, m_fFontSize);

	// setup style
	ImGuiStyle& style = ImGui::GetStyle();
	ImGui::StyleColorsDark();
	
	style.ScrollbarSize = ScaleY(45.0f);
	style.WindowPadding = ImVec2(4, 4);
	style.WindowBorderSize = 0.0f;
	style.ChildBorderSize = 0.0f;
	style.FrameBorderSize = 3.0f;
	style.WindowRounding = 0.75f;
	style.FrameRounding = 0.75f;
	style.ChildRounding = 0.75f;
	
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]  = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.13f, 0.13f, 0.75f);
    
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.79f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);
	
	style.Colors[ImGuiCol_Button] = ImVec4(0.13f, 0.13f, 0.13f, 0.75f);
	style.Colors[ImGuiCol_ButtonHovered] = PRIMARY_COLOR;
	style.Colors[ImGuiCol_ButtonActive] = PRIMARY_COLOR;
	
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = PRIMARY_COLOR;
	style.Colors[ImGuiCol_ScrollbarGrabActive] = PRIMARY_COLOR;
	
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

	style.Colors[ImGuiCol_HeaderActive] = PRIMARY_COLOR;
	style.Colors[ImGuiCol_Header] = PRIMARY_COLOR;
	style.Colors[ImGuiCol_HeaderHovered] = PRIMARY_COLOR;

	style.Colors[ImGuiCol_Border] = PRIMARY_COLOR;
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.85f);

	Log("GUI | Loading font: Arial.ttf");
	m_pFont = LoadFont("Arial.ttf");
	Log("GUI | ImFont pointer = 0x%X", m_pFont);

	Log("GUI | Loading font: gtaweap3.ttf", pSettings->Get().szFont);
	m_pFontGTAWeap = LoadFont("gtaweap3.ttf");
	Log("GUI | ImFont pointer = 0x%X", m_pFontGTAWeap);

	// voice
	for(const auto& deviceInitCallback : Render::deviceInitCallbacks)
    {
        if(deviceInitCallback != nullptr) 
			deviceInitCallback();
    }
    
	bShowDebugLabels = false;
	Log("GUI Initialized.");
}

CGUI::~CGUI()
{
	// voice 
	for(const auto& deviceFreeCallback : Render::deviceFreeCallbacks)
	{
		if(deviceFreeCallback != nullptr) 
			deviceFreeCallback();
	}

//	ImGui_ImplRenderWare_ShutDown();
	ImGui::DestroyContext();
}

ImFont* CGUI::LoadFont(char *font, float fontsize)
{
	ImGuiIO &io = ImGui::GetIO();

	// load fonts
	char path[0xFF];
	sprintf(path, "%sSAMP/fonts/%s", g_pszStorage, font);
	
	// cp1251 ranges
	static const ImWchar ranges[] =
	{
		0x0020, 0x0080,
		0x00A0, 0x00C0,
		0x0400, 0x0460,
		0x0490, 0x04A0,
		0x2010, 0x2040,
		0x20A0, 0x20B0,
		0x2110, 0x2130,
		0
	};

	ImFont* pFont = io.Fonts->AddFontFromFileTTF(path, fontsize == 0.0 ? GetFontSize() : fontsize, nullptr, ranges);
	return pFont;
}

void CGUI::Render()
{
	//if(pNetGame && pNetGame->GetTextDrawPool()) pNetGame->GetTextDrawPool()->Draw();

	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplRenderWare_NewFrame();
	ImGui::NewFrame();

	RenderVersion();
	RenderRakNetStatistics();
	
	// voice
	if(pNetGame)
	{
		if(pDialogWindow->m_bIsActive || pScoreBoard->m_bToggle)
		{
			SpeakerList::Hide();
			MicroIcon::Hide();
		}
		else 
		{
			if(MicroIcon::hasShowed)
			{
				SpeakerList::Show();
				MicroIcon::Show();
			}
		}

		for(const auto& renderCallback : Render::renderCallbacks)
		{
			if(renderCallback != nullptr) 
				renderCallback();
		}
	}

	if(pDebug) pDebug->Render();
	if(pPlayerTags) pPlayerTags->Render();
	if(pNetGame && pNetGame->GetLabelPool()) pNetGame->GetLabelPool()->Render();
	if(pNetGame && pNetGame->GetChatBubblePool()) pNetGame->GetChatBubblePool()->Render();
	if(pChatWindow) pChatWindow->Render();
	if(pDialogWindow) pDialogWindow->Render();
	if(pPassengerButton) pPassengerButton->Render();
	if(pSpawnScreen) pSpawnScreen->Render();
	if(pKeyBoard) pKeyBoard->Render();
	if(pExtraKeyBoard) pExtraKeyBoard->Render();
	if(pScoreBoard) pScoreBoard->Render();
	if(pDeathMessage) pDeathMessage->Render();

	if(pNetGame)
	{
		CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
		if(pPlayerPed)
		{
			PED_TYPE *pActor = pPlayerPed->GetGtaActor();
			if(pActor)
			{
				char szCoordText[128];
				sprintf(szCoordText, "X: %.4f - Y: %.4f - Z: %.4f", pActor->entity.mat->pos.X, pActor->entity.mat->pos.Y, pActor->entity.mat->pos.Z);
				ImVec2 _ImVec2 = ImVec2(ScaleX(1350), RsGlobal->maximumHeight - ImGui::GetFontSize() * 0.85);
				RenderText(_ImVec2, ImColor(0xFF12F0FF), false, szCoordText, nullptr, ImGui::GetFontSize() * 0.85);
			}
		}
	}

	// Debug label
	if(bShowDebugLabels)
	{
		if(pNetGame)
		{
			CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
			if(pVehiclePool)
			{
				for(VEHICLEID x = 0; x < MAX_VEHICLES; x++)
				{
					CVehicle *pVehicle = pVehiclePool->GetAt(x);
					if(pVehicle)
					{
						if(pVehicle->GetDistanceFromLocalPlayerPed() <= 20.0f)
						{
							MATRIX4X4 matVehicle;
							pVehicle->GetMatrix(&matVehicle);

							RwV3d rwPosition;
							rwPosition.x = matVehicle.pos.X;
							rwPosition.y = matVehicle.pos.Y;
							rwPosition.z = matVehicle.pos.Z;

							RwV3d rwOutResult;

							// CSPrite::CalcScreenCoors(RwV3d const&, RwV3d *, float *, float *, bool, bool) - 0x54EEC0
							((void (*)(RwV3d const&, RwV3d *, float *, float *, bool, bool))(g_libGTASA + 0x54EEC0 + 1))(rwPosition, &rwOutResult, 0, 0, 0, 0);
							if(rwOutResult.z < 1.0f)
								break;

							char szTextLabel[256];
							sprintf(szTextLabel, "[id: %d | model: %d | subtype: %d | Health: %.1f | preloaded: %d]\nDistance: %.2fm\ncPos: %.3f, %.3f, %.3f\nsPos: %.3f, %.3f, %.3f",
								x, pVehicle->GetModelIndex(), pVehicle->GetVehicleSubtype(), 
								pVehicle->GetHealth(), pGame->m_bIsVehiclePreloaded[pVehicle->GetModelIndex()],
								pVehicle->GetDistanceFromLocalPlayerPed(),
								matVehicle.pos.X, matVehicle.pos.Y, matVehicle.pos.Z,
								pVehiclePool->m_vecSpawnPos[x].X, pVehiclePool->m_vecSpawnPos[x].Y, pVehiclePool->m_vecSpawnPos[x].Z 
							);

							ImVec2 vecRealPos = ImVec2(rwOutResult.x, rwOutResult.y);
							Render3DLabel(vecRealPos, szTextLabel, 0x358BD4FF);
						}
					}
				}
			}
		}
	}
	
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplRenderWare_RenderDrawData(ImGui::GetDrawData());

	if(m_bNeedClearMousePos)
	{
		io.MousePos = ImVec2(-1, -1);
		m_bNeedClearMousePos = false;
	}
}

bool CGUI::OnTouchEvent(int type, bool multi, int x, int y)
{
	ImGuiIO& io = ImGui::GetIO();

	if(!pKeyBoard->OnTouchEvent(type, multi, x, y)) return false;
	if(!pChatWindow->OnTouchEvent(type, multi, x, y)) return false;

	if(pNetGame && pNetGame->GetTextDrawPool()) pNetGame->GetTextDrawPool()->OnTouchEvent(type, multi, x, y);

	switch(type)
	{
		case TOUCH_PUSH:
		io.MousePos = ImVec2(x, y);
		io.MouseDown[0] = true;
		break;

		case TOUCH_POP:
		io.MouseDown[0] = false;
		m_bNeedClearMousePos = true;
		break;

		case TOUCH_MOVE:
		io.MousePos = ImVec2(x, y);
		break;
	}

	return true;
}

void CGUI::RenderVersion()
{
	ImVec2 _ImVec2 = ImVec2(ScaleX(10), ScaleY(5));
	RenderText(_ImVec2, ImColor(255, 255, 255), true, "LLRP Client", nullptr, ImGui::GetFontSize() * 0.85);
}

void CGUI::RenderRakNetStatistics()
{
		// StatisticsToString(rss, message, 0);

		//ImGui::GetOverlayDrawList()->AddText(
		//	ImVec2(ScaleX(10), ScaleY(400)),
		//	ImColor(IM_COL32_BLACK), message);
}

void CGUI::RenderText(ImVec2& posCur, ImU32 col, bool bOutline, const char* text_begin, const char* text_end, float font_size, ImFont *font, bool bOutlineUseTextColor)
{
	int iOffset = bOutlineUseTextColor ? 1 : pSettings->Get().iFontOutline;
	if(bOutline)
	{
		// left
		posCur.x -= iOffset;
		ImGui::GetBackgroundDrawList()->AddText(font, font_size == 0.0f ? GetFontSize() : font_size, posCur, bOutlineUseTextColor ? ImColor(col) : ImColor(IM_COL32_BLACK), text_begin, text_end);
		posCur.x += iOffset;
		// right
		posCur.x += iOffset;
		ImGui::GetBackgroundDrawList()->AddText(font, font_size == 0.0f ? GetFontSize() : font_size, posCur, bOutlineUseTextColor ? ImColor(col) : ImColor(IM_COL32_BLACK), text_begin, text_end);
		posCur.x -= iOffset;
		// above
		posCur.y -= iOffset;
		ImGui::GetBackgroundDrawList()->AddText(font, font_size == 0.0f ? GetFontSize() : font_size, posCur, bOutlineUseTextColor ? ImColor(col) : ImColor(IM_COL32_BLACK), text_begin, text_end);
		posCur.y += iOffset;
		// below
		posCur.y += iOffset;
		ImGui::GetBackgroundDrawList()->AddText(font, font_size == 0.0f ? GetFontSize() : font_size, posCur, bOutlineUseTextColor ? ImColor(col) : ImColor(IM_COL32_BLACK), text_begin, text_end);
		posCur.y -= iOffset;
	}

	ImGui::GetBackgroundDrawList()->AddText(font, font_size == 0.0f ? GetFontSize() : font_size, posCur, col, text_begin, text_end);
}
