#include "main.h"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_internal.h"
#include "ImGuiRenderer.h"
#include "gui.h"
#include "../chatwindow.h"
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <string>

extern CGUI *pGUI;
extern CChatWindow *pChatWindow;  

bool ProcessInlineHexColor(const char* start, const char* end, ImVec4& color)
{
	const int hexCount = (int)(end-start);
	if(hexCount == 6 || hexCount == 8)
	{
		char hex[9];
		strncpy(hex, start, hexCount);
		hex[hexCount] = 0;

		unsigned int hexColor = 0;
		if(sscanf(hex, "%x", &hexColor)	> 0)
		{
			color.x = static_cast< float >((hexColor & 0x00FF0000) >> 16) / 255.0f;
			color.y = static_cast< float >((hexColor & 0x0000FF00) >> 8) / 255.0f;
			color.z = static_cast< float >((hexColor & 0x000000FF)) / 255.0f;
			color.w = 1.0f;

			if(hexCount == 8)
				color.w = static_cast< float >((hexColor & 0xFF000000) >> 24) / 255.0f;

			return true;
		}
	}

	return false;
}

std::string removeColorTags(std::string line)
{
	std::string string;
	for(uint32_t it = 0; it < line.size(); it++)
	{
		if(it+7 < line.size())
		{
			if(line[it] == '{' && line[it+7] == '}')
			{
				it += 7;
				continue;
			}
		}
		string.push_back(line[it]);
	}
	return string;
}

ImVec2 CalcTextSizeWithoutTags(char* szStr)
{
	if(!szStr) return ImVec2(0, 0);

	char szNonColored[2048+1];
	int iNonColoredMsgLen = 0;

	for(int pos = 0; pos < strlen(szStr) && szStr[pos] != '\0'; pos++)
	{
		if(pos+7 < strlen(szStr))
		{
			if(szStr[pos] == '{' && szStr[pos+7] == '}')
			{
				pos += 7;
				continue;
			}
		}

		szNonColored[iNonColoredMsgLen] = szStr[pos];
		iNonColoredMsgLen++;
	}

	szNonColored[iNonColoredMsgLen] = 0;

	return ImGui::CalcTextSize(szNonColored);
}

ImVec2 CalcTextSizeWithoutTags(const float font_size, const char* szStr)
{
	if(!szStr) return ImVec2(0, 0);

	char szNonColored[2048];
	int iNonColoredMsgLen = 0;

	for(int pos = 0; pos < strlen(szStr) && szStr[pos] != '\0'; pos++)
	{
		if(pos + 7 < strlen(szStr))
		{
			if(szStr[pos] == '{' && szStr[pos + 7] == '}')
			{
				pos += 7;
				continue;
			}
		}

		szNonColored[iNonColoredMsgLen] = szStr[pos];
		iNonColoredMsgLen++;
	}

	szNonColored[iNonColoredMsgLen] = 0;
	return ImGui::CalcTextSize(font_size, szNonColored);
}

void TextWithColors(ImVec2 pos, ImColor col, const char* szStr)
{
	if(!pGUI) return;

	char tempStr[4096];

	ImVec2 vecPos = pos;

	strcpy(tempStr, szStr);
	tempStr[sizeof(tempStr) - 1] = '\0';

	bool pushedColorStyle = false;
	const char* textStart = tempStr;
	const char* textCur = tempStr;
	while(textCur < (tempStr + sizeof(tempStr)) && *textCur != '\0')
	{
		if (*textCur == '{')
		{
			// Print accumulated text
			if (textCur != textStart)
			{
				//ImGui::TextUnformatted(textStart, textCur);
				pGUI->RenderText(vecPos, col, true, textStart, textCur);
				//ImGui::SameLine(0.0f, 0.0f);
				vecPos.x += ImGui::CalcTextSize(textStart, textCur).x;
			}

			// Process color code
			const char* colorStart = textCur + 1;
			do
			{
				++textCur;
			} while (*textCur != '\0' && *textCur != '}');

			// Change color
			if (pushedColorStyle)
			{
				//ImGui::PopStyleColor();
				pushedColorStyle = false;
			}

			ImVec4 textColor;
			if (ProcessInlineHexColor(colorStart, textCur, textColor))
			{
				//ImGui::PushStyleColor(ImGuiCol_Text, textColor);
				col = textColor;
				pushedColorStyle = true;
			}

			textStart = textCur + 1;
		}
		else if (*textCur == '\n')
		{
			// Print accumulated text an go to next line
			//ImGui::TextUnformatted(textStart, textCur);
			pGUI->RenderText(vecPos, col, true, textStart, textCur);
			vecPos.x = pos.x;//+= ImGui::CalcTextSize(textStart, textCur).x;
			vecPos.y += pGUI->GetFontSize();
			textStart = textCur + 1;
		}

		++textCur;
	}

	if (textCur != textStart)
	{
		pGUI->RenderText(vecPos, col, true, textStart, textCur);
		vecPos.x += ImGui::CalcTextSize(textStart, textCur).x;
	}
	else
	{
		vecPos.y += pGUI->GetFontSize();
	}
}

void TextWithColors(const char* fmt, ...)
{
	char tempStr[4096];

	va_list argPtr;
	va_start(argPtr, fmt);
	vsnprintf(tempStr, sizeof(tempStr), fmt, argPtr);
	va_end(argPtr);
	tempStr[sizeof(tempStr) - 1] = '\0';

	bool pushedColorStyle = false;
	const char* textStart = tempStr;
	const char* textCur = tempStr;
	while(textCur < (tempStr + sizeof(tempStr)) && *textCur != '\0')
	{
		if (*textCur == '{')
		{
			// Print accumulated text
			if (textCur != textStart)
			{
				ImGui::TextUnformatted(textStart, textCur);
				ImGui::SameLine(0.0f, 0.0f);
			}

			// Process color code
			const char* colorStart = textCur + 1;
			do
			{
				++textCur;
			} while (*textCur != '\0' && *textCur != '}');

			// Change color
			if (pushedColorStyle)
			{
				ImGui::PopStyleColor();
				pushedColorStyle = false;
			}

			ImVec4 textColor;
			if (ProcessInlineHexColor(colorStart, textCur, textColor))
			{
				ImGui::PushStyleColor(ImGuiCol_Text, textColor);
				pushedColorStyle = true;
			}

			textStart = textCur + 1;
		}
		else if (*textCur == '\n')
		{
			// Print accumulated text an go to next line
			ImGui::TextUnformatted(textStart, textCur);
			textStart = textCur + 1;
		}

		++textCur;
	}

	if (textCur != textStart)
		ImGui::TextUnformatted(textStart, textCur);
	else
		ImGui::NewLine();

	if(pushedColorStyle)
		ImGui::PopStyleColor();
}

void TextWithColors(ImFont *font, const float font_size, uint8_t outline, ImVec2 pos, ImColor col, const char* szStr)
{
	char tempStr[4096];

	ImVec2 vecPos = pos;

	strcpy(tempStr, szStr);
	tempStr[sizeof(tempStr) - 1] = '\0';

	bool pushedColorStyle = false;
	const char* textStart = tempStr;
	const char* textCur = tempStr;
	while(textCur < (tempStr + sizeof(tempStr)) && *textCur != '\0')
	{
		if (*textCur == '{')
		{
			// Print accumulated text
			if (textCur != textStart)
			{
				pGUI->RenderText(vecPos, col, outline, textStart, textCur, font_size, font, true);
				vecPos.x += ImGui::CalcTextSize(font_size, textStart, textCur).x;
			}

			// Process color code
			const char* colorStart = textCur + 1;
			do
			{
				++textCur;
			} while (*textCur != '\0' && *textCur != '}');

			// Change color
			if (pushedColorStyle)
				pushedColorStyle = false;

			ImVec4 textColor;
			if (ProcessInlineHexColor(colorStart, textCur, textColor))
			{
				col = textColor;
				pushedColorStyle = true;
			}

			textStart = textCur + 1;
		}
		else if (*textCur == '\n')
		{
			pGUI->RenderText(vecPos, col, outline, textStart, textCur, font_size, font, true);
			vecPos.x = pos.x;
			vecPos.y += font_size;
			textStart = textCur + 1;
		}

		++textCur;
	}

	if (textCur != textStart)
	{
		pGUI->RenderText(vecPos, col, outline, textStart, textCur, font_size, font, true);
		vecPos.x += ImGui::CalcTextSize(font_size, textStart, textCur).x;
	}
	else
		vecPos.y += font_size;
}

void Render3DLabel(ImVec2 pos, char* utf8string, uint32_t dwColor)
{
	uint16_t linesCount = 0;
	std::string strUtf8 = utf8string;
	int size = strUtf8.length();
	std::string color;

	for(uint32_t i = 0; i < size; i++)
	{
		if(i+7 < strUtf8.length())
		{
			if(strUtf8[i] == '{' && strUtf8[i+7] == '}' )
			{
				color = strUtf8.substr(i, 7+1);
			}
		}
		if(strUtf8[i] == '\n')
		{
			linesCount++;
			if(i+1 < strUtf8.length() && !color.empty())
			{
				strUtf8.insert(i+1 , color);
				size += color.length();
				color.clear();
			}
		}
		if(strUtf8[i] == '\t')
		{
			strUtf8.replace(i, 1, " ");
		}
	}
	pos.y += pGUI->GetFontSize()*(linesCount / 2);
	if(linesCount)
	{
		uint16_t curLine = 0;
		uint16_t curIt = 0;
		for(uint32_t i = 0; i < strUtf8.length(); i++)
		{
			if(strUtf8[i] == '\n')
			{
				if(strUtf8[curIt] == '\n' )
				{
					curIt++;
				}
				ImVec2 _pos = pos;
				_pos.x -= CalcTextSizeWithoutTags((char*)strUtf8.substr(curIt, i-curIt).c_str()).x / 2;
				_pos.y -= ( pGUI->GetFontSize()*(linesCount - curLine) );
				TextWithColors( _pos, __builtin_bswap32(dwColor), (char*)strUtf8.substr(curIt, i-curIt).c_str() );
				curIt = i;
				curLine++;
			}
		}
		if(strUtf8[curIt] == '\n')
		{
			curIt++;
		}
		if(strUtf8[curIt] != '\0')
		{
			ImVec2 _pos = pos;
			_pos.x -= CalcTextSizeWithoutTags((char*)strUtf8.substr(curIt, strUtf8.size()-curIt).c_str()).x / 2;
			_pos.y -= ( pGUI->GetFontSize()*(linesCount - curLine) );
			TextWithColors( _pos, __builtin_bswap32(dwColor), (char*)strUtf8.substr(curIt, strUtf8.length()-curIt).c_str() );
		}
	}
	else
	{
		pos.x -= CalcTextSizeWithoutTags((char*)strUtf8.c_str()).x / 2;
		TextWithColors( pos, __builtin_bswap32(dwColor), (char*)strUtf8.c_str() );
	}
}

void ScrollWhenDraggingOnVoid()
{
	ImGuiIO &io = ImGui::GetIO();
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 windowSize = ImGui::GetWindowSize();
	if(io.MousePos.x >= windowPos.x && io.MousePos.x <= windowPos.x + windowSize.x &&
		io.MousePos.y >= windowPos.y && io.MousePos.y <= windowPos.y + windowSize.y)
	{
		ImVec2 mouse_delta = io.MouseDelta;
		//pChatWindow->AddDebugMessage("X: %f - Y: %f", mouse_delta.x, mouse_delta.y);
		if(mouse_delta.x != 0.0f && mouse_delta.x <= 100.0f) ImGui::SetScrollX(ImGui::GetScrollX() + -mouse_delta.x);
		if(mouse_delta.y != 0.0f && mouse_delta.y <= 100.0f) ImGui::SetScrollY(ImGui::GetScrollY() + -mouse_delta.y); 
	}
}
