#include "../main.h"
#include "game.h"
#include "RW/RenderWare.h"
#include "materialtext.h"
#include "gui/gui.h"
#include "vendor/imgui/imgui_internal.h"

extern CGame *pGame;
extern CGUI *pGUI;

/* imgui_impl_renderware.h */
void ImGui_ImplRenderWare_RenderDrawData(ImDrawData* draw_data);
bool ImGui_ImplRenderWare_Init();
void ImGui_ImplRenderWare_NewFrame();

CMaterialText::CMaterialText()
{
	m_camera = 0;
	m_zBuffer = 0;

	SetUpScene();
}

void CMaterialText::SetUpScene()
{
	// RwCameraCreate
	m_camera = ((uintptr_t(*)())(g_libGTASA + 0x1ADA1C + 1))();

	// RwFrameCreate
	m_zBuffer = ((uintptr_t(*)())(g_libGTASA + 0x1AE9E0 + 1))();

	//proverki
	if(!m_camera || !m_zBuffer) return;

	// RwObjectHasFrameSetFrame
	((void(*)(uintptr_t, uintptr_t))(g_libGTASA + 0x1B2988 + 1))(m_camera, m_zBuffer);

	// RwCameraSetFarClipPlane
	((void(*)(uintptr_t, float))(g_libGTASA + 0x1AD710 + 1))(m_camera, 300.0f);

	// RwCameraSetNearClipPlane
	((void(*)(uintptr_t, float))(g_libGTASA + 0x1AD6F4 + 1))(m_camera, 0.01f);

	// RwCameraSetViewWindow
	float view[2] = { 0.5f, 0.5f };
	((void(*)(uintptr_t, float*))(g_libGTASA + 0x1AD924 + 1))(m_camera, view);

	// RwCameraSetProjection
	((void(*)(uintptr_t, int))(g_libGTASA + 0x1AD8DC+ 1))(m_camera, 1);

	// RpWorldAddCamera
	uintptr_t pRwWorld = *(uintptr_t*)(g_libGTASA + 0x95B060);
	if (pRwWorld)
		((void(*)(uintptr_t, uintptr_t))(g_libGTASA + 0x1EB118 + 1))(pRwWorld, m_camera);
}

uintptr_t CMaterialText::Generate(int iSizeX, int iSizeY, const char* szFontName, uint8_t byteFontSize, uint8_t byteFontBold, uint32_t dwFontColor, uint32_t dwBackgroundColor, uint8_t byteAlign, const char* szText)
{
	// RwRasterCreate
	uintptr_t raster = (uintptr_t)RwRasterCreate(iSizeX, iSizeY, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
	
	// RwTextureCreate
	uintptr_t bufferTexture = ((uintptr_t(*)(uintptr_t))(g_libGTASA + 0x1B1B4C + 1))(raster);

	//proverki
	if(!raster || !bufferTexture) return 0;

    // set camera frame buffer
	*(uintptr_t*)(m_camera + 0x60) = raster;

	// CVisibilityPlugins::SetRenderWareCamera
	((void(*)(uintptr_t))(g_libGTASA + 0x55CFA4 + 1))(m_camera);

	// background color
	int b = (dwBackgroundColor) & 0xFF;
	int g = (dwBackgroundColor >> 8) & 0xFF;
	int r = (dwBackgroundColor >> 16) & 0xFF;
	int a = (dwBackgroundColor >> 24) & 0xFF;
	unsigned int dwBackgroundABGR = (r | (g << 8) | (b << 16) | (a << 24));

	// RwCameraClear
	((void(*)(uintptr_t, unsigned int*, int))(g_libGTASA + 0x1AD8A0 + 1))(m_camera, &dwBackgroundABGR, 3);
	
	RwCameraBeginUpdate((RwCamera*)m_camera);

	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODENASHADEMODE);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);
	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODENACULLMODE);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);

	// DefinedState
	((void(*) (void))(g_libGTASA + 0x559008 + 1))();
	
	if(pGUI)
		Render(iSizeX, iSizeY, szFontName, byteFontSize, byteFontBold, dwFontColor, byteAlign, szText);
	
	RwCameraEndUpdate((RwCamera*)m_camera);
	return bufferTexture;
}

void CMaterialText::Render(int iSizeX, int iSizeY, const char* szFontName, uint8_t byteFontSize, uint8_t byteFontBold, uint32_t dwFontColor, uint8_t byteAlign, const char* szText)
{
	// load fonts
	ImFont *pFont = pGUI->LoadFont((char*)szFontName, (float)byteFontSize);

	// setup frame
	ImGui_ImplRenderWare_NewFrame();
	ImGui::NewFrame();

	// cp1251 to utf8
	char utf8[2048];
	cp1251_to_utf8(utf8, szText, 2048);

	// text color
	int b = (dwFontColor) & 0xFF;
	int g = (dwFontColor >> 8) & 0xFF;
	int r = (dwFontColor >> 16) & 0xFF;
	int a = (dwFontColor >> 24) & 0xFF;

	// text align
	ImVec2 _veecPos;
	switch(byteAlign)
	{
		case OBJECT_MATERIAL_TEXT_ALIGN_LEFT:
		{
			// render text with left align
			ImVec2 veecPos = ImVec2(pGUI->ScaleX(0), pGUI->ScaleY(0));

			// TextWithColors aleardy filter \n (newline)
			TextWithColors(pFont, byteFontSize, false, veecPos, ImColor(r, g, b, a), utf8);
			break;
		}
		case OBJECT_MATERIAL_TEXT_ALIGN_CENTER:
		{
			// render text with center align
			int newLineCount = 0;
			std::string strText = utf8;
			std::stringstream ssLine(strText);
			std::string tmpLine;
			while(std::getline(ssLine, tmpLine, '\n'))
			{
				if(newLineCount == 0)
					_veecPos.y = (iSizeY - CalcTextSizeWithoutTags(byteFontSize, tmpLine.c_str()).y) / 2;
				else _veecPos.y -= (CalcTextSizeWithoutTags(byteFontSize, tmpLine.c_str()).y / 2);
				newLineCount++;
			}

			// if pos y is minus change the pos to 0 (so make it like samp pc?)
			if(_veecPos.y < 0.0)
				_veecPos.y = 0.0;

			std::string strTexts = utf8;
			std::stringstream ssLines(strTexts);
			std::string tmpLines;
			while(std::getline(ssLines, tmpLines, '\n'))
			{
				if(tmpLines[0] != 0)
				{
					_veecPos.x = (iSizeX - CalcTextSizeWithoutTags(byteFontSize, tmpLines.c_str()).x) / 2;
					TextWithColors(pFont, byteFontSize, false, _veecPos, ImColor(r, g, b, a), tmpLines.c_str());
					_veecPos.y += byteFontSize;
				}
			}
			break;
		}
		case OBJECT_MATERIAL_TEXT_ALIGN_RIGHT:
		{
			// render text with right align
			std::string strText = utf8;
			std::stringstream ssLine(strText);
			std::string tmpLine;
			while(std::getline(ssLine, tmpLine, '\n'))
			{
				if(tmpLine[0] != 0)
				{
					_veecPos.x = (iSizeX - CalcTextSizeWithoutTags(byteFontSize, tmpLine.c_str()).x);
					TextWithColors(pFont, byteFontSize, false, _veecPos, ImColor(r, g, b, a), tmpLine.c_str());
					_veecPos.y += byteFontSize;
				}
			}
			break;
		}
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplRenderWare_RenderDrawData(ImGui::GetDrawData());
}
