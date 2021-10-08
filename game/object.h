#pragma once

typedef struct _ATTACHED_OBJECT
{
	int iModel;
	int iBoneID;
	VECTOR vecOffset;
	VECTOR vecRotation;
	VECTOR vecScale;
	//uint32_t dwMaterialColor1;
	//uint32_t dwMaterialColor2;
} ATTACHED_OBJECT;

class CObject : public CEntity
{
public:
	MATRIX4X4	m_matPositionTarget;
	uint8_t		m_byteMovingPosition;
	
	MATRIX4X4	m_matRotationTarget;
	uint8_t		m_byteMovingRotation;

	float		m_fMoveSpeed;
	bool		m_bIsPlayerSurfing;

	CObject(int iModel, float fPosX, float fPosY, float fPosZ, VECTOR vecRot, float fDrawDistance);
	~CObject();

	void Process(float fElapsedTime);
	float DistanceRemaining(MATRIX4X4 *matCurrent, MATRIX4X4 *matTarget);
	
	void EditObject();
	void EnterEditObject();

	void SetPos(float x, float y, float z);
	void InstantRotate(float x, float y, float z);

	void SetMovingSpeed(float speed);
	void MovePositionTo(float x, float y, float z);
	void MoveRotationTo(float X, float Y, float Z);
	void StopMovingObject();

	void SetMaterial(int iModel, uint8_t byteMaterialIndex, char *txdname, char *texturename, uint32_t dwColor);
	void SetMaterialText(uint8_t byteMaterialIndex, uint8_t byteMaterialSize, const char *szFontName, uint8_t byteFontSize, uint8_t byteFontBold, uint32_t dwFontColor, uint32_t dwBackgroundColor, uint8_t byteAlign, const char *szText);
	void MaterialTextProcess();

public:
	// material attribute
	uintptr_t 		m_MaterialTexture[16];
	uint32_t 		m_dwMaterialColor[16];
	bool 			m_bHasMaterial;
	bool			m_bIsMaterialtext;

private:
	// material index
	uint8_t 		m_byteMaterialIndex;

	// material text
	uint8_t 		m_byteMaterialSize;
	char 			m_szFontName[32]; 
	uint8_t 		m_byteFontSize; 
	uint8_t 		m_byteFontBold;
	uint32_t 		m_dwFontColor; 
	uint32_t 		m_dwBackgroundColor; 
	uint8_t 		m_byteAlign; 
	char 			m_szText[2048];
};