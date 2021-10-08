#include "main.h"
#include "game.h"
#include "net/netgame.h"
#include "materialtext.h"
#include <cmath>

extern CGame *pGame;
extern CNetGame *pNetGame;
extern CMaterialText *pMaterialText;

CObject::CObject(int iModel, float fPosX, float fPosY, float fPosZ, VECTOR vecRot, float fDrawDistance)
{
	uint32_t dwRetID = 0;
	m_pEntity = 0;
	m_dwGTAId = 0;

	ScriptCommand(&create_object, iModel, fPosX, fPosY, fPosZ, &dwRetID);
	ScriptCommand(&put_object_at, dwRetID, fPosX, fPosY, fPosZ);

	m_pEntity = GamePool_Object_GetAt(dwRetID);
	m_dwGTAId = dwRetID;
	m_byteMovingPosition = 0;
	m_byteMovingRotation = 0;
	m_fMoveSpeed = 0.0;

	m_bIsPlayerSurfing = false;

	InstantRotate(vecRot.X, vecRot.Y, vecRot.Z);

	for(int i = 0; i <= MAX_MATERIALS_PER_MODEL; i++)
	{
		m_MaterialTexture[i] = 0;
		m_dwMaterialColor[i] = 0;
	}

	m_bHasMaterial = false;
	m_bIsMaterialtext = false;
}

CObject::~CObject()
{
	m_pEntity = GamePool_Object_GetAt(m_dwGTAId);
	if(m_pEntity)
		ScriptCommand(&destroy_object, m_dwGTAId);
}

void CObject::Process(float fElapsedTime)
{
	if(m_byteMovingPosition) 
	{
		MATRIX4X4 matPos;
		GetMatrix(&matPos);
		
		float fDistance = fElapsedTime * m_fMoveSpeed;
		float fRemainDist = DistanceRemaining(&matPos, &m_matPositionTarget);
		if(fDistance >= fRemainDist) 
		{
			m_byteMovingPosition = 0;
			TeleportTo(m_matPositionTarget.pos.X, m_matPositionTarget.pos.Y, m_matPositionTarget.pos.Z);
		} 
		else 
		{
			fRemainDist /= fDistance;
			matPos.pos.X += (m_matPositionTarget.pos.X - matPos.pos.X) / fRemainDist;
			matPos.pos.Y += (m_matPositionTarget.pos.Y - matPos.pos.Y) / fRemainDist;
			matPos.pos.Z += (m_matPositionTarget.pos.Z - matPos.pos.Z) / fRemainDist;
			TeleportTo(matPos.pos.X, matPos.pos.Y, matPos.pos.Z);
		}
	}

	if(m_byteMovingRotation) 
	{
		MATRIX4X4 matPos;
		GetMatrix(&matPos);
		
		float fDistance = fElapsedTime * m_fMoveSpeed;
		float fRemainDist = DistanceRemaining(&matPos, &m_matRotationTarget);
		if(fDistance >= fRemainDist) 
		{
			m_byteMovingRotation = 0;
			InstantRotate(m_matRotationTarget.pos.X, m_matRotationTarget.pos.Y, m_matRotationTarget.pos.Z);
		} 
		else 
		{
			fRemainDist /= fDistance;
			matPos.pos.X += (m_matRotationTarget.pos.X - matPos.pos.X) / fRemainDist;
			matPos.pos.Y += (m_matRotationTarget.pos.Y - matPos.pos.Y) / fRemainDist;
			matPos.pos.Z += (m_matRotationTarget.pos.Z - matPos.pos.Z) / fRemainDist;
			InstantRotate(matPos.pos.X, matPos.pos.Y, matPos.pos.Z);
		}
	}
}

void CObject::EditObject()
{
	// ��������
}

void CObject::EnterEditObject()
{
	// ��������
}

float CObject::DistanceRemaining(MATRIX4X4 *matCurrent, MATRIX4X4 *matTarget)
{
	float fSX = (matCurrent->pos.X - matTarget->pos.X) * (matCurrent->pos.X - matTarget->pos.X);
	float fSY = (matCurrent->pos.Y - matTarget->pos.Y) * (matCurrent->pos.Y - matTarget->pos.Y);
	float fSZ = (matCurrent->pos.Z - matTarget->pos.Z) * (matCurrent->pos.Z - matTarget->pos.Z);
	return (float)sqrt(fSX + fSY + fSZ);
}

void CObject::SetPos(float x, float y, float z)
{
	ScriptCommand(&put_object_at, m_dwGTAId, x, y, z);
}

void CObject::InstantRotate(float x, float y, float z)
{
	ScriptCommand(&set_object_rotation, m_dwGTAId, x, y, z);
}

void CObject::SetMovingSpeed(float speed)
{
	m_fMoveSpeed = speed;
}

void CObject::MovePositionTo(float X, float Y, float Z)
{
	m_matPositionTarget.pos.X = X;
	m_matPositionTarget.pos.Y = Y;
	m_matPositionTarget.pos.Z = Z;
	m_byteMovingPosition |= 1;
}

void CObject::MoveRotationTo(float X, float Y, float Z)
{
	m_matRotationTarget.pos.X = X;
	m_matRotationTarget.pos.Y = Y;
	m_matRotationTarget.pos.Z = Z;
	m_byteMovingRotation |= 1;
}

void CObject::StopMovingObject()
{
	m_byteMovingPosition = 0;
	m_byteMovingRotation = 0;
}

void CObject::SetMaterial(int iModel, uint8_t byteMaterialIndex, char *txdname, char *texturename, uint32_t dwColor)
{
	if(byteMaterialIndex <= MAX_MATERIALS_PER_MODEL)
	{
		if(!txdname || !strlen(txdname) || !texturename || !strlen(texturename)) return;

		if(m_MaterialTexture[byteMaterialIndex]) 
		{
			//DeleteRwTexture(m_MaterialTexture[byteMaterialIndex]);
			m_MaterialTexture[byteMaterialIndex] = 0;
		}

		// create object texture
		uintptr_t pRwTexture;

		// what does people name the txd or texture to "none"
		// make a object to not visible and only have a collision?
		if(!strcmp(txdname, "none") || !strcmp(texturename, "none"))
			pRwTexture = (uintptr_t)pMaterialText->Generate(32, 32, "arial", 30, false, 0, 0, 0, "");
		else pRwTexture = (uintptr_t)LoadTexture(texturename);
		if(!pRwTexture)
			return;

		// set object material
		m_MaterialTexture[byteMaterialIndex] = pRwTexture;
		m_dwMaterialColor[byteMaterialIndex] = dwColor;
		m_bHasMaterial = true;
	}
}

void CObject::SetMaterialText(uint8_t byteMaterialIndex, uint8_t byteMaterialSize, const char *szFontName, uint8_t byteFontSize, uint8_t byteFontBold, uint32_t dwFontColor, uint32_t dwBackgroundColor, uint8_t byteAlign, const char *szText)
{
	if(byteMaterialIndex <= MAX_MATERIALS_PER_MODEL)
	{
		if(!szText || !strlen(szText)) return;

		if(m_MaterialTexture[byteMaterialIndex]) 
		{
			//DeleteRwTexture(m_MaterialTexture[byteMaterialIndex]);
			m_MaterialTexture[byteMaterialIndex] = 0;
		}

		m_byteMaterialIndex = byteMaterialIndex;
		m_byteMaterialSize = byteMaterialSize;
		memset(m_szFontName, 0, 32);
		strncpy(m_szFontName, szFontName, 32);
		m_szFontName[32] = 0;
		m_byteFontSize = byteFontSize;
		m_byteFontBold = byteFontBold;
		m_dwFontColor = dwFontColor;
		m_dwBackgroundColor = dwBackgroundColor;
		m_byteAlign = byteAlign;
		memset(m_szText, 0, 2048);
		strncpy(m_szText, szText, 2048);
		m_szText[2048] = 0;

		m_bHasMaterial = false;
		m_bIsMaterialtext = true;
	}
}

void CObject::MaterialTextProcess()
{
	if(m_bHasMaterial || !m_bIsMaterialtext)
		return;

	if(!m_szText || !strlen(m_szText)) return;

	// materialsize
	if(m_byteMaterialSize < 10)
		m_byteMaterialSize = 10;
	else if(m_byteMaterialSize > 140)
		m_byteMaterialSize = 140;

	// align
	if(m_byteAlign < OBJECT_MATERIAL_TEXT_ALIGN_LEFT || m_byteAlign > OBJECT_MATERIAL_TEXT_ALIGN_RIGHT)
		m_byteAlign = OBJECT_MATERIAL_TEXT_ALIGN_CENTER;
		
	// get material size XY
	static uint16_t sizes[14][2] = {
		{ 32, 32 } , { 64, 32 }, { 64, 64 }, { 128, 32 }, { 128, 64 }, { 128,128 }, { 256, 32 },
		{ 256, 64 } , { 256, 128 } , { 256, 256 } , { 512, 64 } , { 512,128 } , { 512,256 } , { 512,512 } 
	};
	int del = (m_byteMaterialSize / 10) - 1;
	int iSizeX = sizes[del][0];
	int iSizeY = sizes[del][1];

	// correct font size
	m_byteFontSize = m_byteFontSize * 0.75;

	// create text texture
	uintptr_t pRwTexture = (uintptr_t)pMaterialText->Generate(iSizeX, iSizeY, m_szFontName, m_byteFontSize, m_byteFontBold, m_dwFontColor, m_dwBackgroundColor, m_byteAlign, m_szText);
	if(!pRwTexture)
		return;

	// set object material text
	m_MaterialTexture[m_byteMaterialIndex] = pRwTexture;
	m_bHasMaterial = true;
	m_bIsMaterialtext = false;
}