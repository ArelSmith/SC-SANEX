﻿#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "gui/gui.h"
#include "dialog.h"
#include "vendor/imgui/imgui_internal.h"
#include "keyboard.h"

extern CGUI *pGUI;
extern CGame *pGame;
extern CNetGame *pNetGame;
extern CKeyBoard *pKeyBoard;

char szDialogInputBuffer[100];
char utf8DialogInputBuffer[100*3];

CDialogWindow::CDialogWindow()
{
	m_bIsActive = false;
	m_putf8Info = nullptr;
	m_pszInfo = nullptr;
}

CDialogWindow::~CDialogWindow()
{

}

void CDialogWindow::Show(bool bShow)
{
	if(pGame)
		pGame->FindPlayerPed()->TogglePlayerControllable(!bShow);

	m_bIsActive = bShow;
	if (bShow) {
		if (m_byteDialogStyle == DIALOG_STYLE_TABLIST_HEADERS)
		{
			m_iSelectedItem = 1;
		}
		else
		{
			m_iSelectedItem = 0;
		}
	}
}

void CDialogWindow::Clear()
{
	if(m_putf8Info)
	{
		free(m_putf8Info);
		m_putf8Info = nullptr;
	}

	if(m_pszInfo)
	{
		free(m_pszInfo);
		m_pszInfo = nullptr;
	}

	m_bIsActive = false;
	m_byteDialogStyle = 0;
	m_wDialogID = -1;
	m_utf8Title[0] = 0;
	m_utf8Button1[0] = 0;
	m_utf8Button2[0] = 0;

	m_fSizeX = -1.0f;
	m_fSizeY = -1.0f;

	memset(szDialogInputBuffer, 0, 100);
	memset(utf8DialogInputBuffer, 0, 100*3);
}

void CDialogWindow::SetInfo(char* szInfo, int length)
{
	if(!szInfo || !length) return;

	if(m_putf8Info)
	{
		free(m_putf8Info);
		m_putf8Info = nullptr;
	}

	if(m_pszInfo)
	{
		free(m_pszInfo);
		m_pszInfo = nullptr;
	}

	m_putf8Info = (char*)malloc((length * 3) + 1);
	if(!m_putf8Info)
	{
		Log("CDialog::SetInfo: can't allocate memory");
		return;
	}

	m_pszInfo = (char*)malloc((length * 3) + 1);
	if(!m_pszInfo)
	{
		Log("CDialog::SetInfo: can't allocate memory");
		return;
	}

	cp1251_to_utf8(m_putf8Info, szInfo);

	// =========
	char szNonColorEmbeddedMsg[4200];
	int iNonColorEmbeddedMsgLen = 0;

	for (size_t pos = 0; pos < strlen(szInfo) && szInfo[pos] != '\0'; pos++)
	{
		if(pos+7 < strlen(szInfo))
		{
			if (szInfo[pos] == '{' && szInfo[pos+7] == '}')
			{
				pos += 7;
				continue;
			}
		}

		szNonColorEmbeddedMsg[iNonColorEmbeddedMsgLen] = szInfo[pos];
		iNonColorEmbeddedMsgLen++;
	}

	szNonColorEmbeddedMsg[iNonColorEmbeddedMsgLen] = 0;
	// ========

	cp1251_to_utf8(m_pszInfo, szNonColorEmbeddedMsg);
}

void DialogWindowInputHandler(const char* str)
{
	if(!str || *str == '\0') return;
	strcpy(szDialogInputBuffer, str);
	cp1251_to_utf8(utf8DialogInputBuffer, str);
}

void CDialogWindow::RenderTabList(int dStyle)
{
	std::string strUtf8 = m_putf8Info;
	int iTabsCount = 0;
	std::vector<std::string> vLines;
	std::vector<std::string> firstTabs;
	ImVec2 fTabSize[4];

	std::stringstream sLine(strUtf8);
	std::string s_tmpLine;
	int tmplineid;
	ImGui::Begin(" ", nullptr ,
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.00f,0.00f,0.00f,0.90f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0.70f,0.00f,0.00f,0.10f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImVec4)ImColor(0.00f,0.00f,0.00f,0.80f));

	ImVec2 vecWinSize;
    ImGui::GetStyle().WindowPadding = ImVec2(8, 8);
	float buttonFactor = pGUI->ScaleX(187.5f)*2+pGUI->GetFontSize();

	while(std::getline(sLine, s_tmpLine, '\n'))
	{
		vLines.push_back(s_tmpLine);
		int tmpTabId = 0;
		if(dStyle != DIALOG_STYLE_LIST)
		{
			std::stringstream sTabLine(s_tmpLine);
			std::string tmpTabLine;
			while(std::getline(sTabLine, tmpTabLine, '\t'))
			{
				if(tmpTabId > 4) continue;
				if(vLines.size() == 1 && iTabsCount <= 4)
					iTabsCount++;
				ImVec2 tabSize;
			//	tabSize = CalcTextSizeWithoutTags((char*)tmpTabLine.c_str());

				if(tmpTabId == 0)
					firstTabs.push_back(removeColorTags(tmpTabLine));


				/*if(tabSize.x > fTabSize[tmpTabId].x)
				{
					fTabSize[tmpTabId].x = tabSize.x;
				}
				tmpTabId++;*/

			}
		}
		else
		{
			ImVec2 tabSize;
		//	tabSize = CalcTextSizeWithoutTags((char*)s_tmpLine.c_str());

			if(tmpTabId == 0)
				firstTabs.push_back(s_tmpLine);

			if(tabSize.x > fTabSize[tmpTabId].x)
			{
				fTabSize[tmpTabId].x = tabSize.x;
			}
		}
		tmplineid++;
	}

	ImGuiIO &io = ImGui::GetIO();
	int x = io.DisplaySize.x, y = io.DisplaySize.y;


	if(dStyle != DIALOG_STYLE_LIST)
	{
		for(uint8_t i = 0; i < iTabsCount; i++)
		{
			vecWinSize.x += fTabSize[i].x*( i == iTabsCount-1 ? 1.25 : 1.5 );
		}
	}
	else {
		vecWinSize.x += fTabSize[0].x * 1.25;
	}

	if(buttonFactor > vecWinSize.x)
	{
		vecWinSize.x = 0.0f;

		if(dStyle != DIALOG_STYLE_LIST)
		{
			buttonFactor /= iTabsCount;
			for(uint8_t i = 0; i < iTabsCount; i++)
			{
				vecWinSize.x += fTabSize[i].x + buttonFactor;
			}
		}
		else vecWinSize.x += fTabSize[0].x + buttonFactor;
	}
	else {
		buttonFactor = 0;
	}

	vecWinSize.y = 720;

	if(vLines.size() > 7) {
		vecWinSize.x += ImGui::GetStyle().ScrollbarSize;
		ImGui::SetNextWindowSize(ImVec2(pGUI->ScaleX(vecWinSize.x), pGUI->ScaleY(vecWinSize.y)), 0.9f);
	}


	TextWithColors(m_utf8Title);
	ImGui::ItemSize( ImVec2(0, pGUI->GetFontSize()/2 + 2.75) );

	ImVec2 cursonPosition;
	cursonPosition = ImGui::GetCursorPos();

	if(dStyle == DIALOG_STYLE_TABLIST_HEADERS)
	{
		ImGui::Columns(iTabsCount, "###tablistHeader", false);
		for(uint16_t i = 0; i < iTabsCount; i++)
		{
			Log("crash here?");
			ImGui::SetColumnWidth(-1, fTabSize[i].x*( i == iTabsCount-1 ? 1.25 : 1.5 )+buttonFactor );
			Log("or not?");
			ImGui::NextColumn();
		}

		std::stringstream sTabLine(vLines[0]);
		std::string tmpTabLine;
		int tmpTabId = 0;
		while(std::getline(sTabLine, tmpTabLine, '\t'))
		{
			if(tmpTabId > iTabsCount) continue;
			tmpTabLine.insert(0, "{a9c4e4}");
			if(tmpTabId == 0)
			{
				cursonPosition = ImGui::GetCursorPos();
				ImGui::SetCursorPosX(cursonPosition.x+pGUI->GetFontSize()/3);
			}
			TextWithColors(tmpTabLine.c_str());
			ImGui::NextColumn();
			tmpTabId++;

		}
		ImGui::Columns(1);
	}

	ImGui::BeginChild("###tablist", ImVec2(700,350), ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	if(dStyle != DIALOG_STYLE_LIST)
	{
		ImGui::Columns(iTabsCount, "###tablistColumn", false);
		for(uint16_t i = 0; i < iTabsCount; i++)
		{
			ImGui::SetColumnWidth(-1, fTabSize[i].x*( i == iTabsCount-1 ? 1.25 : 1.5 )+buttonFactor );
			ImGui::NextColumn();
		}
	}

	for(uint32_t line = dStyle == DIALOG_STYLE_TABLIST_HEADERS ? 1 : 0; line < vLines.size(); line++)
	{
		std::stringstream sTabLine(vLines[line]);
		std::string tmpTabLine;
		int tmpTabId = 0;

		ImVec2 differentOffsets;
		if(tmpTabId == 0)
		{
			ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] = (ImVec4)ImColor(119, 4, 4, 255);
			ImGui::GetStyle().Colors[ImGuiCol_Header] = ImGui::GetStyle().Colors[ImGuiCol_HeaderActive];
			ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered] = ImGui::GetStyle().Colors[ImGuiCol_HeaderActive];

			ImGui::PushID(tmpTabId+line);
			std::string itemid = "##" + std::to_string(line+tmpTabId);
			bool is_selected = (m_iSelectedItem == line);

			ImVec2 cursonPosition;
			cursonPosition = ImGui::GetCursorPos();

			if(ImGui::Selectable(itemid.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick ))
				if(ImGui::IsMouseDoubleClicked(0))
				{
					Show(false);
					if (dStyle == DIALOG_STYLE_LIST && (m_wDialogID == 0) && !pNetGame)
					{
						
					}
					if(pNetGame)
					{
						if(dStyle == DIALOG_STYLE_TABLIST_HEADERS) m_iSelectedItem--;
						pNetGame->SendDialogResponse(m_wDialogID, 1, m_iSelectedItem, (char*)firstTabs[m_iSelectedItem].c_str());
					}
				}
			if(ImGui::IsItemHovered())
			{
				m_iSelectedItem = line;
			}
			itemid = "##" + std::to_string(line+tmpTabId+1);
			if(ImGui::Selectable(itemid.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick ))
				if(ImGui::IsMouseDoubleClicked(0))
				{
					Show(false);
					if (dStyle == DIALOG_STYLE_LIST && (m_wDialogID == 0) && !pNetGame)
					{
						
					}
					if(pNetGame)
					{
						if(dStyle == DIALOG_STYLE_TABLIST_HEADERS) m_iSelectedItem--;
						pNetGame->SendDialogResponse(m_wDialogID, 1, m_iSelectedItem, (char*)firstTabs[m_iSelectedItem].c_str());
					}
				}
			if(ImGui::IsItemHovered())
			{
				m_iSelectedItem = line;
			}
			ImVec2 cursonPositionLast;
			cursonPositionLast = ImGui::GetCursorPos();

			differentOffsets.x = cursonPositionLast.x - cursonPosition.x;
			differentOffsets.y = cursonPositionLast.y - cursonPosition.y;

			if (is_selected) ImGui::SetItemDefaultFocus();

			if(dStyle != DIALOG_STYLE_LIST)
				ImGui::SameLine();

		}
		if(dStyle != DIALOG_STYLE_LIST)
		{
			while(std::getline(sTabLine, tmpTabLine, '\t'))
			{
				if(tmpTabId > iTabsCount) continue;

				ImVec2 cursonPos;
				cursonPos = ImGui::GetCursorPos();

				if(tmpTabId == 0)
				{
					if(line == m_iSelectedItem)
						m_strSelectedItemText = tmpTabLine;
					ImGui::SetCursorPos(ImVec2(cursonPos.x, cursonPos.y-differentOffsets.y/4 ));
				}
				else
					ImGui::SetCursorPos(ImVec2(cursonPos.x, cursonPos.y+differentOffsets.y/4 ));

				tmpTabLine.insert(0, "{ffffff}");
				TextWithColors((char*)tmpTabLine.c_str());
				ImGui::SetCursorPos(ImVec2(cursonPos.x, cursonPos.y+differentOffsets.y/2 ));

				if(dStyle != DIALOG_STYLE_LIST)
					ImGui::NextColumn();
				tmpTabId++;

			}
		}
		else
		{
			ImVec2 cursonPos;
			cursonPos = ImGui::GetCursorPos();

			if(line == m_iSelectedItem)
				m_strSelectedItemText = vLines[line];
			ImGui::SetCursorPos(ImVec2(cursonPos.x, cursonPos.y-differentOffsets.y+pGUI->GetFontSize()/2 ));

			vLines[line].insert(0, "{ffffff}");
			TextWithColors((char*)vLines[line].c_str());
			ImGui::SetCursorPos(ImVec2(cursonPos.x, cursonPos.y));

		}
	}

	if(dStyle != DIALOG_STYLE_LIST) {
		ImGui::Columns(1);
	}
	ImGui::EndChild();

	if(m_utf8Button1[0] != 0 && m_utf8Button2[0] != 0) {
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - pGUI->ScaleX(417.0f) + ImGui::GetStyle().ItemSpacing.x) / 2);
	}
	else {
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - pGUI->ScaleX(230.0f) + ImGui::GetStyle().ItemSpacing.x) / 2);
	}

	// Обработка нажатия на Кнопку #1
	if(m_utf8Button1[0] != 0)
	{
		if(ImGui::Button(m_utf8Button1, ImVec2(pGUI->ScaleX(187.5f), pGUI->ScaleY(75.0f))))
		{
			Show(false);
			if (dStyle == DIALOG_STYLE_LIST && (m_wDialogID == 0) && !pNetGame)
			{
				
			}
			if(pNetGame)
			{
				if(dStyle == DIALOG_STYLE_TABLIST_HEADERS) m_iSelectedItem--;
				pNetGame->SendDialogResponse(m_wDialogID, 1, m_iSelectedItem, (char*)firstTabs[m_iSelectedItem].c_str());
			}
		}
	}

	ImGui::SameLine(0, pGUI->GetFontSize());

	// Обработка нажатия на Кнопку #2
	if(m_utf8Button2[0] != 0)
	{
		if(ImGui::Button(m_utf8Button2, ImVec2(pGUI->ScaleX(187.5f),pGUI->ScaleY(75.0f))))
		{
			Show(false);
			if (dStyle == DIALOG_STYLE_LIST && (m_wDialogID == 0) && !pNetGame)
			{
				std::terminate();
				return;
			}
			if(pNetGame)
			{
				if(firstTabs.size() <= m_iSelectedItem)
				{
					m_iSelectedItem = firstTabs.size();
					m_iSelectedItem--;
				}
				pNetGame->SendDialogResponse(m_wDialogID, 0, m_iSelectedItem, (char*)firstTabs[m_iSelectedItem].c_str());
			}
		}
	}
    ImGui::SetWindowSize(ImVec2(-1, -1));
//	ImGui::SetWindowSize(ImVec2(vecWinSize.x, pGUI->ScaleY(vecWinSize.y + dStyle == DIALOG_STYLE_TABLIST_HEADERS ? pGUI->ScaleY(250.5f) / 3 : 0)));
	ImVec2 winsize = ImGui::GetWindowSize();
	ImGui::SetWindowPos(ImVec2(((io.DisplaySize.x - winsize.x) / 2), ((io.DisplaySize.y - winsize.y) / 2)));
	//

	ImGui::End();

}

void CDialogWindow::Render()
{
	if(!m_bIsActive || !m_putf8Info) return;

	ImGuiIO &io = ImGui::GetIO();


	if( m_byteDialogStyle == DIALOG_STYLE_TABLIST_HEADERS || m_byteDialogStyle == DIALOG_STYLE_TABLIST || m_byteDialogStyle == DIALOG_STYLE_LIST )
		return RenderTabList(m_byteDialogStyle);
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.13f, 0.13f, 0.13f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0.13f, 0.13f, 0.13f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImVec4)ImColor(0.13f, 0.13f, 0.13f, 0.75f));
	//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pGUI->GetFontSize(), pGUI->GetFontSize()));

	ImGui::Begin(" ", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_AlwaysAutoResize);

	TextWithColors(m_utf8Title);
	ImGui::ItemSize(ImVec2(0, pGUI->GetFontSize() / 2 + 5));

	switch(m_byteDialogStyle)
	{
		case DIALOG_STYLE_MSGBOX:
		TextWithColors(m_putf8Info);
		ImGui::ItemSize(ImVec2(0, pGUI->GetFontSize() / 2));
		break;

		case DIALOG_STYLE_INPUT:
		TextWithColors(m_putf8Info);
		ImGui::ItemSize(ImVec2(0, pGUI->GetFontSize() / 2));

		if(ImGui::Button(utf8DialogInputBuffer, ImVec2(ImGui::CalcTextSize(m_pszInfo).x, pGUI->GetFontSize() * 1.5f)))
		{
			if(!pKeyBoard->IsOpen()) pKeyBoard->Open(&DialogWindowInputHandler, false);
		}

		ImGui::ItemSize(ImVec2(0, pGUI->GetFontSize() / 2));
		break;

		case DIALOG_STYLE_PASSWORD:
		TextWithColors(m_putf8Info);
		ImGui::ItemSize(ImVec2(0, pGUI->GetFontSize() / 2 + 10));

		char _utf8DialogInputBuffer[100*3+1];
		strcpy(_utf8DialogInputBuffer, utf8DialogInputBuffer);

		for(int i = 0; i < strlen(_utf8DialogInputBuffer); i++)
		{
			if(_utf8DialogInputBuffer[i] == '\0')
				break;
			_utf8DialogInputBuffer[i] = '*';
		}

		if(ImGui::Button(_utf8DialogInputBuffer, ImVec2(ImGui::CalcTextSize(m_pszInfo).x+pGUI->GetFontSize(), pGUI->GetFontSize() * 1.5f)))
		{
			if(!pKeyBoard->IsOpen()) pKeyBoard->Open(&DialogWindowInputHandler, true);
		}
		ImGui::ItemSize(ImVec2(0, pGUI->GetFontSize() / 2 + 5));
		break;
	}

	if(m_utf8Button1[0] != 0 && m_utf8Button2[0] != 0) {
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - pGUI->ScaleX(417.0f) + ImGui::GetStyle().ItemSpacing.x) / 2);
	}
	else {
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - pGUI->ScaleX(230.0f) + ImGui::GetStyle().ItemSpacing.x) / 2);
	}

	// Обработка нажатия на Кнопку #1
	if(m_utf8Button1[0] != 0)
	{
		// ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(63, 63, 63, 255));
		if(ImGui::Button(m_utf8Button1, ImVec2(pGUI->ScaleX(187.5f), pGUI->ScaleY(75.0f))))
		{
			Show(false);
			if(pNetGame)
				pNetGame->SendDialogResponse(m_wDialogID, 1, 0, szDialogInputBuffer);
		}
		// ImGui::PopStyleColor();
	}

	ImGui::SameLine();

	// Обработка нажатия на Кнопку #2
	if(m_utf8Button2[0] != 0)
	{
		// ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(63, 63, 63, 255));
		if(ImGui::Button(m_utf8Button2, ImVec2(pGUI->ScaleX(187.5f),pGUI->ScaleY(75.0f))))
		{
			Show(false);
			if(pNetGame)
				pNetGame->SendDialogResponse(m_wDialogID, 0, -1, szDialogInputBuffer);
		}
		// ImGui::PopStyleColor();
	}

	ImVec2 size = ImGui::GetWindowSize();
	ImGui::SetWindowPos(ImVec2(((io.DisplaySize.x - size.x) / 2), ((io.DisplaySize.y - size.y) / 2)));
	ImGui::End();

	//ImGui::PopStyleVar();
}
