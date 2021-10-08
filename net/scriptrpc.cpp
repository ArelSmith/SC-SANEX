#include "main.h"
#include "game/game.h"
#include "netgame.h"
#include "chatwindow.h"
#include "gui/gui.h"
#include "deathmessage.h"
#include "game/audiostream.h"

extern CGame *pGame;
extern CNetGame *pNetGame;
extern CChatWindow *pChatWindow;
extern CTextDraw *pTextDraw;
extern CDeathMessage *pDeathMessage;
extern CAudioStream *pAudioStream;

void ScrPlayAudioStream(RPCParameters *rpcParams)
{
    Log("RPC: PlayAudioStreamForPlayer");

    RakNet::BitStream bsData(rpcParams->input, (rpcParams->numberOfBitsOfData / 8) + 1, false);

    uint8_t byteTextLen;
    char szURL[1024];

    float X, Y, Z;
    float fRadius;

    bool bUsePos;

    bsData.Read(byteTextLen);
    bsData.Read(szURL, byteTextLen);

    bsData.Read(X);
    bsData.Read(Y);
    bsData.Read(Z);

    bsData.Read(fRadius);

    bsData.Read(bUsePos);

    szURL[byteTextLen] = '\0';

    if(pAudioStream) pAudioStream->Play(szURL, X, Y, Z, fRadius, bUsePos);

    if(pChatWindow) {
        pChatWindow->AddInfoMessage("{FFFFFF}Audio Stream: %s", szURL);
    }
}

void ScrStopAudioStream(RPCParameters *rpcParams)
{
    Log("RPC: StopAudioStreamForPlayer");

    if(pAudioStream) pAudioStream->Stop(true);
}


void DisplayGameText(RPCParameters *rpcParams)
{
	Log("RPC: DisplayGameText");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	char szMessage[512];
	int iType;
	int iTime;
	int iLength;

	bsData.Read(iType);
	bsData.Read(iTime);
	bsData.Read(iLength);
	if(iLength >= sizeof(szMessage)) 
		return;

	bsData.Read(szMessage,iLength);
	szMessage[iLength] = '\0';

	if(pGame)
		pGame->DisplayGameText(szMessage, iTime, iType);
}

void SetPlayerGravity(RPCParameters *rpcParams)
{
	Log("RPC: SetGravity");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	float fGravity;

	bsData.Read(fGravity);
	
	if(pGame)
		pGame->SetGravity(fGravity);
}

void ForceClassSelection(RPCParameters *rpcParams)
{
	Log("RPC: ForceClassSelection");
	if(pNetGame)
	{
		CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
		if(pPlayerPool)
		{
			CLocalPlayer *pLocalPlayer = pPlayerPool->GetLocalPlayer();
			if(pLocalPlayer)
				pLocalPlayer->ReturnToClassSelection();
		}
	}
}

void SetPlayerPos(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerPos");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VECTOR vecPos;

	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);

	if(pGame)
	{
		CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
		if(pPlayerPed)
			pPlayerPed->TeleportTo(vecPos.X,vecPos.Y,vecPos.Z);
	}
}

void SetPlayerCameraPos(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerCameraPos");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VECTOR vecPos;

	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);

	if(pGame)
	{
		CCamera *pCamera = pGame->GetCamera();
		if(pCamera)
			pCamera->SetPosition(vecPos.X, vecPos.Y, vecPos.Z, 0.0f, 0.0f, 0.0f);
	}
}

void SetPlayerCameraLookAt(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerCameraLookAt");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VECTOR vecPos;

	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);

	if(pGame)
	{
		CCamera *pCamera = pGame->GetCamera();
		if(pCamera)
			pCamera->LookAtPoint(vecPos.X,vecPos.Y,vecPos.Z,2);	
	}
}

void SetPlayerFacingAngle(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerFacingAngle");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	float fAngle;
	
	bsData.Read(fAngle);

	if(pGame)
	{
		CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
		if(pPlayerPed)
			pPlayerPed->ForceTargetRotation(fAngle);
	}
}

void SetPlayerFightingStyle(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerFightingStyle");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	PLAYERID playerId;
	uint8_t byteFightingStyle = 0;
	
	bsData.Read(playerId);
	bsData.Read(byteFightingStyle);
	
	if(pNetGame)
	{
		CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
		if(pPlayerPool)
		{
			CPlayerPed *pPlayerPed;
			if(playerId == pPlayerPool->GetLocalPlayerID())
				pPlayerPed = pPlayerPool->GetLocalPlayer()->GetPlayerPed();
			else if(pPlayerPool->GetSlotState(playerId)) 
				pPlayerPed = pPlayerPool->GetAt(playerId)->GetPlayerPed();

			if(pPlayerPed)
				pPlayerPed->SetFightingStyle(byteFightingStyle);
		}
	}
}

void SetPlayerSkin(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerSkin");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	int playerId;
	unsigned int uiSkin;
	
	bsData.Read(playerId);
	bsData.Read(uiSkin);

	if(pNetGame)
	{
		CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
		if(pPlayerPool)
		{
			CPlayerPed *pPlayerPed;
			if(playerId == pPlayerPool->GetLocalPlayerID())
				pPlayerPed = pPlayerPool->GetLocalPlayer()->GetPlayerPed();
			else if(pPlayerPool->GetSlotState(playerId)) 
				pPlayerPed = pPlayerPool->GetAt(playerId)->GetPlayerPed();

			if(pPlayerPed)
				pPlayerPed->SetModelIndex(uiSkin);
		}
	}
}

void ApplyAnimation(RPCParameters *rpcParams)
{
	Log("RPC: ApplyAnimation");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	PLAYERID playerId;
	uint8_t byteAnimLibLen;
	uint8_t byteAnimNameLen;
	char szAnimLib[256];
	char szAnimName[256];
	float fS;
	bool opt1, opt2, opt3, opt4;
	int opt5;

	memset(szAnimLib, 0, 256);
	memset(szAnimName, 0, 256);

	bsData.Read(playerId);
	bsData.Read(byteAnimLibLen);
	bsData.Read(szAnimLib, byteAnimLibLen);
	bsData.Read(byteAnimNameLen);
	bsData.Read(szAnimName, byteAnimNameLen);
	bsData.Read(fS);
	bsData.Read(opt1);
	bsData.Read(opt2);
	bsData.Read(opt3);
	bsData.Read(opt4);
	bsData.Read(opt5);

	szAnimLib[byteAnimLibLen] = '\0';
	szAnimName[byteAnimNameLen] = '\0';

	Log("Animation: %s, %s", szAnimLib, szAnimName);

	if(pNetGame)
	{
		CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
		if(pPlayerPool)
		{
			CPlayerPed *pPlayerPed;
			if(playerId == pPlayerPool->GetLocalPlayerID())
				pPlayerPed = pPlayerPool->GetLocalPlayer()->GetPlayerPed();
			else if(pPlayerPool->GetSlotState(playerId)) 
				pPlayerPed = pPlayerPool->GetAt(playerId)->GetPlayerPed();

			if(pPlayerPed)
				pPlayerPed->ApplyAnimation(szAnimName, szAnimLib, fS, (int)opt1, (int)opt2, (int)opt3, (int)opt4, (int)opt5);
		}
	}
}

void ClearAnimations(RPCParameters *rpcParams)
{
	Log("RPC: ClearAnimations");
	unsigned char* Data = reinterpret_cast<unsigned char*>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	PLAYERID playerId;

	bsData.Read(playerId);

	if(pNetGame)
	{
		CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
		if(pPlayerPool)
		{
			CPlayerPed *pPlayerPed;
			if(playerId == pPlayerPool->GetLocalPlayerID())
				pPlayerPed = pPlayerPool->GetLocalPlayer()->GetPlayerPed();
			else if(pPlayerPool->GetSlotState(playerId)) 
				pPlayerPed = pPlayerPool->GetAt(playerId)->GetPlayerPed();

			if(pPlayerPed)
				ScriptCommand(&disembark_instantly_actor, pPlayerPed->m_dwGTAId);
		}
	}
}

void SetSpawnInfo(RPCParameters *rpcParams)
{
	Log("RPC: SetSpawnInfo");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data, (iBitLength/8)+1, false);

	PLAYER_SPAWN_INFO SpawnInfo;

	bsData.Read((char*)&SpawnInfo, sizeof(PLAYER_SPAWN_INFO));

	if(pNetGame)
	{
		CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
		if(pPlayerPool)
		{
			CLocalPlayer *pLocalPlayer = pPlayerPool->GetLocalPlayer();
			if(pLocalPlayer)
				pLocalPlayer->SetSpawnInfo(&SpawnInfo);
		}
	}
}

void CreateExplosion(RPCParameters *rpcParams)
{
	Log("RPC: CreateExplosion");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	float X, Y, Z, Radius;
	int iType;

	bsData.Read(X);
	bsData.Read(Y);
	bsData.Read(Z);
	bsData.Read(iType);
	bsData.Read(Radius);

	ScriptCommand(&create_explosion_with_radius, X, Y, Z, iType, Radius);
}

void SetPlayerHealth(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerHealth");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	float fHealth;

	bsData.Read(fHealth);

	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
	if(pLocalPlayer)
		pLocalPlayer->GetPlayerPed()->SetHealth(fHealth);
}

void SetPlayerArmour(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerArmour");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	float fHealth;

	bsData.Read(fHealth);

	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
	if(pLocalPlayer)
		pLocalPlayer->GetPlayerPed()->SetArmour(fHealth);
}

void SetPlayerColor(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerColor");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	
	PLAYERID playerId;
	uint32_t dwColor;

	bsData.Read(playerId);
	bsData.Read(dwColor);

	//Log("Color: 0x%X", dwColor);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
	{
		if(playerId == pPlayerPool->GetLocalPlayerID()) 
		{
			CLocalPlayer *pLocalPlayer = pPlayerPool->GetLocalPlayer();
			if(pLocalPlayer)
				pLocalPlayer->SetPlayerColor(dwColor);
		} 
		else 
		{
			CRemotePlayer *pPlayer = pPlayerPool->GetAt(playerId);
			if(pPlayer)	
				pPlayer->SetPlayerColor(dwColor);
		}
	}
}

void SetPlayerName(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerName");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	PLAYERID playerId;
	uint8_t byteNickLen;
	char szNewName[MAX_PLAYER_NAME+1];
	uint8_t byteSuccess;

	bsData.Read(playerId);
	bsData.Read(byteNickLen);

	if(byteNickLen > MAX_PLAYER_NAME) return;

	bsData.Read(szNewName, byteNickLen);
	bsData.Read(byteSuccess);

	szNewName[byteNickLen] = '\0';

	//Log("byteSuccess = %d", byteSuccess);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
	{
		if(byteSuccess == 1)
			pPlayerPool->SetPlayerName(playerId, szNewName);

		if(pPlayerPool->GetLocalPlayerID() == playerId)
			pPlayerPool->SetLocalPlayerName(szNewName);
	}
}

void SetPlayerPosFindZ(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerPosFindZ");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VECTOR vecPos;

	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);

	vecPos.Z = pGame->FindGroundZForCoord(vecPos.X, vecPos.Y, vecPos.Z);
	vecPos.Z += 1.75f;

	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
	if(pLocalPlayer)
		pLocalPlayer->GetPlayerPed()->TeleportTo(vecPos.X, vecPos.Y, vecPos.Z);
}

void SetInteriorId(RPCParameters *rpcParams)
{
	Log("RPC: SetInteriorId");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint8_t byteInterior;

	bsData.Read(byteInterior);

	pGame->FindPlayerPed()->SetInterior(byteInterior);	
}

void SetPlayerMapIcon(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerMapIcon");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint8_t byteIndex;
	uint8_t byteIcon;
	uint32_t dwColor;
	float fPos[3];
	uint8_t byteStyle;

	bsData.Read(byteIndex);
	bsData.Read(fPos[0]);
	bsData.Read(fPos[1]);
	bsData.Read(fPos[2]);
	bsData.Read(byteIcon);
	bsData.Read(dwColor);
	bsData.Read(byteStyle);

	pNetGame->SetMapIcon(byteIndex, fPos[0], fPos[1], fPos[2], byteIcon, dwColor, byteStyle);
}

void RemovePlayerMapIcon(RPCParameters *rpcParams)
{
	Log("RPC: RemovePlayerMapIcon");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint8_t byteIndex;

	bsData.Read(byteIndex);

	pNetGame->DisableMapIcon(byteIndex);
}

void SetCameraBehindPlayer(RPCParameters *rpcParams)
{
	Log("RPC: SetCameraBehindPlayer");

	pGame->GetCamera()->SetBehindPlayer();	
}

void SetPlayerSpecialAction(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerSpecialAction");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint8_t byteSpecialAction;

	bsData.Read(byteSpecialAction);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool) 
	{
		CLocalPlayer *pLocalPlayer = pPlayerPool->GetLocalPlayer();
		if(pLocalPlayer)
			pLocalPlayer->ApplySpecialAction(byteSpecialAction);
	}
}

void TogglePlayerSpectating(RPCParameters *rpcParams)
{
	Log("RPC: TogglePlayerSpectating");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	
	uint8_t bToggle;

	bsData.Read(bToggle);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool) 
	{
		CLocalPlayer *pLocalPlayer = pPlayerPool->GetLocalPlayer();
		if(pLocalPlayer)
			pLocalPlayer->ToggleSpectating(bToggle);
	}
}

void SetPlayerSpectating(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerSpectating");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	PLAYERID playerId;

	bsData.Read(playerId);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool && pPlayerPool->GetSlotState(playerId))
		pPlayerPool->GetAt(playerId)->SetState(PLAYER_STATE_SPECTATING);
}

#define SPECTATE_TYPE_NORMAL	1
#define SPECTATE_TYPE_FIXED		2
#define SPECTATE_TYPE_SIDE		3

void PlayerSpectatePlayer(RPCParameters *rpcParams)
{
	Log("RPC: PlayerSpectatePlayer");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	
	PLAYERID playerId;
    uint8_t byteMode;
	
	bsData.Read(playerId);
	bsData.Read(byteMode);

	switch(byteMode) 
	{
		case SPECTATE_TYPE_FIXED:
			byteMode = 15;
			break;
		case SPECTATE_TYPE_SIDE:
			byteMode = 14;
			break;
		default:
			byteMode = 4;
	}

	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
	if(pLocalPlayer)
	{
		pLocalPlayer->m_byteSpectateMode = byteMode;
		pLocalPlayer->SpectatePlayer(playerId);
	}
}

void PlayerSpectateVehicle(RPCParameters *rpcParams)
{
	Log("RPC: PlayerSpectateVehicle");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VEHICLEID VehicleID;
	uint8_t byteMode;

	bsData.Read(VehicleID);
	bsData.Read(byteMode);

	switch (byteMode) 
	{
		case SPECTATE_TYPE_FIXED:
			byteMode = 15;
			break;
		case SPECTATE_TYPE_SIDE:
			byteMode = 14;
			break;
		default:
			byteMode = 3;
	}

	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
	if(pLocalPlayer)
	{
		pLocalPlayer->m_byteSpectateMode = byteMode;
		pLocalPlayer->SpectateVehicle(VehicleID);
	}
}

void PutPlayerInVehicle(RPCParameters *rpcParams)
{
	Log("RPC: PutPlayerInVehicle");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VEHICLEID vehicleid;
	uint8_t seatid;

	bsData.Read(vehicleid);
	bsData.Read(seatid);

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(pVehiclePool)
	{
		int iVehicleIndex = pVehiclePool->FindGtaIDFromID(vehicleid);
		CVehicle *pVehicle = pVehiclePool->GetAt(vehicleid);
		if(iVehicleIndex && pVehicle)
			pGame->FindPlayerPed()->PutDirectlyInVehicle(iVehicleIndex, seatid);
	}
}

void SetVehicleParamsForPlayer(RPCParameters *rpcParams)
{
	Log("RPC: SetVehicleParamsForPlayer");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VEHICLEID VehicleId;
	uint8_t byteObjectiveVehicle;
	uint8_t byteDoorsLocked;

	bsData.Read(VehicleId);
	if(VehicleId < 0 || VehicleId > MAX_VEHICLES) return;
	
	bsData.Read(byteObjectiveVehicle);
	bsData.Read(byteDoorsLocked);

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(pVehiclePool)
		pVehiclePool->AssignSpecialParamsToVehicle(VehicleId, byteObjectiveVehicle, byteDoorsLocked);
}

void SetVehicleParamsEx(RPCParameters *rpcParams)
{
	Log("RPC: SetVehicleParamsEx");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VEHICLEID VehicleId;
	uint8_t engine, lights, alarm, doors, bonnet, boot, objective;

	bsData.Read(VehicleId);
	if(VehicleId < 0 || VehicleId > MAX_VEHICLES) return;

	bsData.Read(engine);
	bsData.Read(lights);
	bsData.Read(alarm);
	bsData.Read(doors);
	bsData.Read(bonnet);
	bsData.Read(boot);
	bsData.Read(objective);

	Log("VehicleId: %d", VehicleId);
	Log("engine: %d, lights: %d, alarm: %d, doors: %d, bonnet: %d, boot: %d, obj: %d", engine, lights, alarm, doors, bonnet, boot, objective);

	if(pNetGame)
	{
		CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
		if(pVehiclePool) 
		{
			CVehicle *pVehicle = pVehiclePool->GetAt(VehicleId);
			if(pVehicle) 
			{
				pVehicle->SetEngineState(engine);
				pVehicle->SetLightsState(lights);
				pVehicle->SetAlarmState(alarm);
				pVehicle->SetDoorState(doors);
				pVehicle->SetDoorOpenFlag(bonnet, 0);
				pVehicle->SetDoorOpenFlag(boot, 1);
				pVehicle->SetObjState(objective);
			}
		}
	}
}

void GivePlayerMoney(RPCParameters *rpcParams)
{
	Log("RPC: GivePlayerMoney");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	int iAmmount;
	
	bsData.Read(iAmmount);

	pGame->AddToLocalMoney(iAmmount);
}

void ResetPlayerMoney(RPCParameters *rpcParams)
{
	Log("RPC: ResetPlayerMoney");

	pGame->ResetLocalMoney();
}

void LinkVehicleToInterior(RPCParameters *rpcParams)
{
	Log("RPC: LinkVehicleToInterior");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VEHICLEID VehicleID;
	uint8_t byteInterior;

	bsData.Read(VehicleID);
	bsData.Read(byteInterior);

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(pVehiclePool)
		pVehiclePool->LinkToInterior(VehicleID, (int)byteInterior);
}

void RemovePlayerFromVehicle(RPCParameters *rpcParams)
{
	Log("RPC: RemovePlayerFromVehicle");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
	{
		CLocalPlayer *pLocalPlayer = pPlayerPool->GetLocalPlayer();
		if(pLocalPlayer)
		{
			CPlayerPed *pPlayerPed = pLocalPlayer->GetPlayerPed();
			if(pPlayerPed)
			{
				MATRIX4X4 matPlayer;
				pPlayerPed->GetMatrix(&matPlayer);
				pPlayerPed->TeleportTo(matPlayer.pos.X - 1.4, matPlayer.pos.Y + 0.3, matPlayer.pos.Z + 0.3); //ExitCurrentVehicle();
			}
		}
	}
}

void SetVehicleHealth(RPCParameters *rpcParams)
{
	Log("RPC: SetVehicleHealth");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	float fHealth;
	VEHICLEID VehicleId;

	bsData.Read(VehicleId);
	bsData.Read(fHealth);

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(pVehiclePool)
	{
		if(pVehiclePool->GetSlotState(VehicleId))
		{
			CVehicle *pVehicle = pVehiclePool->GetAt(VehicleId);
			if(pVehicle)
				pVehicle->SetHealth(fHealth);
		}
	}
}

void SetVehiclePos(RPCParameters *rpcParams)
{
	Log("RPC: SetVehiclePos");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VEHICLEID VehicleId;
	float fX, fY, fZ;

	bsData.Read(VehicleId);
	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(pVehiclePool)
	{
		if(pVehiclePool->GetSlotState(VehicleId))
		{
			CVehicle *pVehicle = pVehiclePool->GetAt(VehicleId);
			if(pVehicle)
				pVehicle->TeleportTo(fX, fY, fZ);
		}
	}
}

void SetVehicleVelocity(RPCParameters *rpcParams)
{
	Log("RPC: SetVehicleVelocity");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint8_t bTurn = false;
	VECTOR vecMoveSpeed;

	bsData.Read(bTurn);
	bsData.Read(vecMoveSpeed.X);
	bsData.Read(vecMoveSpeed.Y);
	bsData.Read(vecMoveSpeed.Z);

	Log("X: %f, Y: %f, Z: %f", vecMoveSpeed.X, vecMoveSpeed.Y, vecMoveSpeed.Z);

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(pVehiclePool)
	{
		CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
		if(pPlayerPed)
		{
			CVehicle *pVehicle = pVehiclePool->GetAt(pVehiclePool->FindIDFromGtaPtr(pPlayerPed->GetGtaVehicle()));
			if(pVehicle)
				pVehicle->SetMoveSpeedVector(vecMoveSpeed);
		}
	}
}

void SetNumberPlate(RPCParameters *rpcParams)
{
	Log("RPC: SetNumberPlate");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VEHICLEID vehicleId;

	bsData.Read(vehicleId);

	if(vehicleId > 0 || vehicleId <= MAX_VEHICLES)
	{
		uint8_t byteLen;
		char szNumberPlate[32 + 1];
		bsData.Read(byteLen);
		bsData.Read(szNumberPlate, byteLen);
		szNumberPlate[byteLen] = '\0';

		if(strlen(szNumberPlate) > 0)
			ScriptCommand(&set_car_numberplate, vehicleId, szNumberPlate);
		else ScriptCommand(&set_car_numberplate, vehicleId, "ASD123");
	}
}

void SetVehicleZAngle(RPCParameters *rpcParams)
{
	Log("RPC: SetVehicleZAngle");
    unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	VEHICLEID vehicleId;
	float fZAngle;

	bsData.Read(vehicleId);
    bsData.Read(fZAngle);
    
    CVehicle *pVehicle = pNetGame->GetVehiclePool()->GetAt(vehicleId);
    if(pVehicle)
    	ScriptCommand(&set_car_z_angle, pVehicle->m_dwGTAId, fZAngle);
}

void AttachTrailerToVehicle(RPCParameters *rpcParams)
{
    unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
    int iBitLength = rpcParams->numberOfBitsOfData;
    PlayerID sender = rpcParams->sender;

    VEHICLEID TrailerID, VehicleID;
    RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);
    bsData.Read(TrailerID);
    bsData.Read(VehicleID);
    CVehicle* pTrailer = pNetGame->GetVehiclePool()->GetAt(TrailerID);
    CVehicle* pVehicle = pNetGame->GetVehiclePool()->GetAt(VehicleID);

    pVehicle->SetTrailer(pTrailer);
    pVehicle->AttachTrailer();
}

void DetachTrailerFromVehicle(RPCParameters *rpcParams)
{
    unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
    int iBitLength = rpcParams->numberOfBitsOfData;
    PlayerID sender = rpcParams->sender;

    VEHICLEID VehicleID;
    RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);
    bsData.Read(VehicleID);
    CVehicle* pVehicle = pNetGame->GetVehiclePool()->GetAt(VehicleID);

    pVehicle->DetachTrailer();
    pVehicle->SetTrailer(nullptr);
}

void InterpolateCamera(RPCParameters *rpcParams)
{
	Log("RPC: InterpolateCamera");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	bool bSetPos = true;
	VECTOR vecFrom, vecDest;
	int time;
	uint8_t mode;

	bsData.Read(bSetPos);
	bsData.Read(vecFrom.X);
	bsData.Read(vecFrom.Y);
	bsData.Read(vecFrom.Z);
	bsData.Read(vecDest.X);
	bsData.Read(vecDest.Y);
	bsData.Read(vecDest.Z);
	bsData.Read(time);
	bsData.Read(mode);

	if(mode < 1 || mode > 2)
		mode = 2;

	if(time > 0)
	{
		if(bSetPos) 
			pGame->GetCamera()->InterpolateCameraPos(&vecFrom, &vecDest, time, mode);
		else pGame->GetCamera()->InterpolateCameraLookAt(&vecFrom, &vecDest, time, mode);
	}
}

void GangZoneCreate(RPCParameters *rpcParams)
{
	Log("RPC: GangZoneCreate");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint16_t wZoneID;
	float minx, miny, maxx, maxy;
	uint32_t dwColor;

	CGangZonePool *pGangZonePool = pNetGame->GetGangZonePool();
	if(pGangZonePool)
	{
		bsData.Read(wZoneID);
		bsData.Read(minx);
		bsData.Read(miny);
		bsData.Read(maxx);
		bsData.Read(maxy);
		bsData.Read(dwColor);

		pGangZonePool->New(wZoneID, minx, miny, maxx, maxy, dwColor);
	}
}

void GangZoneDestroy(RPCParameters *rpcParams)
{
	Log("RPC: GangZoneDestroy");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint16_t wZoneID;

	CGangZonePool *pGangZonePool = pNetGame->GetGangZonePool();

	if(pGangZonePool)
	{
		bsData.Read(wZoneID);

		pGangZonePool->Delete(wZoneID);
	}
}

void GangZoneFlash(RPCParameters *rpcParams)
{
	Log("RPC: GangZoneFlash");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint16_t wZoneID;
	uint32_t dwColor;

	CGangZonePool* pGangZonePool = pNetGame->GetGangZonePool();
	if(pGangZonePool)
	{
		bsData.Read(wZoneID);
		bsData.Read(dwColor);

		pGangZonePool->Flash(wZoneID, dwColor);
	}
}

void GangZoneStopFlash(RPCParameters *rpcParams)
{
	Log("RPC: GangZoneStopFlash");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint16_t wZoneID;

	CGangZonePool *pGangZonePool = pNetGame->GetGangZonePool();
	if(pGangZonePool)
	{
		bsData.Read(wZoneID);
		
		pGangZonePool->StopFlash(wZoneID);
	}
}

int iTotalObjects = 0;

void CreateObject(RPCParameters *rpcParams)
{
	Log("RPC: CreateObject (%d)", iTotalObjects);
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	uint16_t wObjectID;
	unsigned long ModelID;
	float fDrawDistance;
	VECTOR vecPos, vecRot;
	uint8_t byteNoCameraCol; 
	uint16_t wAttachedObject; 
	VEHICLEID wAttachedVehicle; 
	VECTOR AttachOffset, AttachRot;
	uint8_t wSyncRotation;
	uint8_t wTexturesCount;

	bsData.Read(wObjectID);
	bsData.Read(ModelID);

	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);

	bsData.Read(vecRot.X);
	bsData.Read(vecRot.Y);
	bsData.Read(vecRot.Z);

	bsData.Read(fDrawDistance);
	bsData.Read(byteNoCameraCol);

	bsData.Read(wAttachedVehicle);
	bsData.Read(wAttachedObject);
	bsData.Read(AttachOffset.X);
	bsData.Read(AttachOffset.Y);
	bsData.Read(AttachOffset.Z);
	bsData.Read(AttachRot.X);
	bsData.Read(AttachRot.Y);
	bsData.Read(AttachRot.Z);
	bsData.Read(wSyncRotation);
	bsData.Read(wTexturesCount);

	CObjectPool *pObjectPool = pNetGame->GetObjectPool();
	if(pObjectPool)
	{
		pObjectPool->New(wObjectID, ModelID, vecPos, vecRot, fDrawDistance);

		if(wAttachedObject >= 0 && wAttachedObject < MAX_OBJECTS) 
		{
			Log("Attaching object to object %u", wAttachedObject);
			pObjectPool->AttachObjectToObject(wObjectID, wAttachedObject, AttachOffset, AttachRot, wSyncRotation);
		}
		else if(wAttachedVehicle >= 0 && wAttachedVehicle < MAX_VEHICLES) 
		{
			Log("Attaching object to vehicle %u", wAttachedVehicle);
			CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
			if(pVehiclePool)
				pVehiclePool->AttachObjectToVehicle(wObjectID, wAttachedVehicle, AttachOffset, AttachRot, wSyncRotation);
		}
		// ObjectMaterial
		uint16_t ObjectID;
		bsData.Read(ObjectID);
		if(ObjectID < 0 || ObjectID > MAX_OBJECTS) return;

		CObject *pObject = pObjectPool->GetAt(ObjectID);
		if(pObject)
		{
			uint8_t byteMaterialType;
			uint8_t byteMaterialIndex;
			uint8_t byteLength;
			bsData.Read(byteMaterialType);
			bsData.Read(byteMaterialIndex);
			switch(byteMaterialType)
			{
				case OBJECT_MATERIAL:
				{
					uint16_t wModelID;
					char szTxdName[32], szTexName[32];
					uint32_t dwColor;

					bsData.Read(wModelID);
					bsData.Read(byteLength);
					if(byteLength >= sizeof(szTxdName))
						break;

					bsData.Read(szTxdName, byteLength);
					szTxdName[byteLength] = '\0';

					bsData.Read(byteLength);
					if(byteLength >= sizeof(szTexName))
						break;

					bsData.Read(szTexName, byteLength);
					szTexName[byteLength] = '\0';

					bsData.Read(dwColor);

					pObject->SetMaterial(wModelID, byteMaterialIndex, szTxdName, szTexName, dwColor);
					break;
				}
				case OBJECT_MATERIAL_TEXT:
				{
					uint8_t byteMaterialSize;
					char szFontName[32];
					uint8_t byteFontSize;
					uint8_t byteFontBold;
					uint32_t dwFontColor;
					uint32_t dwBackgroundColor;
					uint8_t byteAlign;
					char szText[2048];

					bsData.Read(byteMaterialSize);
					bsData.Read(byteLength);
					if(byteLength >= sizeof(szFontName))
						break;

					bsData.Read(szFontName, byteLength);
					szFontName[byteLength] = '\0';
					bsData.Read(byteFontSize);
					bsData.Read(byteFontBold);
					bsData.Read(dwFontColor);
					bsData.Read(dwBackgroundColor);
					bsData.Read(byteAlign);
					stringCompressor->DecodeString(szText, 2048, &bsData);

					pObject->SetMaterialText(byteMaterialIndex, byteMaterialSize, szFontName, byteFontSize, byteFontBold, dwFontColor, dwBackgroundColor, byteAlign, szText);		
					break;
				}
			}
		}
	}

	iTotalObjects++;
}

void SetObjectPos(RPCParameters *rpcParams)
{
	Log("RPC: SetObjectPos");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	uint16_t wObjectID;
	float fRotation;
	VECTOR vecPos, vecRot;
	
	bsData.Read(wObjectID);
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	bsData.Read(fRotation);

	CObjectPool *pObjectPool = pNetGame->GetObjectPool();
	if(pObjectPool)
	{
		CObject *pObject = pObjectPool->GetAt(wObjectID);
		if(pObject)
			pObject->SetPos(vecPos.X, vecPos.Y, vecPos.Z);
	}
}

void DestroyObject(RPCParameters *rpcParams)
{
	Log("RPC: DestroyObject");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	uint16_t wObjectID;
	
	bsData.Read(wObjectID);

	iTotalObjects--;

	CObjectPool* pObjectPool =	pNetGame->GetObjectPool();
	if(pObjectPool->GetAt(wObjectID))
		pObjectPool->Delete(wObjectID);
}

void SetObjectRot(RPCParameters* rpcParams)
{
	Log("RPC: SetObjectRot");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
	
	uint16_t wObjectID;
	bsData.Read(wObjectID);
	if(wObjectID < 0 || wObjectID >= MAX_OBJECTS)
		return;
	
	VECTOR vecRot;
	bsData.Read(vecRot.X);
	bsData.Read(vecRot.Y);
	bsData.Read(vecRot.Z);

	CObjectPool* pObjectPool = pNetGame->GetObjectPool();
	if(pObjectPool) 
	{
		if(pObjectPool->GetSlotState(wObjectID)) 
		{
			CObject* pObject = pObjectPool->GetAt(wObjectID);
			if(pObject)
				pObject->InstantRotate(vecRot.X, vecRot.Y, vecRot.Z);
		}
	}
}

void PlaySound(RPCParameters *rpcParams)
{
	Log("RPC: PlaySound");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	int iSound;
	float fX, fY, fZ;

	bsData.Read(iSound);
	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);

	pGame->PlaySound(iSound, fX, fY, fZ);
}

void SetPlayerWantedLevel(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerWantedLevel");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	
	uint8_t byteLevel;
	
	bsData.Read(byteLevel);

	pGame->SetWantedLevel(byteLevel);
}

void TogglePlayerControllable(RPCParameters *rpcParams)
{
	Log("RPC: TogglePlayerControllable");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	uint8_t byteControllable;

	bsData.Read(byteControllable);

	pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->TogglePlayerControllable((int)byteControllable);
}

void GivePlayerWeapon(RPCParameters *rpcParams)
{
	Log("RPC: GivePlayerWeapon");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	int weaponID;
	int weaponAmmo;

	bsData.Read(weaponID);
	bsData.Read(weaponAmmo);

	CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
	if(pPlayerPed)
		pPlayerPed->GiveWeapon(weaponID, weaponAmmo);
}

void SetPlayerArmedWeapon(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerArmedWeapon");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	
	int iWeapon;

	bsData.Read(iWeapon);

	CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
	if(pPlayerPed) 
	{
		if(iWeapon >= 0 && iWeapon <= 46)
			pPlayerPed->SetArmedWeapon(iWeapon);
	}
}

void SetPlayerAmmo(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerAmmo");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint8_t byteWeapon;
	uint16_t wAmmo;
	
	bsData.Read(byteWeapon);
	bsData.Read(wAmmo);
	
	CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
	if(pPlayerPed)
		pPlayerPed->SetAmmo(byteWeapon, wAmmo);
}

void ResetPlayerWeapons(RPCParameters *rpcParams)
{
	Log("RPC: ResetPlayerWeapons");

	CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
	if(pPlayerPed)
		pPlayerPed->ClearAllWeapons();
}

void ShowTextDraw(RPCParameters* rpcParams)
{
	Log("RPC: ShowTextDraw");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data, (iBitLength/8)+1, false);

	uint16_t wTextID;
	TEXT_DRAW_TRANSMIT TextDrawTransmit;
	char cText[1024];
	unsigned short cTextLen = 0;

	CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
	if(pTextDrawPool)
	{
		bsData.Read(wTextID);
		bsData.Read((char*)&TextDrawTransmit, sizeof(TEXT_DRAW_TRANSMIT));
		bsData.Read(cTextLen);
		bsData.Read(cText, cTextLen);
		cText[cTextLen] = '\0';

		pTextDrawPool->New(wTextID, &TextDrawTransmit, cText);
	}
}

void TextDrawHideForPlayer(RPCParameters* rpcParams)
{
	Log("RPC: TextDrawHideForPlayer");
	unsigned char* Data = reinterpret_cast<unsigned char*>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint16_t wTextID;

	CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
	if(pTextDrawPool)
	{
		bsData.Read(wTextID);

		pTextDrawPool->Delete(wTextID);
	}
}

void TextDrawSetString(RPCParameters* rpcParams)
{
	Log("RPC: TextDrawSetString");
	unsigned char* Data = reinterpret_cast<unsigned char*>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint16_t wTextID;
	char cText[MAX_TEXT_DRAW_LINE];
	unsigned short cTextLen = 0;

	CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
	if(pTextDrawPool)
	{
		bsData.Read(wTextID);
		bsData.Read(cTextLen);
		bsData.Read(cText, cTextLen);
		cText[cTextLen] = '\0';

		CTextDraw* pText = pTextDrawPool->GetAt(wTextID);
		if(pText)
			pText->SetText(cText);
	}
}

void ClickTextDraw(RPCParameters* rpcParams)
{
	Log("RPC: ClickTextDraw");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data, (iBitLength/8)+1, false);

	bool bEnable = false;
	uint32_t dwColor = 0;

	CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
	if(pTextDrawPool)
	{
		bsData.Read(bEnable);
		bsData.Read(dwColor);
	
		pTextDrawPool->SetSelectState(bEnable ? true : false, dwColor);
	}
}


void SetPlayerAttachedObject(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerAttachedObject");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data, (iBitLength/8)+1, false);

	uint16_t playerId;
	uint32_t iIndexID, iModelID, iBoneID;
	bool bCreate;
	VECTOR vecOffset;
	VECTOR vecRotation;
	VECTOR vecScale;
	//uint32_t materialcolor1, materialcolor2;

	bsData.Read(playerId);
	bsData.Read(iIndexID);
	if(iIndexID < 0 || iIndexID >= 10)
		return;

	bsData.Read(bCreate);
	bsData.Read(iModelID);
	bsData.Read(iBoneID);
	
	// offset
	bsData.Read(vecOffset.X);
	bsData.Read(vecOffset.Y);
	bsData.Read(vecOffset.Z);

	// rotation
	bsData.Read(vecRotation.X);
	bsData.Read(vecRotation.Y);
	bsData.Read(vecRotation.Z);

	// scales
	bsData.Read(vecScale.X);
	bsData.Read(vecScale.Y);
	bsData.Read(vecScale.Z);

	// materialcolor
	//bsData.Read(materialcolor1);
	//bsData.Read(materialcolor2);

	if(pNetGame)
	{
		CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
		if(pPlayerPool)
		{
			CPlayerPed *pPlayerPed;
			if(playerId == pPlayerPool->GetLocalPlayerID())
				pPlayerPed = pPlayerPool->GetLocalPlayer()->GetPlayerPed();
			else
			{
				CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(playerId);
				if(pRemotePlayer)
					pPlayerPed = pRemotePlayer->GetPlayerPed();
			}

			if(pPlayerPed)
			{
				//Log("Create: %d, Index: %d, Bone: %d, Model: %d", bCreate, iIndexID, iBoneID, iModelID);
				if(bCreate)
				{
					if(iBoneID <= 0 && iBoneID > 18)
						return;

					pPlayerPed->UpdateAttachedObject(iIndexID, iModelID, iBoneID, vecOffset, vecRotation, vecScale);
				}
				else pPlayerPed->DeleteAttachedObjects(iIndexID);
			}
		}
	}
}

void MoveObject(RPCParameters* rpcParams)
{
	Log("RPC: MoveObject");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	int16_t objectId;
	VECTOR vecPosition;
	VECTOR vecNewPosition;
	float fMoveSpeed;
	VECTOR vecNewRotation;

	bsData.Read(objectId);
	if(objectId < 0 || objectId > MAX_OBJECTS)
		return;
	
	bsData.Read(vecPosition.X);
	bsData.Read(vecPosition.Y);
	bsData.Read(vecPosition.Z);
	bsData.Read(vecNewPosition.X);
	bsData.Read(vecNewPosition.Y);
	bsData.Read(vecNewPosition.Z);
	bsData.Read(fMoveSpeed);
	//bsData.Read(vecNewRotation.X);
	//bsData.Read(vecNewRotation.Y);
	//bsData.Read(vecNewRotation.Z);

	CObject* pObject = pNetGame->GetObjectPool()->GetAt(objectId);
	if(pObject) 
	{
		pObject->TeleportTo(vecPosition.X, vecPosition.Y, vecPosition.Z);
		pObject->SetMovingSpeed(fMoveSpeed);
		pObject->MovePositionTo(vecNewPosition.X, vecNewPosition.Y, vecNewPosition.Z);
		//pObject->MoveRotationTo(vecNewRotation.X, vecNewRotation.Y, vecNewRotation.Z);
	}
}
void StopObject(RPCParameters* rpcParams)
{
	Log("RPC: StopObject");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
	
	int16_t sObjectId;

	bsData.Read(sObjectId);
	if(sObjectId < 0 || sObjectId > MAX_OBJECTS)
		return;
	
	CObject* pObject = pNetGame->GetObjectPool()->GetAt(sObjectId);
	if(pObject)
		pObject->StopMovingObject();
}

void SetPlayerTeam(RPCParameters* rpcParams)
{
	Log("RPC: SetPlayerTeam");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	PLAYERID playerId;
	uint8_t byteTeamId;

	bsData.Read(playerId);
	bsData.Read(byteTeamId);
}

void ApplyActorAnimation(RPCParameters *rpcParams)
{
	Log("RPC: ApplyActorAnimation");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	uint16_t actorId;
	uint8_t uiAnimLibLen;
	char szAnimLib[64];
	uint8_t uiAnimNameLen;
	char szAnimName[64];
	float fDelta;
	bool bLoop;
	bool bLockX;
	bool bLockY;
	bool bFreeze;
	uint32_t uiTime;

	bsData.Read(actorId);
	
	bsData.Read(uiAnimLibLen);
	if(uiAnimLibLen < 0 || uiAnimLibLen >= sizeof(szAnimLib))
		return;
	
	bsData.Read(szAnimLib, uiAnimLibLen);
	szAnimLib[uiAnimLibLen] = '\0';
	
	bsData.Read(uiAnimNameLen);
	if(uiAnimNameLen < 0 || uiAnimNameLen >= sizeof(szAnimName))
		return;
	
	bsData.Read(szAnimName, uiAnimNameLen);
	szAnimName[uiAnimNameLen] = '\0';
	
	bsData.Read(fDelta);
	bsData.Read(bLoop);
	bsData.Read(bLockX);
	bsData.Read(bLockY);
	bsData.Read(bFreeze);
	bsData.Read(uiTime);

	CActorPool *pActorPool = pNetGame->GetActorPool();
	if(pActorPool) 
	{
		CRemoteActor *pRemoteActor = pActorPool->GetAt(actorId);
		if(pRemoteActor) 
		{
			CActorPed *pActorPed = pRemoteActor->GetActorPed();
			if(pActorPed)
				pActorPed->ApplyAnimation(szAnimName, szAnimLib, fDelta, bLoop, bLockX, bLockY, bFreeze, uiTime);
		}
	}
}

void ClearActorAnimations(RPCParameters *rpcParams)
{
	Log("RPC: ClearActorAnimation");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
	
	uint16_t actorId;
	
	bsData.Read(actorId);
	
	CActorPool *pActorPool = pNetGame->GetActorPool();
	if(pActorPool) 
	{
		CRemoteActor *pRemoteActor = pActorPool->GetAt(actorId);
		if(pRemoteActor) 
		{
			CActorPed *pActorPed = pRemoteActor->GetActorPed();
			if(pActorPed) 
			{
				MATRIX4X4 mat;
				pActorPed->GetMatrix(&mat);
				pActorPed->TeleportTo(mat.pos.X, mat.pos.Y, mat.pos.Z);
			}
		}
	}
}
void SetActorFacingAngle(RPCParameters *rpcParams)
{
	Log("RPC: SetActorFacingAngle");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
	
	uint16_t actorId;
	float fRotation;
	
	bsData.Read(actorId);
	bsData.Read(fRotation);
	
	CActorPool *pActorPool = pNetGame->GetActorPool();
	if(pActorPool) 
	{
		CRemoteActor *pRemoteActor = pActorPool->GetAt(actorId);
		if(pRemoteActor) 
		{
			CActorPed *pActorPed = pRemoteActor->GetActorPed();
			if(pActorPed)
				pActorPed->ForceTargetRotation(fRotation);
		}
	}
}

void SetActorPos(RPCParameters *rpcParams)
{
	Log("RPC: SetActorPos");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
	
	uint16_t actorId;
	VECTOR vecPos;

	bsData.Read(actorId);
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);

	CActorPool *pActorPool = pNetGame->GetActorPool();
	if(pActorPool) 
	{
		CRemoteActor *pRemoteActor = pActorPool->GetAt(actorId);
		if(pRemoteActor) 
		{
			CActorPed *pActorPed = pRemoteActor->GetActorPed();
			if(pActorPed)
				pActorPed->TeleportTo(vecPos.X, vecPos.Y, vecPos.Z);
		}
	}
}

void SetActorHealth(RPCParameters *rpcParams)
{
	Log("RPC: SetActorHealth");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
	
	uint16_t actorId;
	float fHealth;

	bsData.Read(actorId);
	bsData.Read(fHealth);

	CActorPool *pActorPool = pNetGame->GetActorPool();
	if(pActorPool) 
	{
		CRemoteActor *pRemoteActor = pActorPool->GetAt(actorId);
		if(pRemoteActor) 
		{
			CActorPed *pActorPed = pRemoteActor->GetActorPed();
			if(pActorPed)
				pActorPed->SetHealth(fHealth);
		}
	}
}

void RemoveBuildingForPlayer(RPCParameters *rpcParams) 
{
	Log("RPC: RemoveBuildingForPlayer");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
	
	uint32_t dwModel;
	VECTOR vecPosition;
	float fRange;

	bsData.Read(dwModel); 
	bsData.Read(vecPosition.X);
	bsData.Read(vecPosition.Y);
	bsData.Read(vecPosition.Z);
	bsData.Read(fRange);

	pGame->RemoveBuilding(dwModel, vecPosition, fRange, 0);
}

void AttachObjectToPlayer(RPCParameters *rpcParams)
{
	Log("RPC: AttachObjectToPlayer");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
	
	int16_t sObjectId;
	int16_t sPlayerID;
	VECTOR vecOff;
	VECTOR vecRot;

	bsData.Read(sObjectId);
	if(sObjectId < 0 || sObjectId > MAX_OBJECTS) return;
	
	bsData.Read(sPlayerID);
	bsData.Read(vecOff.X);
	bsData.Read(vecOff.Y);
	bsData.Read(vecOff.Z);
	bsData.Read(vecRot.X);
	bsData.Read(vecRot.Y);
	bsData.Read(vecRot.Z);

	CObjectPool* pObjectPool = pNetGame->GetObjectPool();
	if(pObjectPool) 
	{
		if(!pObjectPool->GetSlotState(sObjectId)) return;
		
		CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
		if(pPlayerPool) 
		{
			CObject* pObject = pObjectPool->GetAt(sObjectId);
			if(pObject) 
			{
				if(sPlayerID == pPlayerPool->GetLocalPlayerID()) {
					CLocalPlayer *pPlayer = pPlayerPool->GetLocalPlayer();
					if(pPlayer)
						ScriptCommand(&attach_object_to_actor, pObject->m_dwGTAId, pPlayer->GetPlayerPed()->m_dwGTAId, vecOff.X, vecOff.Y, vecOff.X, vecRot.X, vecRot.Y, vecRot.Z);
				} 
				else 
				{
					CRemotePlayer *pPlayer = pNetGame->GetPlayerPool()->GetAt(sPlayerID);
					if(pPlayer)
						ScriptCommand(&attach_object_to_actor, pObject->m_dwGTAId, pPlayer->GetPlayerPed()->m_dwGTAId, vecOff.X, vecOff.Y, vecOff.Z, vecRot.X, vecRot.Y, vecRot.Z);
				}
			}
		}
	}
}

void SetObjectMaterial(RPCParameters *rpcParams)
{
	Log("RPC: SetObjectMaterial");
	unsigned char *Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data, (iBitLength / 8) + 1, false);

	CObjectPool *pObjectPool = pNetGame->GetObjectPool();
	if(pObjectPool)
	{
		uint16_t ObjectID;
		bsData.Read(ObjectID);
		if(ObjectID < 0 || ObjectID > MAX_OBJECTS) return;

		CObject *pObject = pObjectPool->GetAt(ObjectID);
		if(pObject)
		{
			uint8_t byteMaterialType;
			uint8_t byteMaterialIndex;
			uint8_t byteLength;
			bsData.Read(byteMaterialType);
			bsData.Read(byteMaterialIndex);
			switch(byteMaterialType)
			{
				case OBJECT_MATERIAL:
				{
					uint16_t wModelID;
					char szTxdName[32], szTexName[32];
					uint32_t dwColor;

					bsData.Read(wModelID);
					bsData.Read(byteLength);
					if(byteLength >= sizeof(szTxdName))
						break;

					bsData.Read(szTxdName, byteLength);
					szTxdName[byteLength] = '\0';

					bsData.Read(byteLength);
					if(byteLength >= sizeof(szTexName))
						break;

					bsData.Read(szTexName, byteLength);
					szTexName[byteLength] = '\0';

					bsData.Read(dwColor);

					pObject->SetMaterial(wModelID, byteMaterialIndex, szTxdName, szTexName, dwColor);
					break;
				}
				case OBJECT_MATERIAL_TEXT:
				{
					uint8_t byteMaterialSize;
					char szFontName[32];
					uint8_t byteFontSize;
					uint8_t byteFontBold;
					uint32_t dwFontColor;
					uint32_t dwBackgroundColor;
					uint8_t byteAlign;
					char szText[2048];

					bsData.Read(byteMaterialSize);
					bsData.Read(byteLength);
					if(byteLength >= sizeof(szFontName))
						break;

					bsData.Read(szFontName, byteLength);
					szFontName[byteLength] = '\0';
					bsData.Read(byteFontSize);
					bsData.Read(byteFontBold);
					bsData.Read(dwFontColor);
					bsData.Read(dwBackgroundColor);
					bsData.Read(byteAlign);
					stringCompressor->DecodeString(szText, 2048, &bsData);

					pObject->SetMaterialText(byteMaterialIndex, byteMaterialSize, szFontName, byteFontSize, byteFontBold, dwFontColor, dwBackgroundColor, byteAlign, szText);		
					break;
				}
			}
		}
	}

	return;
}

void SetPlayerVelocity(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerVelocity");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VECTOR vecMoveSpeed;

	bsData.Read(vecMoveSpeed.X);
	bsData.Read(vecMoveSpeed.Y);
	bsData.Read(vecMoveSpeed.Z);

	CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
	if(pPlayerPed)
	{
		if(pPlayerPed->IsOnGround())
		{
			uint32_t dwStateFlags = pPlayerPed->GetStateFlags();
			dwStateFlags ^= 3; // Make the game think the ped is off the ground so SetMoveSpeed works
			pPlayerPed->SetStateFlags(dwStateFlags);
		}

		pPlayerPed->SetMoveSpeedVector(vecMoveSpeed);
	}
}

void DeathMessage(RPCParameters* rpcParams)
{
	Log("RPC: DeathMessage");
    unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	PLAYERID playerId, killerId;
	uint8_t reason;

	bsData.Read(killerId);
	bsData.Read(playerId);
	bsData.Read(reason);

	std::string killername, playername;
	unsigned int killercolor, playercolor;
	killername.clear();
	playername.clear();

	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
	{
		if(pPlayerPool->GetLocalPlayerID() == playerId)
		{
			playername = pPlayerPool->GetLocalPlayerName();
			playercolor = pPlayerPool->GetLocalPlayer()->GetPlayerColorAsARGB();
		}
		else
		{
			if(pPlayerPool->GetSlotState(playerId))
			{
				playername = pPlayerPool->GetPlayerName(playerId);
				playercolor = pPlayerPool->GetAt(playerId)->GetPlayerColorAsARGB();
			}
		}

		if(pPlayerPool->GetLocalPlayerID() == killerId)
		{
			killername = pPlayerPool->GetLocalPlayerName();
			killercolor = pPlayerPool->GetLocalPlayer()->GetPlayerColorAsARGB();
		}
		else
		{
			if(pPlayerPool->GetSlotState(killerId))
			{
				killername = pPlayerPool->GetPlayerName(killerId);
				killercolor = pPlayerPool->GetAt(killerId)->GetPlayerColorAsARGB();
			}
		}
	}

	pDeathMessage->MakeRecord(playername.c_str(), playercolor, killername.c_str(), killercolor, reason);
}

void ShowPlayerNameTagForPlayer(RPCParameters* rpcParams)
{
	Log("RPC: ShowPlayerNameTagForPlayer");
    unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	PLAYERID playerId;
	bool bShow;

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool) 
	{
		CRemotePlayer *pPlayer = pPlayerPool->GetAt(playerId);
		if(pPlayer)
			pPlayer->m_bShowNameTag = bShow;
	}
}

void RegisterScriptRPCs(RakClientInterface* pRakClient)
{
	Log("Registering ScriptRPC's..");
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_DisplayGameText, DisplayGameText);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerGravity, SetPlayerGravity);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ForceClassSelection, ForceClassSelection);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerPos, SetPlayerPos);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerCameraPos, SetPlayerCameraPos);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerCameraLookAt, SetPlayerCameraLookAt);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerFacingAngle, SetPlayerFacingAngle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerFightingStyle, SetPlayerFightingStyle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerSkin, SetPlayerSkin);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ApplyAnimation, ApplyAnimation);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ClearAnimations, ClearAnimations);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetSpawnInfo, SetSpawnInfo);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_CreateExplosion, CreateExplosion);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerHealth, SetPlayerHealth);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerArmour, SetPlayerArmour);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerColor, SetPlayerColor);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerName, SetPlayerName);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerPosFindZ, SetPlayerPosFindZ);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerInterior, SetInteriorId);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerMapIcon, SetPlayerMapIcon);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_RemovePlayerMapIcon, RemovePlayerMapIcon);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetCameraBehindPlayer, SetCameraBehindPlayer);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerSpecialAction, SetPlayerSpecialAction);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_TogglePlayerSpectating, TogglePlayerSpectating);
	//pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerSpectating, SetPlayerSpectating);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_PlayerSpectatePlayer, PlayerSpectatePlayer);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_PlayerSpectateVehicle, PlayerSpectateVehicle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_PutPlayerInVehicle, PutPlayerInVehicle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetVehicleParamsForPlayer, SetVehicleParamsForPlayer);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetVehicleParamsEx, SetVehicleParamsEx);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_GivePlayerMoney, GivePlayerMoney);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ResetPlayerMoney, ResetPlayerMoney);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_LinkVehicleToInterior, LinkVehicleToInterior);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_RemovePlayerFromVehicle, RemovePlayerFromVehicle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetVehicleHealth, SetVehicleHealth);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetVehiclePos, SetVehiclePos);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetVehicleVelocity, SetVehicleVelocity);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetNumberPlate, SetNumberPlate);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetVehicleZAngle, SetVehicleZAngle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_AttachTrailerToVehicle, AttachTrailerToVehicle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_DetachTrailerFromVehicle, DetachTrailerFromVehicle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_InterpolateCamera, InterpolateCamera);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_GangZoneCreate, GangZoneCreate);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_GangZoneDestroy, GangZoneDestroy);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_GangZoneFlash, GangZoneFlash);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_GangZoneStopFlash, GangZoneStopFlash);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_CreateObject, CreateObject);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetObjectPos, SetObjectPos);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetObjectRot, SetObjectRot);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_DestroyObject, DestroyObject);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_PlaySound, PlaySound);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerWantedLevel, SetPlayerWantedLevel);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_TogglePlayerControllable, TogglePlayerControllable);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_GivePlayerWeapon, GivePlayerWeapon);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerArmedWeapon, SetPlayerArmedWeapon);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerAmmo, SetPlayerAmmo);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ResetPlayerWeapons, ResetPlayerWeapons);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ShowTextDraw, ShowTextDraw);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_TextDrawHideForPlayer, TextDrawHideForPlayer);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_TextDrawSetString, TextDrawSetString);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ClickTextDraw, ClickTextDraw);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerAttachedObject, SetPlayerAttachedObject);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_MoveObject, MoveObject);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_StopObject, StopObject);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerTeam, SetPlayerTeam);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ApplyActorAnimation, ApplyActorAnimation);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ClearActorAnimations, ClearActorAnimations);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetActorFacingAngle, SetActorFacingAngle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetActorPos, SetActorPos);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetActorHealth, SetActorHealth);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_RemoveBuildingForPlayer, RemoveBuildingForPlayer);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_AttachObjectToPlayer, AttachObjectToPlayer);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetObjectMaterial, SetObjectMaterial);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerVelocity, SetPlayerVelocity);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_DeathMessage, DeathMessage);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ShowPlayerNameTagForPlayer, ShowPlayerNameTagForPlayer);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_PlayAudioStream, ScrPlayAudioStream);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_StopAudioStream, ScrStopAudioStream);
}

void UnRegisterScriptRPCs(RakClientInterface* pRakClient)
{
	Log("Unregistering ScriptRPC's..");
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DisplayGameText);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerGravity);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ForceClassSelection);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerPos);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerCameraPos);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerCameraLookAt);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerFacingAngle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerFightingStyle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerSkin);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ApplyAnimation);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ClearAnimations);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetSpawnInfo);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_CreateExplosion);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerHealth);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerArmour);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerColor);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerName);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerPosFindZ);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerInterior);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerMapIcon);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_RemovePlayerMapIcon);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetCameraBehindPlayer);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerSpecialAction);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_TogglePlayerSpectating);
	//pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerSpectating);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_PlayerSpectatePlayer);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_PlayerSpectateVehicle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_PutPlayerInVehicle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetVehicleParamsForPlayer);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetVehicleParamsEx);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_GivePlayerMoney);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ResetPlayerMoney);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_LinkVehicleToInterior);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_RemovePlayerFromVehicle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetVehicleHealth);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetVehiclePos);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetVehicleVelocity);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetNumberPlate);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetVehicleZAngle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_AttachTrailerToVehicle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DetachTrailerFromVehicle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_InterpolateCamera);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_GangZoneCreate);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_GangZoneDestroy);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_GangZoneFlash);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_GangZoneStopFlash);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_CreateObject);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetObjectPos);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetObjectRot);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DestroyObject);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_PlaySound);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerWantedLevel);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_TogglePlayerControllable);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_GivePlayerWeapon);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerArmedWeapon);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerAmmo);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ResetPlayerWeapons);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ShowTextDraw);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_TextDrawHideForPlayer);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_TextDrawSetString);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ClickTextDraw);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerAttachedObject);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_MoveObject);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_StopObject);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerTeam);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ApplyActorAnimation);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ClearActorAnimations);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetActorFacingAngle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetActorPos);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetActorHealth);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_RemoveBuildingForPlayer);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_AttachObjectToPlayer);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetObjectMaterial);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerVelocity);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DeathMessage);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ShowPlayerNameTagForPlayer);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_PlayAudioStream);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_StopAudioStream);
}
