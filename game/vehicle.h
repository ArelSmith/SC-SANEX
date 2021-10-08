#pragma once

#pragma pack(1)
enum eLightStatus 
{
	DT_LIGHT_OK = 0,
	DT_LIGHT_SMASHED
};

enum eDoors
{
	BONNET = 0,
	BOOT,
	FRONT_LEFT_DOOR,
	FRONT_RIGHT_DOOR,
	REAR_LEFT_DOOR,
	REAR_RIGHT_DOOR,

	MAX_DOORS
};

enum eWheels
{
	FRONT_LEFT_WHEEL = 0,
	REAR_LEFT_WHEEL,
	FRONT_RIGHT_WHEEL,
	REAR_RIGHT_WHEEL,

	MAX_WHEELS

};

enum ePanels
{
	FRONT_LEFT_PANEL = 0,
	FRONT_RIGHT_PANEL,
	REAR_LEFT_PANEL,
	REAR_RIGHT_PANEL,
	WINDSCREEN_PANEL,	// needs to be in same order as in component.h
	FRONT_BUMPER,
	REAR_BUMPER,

	MAX_PANELS		// MUST BE 8 OR LESS
};

enum eLights
{
	// these have to correspond to their respective panels
	LEFT_HEADLIGHT = 0,
	RIGHT_HEADLIGHT,
	LEFT_TAIL_LIGHT,
	RIGHT_TAIL_LIGHT,
/*	LEFT_BRAKE_LIGHT,
	RIGHT_BRAKE_LIGHT,
	FRONT_LEFT_INDICATOR,
	FRONT_RIGHT_INDICATOR,
	REAR_LEFT_INDICATOR,
	REAR_RIGHT_INDICATOR,*/

	MAX_LIGHTS			// MUST BE 16 OR LESS
};

class CVehicle : public CEntity
{
public:
	CVehicle(int iType, float fPosX, float fPosY, float fPosZ, float fRotation = 0.0f, bool bSiren = false);
	~CVehicle();

	void LinkToInterior(int iInterior);
	void SetColor(int iColor1, int iColor2);

	void SetHealth(float fHealth);
	float GetHealth();

	// 0.3.7
	bool IsOccupied();

	// 0.3.7
	void SetInvulnerable(bool bInv);
	// 0.3.7
	bool IsDriverLocalPlayer();
	// 0.3.7
	bool HasSunk();

	void ProcessMarkers();

	void RemoveEveryoneFromVehicle();

	void SetObjState(uint8_t byteState);
	void SetDoorState(int iState);
	int GetDoorState();
	void SetDoorOpenFlag(uint8_t byteState, int iCurrentDoorId);
	void SetEngineState(int iState);
	void SetLightsState(int iState);
	void SetAlarmState(int iState);
	void SetSirenState(bool iState);
	bool IsSirenOn();

	void UpdateDamageStatus(uint32_t dwPanelDamage, uint32_t dwDoorDamage, uint8_t byteLightDamage, uint8_t byteTire);
	uint32_t GetPanelDamageStatus();
	uint32_t GetDoorDamageStatus();
	uint16_t GetLightDamageStatus();
	uint16_t GetWheelPoppedStatus();

	unsigned int GetVehicleSubtype();

	void AttachTrailer();
	void DetachTrailer();
	void SetTrailer(CVehicle *pTrailer);
	CVehicle* GetTrailer();

	bool IsATrainPart();
	float GetTrainSpeed();
	void SetTrainSpeed(float fSpeed);

public:
	VEHICLE_TYPE	*m_pVehicle;
	bool 			m_bIsLocked;
	CVehicle		*m_pTrailer;
	uint32_t		m_dwMarkerID;
	bool 			m_bIsInvulnerable;
	bool 			m_bDoorsLocked;
	bool 			m_bIsEngineOn;
	bool 			m_bIsLightsOn;
	uint8_t			m_byteObjectiveVehicle; // Is this a special objective vehicle? 0/1
	uint8_t			m_bSpecialMarkerEnabled;

	uint8_t			m_byteColor1;
	uint8_t			m_byteColor2;
	bool 			m_bColorChanged;

	uint32_t 		m_dwPanelDamage;
	uint32_t 		m_dwDoorDamage;
	uint8_t 		m_byteLightDamage;
	uint8_t 		m_byteTireDamage;
};