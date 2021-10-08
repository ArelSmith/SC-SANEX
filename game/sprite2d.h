#pragma once

#include "rgba.h"
#include "RW/RenderWare.h"

class CSprite2D 
{
public:
	CSprite2D();
	~CSprite2D();
	
	void Draw(float x, float y, float width, float height, CRGBA const& color);
	void Delete();
};