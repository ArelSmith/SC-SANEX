#pragma once

#include "game/RW/RenderWare.h"

bool ProcessInlineHexColor(const char* start, const char* end, ImVec4& color);
std::string removeColorTags(std::string line);
ImVec2 CalcTextSizeWithoutTags(char* szStr);
ImVec2 CalcTextSizeWithoutTags(const float font_size, const char* szStr);
void TextWithColors(const char* fmt, ...);
void TextWithColors(ImVec2 pos, ImColor col, const char* szStr);
void TextWithColors(ImFont *font, const float font_size, uint8_t outline, ImVec2 pos, ImColor col, const char* szStr);
void Render3DLabel(ImVec2 pos, char* utf8string, uint32_t dwColor);
void ScrollWhenDraggingOnVoid();