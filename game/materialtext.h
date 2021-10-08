#pragma once

class CMaterialText
{
public:
	CMaterialText();

	uintptr_t Generate(int iSizeX, int iSizeY, const char* szFontName, uint8_t byteFontSize, uint8_t byteFontBold, uint32_t dwFontColor, uint32_t dwBackgroundColor, uint8_t byteAlign, const char* szText);
	
private:
	void SetUpScene();
	void Render(int iSizeX, int iSizeY, const char* szFontName, uint8_t byteFontSize, uint8_t byteFontBold, uint32_t dwFontColor, uint8_t byteAlign, const char* szText);

	uintptr_t m_camera;
	uintptr_t m_zBuffer;
};
