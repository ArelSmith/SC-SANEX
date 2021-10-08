#pragma once
#include "common.h"
#include "RW/RenderWare.h"

struct stMaterialData
{
	bool bChange;
	uint16_t wModelIndex;
	uintptr_t *materialTexture;
	uint32_t *dwMaterialColor; 
};

void InitObjectMaterial();
void SetObjectMaterial(uintptr_t rwObject, uint16_t wModelIndex, uintptr_t *materialTexture, uint32_t *dwMaterialColor, bool bChange = true);
uintptr_t AtomicCallback(uintptr_t atomic, stMaterialData *data);