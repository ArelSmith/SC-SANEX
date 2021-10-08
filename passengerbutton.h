#pragma once
#include "game/RW/RenderWare.h"

class CPassengerButton
{
public:
	CPassengerButton();
	~CPassengerButton();

	void Render();
	RwTexture* passengerButton = nullptr;

private:
	float ScaleX(float x) { return m_vecScale.x * x; }
	float ScaleY(float y) { return m_vecScale.y * y; }
	ImVec2		m_vecScale;
};

