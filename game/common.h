#pragma once

#define PLAYER_PED_SLOTS			120
#define MAX_PLAYER_ATTACHED 		10
#define MAX_REMOVEBUILDING_COUNT 	1024
#define MAX_TEXT_DRAW_LINE 			1024
#define MAX_MATERIALS_PER_MODEL		15

#define EVENT_TYPE_PAINTJOB 1
#define EVENT_TYPE_CARCOMPONENT 2
#define EVENT_TYPE_CARCOLOR 3
#define EVENT_ENTEREXIT_MODSHOP 4

#define OBJECT_MATERIAL						1
#define OBJECT_MATERIAL_TEXT				2

#define OBJECT_MATERIAL_TEXT_ALIGN_LEFT		0
#define OBJECT_MATERIAL_TEXT_ALIGN_CENTER	1
#define OBJECT_MATERIAL_TEXT_ALIGN_RIGHT	2

typedef unsigned short VEHICLEID;
typedef unsigned short PLAYERID;

#define PADDING(x,y) uint8_t x[y]
#define IN_VEHICLE(x) ((x->dwStateFlags & 0x100) >> 8)
#define IS_VEHICLE_MOD(x) (x >= 1000 && x <= 1193)
#define IS_CROUCHING(x) ((x->dwStateFlags >> 26) & 1)

enum eStreamingFlags {
    GAME_REQUIRED = 0x2,
    MISSION_REQUIRED = 0x4,
    KEEP_IN_MEMORY = 0x8,
    PRIORITY_REQUEST = 0x10
};

//-----------------------------------------------------------

#pragma pack(1)
typedef struct _GTA_CONTROLSET
{
	uint16_t wKeys1[24];
	uint16_t wKeys2[24];
	uint8_t bytePadding1[212];
        uint16_t 	wWalkLR;
	uint16_t 	wWalkUD;
	uint16_t	wSteeringLR;
	uint16_t	wSteeringUD;
	uint16_t 	wAccelerate;
	uint16_t	wBrake;
	uint16_t 	wHandBrake;
	uint8_t		bDuckJustDown;
	bool		bDuckState;
	uint8_t		bJumpJustDown;
	bool		bJumpState;
	uint8_t		bSprintJustDown;
	uint8_t		bMeleeAttackJustDown;
	bool		bMeleeAttackState;
} GTA_CONTROLSET;

#pragma pack(1)
typedef struct _VECTOR 
{
	float X,Y,Z;
} VECTOR, *PVECTOR;

typedef struct _RECT
{
	float fLeft;
	float fBottom;
	float fRight;
	float fTop;
} RECT, *PRECT;


#pragma pack(1)
typedef struct _MATRIX4X4 
{
	VECTOR right;		// 0-12 	; r11 r12 r13
	uint32_t  flags;	// 12-16
	VECTOR up;			// 16-28	; r21 r22 r23
	float  pad_u;		// 28-32
	VECTOR at;			// 32-44	; r31 r32 r33
	float  pad_a;		// 44-48
	VECTOR pos;			// 48-60
	float  pad_p;		// 60-64
} MATRIX4X4, *PMATRIX4X4;

//-----------------------------------------------------------

#pragma pack(1)
typedef struct _ENTITY_TYPE
{
	uint32_t vtable;
	PADDING(_pad0, 12);
	float fRotZBeforeMat;
	MATRIX4X4 *mat;
	
	union {
		uintptr_t m_pRwObject;
		uintptr_t m_pRpClump;
		uintptr_t m_pRpAtomic;
	};
	
	union {
		uint32_t m_nEntityFlags;
		struct {
			uint32_t m_bUsesCollision : 1;
			uint32_t m_bCollisionProcessed : 1;
			uint32_t m_bIsStatic : 1;
			uint32_t m_bHasContacted : 1;
			uint32_t m_bIsStuck : 1;
			uint32_t m_bIsInSafePosition : 1;
			uint32_t m_bWasPostponed : 1;
			uint32_t m_bIsVisible : 1;

			uint32_t m_bIsBIGBuilding : 1;
			uint32_t m_bRenderDamaged : 1;
			uint32_t m_bStreamingDontDelete : 1;
			uint32_t m_bRemoveFromWorld : 1;
			uint32_t m_bHasHitWall : 1;
			uint32_t m_bImBeingRendered : 1;
			uint32_t m_bDrawLast : 1;
			uint32_t m_bDistanceFade : 1;
		 
			uint32_t m_bDontCastShadowsOn : 1;
			uint32_t m_bOffscreen : 1;
			uint32_t m_bIsStaticWaitingForCollision : 1;
			uint32_t m_bDontStream : 1;
			uint32_t m_bUnderwater : 1;
			uint32_t m_bHasPreRenderEffects : 1;
			uint32_t m_bIsTempBuilding : 1;
			uint32_t m_bDontUpdateHierarchy : 1;
		 
			uint32_t m_bHasRoadsignText : 1;
			uint32_t m_bDisplayedSuperLowLOD : 1;
			uint32_t m_bIsProcObject : 1;
			uint32_t m_bBackfaceCulled : 1;
			uint32_t m_bLightObject : 1;
			uint32_t m_bUnimportantStream : 1;
			uint32_t m_bTunnel : 1;
			uint32_t m_bTunnelTransition : 1;
		} nEntityFlags;
	};
	
	PADDING(_pad1, 2);
	uint16_t nModelIndex;
	PADDING(_pad2, 32);
	VECTOR vecMoveSpeed;
	VECTOR vecTurnSpeed;
	PADDING(_pad3, 88);
	uint32_t dwUnkModelRel;
} ENTITY_TYPE;

//-----------------------------------------------------------

typedef struct _WEAPON_SLOT_TYPE
{
	uint32_t dwType;
	uint32_t dwState;
	uint32_t dwAmmoInClip;
	uint32_t dwAmmo;
	PADDING(_pwep1, 12);
} WEAPON_SLOT_TYPE;  // MUST BE EXACTLY ALIGNED TO 28 bytes

typedef struct
{
	char unk[0x14];
	int iNodeId;
} AnimBlendFrameData;

#pragma pack(1)
typedef struct _PED_TYPE
{
	ENTITY_TYPE entity; 				// 0000-0184	;entity
	PADDING(_pad106, 174);				// 0184-0358
	uint32_t _pad107;					// 0358-0362	;dwPedType
	PADDING(_pad101, 734);				// 0362-1096
	uint32_t dwAction;					// 1096-1100	;Action
	PADDING(_pad102, 36);				// 1100-1136
	uintptr_t dwInvulFlags; 			// 1136-1140	0x1000 = can_decap
	PADDING(_pad228, 8); 				// 1140-1148
	uintptr_t Tasks; 					// 1148-1152
	uint32_t dwStateFlags; 				// 1152-1156	;StateFlags
	PADDING(_pad103, 12);				// 1156-1168
	AnimBlendFrameData* aPedBones[19];	// 1168-1244
	PADDING(_pad103_, 100);				// 1244-1344
	float fHealth;		 				// 1344-1348	;Health
	float fMaxHealth;					// 1348-1352	;MaxHealth
	float fArmour;						// 1352-1356	;Armour
	float fAim;							// 1356-1360	;Aim
	PADDING(_pad104, 8);				// 1360-1368
	float fRotation1;					// 1368-1372	;Rotation1
	float fRotation2;					// 1372-1376	;Rotation2
	PADDING(_pad292, 8);				// 1376-1384
	uint32_t pContactVehicle; 			// 1384-1388
	PADDING(_pad222, 24);				// 1388-1412
	uint32_t pContactEntity; 			// 1412-1416
	PADDING(_pad105, 4);				// 1416-1420
	uint32_t pVehicle;					// 1420-1424	;pVehicle
	PADDING(_pad108, 8);				// 1424-1432
	uint32_t dwPedType;					// 1432-1436	;dwPedType
	uint32_t dwUnk1;	 				// 1436-1440
	WEAPON_SLOT_TYPE WeaponSlots[13]; 	// 1440-1804
	PADDING(_pad270, 12); 				// 1804-1816
	uint8_t byteCurWeaponSlot; 			// 1816-1817
	PADDING(_pad280, 23); 				// 1817-1840
	uint32_t pFireObject;	 			// 1840-1844
	PADDING(_pad281, 44);		 		// 1844-1888
	uint32_t dwWeaponUsed; 				// 1888-1892
	uintptr_t pdwDamageEntity; 			// 1892-1896
	PADDING(_pad290,52); 				// 1896-1948
	uint32_t pTarget; 					// 1948-1952
} PED_TYPE;

//-----------------------------------------------------------

#pragma pack(1)
typedef struct _BULLET_DATA 
{
	uint32_t unk;
	VECTOR vecOrigin;
	VECTOR vecPos;
	VECTOR vecOffset;
	ENTITY_TYPE* pEntity;
} BULLET_DATA;

#pragma pack(1)
typedef struct _VEHICLE_TYPE
{
	ENTITY_TYPE entity;					// 0000-0184
	PADDING(_pad201, 880);				// 0184-1064

	struct {
		unsigned char bIsLawEnforcer : 1;
		unsigned char bIsAmbulanceOnDuty : 1;
		unsigned char bIsFireTruckOnDuty : 1;
		unsigned char bIsLocked : 1;
		unsigned char bEngineOn : 1;
		unsigned char bIsHandbrakeOn : 1;
		unsigned char bLightsOn : 1;
		unsigned char bFreebies : 1;

		unsigned char bIsVan : 1;
		unsigned char bIsBus : 1;
		unsigned char bIsBig : 1;
		unsigned char bLowVehicle : 1;
		unsigned char bComedyControls : 1;
		unsigned char bWarnedPeds : 1;
		unsigned char bCraneMessageDone : 1;
		unsigned char bTakeLessDamage : 1;

		unsigned char bIsDamaged : 1;
		unsigned char bHasBeenOwnedByPlayer : 1;
		unsigned char bFadeOut : 1;
		unsigned char bIsBeingCarJacked : 1;
		unsigned char bCreateRoadBlockPeds : 1;
		unsigned char bCanBeDamaged : 1;
		unsigned char bOccupantsHaveBeenGenerated : 1;
		unsigned char bGunSwitchedOff : 1;

		unsigned char bVehicleColProcessed : 1;
		unsigned char bIsCarParkVehicle : 1;
		unsigned char bHasAlreadyBeenRecorded : 1;
		unsigned char bPartOfConvoy : 1;
		unsigned char bHeliMinimumTilt : 1;
		unsigned char bAudioChangingGear : 1;
		unsigned char bIsDrowning : 1;
		unsigned char bTyresDontBurst : 1;

		unsigned char bCreatedAsPoliceVehicle : 1;
		unsigned char bRestingOnPhysical : 1;
		unsigned char bParking : 1;
		unsigned char bCanPark : 1;
		unsigned char bFireGun : 1;
		unsigned char bDriverLastFrame : 1;
		unsigned char bNeverUseSmallerRemovalRange : 1;
		unsigned char bIsRCVehicle : 1;

		unsigned char bAlwaysSkidMarks : 1;
		unsigned char bEngineBroken : 1;
		unsigned char bVehicleCanBeTargetted : 1;
		unsigned char bPartOfAttackWave : 1;
		unsigned char bWinchCanPickMeUp : 1;
		unsigned char bImpounded : 1;
		unsigned char bVehicleCanBeTargettedByHS : 1;
		unsigned char bSirenOrAlarm : 1;

		unsigned char bHasGangLeaningOn : 1;
		unsigned char bGangMembersForRoadBlock : 1;
		unsigned char bDoesProvideCover : 1;
		unsigned char bMadDriver : 1;
		unsigned char bUpgradedStereo : 1;
		unsigned char bConsideredByPlayer : 1;
		unsigned char bPetrolTankIsWeakPoint : 1;
		unsigned char bDisableParticles : 1;

		unsigned char bHasBeenResprayed : 1;
		unsigned char bUseCarCheats : 1;
		unsigned char bDontSetColourWhenRemapping : 1;
		unsigned char bUsedForReplay : 1;
	} nFlags;

	unsigned int nCrationTime;
	uint8_t byteColor1;
	uint8_t byteColor2;	
	PADDING(_pad204, 42);
	PED_TYPE * pDriver;	
	PED_TYPE * pPassengers[8];
	PADDING(_pad205, 12);
	uint32_t pFireObject;
	PADDING(_pad206, 52);
	float fHealth;
	PADDING(_pad207, 4);
	uint32_t dwTrailer;	
	PADDING(_pad208, 48);
	uint32_t dwDoorsLocked;
	PADDING(_pad209, 77);
	uint32_t dwTrainUnk;
	float fTrainSpeed;
} VEHICLE_TYPE;

typedef struct _OBJECT_REMOVE {
	uint32_t dwModel;
	float fRange;
	VECTOR vecPosition;
} OBJECT_REMOVE;

union tScriptParam {
	unsigned int uParam;
	int iParam;
	float fParam;
	void *pParam;
	char *szParam;
};

enum eWeaponSampId {
	WEAPON_FIST,
	WEAPON_BRASSKNUCKLE,
	WEAPON_GOLFCLUB,
	WEAPON_NITESTICK,
	WEAPON_KNIFE,
	WEAPON_BAT,
	WEAPON_SHOVEL,
	WEAPON_POOLSTICK,
	WEAPON_KATANA,
	WEAPON_CHAINSAW,
	WEAPON_DILDO,
	WEAPON_DILDO2,
	WEAPON_VIBRATOR,
	WEAPON_VIBRATOR2,
	WEAPON_FLOWER,
	WEAPON_CANE,
	WEAPON_GRENADE,
	WEAPON_TEARGAS,
	WEAPON_MOLTOV,
	WEAPON_COLT45 = 22,
	WEAPON_SILENCED,
	WEAPON_DEAGLE,
	WEAPON_SHOTGUN,
	WEAPON_SAWEDOFF,
	WEAPON_SHOTGSPA,
	WEAPON_UZI,
	WEAPON_MP5,
	WEAPON_AK47,
	WEAPON_M4,
	WEAPON_TEC9,
	WEAPON_RIFLE,
	WEAPON_SNIPER,
	WEAPON_ROCKETLAUNCHER,
	WEAPON_HEATSEEKER,
	WEAPON_FLAMETHROWER,
	WEAPON_MINIGUN,
	WEAPON_SATCHEL,
	WEAPON_BOMB,
	WEAPON_SPRAYCAN,
	WEAPON_FIREEXTINGUISHER,
	WEAPON_CAMERA,
	WEAPON_NIGHTVISION,
	WEAPON_INFRARED,
	WEAPON_PARACHUTE,
	WEAPON_JETPACK,
	WEAPON_VEHICLE = 49,
	WEAPON_HELIBLADES,
	WEAPON_EXPLOSION,
	WEAPON_DROWN,
	WEAPON_COLLISION,
	WEAPON_SPECIAL_CONNECT = 200,
	WEAPON_SPECIAL_DISCONNECT,
	WEAPON_UNKNOWN = 255
};
#pragma pack(pop)

//-----------------------------------------------------------

#define	VEHICLE_SUBTYPE_CAR				1
#define	VEHICLE_SUBTYPE_BIKE			2
#define	VEHICLE_SUBTYPE_HELI			3
#define	VEHICLE_SUBTYPE_BOAT			4
#define	VEHICLE_SUBTYPE_PLANE			5
#define	VEHICLE_SUBTYPE_PUSHBIKE		6
#define	VEHICLE_SUBTYPE_TRAIN			7

//-----------------------------------------------------------

#define TRAIN_PASSENGER_LOCO			538
#define TRAIN_FREIGHT_LOCO				537
#define TRAIN_PASSENGER					570
#define TRAIN_FREIGHT					569
#define TRAIN_TRAM						449
#define HYDRA							520

//-----------------------------------------------------------

#define ACTION_WASTED					55
#define ACTION_DEATH					54
#define ACTION_INCAR					50
#define ACTION_NORMAL					1
#define ACTION_SCOPE					12
#define ACTION_NONE						0 

//-----------------------------------------------------------

#define WEAPON_MODEL_BRASSKNUCKLE		331 // was 332
#define WEAPON_MODEL_GOLFCLUB			333
#define WEAPON_MODEL_NITESTICK			334
#define WEAPON_MODEL_KNIFE				335
#define WEAPON_MODEL_BAT				336
#define WEAPON_MODEL_SHOVEL				337
#define WEAPON_MODEL_POOLSTICK			338
#define WEAPON_MODEL_KATANA				339
#define WEAPON_MODEL_CHAINSAW			341
#define WEAPON_MODEL_DILDO				321
#define WEAPON_MODEL_DILDO2				322
#define WEAPON_MODEL_VIBRATOR			323
#define WEAPON_MODEL_VIBRATOR2			324
#define WEAPON_MODEL_FLOWER				325
#define WEAPON_MODEL_CANE				326
#define WEAPON_MODEL_GRENADE			342 // was 327
#define WEAPON_MODEL_TEARGAS			343 // was 328
#define WEAPON_MODEL_MOLOTOV			344 // was 329
#define WEAPON_MODEL_COLT45				346
#define WEAPON_MODEL_SILENCED			347
#define WEAPON_MODEL_DEAGLE				348
#define WEAPON_MODEL_SHOTGUN			349
#define WEAPON_MODEL_SAWEDOFF			350
#define WEAPON_MODEL_SHOTGSPA			351
#define WEAPON_MODEL_UZI				352
#define WEAPON_MODEL_MP5				353
#define WEAPON_MODEL_AK47				355
#define WEAPON_MODEL_M4					356
#define WEAPON_MODEL_TEC9				372
#define WEAPON_MODEL_RIFLE				357
#define WEAPON_MODEL_SNIPER				358
#define WEAPON_MODEL_ROCKETLAUNCHER		359
#define WEAPON_MODEL_HEATSEEKER			360
#define WEAPON_MODEL_FLAMETHROWER		361
#define WEAPON_MODEL_MINIGUN			362
#define WEAPON_MODEL_SATCHEL			363
#define WEAPON_MODEL_BOMB				364
#define WEAPON_MODEL_SPRAYCAN			365
#define WEAPON_MODEL_FIREEXTINGUISHER	366
#define WEAPON_MODEL_CAMERA				367
#define WEAPON_MODEL_NIGHTVISION		368	// newly added
#define WEAPON_MODEL_INFRARED			369	// newly added
#define WEAPON_MODEL_JETPACK			370	// newly added
#define WEAPON_MODEL_PARACHUTE			371

#define WEAPON_FIST                     0
#define WEAPON_BRASSKNUCKLE             1
#define WEAPON_GOLFCLUB                 2
#define WEAPON_NITESTICK                3
#define WEAPON_KNIFE                    4
#define WEAPON_BAT                      5
#define WEAPON_SHOVEL                   6
#define WEAPON_POOLSTICK                7
#define WEAPON_KATANA                   8
#define WEAPON_CHAINSAW                 9
#define WEAPON_DILDO                    10
#define WEAPON_DILDO2                   11
#define WEAPON_VIBRATOR                 12
#define WEAPON_VIBRATOR2                13
#define WEAPON_FLOWER                   14
#define WEAPON_CANE                     15
#define WEAPON_GRENADE                  16
#define WEAPON_TEARGAS                  17
#define WEAPON_MOLTOV                   18
#define WEAPON_COLT45                   22
#define WEAPON_SILENCED                 23
#define WEAPON_DEAGLE                   24
#define WEAPON_SHOTGUN                  25
#define WEAPON_SAWEDOFF                 26
#define WEAPON_SHOTGSPA                 27
#define WEAPON_UZI                      28
#define WEAPON_MP5                      29
#define WEAPON_AK47                     30
#define WEAPON_M4                       31
#define WEAPON_TEC9                     32
#define WEAPON_RIFLE                    33
#define WEAPON_SNIPER                   34
#define WEAPON_ROCKETLAUNCHER           35
#define WEAPON_HEATSEEKER               36
#define WEAPON_FLAMETHROWER             37
#define WEAPON_MINIGUN                  38
#define WEAPON_SATCHEL                  39
#define WEAPON_BOMB                     40
#define WEAPON_SPRAYCAN                 41
#define WEAPON_FIREEXTINGUISHER         42
#define WEAPON_CAMERA                   43
#define WEAPON_NIGHTVISION				44
#define WEAPON_INFRARED					45
#define WEAPON_PARACHUTE                46
#define WEAPON_JETPACK 					47
#define WEAPON_VEHICLE                  49
#define WEAPON_HELIBLADES				50
#define WEAPON_EXPLOSION				51
#define WEAPON_DROWN                    53
#define WEAPON_COLLISION                54
#define WEAPON_UNKNOWN					0xFF

#define OBJECT_PARACHUTE				3131
#define OBJECT_CJ_CIGGY					1485
#define OBJECT_DYN_BEER_1				1486
#define OBJECT_CJ_BEER_B_2				1543
#define OBJECT_CJ_PINT_GLASS			1546
#define OBJECT_NOMODELFILE				18631
