#pragma once

PED_TYPE* GamePool_FindPlayerPed();
PED_TYPE* GamePool_Ped_GetAt(int iID);
int GamePool_Ped_GetIndex(PED_TYPE *pActor);

VEHICLE_TYPE *GamePool_Vehicle_GetAt(int iID);
int GamePool_Vehicle_GetIndex(VEHICLE_TYPE *pVehicle);

ENTITY_TYPE *GamePool_Object_GetAt(int iID);

int LineOfSight(VECTOR* start, VECTOR* end, void* colpoint, uintptr_t ent, char buildings, char vehicles, char peds, char objects, char dummies, bool seeThrough, bool camera, bool unk);

bool IsPedModel(unsigned int uiModel);
bool IsValidModel(unsigned int uiModelID);
bool IsTrueModelID(unsigned int uiModelID);

uint16_t GetModelReferenceCount(int nModelIndex);
uintptr_t GetModelInfoByID(int iModelID);
uintptr_t ModelInfoCreateInstance(int iModel);

void InitPlayerPedPtrRecords();
void SetPlayerPedPtrRecord(uint8_t bytePlayer, uintptr_t dwPedPtr);
uint8_t FindPlayerNumFromPedPtr(uintptr_t dwPedPtr);
void GamePrepareTrain(VEHICLE_TYPE *pVehicle);
uintptr_t FindRwTexture(std::string szTexDb, std::string szTexName);
uintptr_t GetTexture(std::string szTexName);
uintptr_t LoadTextureFromDB(const char* dbname, const char* texture);
uintptr_t LoadTexture(const char *texname);
uintptr_t LoadTexture(const char *txdname, const char *texname);

void DefinedState2d();
void SetScissorRect(void* pRect);
float DegToRad(float fDegrees);
// 0.3.7
float FloatOffset(float f1, float f2);

bool IsGameEntityArePlaceable(ENTITY_TYPE *pEntity);
bool IsValidGamePed(PED_TYPE *pPed);

void RwMatrixRotate(MATRIX4X4 *mat, int axis, float angle);
void ProjectMatrix(VECTOR* vecOut, MATRIX4X4* mat, VECTOR *vecPos);
void RwMatrixScale(MATRIX4X4 *matrix, VECTOR *vecScale);
void RwMatrixOrthoNormalize(MATRIX4X4 *matIn, MATRIX4X4 *matOut);
void RwMatrixInvert(MATRIX4X4 *matOut, MATRIX4X4 *matIn);

void WorldAddEntity(uintptr_t pEnt);
void WorldRemoveEntity(uintptr_t pEnt);

void RenderClumpOrAtomic(uintptr_t rwObject);
float GetModelColSphereRadius(int iModel);
void GetModelColSphereVecCenter(int iModel, VECTOR * vec);
void DestroyAtomicOrClump(uintptr_t rwObject);

bool IsPointInRect(float x, float y, RECT* rect);
int GetFreeTextDrawTextureSlot();
void DestroyTextDrawTexture(int index);
void DrawTextureUV(uintptr_t texture, RECT * rect, uint32_t dwColor, float *uv);

int GameGetWeaponModelIDFromWeaponID(int iWeaponID);
const char* GetWeaponName(int iWeaponID);
int Weapon_FireSniper(WEAPON_SLOT_TYPE* pWeaponSlot, PED_TYPE* pPed);
uintptr_t GetWeaponInfo(int iWeapon, int iSkill);

bool IsValidPosition(VECTOR const& vecPosition);
int GetWeaponSkill(PED_TYPE* pPed);