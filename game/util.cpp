#include "../main.h"
#include "game.h"

extern CGame *pGame;

#define PI 3.14159265

uintptr_t dwPlayerPedPtrs[PLAYER_PED_SLOTS];

extern char* PLAYERS_REALLOC;

PED_TYPE* GamePool_FindPlayerPed()
{
	return *(PED_TYPE**)PLAYERS_REALLOC;
}

PED_TYPE* GamePool_Ped_GetAt(int iID)
{
	return (( PED_TYPE* (*)(int))(g_libGTASA+0x41DD7C+1))(iID);
}

int GamePool_Ped_GetIndex(PED_TYPE *pActor)
{
    return (( int (*)(PED_TYPE*))(g_libGTASA+0x41DD60+1))(pActor);
}

void GamePrepareTrain(VEHICLE_TYPE *pVehicle)
{
    PED_TYPE *pDriver = pVehicle->pDriver;

    // GET RID OF THE PED DRIVER CREATED
    if(pDriver)
    {
        if (pDriver->dwPedType >= 2)
        {
            (( void (*)(PED_TYPE*))(*(void**)(pDriver->entity.vtable + 0x4)))(pDriver);
            pVehicle->pDriver = 0;
        }
    }
}

VEHICLE_TYPE *GamePool_Vehicle_GetAt(int iID)
{
	return (( VEHICLE_TYPE* (*)(int))(g_libGTASA+0x41DD44+1))(iID);
}

int GamePool_Vehicle_GetIndex(VEHICLE_TYPE *pVehicle)
{
    return (( int (*)(VEHICLE_TYPE*))(g_libGTASA+0x41DD28+1))(pVehicle);
}

ENTITY_TYPE *GamePool_Object_GetAt(int iID)
{
	return ((ENTITY_TYPE *(*)(int))(g_libGTASA + 0x0041DDB4 + 1))(iID);
} 

int LineOfSight(VECTOR* start, VECTOR* end, void* colpoint, uintptr_t ent,
	char buildings, char vehicles, char peds, char objects, char dummies, bool seeThrough, bool camera, bool unk)
{
	return (( int (*)(VECTOR*, VECTOR*, void*, uintptr_t,
		char, char, char, char, char, char, char, char))(g_libGTASA+0x3C70C0+1))(start, end, colpoint, ent,
		buildings, vehicles, peds, objects, dummies, seeThrough, camera, unk);
}

// 0.3.7
bool IsPedModel(unsigned int iModelID)
{
	if(iModelID < 0 || iModelID > 20000) return false;
    uintptr_t *dwModelArray = (uintptr_t*)(g_libGTASA+0x87BF48);
    
    uintptr_t ModelInfo = dwModelArray[iModelID];
    if(ModelInfo && *(uintptr_t*)ModelInfo == (uintptr_t)(g_libGTASA+0x5C6E90/*CPedModelInfo vtable*/))
        return true;

    return false;
}

// 0.3.7
bool IsValidModel(unsigned int uiModelID)
{
    if(uiModelID < 0 || uiModelID > 20000) return false;
    uintptr_t *dwModelArray = (uintptr_t*)(g_libGTASA+0x87BF48);

    uintptr_t dwModelInfo = dwModelArray[uiModelID];
    if(dwModelInfo && *(uintptr_t*)(dwModelInfo+0x34/*pRwObject*/))
        return true;

    return false;
}

bool IsTrueModelID(unsigned int uiModelID)
{
	if(uiModelID < 0 || uiModelID > 20000) return false;
	if(uiModelID == 19901 || uiModelID == 19902 || uiModelID == 18860 || uiModelID == 18861 ||
		uiModelID == 19275 || uiModelID == 19276 || uiModelID == 19596)
		return false;

	return true;
}

uintptr_t GetModelInfoByID(int iModelID)
{
	if (iModelID < 0 || iModelID > 20000)
	{
		return false;
	}

	uintptr_t *dwModelArray = (uintptr_t *) (g_libGTASA + 0x87BF48);
	return dwModelArray[iModelID];
}

uintptr_t ModelInfoCreateInstance(int iModel)
{
	uintptr_t modelInfo = GetModelInfoByID(iModel);
	if (modelInfo)
	{
		return ((uintptr_t(*)(uintptr_t)) *
				(uintptr_t *) (*(uintptr_t *) modelInfo + 0x2C)) (modelInfo);
	}

	return 0;
}

uint16_t GetModelReferenceCount(int nModelIndex)
{
	uintptr_t *dwModelarray = (uintptr_t*)(g_libGTASA+0x87BF48);
	uint8_t *pModelInfoStart = (uint8_t*)dwModelarray[nModelIndex];
	
	return *(uint16_t*)(pModelInfoStart+0x1E);
}

void InitPlayerPedPtrRecords()
{
	memset(&dwPlayerPedPtrs[0], 0, sizeof(uintptr_t) * PLAYER_PED_SLOTS);
}

void SetPlayerPedPtrRecord(uint8_t bytePlayer, uintptr_t dwPedPtr)
{
	dwPlayerPedPtrs[bytePlayer] = dwPedPtr;
}

uint8_t FindPlayerNumFromPedPtr(uintptr_t dwPedPtr)
{
	uint8_t x = 0;
	while(x != PLAYER_PED_SLOTS)
	{
		if(dwPlayerPedPtrs[x] == dwPedPtr) return x;
		x++;
	}

	return 0;
}

uintptr_t FindRwTexture(std::string szTexDb, std::string szTexName)
{
	uintptr_t pRwTexture = GetTexture(szTexName + std::string("_") + szTexDb);
	if(!pRwTexture)
	{
		pRwTexture = GetTexture(szTexDb + std::string("_") + szTexName);
		if(!pRwTexture)
		{
			pRwTexture = GetTexture(szTexName);
			if(!pRwTexture)
			{
				Log("Error: File %s or texture %s not found!", szTexDb.c_str(), szTexName.c_str());
				return 0;
			}
		}
	}
	return pRwTexture;
}

uintptr_t GetTexture(std::string szTexName)
{
	Log("GetTexture: %s", szTexName.c_str());
	uintptr_t pRwTexture = ((uintptr_t(*)(char const *))(g_libGTASA + 0x001BE990 + 1)) (szTexName.c_str());
	if(pRwTexture)
		++*(uint32_t *) (pRwTexture + 0x54);

	return pRwTexture; 
}

uintptr_t LoadTextureFromDB(const char* dbname, const char* texture)
{
	// TextureDatabaseRuntime::GetDatabase(dbname)
	uintptr_t db_handle = (( uintptr_t (*)(const char*))(g_libGTASA+0x1BF530+1))(dbname);
	if(!db_handle)
	{
		Log("Error: Database not found! (%s)", dbname);
		return 0;
	}
	// TextureDatabaseRuntime::Register(db)
	(( void (*)(uintptr_t))(g_libGTASA+0x1BE898+1))(db_handle);
	uintptr_t tex = GetTexture(texture);

	if(!tex) Log("Error: Texture (%s) not found in database (%s)", dbname, texture);

	// TextureDatabaseRuntime::Unregister(db)
	(( void (*)(uintptr_t))(g_libGTASA+0x1BE938+1))(db_handle);

	return tex;
}

uintptr_t LoadTexture(const char *texname)
{
	static char* texdb[] = { "samp", "gta3", "gta_int", "player", "menu", "mobile", "txd" };
	for(int i = 0; i < 7; i++)
	{
		uintptr_t texture = LoadTextureFromDB(texdb[i], texname);
		if(texture != 0)
		{
			Log("%s loaded from %s", texname, texdb[i]);
			return texture;
		}
	}

	return 0;
}

uintptr_t LoadTexture(const char *txdname, const char *texname)
{
	uintptr_t pRwTexture = 0;
	// CTxdStore::FindTxdSlot
    int g_iTxdSlot = ((int (__fastcall *)(const char *))(g_libGTASA + 0x55BB84 + 1))(txdname);
    if(g_iTxdSlot != -1)
    {
		// CTxdStore::PushCurrentTxd
        ((void (*)(void))(g_libGTASA + 0x55BD6C + 1))();
        ((void (__fastcall *)(int, uint32_t))(g_libGTASA + 0x55BCDC + 1))(g_iTxdSlot, 0);
		// CSprite2d::SetTexture
        ((void (__fastcall *)(uintptr_t *, const char *))(g_libGTASA + 0x551854 + 1))(&pRwTexture, texname);

        if(!pRwTexture) 
		{
            Log("Error: Texture (%s) not found in database (%s)", texname, txdname);
            LoadTextureFromDB("samp", texname);
        }
		else Log("%s loaded from %s", texname, txdname);

        ((void (*)(void))(g_libGTASA + 0x55BDA8 + 1))(); // CTxdStore::PopCurrentTxd
    }
	else Log("Error: Database not found! (%s)", txdname);

    return pRwTexture;
}

void DefinedState2d()
{
	return (( void (*)())(g_libGTASA+0x5590B0+1))();
}

void SetScissorRect(void* pRect)
{
	return ((void(*)(void*))(g_libGTASA+0x273E8C+1))(pRect);
}

float DegToRad(float fDegrees)
{
    if (fDegrees > 360.0f || fDegrees < 0.0f) return 0.0f;
    
    if (fDegrees > 180.0f) return (float)(-(PI - (((fDegrees - 180.0f) * PI) / 180.0f)));
    else return (float)((fDegrees * PI) / 180.0f);
}

// 0.3.7
float FloatOffset(float f1, float f2)
{   
    if(f1 >= f2) return f1 - f2;
    else return (f2 - f1);
}

bool IsGameEntityArePlaceable(ENTITY_TYPE *pEntity) 
{
	if(pEntity)
	{
		if(pEntity->vtable == g_libGTASA + 0x005C7358)
			return true;
	}

	return false;
}

bool IsValidGamePed(PED_TYPE *pPed) 
{
	// IsPedPointerValid(CPed *) — 0x00435614
	return (((bool (*)(PED_TYPE *))(g_libGTASA + 0x00435614 + 1))(pPed));
}

void RwMatrixRotate(MATRIX4X4 *mat, int axis, float angle)
{
	static float matt[3][3] = 
	{
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f }
	};

	(( void (*)(MATRIX4X4*, float*, float, int))(g_libGTASA+0x1B9118+1))(mat, matt[axis], angle, 1);
}

void ProjectMatrix(VECTOR* vecOut, MATRIX4X4* mat, VECTOR *vecPos)
{
	vecOut->X = mat->at.X * vecPos->Z + mat->up.X * vecPos->Y + mat->right.X * vecPos->X + mat->pos.X;
	vecOut->Y = mat->at.Y * vecPos->Z + mat->up.Y * vecPos->Y + mat->right.Y * vecPos->X + mat->pos.Y;
	vecOut->Z = mat->at.Z * vecPos->Z + mat->up.Z * vecPos->Y + mat->right.Z * vecPos->X + mat->pos.Z;
}

void RwMatrixScale(MATRIX4X4 *matrix, VECTOR *vecScale)
{
	matrix->right.X *= vecScale->X;
	matrix->right.Y *= vecScale->X;
	matrix->right.Z *= vecScale->X;

	matrix->up.X *= vecScale->Y;
	matrix->up.Y *= vecScale->Y;
	matrix->up.Z *= vecScale->Y;

	matrix->at.X *= vecScale->Z;
	matrix->at.Y *= vecScale->Z;
	matrix->at.Z *= vecScale->Z;

	matrix->flags &= 0xFFFDFFFC;
}

void RwMatrixOrthoNormalize(MATRIX4X4 *matIn, MATRIX4X4 *matOut)
{
	((void (*)(MATRIX4X4*, MATRIX4X4*))(g_libGTASA + 0x001B8CC8 + 1))(matIn, matOut);
}

void RwMatrixInvert(MATRIX4X4 *matOut, MATRIX4X4 *matIn)
{
	((void (*)(MATRIX4X4*, MATRIX4X4*))(g_libGTASA + 0x001B91CC + 1))(matOut, matIn);
}

void WorldAddEntity(uintptr_t pEnt)
{
	((void (*)(uintptr_t))(g_libGTASA + 0x3C14B0 + 1)) (pEnt);
}

void WorldRemoveEntity(uintptr_t pEnt)
{
	((void (*)(uintptr_t))(g_libGTASA + 0x3C1500 + 1)) (pEnt);
}

void RenderClumpOrAtomic(uintptr_t rwObject)
{
	if (rwObject)
	{
		if (*(uint8_t *) rwObject == 1)
		{
			// Atomic
			((void (*)(uintptr_t))(*(uintptr_t *) (rwObject + 0x48))) (rwObject);
		}
		else if (*(uint8_t *) rwObject == 2)
		{
			// rpClumpRender
			((void (*)(uintptr_t))(g_libGTASA + 0x1E0E60 + 1)) (rwObject);
		}
	}
}

float GetModelColSphereRadius(int iModel)
{
	uintptr_t modelInfo = GetModelInfoByID(iModel);

	if (modelInfo)
	{
		uintptr_t colModel = *(uintptr_t *) (modelInfo + 0x2C);
		if (colModel != 0)
		{
			return *(float *)(colModel + 0x24);
		}
	}

	return 0.0f;
}

void GetModelColSphereVecCenter(int iModel, VECTOR * vec)
{
	uintptr_t modelInfo = GetModelInfoByID(iModel);

	if (modelInfo)
	{
		uintptr_t colModel = *(uintptr_t *) (modelInfo + 0x2C);
		if (colModel != 0)
		{
			VECTOR *v = (VECTOR *) (colModel + 0x18);

			vec->X = v->X;
			vec->Y = v->Y;
			vec->Z = v->Z;
		}
	}
}

void DestroyAtomicOrClump(uintptr_t rwObject)
{
	if (rwObject)
	{
		int type = *(int *)(rwObject);

		if (type == 1)
		{
			// RpAtomicDestroy
			((void (*)(uintptr_t))(g_libGTASA + 0x1E10D4 + 1)) (rwObject);

			uintptr_t parent = *(uintptr_t *) (rwObject + 4);
			if (parent)
			{
				// RwFrameDestroy
				((void (*)(uintptr_t))(g_libGTASA + 0x1AEC84 + 1)) (parent);
			}

		}
		else if (type == 2)
		{
			// RpClumpDestroy
			((void (*)(uintptr_t))(g_libGTASA + 0x1E1224 + 1)) (rwObject);
		}
	}
}

bool IsPointInRect(float x, float y, RECT* rect)
{
	if (x >= rect->fLeft && x <= rect->fRight &&
		y >= rect->fBottom && y <= rect->fTop)
		return true;

	return false;
}
bool bTextDrawTextureSlotState[200];
uintptr_t TextDrawTexture[200];

int GetFreeTextDrawTextureSlot()
{
	for (int i = 0; i < 200; i++)
	{
		if (bTextDrawTextureSlotState[i] == false)
		{
			bTextDrawTextureSlotState[i] = true;
			return i;
		}
	}

	return -1;
}

void DestroyTextDrawTexture(int index)
{
	if (index >= 0 && index < 200)
	{
		bTextDrawTextureSlotState[index] = false;
		TextDrawTexture[index] = 0x0;
	}
}

void DrawTextureUV(uintptr_t texture, RECT * rect, uint32_t dwColor, float *uv)
{
	if (texture)
	{
		RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void *)rwFILTERLINEAR);
		// CSprite2d::Draw(CRect const& posn, CRGBA const& color, float u1,
		// float v1, float u2, float v2, float u3, float v3, float u4, float
		// v4);
		((void (*)(uintptr_t, RECT *, uint32_t *, float, float, float, float, float, float, float, float))(g_libGTASA + 0x5526CC + 1)) (texture, rect, &dwColor, uv[0], uv[1], uv[2], uv[3], uv[4], uv[5], uv[6], uv[7]);
	}
}

int GameGetWeaponModelIDFromWeaponID(int iWeaponID)
{
	switch(iWeaponID)
	{
	case WEAPON_BRASSKNUCKLE:
		return WEAPON_MODEL_BRASSKNUCKLE;

	case WEAPON_GOLFCLUB:
		return WEAPON_MODEL_GOLFCLUB;

	case WEAPON_NITESTICK:
		return WEAPON_MODEL_NITESTICK;

	case WEAPON_KNIFE:
		return WEAPON_MODEL_KNIFE;

	case WEAPON_BAT:
		return WEAPON_MODEL_BAT;

	case WEAPON_SHOVEL:
		return WEAPON_MODEL_SHOVEL;

	case WEAPON_POOLSTICK:
		return WEAPON_MODEL_POOLSTICK;

	case WEAPON_KATANA:
		return WEAPON_MODEL_KATANA;

	case WEAPON_CHAINSAW:
		return WEAPON_MODEL_CHAINSAW;

	case WEAPON_DILDO:
		return WEAPON_MODEL_DILDO;

	case WEAPON_DILDO2:
		return WEAPON_MODEL_DILDO2;

	case WEAPON_VIBRATOR:
		return WEAPON_MODEL_VIBRATOR;

	case WEAPON_VIBRATOR2:
		return WEAPON_MODEL_VIBRATOR2;

	case WEAPON_FLOWER:
		return WEAPON_MODEL_FLOWER;

	case WEAPON_CANE:
		return WEAPON_MODEL_CANE;

	case WEAPON_GRENADE:
		return WEAPON_MODEL_GRENADE;

	case WEAPON_TEARGAS:
		return WEAPON_MODEL_TEARGAS;

	case WEAPON_MOLTOV:
		return WEAPON_MODEL_MOLOTOV;

	case WEAPON_COLT45:
		return WEAPON_MODEL_COLT45;

	case WEAPON_SILENCED:
		return WEAPON_MODEL_SILENCED;

	case WEAPON_DEAGLE:
		return WEAPON_MODEL_DEAGLE;

	case WEAPON_SHOTGUN:
		return WEAPON_MODEL_SHOTGUN;

	case WEAPON_SAWEDOFF:
		return WEAPON_MODEL_SAWEDOFF;

	case WEAPON_SHOTGSPA:
		return WEAPON_MODEL_SHOTGSPA;

	case WEAPON_UZI:
		return WEAPON_MODEL_UZI;

	case WEAPON_MP5:
		return WEAPON_MODEL_MP5;

	case WEAPON_AK47:
		return WEAPON_MODEL_AK47;

	case WEAPON_M4:
		return WEAPON_MODEL_M4;

	case WEAPON_TEC9:
		return WEAPON_MODEL_TEC9;

	case WEAPON_RIFLE:
		return WEAPON_MODEL_RIFLE;

	case WEAPON_SNIPER:
		return WEAPON_MODEL_SNIPER;

	case WEAPON_ROCKETLAUNCHER:
		return WEAPON_MODEL_ROCKETLAUNCHER;

	case WEAPON_HEATSEEKER:
		return WEAPON_MODEL_HEATSEEKER;

	case WEAPON_FLAMETHROWER:
		return WEAPON_MODEL_FLAMETHROWER;

	case WEAPON_MINIGUN:
		return WEAPON_MODEL_MINIGUN;

	case WEAPON_SATCHEL:
		return WEAPON_MODEL_SATCHEL;

	case WEAPON_BOMB:
		return WEAPON_MODEL_BOMB;

	case WEAPON_SPRAYCAN:
		return WEAPON_MODEL_SPRAYCAN;

	case WEAPON_FIREEXTINGUISHER:
		return WEAPON_MODEL_FIREEXTINGUISHER;

	case WEAPON_CAMERA:
		return WEAPON_MODEL_CAMERA;

	case WEAPON_NIGHTVISION:
		return WEAPON_MODEL_NIGHTVISION;

	case WEAPON_INFRARED:
		return WEAPON_MODEL_INFRARED;

	case WEAPON_PARACHUTE:
		return WEAPON_MODEL_PARACHUTE;

	case WEAPON_JETPACK:
		WEAPON_MODEL_JETPACK;
	}

	return -1;
}

const char* GetWeaponName(int iWeaponID)
{
	switch (iWeaponID) 
	{
	case WEAPON_FIST:
		return "Fist";
	case WEAPON_BRASSKNUCKLE:
		return "Brass Knuckles";
	case WEAPON_GOLFCLUB:
		return "Golf Club";
	case WEAPON_NITESTICK:
		return "Nite Stick";
	case WEAPON_KNIFE:
		return "Knife";
	case WEAPON_BAT:
		return "Baseball Bat";
	case WEAPON_SHOVEL:
		return "Shovel";
	case WEAPON_POOLSTICK:
		return "Pool Cue";
	case WEAPON_KATANA:
		return "Katana";
	case WEAPON_CHAINSAW:
		return "Chainsaw";
	case WEAPON_DILDO:
		return "Dildo";
	case WEAPON_DILDO2:
		return "Dildo";
	case WEAPON_VIBRATOR:
		return "Vibrator";
	case WEAPON_VIBRATOR2:
		return "Vibrator";
	case WEAPON_FLOWER:
		return "Flowers";
	case WEAPON_CANE:
		return "Cane";
	case WEAPON_GRENADE:
		return "Grenade";
	case WEAPON_TEARGAS:
		return "Teargas";
	case WEAPON_MOLTOV:
		return "Molotov";
	case WEAPON_COLT45:
		return "Colt 45";
	case WEAPON_SILENCED:
		return "Silenced Pistol";
	case WEAPON_DEAGLE:
		return "Desert Eagle";
	case WEAPON_SHOTGUN:
		return "Shotgun";
	case WEAPON_SAWEDOFF:
		return "Sawn-off Shotgun";
	case WEAPON_SHOTGSPA: // wtf? 
		return "Combat Shotgun";
	case WEAPON_UZI:
		return "UZI";
	case WEAPON_MP5:
		return "MP5";
	case WEAPON_AK47:
		return "AK47";
	case WEAPON_M4:
		return "M4";
	case WEAPON_TEC9:
		return "TEC9";
	case WEAPON_RIFLE:
		return "Rifle";
	case WEAPON_SNIPER:
		return "Sniper Rifle";
	case WEAPON_ROCKETLAUNCHER:
		return "Rocket Launcher";
	case WEAPON_HEATSEEKER:
		return "Heat Seaker";
	case WEAPON_FLAMETHROWER:
		return "Flamethrower";
	case WEAPON_MINIGUN:
		return "Minigun";
	case WEAPON_SATCHEL:
		return "Satchel Explosives";
	case WEAPON_BOMB:
		return "Bomb";
	case WEAPON_SPRAYCAN:
		return "Spray Can";
	case WEAPON_FIREEXTINGUISHER:
		return "Fire Extinguisher";
	case WEAPON_CAMERA:
		return "Camera";
	case WEAPON_PARACHUTE:
		return "Parachute";
	case WEAPON_VEHICLE:
		return "Vehicle";
	case WEAPON_HELIBLADES:
		return "Heli blades";
	case WEAPON_EXPLOSION:
		return "Explosion";
	case WEAPON_DROWN:
		return "Drowned";
	case WEAPON_COLLISION:
		return "Splat";
	case WEAPON_UNKNOWN:
		return "Suicide";
	}

	return "Unknown";
}

int Weapon_FireSniper(WEAPON_SLOT_TYPE* pWeaponSlot, PED_TYPE* pPed)
{
	return ((int (*)(WEAPON_SLOT_TYPE*, PED_TYPE*))(g_libGTASA + 0x0056668C + 1))(pWeaponSlot, pPed);
}

uintptr_t GetWeaponInfo(int iWeapon, int iSkill)
{
	// CWeaponInfo::GetWeaponInfo
	return ((uintptr_t (*)(int, int))(g_libGTASA + 0x0056BD60 + 1))(iWeapon, iSkill);
}

bool IsValidPosition(VECTOR const& vecPosition) 
{
    if (vecPosition.X < -16000 || vecPosition.X > 16000 || std::isnan(vecPosition.X) || 
		vecPosition.Y < -16000 || vecPosition.Y > 16000 || std::isnan(vecPosition.Y) || 
		vecPosition.Z < -5000 || vecPosition.Z > 100000 || std::isnan(vecPosition.Z))
        return false;

    return true;
}

int GetWeaponSkill(PED_TYPE* pPed)
{
	// CWeaponInfo::GetWeaponSkill
	return ((int (*)(PED_TYPE*))(g_libGTASA+0x434F24+1))(pPed);
}
