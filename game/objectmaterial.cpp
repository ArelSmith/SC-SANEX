#include "../main.h"
#include "objectmaterial.h"

uintptr_t materialTextureStore[20000][16];

void InitObjectMaterial()
{
	for(int i = 0; i < 20000; i++)
    {
        for(int x = 0; x < 15; x++)
            materialTextureStore[i][x] = 0;
    }
}

uintptr_t AtomicCallback(uintptr_t atomic, stMaterialData *data)
{
    if(atomic)
    {
        RpAtomic *pRpAtomic = (RpAtomic*)atomic;
        if(pRpAtomic)
        {
            RpGeometry *pRpGeometry = pRpAtomic->geometry;
            if(pRpGeometry)
            {
                for(int index = 0; index < pRpGeometry->matList.numMaterials; ++index)
                {
                    if(pRpGeometry->matList.materials[index])
                    {
                        if(index > MAX_MATERIALS_PER_MODEL) break;

                        if(!data->bChange)
                        {
                            if(materialTextureStore[data->wModelIndex][index])
                                pRpGeometry->matList.materials[index]->texture = (RwTexture*)materialTextureStore[data->wModelIndex][index];
                        }
                        else
                        {
                            if(!materialTextureStore[data->wModelIndex][index])
                                materialTextureStore[data->wModelIndex][index] = (uintptr_t)pRpGeometry->matList.materials[index]->texture;
                        
                            if(data->materialTexture[index])
                                pRpGeometry->matList.materials[index]->texture = (RwTexture*)data->materialTexture[index];
                        
                            /*if(data->dwMaterialColor[index] != 0)
                            {
                                int b = (data->dwMaterialColor[index]) & 0xFF;
                                int g = (data->dwMaterialColor[index] >> 8) & 0xFF;
                                int r = (data->dwMaterialColor[index] >> 16) & 0xFF;
                                int a = (data->dwMaterialColor[index] >> 24) & 0xFF;

                                pRpGeometry->matList.materials[index]->color.red = r;
                                pRpGeometry->matList.materials[index]->color.green = g;
                                pRpGeometry->matList.materials[index]->color.blue = b;
                                pRpGeometry->matList.materials[index]->color.alpha = a;
                            }*/
                        }
                    }
                }
            }
        }
    }
    
    return atomic;
}

void SetObjectMaterial(uintptr_t rwObject, uint16_t wModelIndex, uintptr_t *materialTexture, uint32_t *dwMaterialColor, bool bChange)
{
    // RwFrame
    uintptr_t parent = *(uintptr_t*)(rwObject + 4);
    if(parent)
    {
        stMaterialData data = {0};
        data.bChange = bChange;
        data.wModelIndex = wModelIndex;
        data.materialTexture = materialTexture;
        data.dwMaterialColor = dwMaterialColor;

        // RwFrameToAllObjects
        ((uintptr_t (*)(uintptr_t, uintptr_t(uintptr_t, stMaterialData*), stMaterialData*))(g_libGTASA+0x1AEE2C+1))(parent, AtomicCallback, &data);
    }
}