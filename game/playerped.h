#pragma once
#include "object.h"

#define FALL_SKYDIVE		1
#define FALL_SKYDIVE_ACCEL	2
#define PARA_DECEL			3
#define PARA_FLOAT			4

class CPlayerPed : public CEntity
{
public:
	CPlayerPed();	// local
	CPlayerPed(uint8_t bytePlayerNumber, int iSkin, float fX, float fY, float fZ, float fRotation); // remote
	~CPlayerPed();

	void Destroy();

	// 0.3.7
	bool IsInVehicle();
	// 0.3.7
	bool IsAPassenger();
	// 0.3.7
	VEHICLE_TYPE* GetGtaVehicle();
	// 0.3.7
	void RemoveFromVehicleAndPutAt(float fX, float fY, float fZ);
	// 0.3.7
	void SetInitialState();
	void ToggleCellphone(int iOn);
	int IsCellphoneEnabled();
	bool IsInJetpackMode();
	void StartJetpack();
	void StopJetpack();
	int HasHandsUp();
	void HandsUp();
	void PlayDance(int danceId);
	void StopDancing();
	bool IsDancing();
	
	// 0.3.7
	void SetHealth(float fHealth);
	void SetArmour(float fArmour);
	// 0.3.7
	float GetHealth();
	float GetArmour();
	// 0.3.7
	void TogglePlayerControllable(bool bToggle);
	void TogglePlayerControllableWithoutLock(bool bToggle);
	// 0.3.7
	void SetModelIndex(unsigned int uiModel);

	void SetInterior(uint8_t byteID);

	void PutDirectlyInVehicle(int iVehicleID, int iSeat);
	void EnterVehicle(int iVehicleID, bool bPassenger);
	// 0.3.7
	void ExitCurrentVehicle();
	// 0.3.7
	int GetCurrentVehicleID();
	int GetVehicleSeatID();

	WEAPON_SLOT_TYPE* GetCurrentWeaponSlot();
	void GiveWeapon(int iWeaponID, int iAmmo);
	WEAPON_SLOT_TYPE* FindWeaponSlot(int iWeapon);
	void SetAmmo(int iWeapon, int iAmmo);
	void SetArmedWeapon(int iWeaponType);
	uint8_t GetCurrentWeapon();
	void ClearAllWeapons();

	ENTITY_TYPE* GetEntityUnderPlayer();

	// ��������
	void DestroyFollowPedTask();
	// ��������
	void ResetDamageEntity();
	void ShakeCam(int time);
	// 0.3.7
	void RestartIfWastedAt(VECTOR *vecRestart, float fRotation);
	// 0.3.7
	void ForceTargetRotation(float fRotation);
	// 0.3.7
	uint8_t GetActionTrigger();
	// 0.3.7
	bool IsDead();
	void ExtinguishFire();
	uint16_t GetKeys(uint16_t *lrAnalog, uint16_t *udAnalog, uint8_t *additionalKey);
	uint8_t GetExtendedKeys();
	void SetKeys(uint16_t wKeys, uint16_t lrAnalog, uint16_t udAnalog);
	// 0.3.7
	void SetMoney(int iAmount);
	// 0.3.7
	void ShowMarker(uint32_t iMarkerColorID);
	// 0.3.7
	void HideMarker();
	// 0.3.7
	void SetFightingStyle(int iStyle);
	// 0.3.7
	void SetRotation(float fRotation);
	// 0.3.7
	void ApplyAnimation( char *szAnimName, char *szAnimFile, float fT, int opt1, int opt2, int opt3, int opt4, int iUnk );
	// 0.3.7
	void GetBonePosition(int iBoneID, VECTOR* vecOut);
	// roflan
	uint8_t FindDeathReasonAndResponsiblePlayer(PLAYERID *nPlayer);

	void GetTargetRotation();

	PED_TYPE * GetGtaActor() { return m_pPed; };

	void ClumpUpdateAnimations(float step, int flag);
        void SetCameraZoomAndAspect(float fZoom, float fAspectRatio);

	void SetPlayerAimState();
	void ClearPlayerAimState();

	void GetBoneMatrix(MATRIX4X4 *matOut, int iBoneID);

	void UpdateAttachedObject(uint32_t index, uint32_t model, uint32_t bone, VECTOR vecOffset, VECTOR vecRotation, VECTOR vecScale);
	bool GetObjectSlotState(int iObjectIndex);
	void ProcessAttachedObjects();
	bool IsHaveAttachedObject();
	void DeleteAttachedObjects(int index);
	void RemoveAllAttachedObjects();

	void	ProcessParachutes();
	void	ProcessParachuteSkydiving();
	void	ProcessParachuting();
	void	CheckVehicleParachute();

	void SetDead();

	void FireInstant();
	void GetWeaponInfoForFire(int bLeft, VECTOR *vecBone, VECTOR *vecOut);
	VECTOR* GetCurrentWeaponFireOffset();
	void ProcessBulletData(BULLET_DATA *btData);

	void ApplyCrouch();
	void ResetCrouch();
	bool IsCrouching();

	bool StartPassengerDriveByMode(bool bDriveBy);

	CAMERA_AIM *GetCurrentAim();
	void SetCurrentAim(CAMERA_AIM * pAim);
	float GetAimZ();
	void SetAimZ(float fAimZ);
	uint16_t GetCameraMode();
	void SetCameraMode(uint16_t byteCamMode);
	float GetCameraExtendedZoom();
	void SetCameraExtendedZoom(float fZoom);

	ENTITY_TYPE* GetGtaContactEntity();
	VEHICLE_TYPE* GetGtaContactVehicle();

	bool IsOnGround();
	uint32_t GetStateFlags();
	void SetStateFlags(uint32_t dwState);

public:
	PED_TYPE*			m_pPed;
	uint8_t				m_bytePlayerNumber;
	uint32_t			m_dwArrow;

	ATTACHED_OBJECT 	m_AttachedObjectInfo[MAX_PLAYER_ATTACHED];
	bool 				m_bObjectSlotUsed[MAX_PLAYER_ATTACHED];
	CObject* 			m_pAttachedObjects[MAX_PLAYER_ATTACHED];

	bool				m_bGoggleState;

	int					m_iDanceState;
	int					m_iDanceStyle;
	int					m_iLastDanceMove;
	int					m_iDancingAnim;

	int					m_iParachuteState;
	uint8_t				m_dwParachuteObject;
	int					m_iParachuteAnim;

	bool 				m_iJetpackState;
	bool				m_iParachuteButton;

	int					m_iCellPhoneEnabled;

	int					m_iHandsUp;

	bool				m_bHaveBulletData;
	BULLET_DATA 		m_bulletData;
};
