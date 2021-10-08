#pragma once
#include "vendor/imgui/imgui.h"
//#include "vendor/imgui/imgui_freetype.h"
#include "ImGuiRenderer.h"

#define RGBA_TO_FLOAT(r,g,b,a) (float)r/255.0f, (float)g/255.0f, (float)b/255.0f, (float)a/255.0f

enum eTouchType
{
	TOUCH_POP = 1,
	TOUCH_PUSH = 2,
	TOUCH_MOVE = 3
};

class CGUI
{
public:
	CGUI();
	~CGUI();

	ImFont* LoadFont(char *font, float fontsize = 0.0f);

	void Render();

	float ScaleX(float x) { return m_vecScale.x * x; }
	float ScaleY(float y) { return m_vecScale.y * y; }
	ImFont* GetFont() { return m_pFont; }
	ImFont* GetWeaponFont() { return m_pFontGTAWeap; }
	ImFont* GetSampFont() { return m_pSampFont; }
	float GetFontSize() { return m_fFontSize; }

	bool OnTouchEvent(int type, bool multi, int x, int y);

	void RenderText(ImVec2& pos, ImU32 col, bool bOutline, const char* text_begin, const char* text_end = nullptr, float font_size = 0.0f, ImFont *font = NULL, bool bOutlineUseTextColor = false);

private:
	void RenderRakNetStatistics();
	void RenderVersion();

public:
	bool bShowDebugLabels;
	
private:
	ImFont* 	m_pFont;
	ImFont*		m_pSampFont;
	ImFont*		m_pFontGTAWeap;
	ImFont*		m_pFontSampAux;

	ImVec2		m_vecScale;
	float 		m_fFontSize;

	bool 		m_bMousePressed;
	ImVec2		m_vecMousePos;

	bool		m_bNeedClearMousePos;

};
