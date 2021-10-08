#include "../main.h"
#include "../game/game.h"
#include "netgame.h"
#include "../chatwindow.h"
#include "../dialog.h"
#include "vehiclepool.h"
#include "playerpool.h"

extern CGame *pGame;
extern CNetGame *pNetGame;
extern CChatWindow *pChatWindow;
extern CDialogWindow *pDialogWindow;

int iNetModeNormalOnfootSendRate	= NETMODE_ONFOOT_SENDRATE;
int iNetModeNormalInCarSendRate		= NETMODE_INCAR_SENDRATE;
int iNetModeFiringSendRate			= NETMODE_FIRING_SENDRATE;
int iNetModeSendMultiplier 			= NETMODE_SEND_MULTIPLIER;

int g_iLagCompensation = 0;

#define EVENT_TYPE_PAINTJOB			1
#define EVENT_TYPE_CARCOMPONENT		2
#define EVENT_TYPE_CARCOLOR			3
#define EVENT_ENTEREXIT_MODSHOP		4

uint16_t moto_models[] = {448,461,462,463,468,471,521,522,581,586};

void ProcessIncomingEvent(PLAYERID playerID, int event, uint32_t param1, uint32_t param2, uint32_t param3)
{
	uint32_t v;
	int iVehicleID;
	int iPaintJob;
	int iComponent;
	int iWait;
	CVehicle *pVehicle;
	CRemotePlayer *pRemote;

	if(!pNetGame) return;
	
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	
	uint16_t veh_model = pVehiclePool->GetAt(param1)->GetModelIndex();
	for(uint16_t i = 0; i < 10; i++)
	{
		if(veh_model == moto_models[i])
			return;
	}

	switch(event)
	{
		case EVENT_TYPE_PAINTJOB:
		iVehicleID = pVehiclePool->FindGtaIDFromID(param1);
		iPaintJob = (int)param2;

		if(iPaintJob < 0 || iPaintJob > 2)
				return;

		if(iVehicleID) ScriptCommand(&change_car_skin, iVehicleID, param2);
		break;
		case EVENT_TYPE_CARCOMPONENT:
		iVehicleID = pVehiclePool->FindGtaIDFromID(param1);
		iComponent = (int)param2;

		if(iComponent < 1000 || iComponent > 1193)
			break;

		//pGame->RequestModel(iComponent);
		//pGame->LoadRequestedModels(false);
		(( int (*)(int,int))(g_libGTASA+0x28EB10+1))(iComponent,1); //RequestModel
		(( int (*)(int,int))(g_libGTASA+0x28F2F8+1))(iComponent,0); //RequestVehicleUpgrade
		(( int (*)(bool))(g_libGTASA+0x294CB4+1))(0); //LoadAllRequestedModels
		//ScriptCommand(&request_car_component, iComponent);

		iWait = 10;
		while(!ScriptCommand(&is_component_available, iComponent) && iWait){
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			iWait--;
		}

		if(!iWait){
			#ifdef DEBUG_MODE
			pChatWindow->AddDebugMessage("Timeout on car component");
			#endif
			break;
		}

		if(iComponent > 1000 && iComponent < 1193)
			ScriptCommand(&add_car_component, iVehicleID, iComponent, &v);
		
		break;
		case EVENT_TYPE_CARCOLOR:
		pVehicle = pVehiclePool->GetAt((VEHICLEID)param1);
		if(pVehicle) pVehicle->SetColor((int)param2, (int)param3);
		break;
	}
}

void InitGame(RPCParameters *rpcParams)
{
	Log("RPC: InitGame");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsInitGame(Data,(iBitLength/8)+1,false);

	PLAYERID MyPlayerID;
	bool bLanMode, bStuntBonus;

	bsInitGame.ReadCompressed(pNetGame->m_bZoneNames);							// unknown
	bsInitGame.ReadCompressed(pNetGame->m_bUseCJWalk);							// native UsePlayerPedAnims(); +
	bsInitGame.ReadCompressed(pNetGame->m_bAllowWeapons);						// native AllowInteriorWeapons(allow); +
	bsInitGame.ReadCompressed(pNetGame->m_bLimitGlobalChatRadius);				// native LimitGlobalChatRadius(Float:chat_radius); +
	bsInitGame.Read(pNetGame->m_fGlobalChatRadius);								// +
	bsInitGame.ReadCompressed(bStuntBonus);										// native EnableStuntBonusForAll(enable); +
	bsInitGame.Read(pNetGame->m_fNameTagDrawDistance);							// native SetNameTagDrawDistance(Float:distance); +
	bsInitGame.ReadCompressed(pNetGame->m_bDisableEnterExits);					// native DisableInteriorEnterExits(); +
	bsInitGame.ReadCompressed(pNetGame->m_bNameTagLOS);							// native DisableNameTagLOS(); +
	bsInitGame.ReadCompressed(pNetGame->m_bManualVehicleEngineAndLight);		// native ManualVehicleEngineAndLights(); +
	bsInitGame.Read(pNetGame->m_iSpawnsAvailable);								// +
	bsInitGame.Read(MyPlayerID);												// 
	bsInitGame.ReadCompressed(pNetGame->m_bShowPlayerTags);						// native ShowNameTags(show); +
	bsInitGame.Read(pNetGame->m_iShowPlayerMarkers);							// native ShowPlayerMarkers(mode); +
	bsInitGame.Read(pNetGame->m_byteWorldTime);									// native SetWorldTime(hour); +
	bsInitGame.Read(pNetGame->m_byteWeather);									// native SetWeather(weatherid); +
	bsInitGame.Read(pNetGame->m_fGravity);										// native SetGravity(Float:gravity); +
	bsInitGame.ReadCompressed(bLanMode);										// 
	bsInitGame.Read(pNetGame->m_iDeathDropMoney);								// native SetDeathDropAmount(amount); +
	bsInitGame.ReadCompressed(pNetGame->m_bInstagib);							// always 0

	bsInitGame.Read(iNetModeNormalOnfootSendRate);
	bsInitGame.Read(iNetModeNormalInCarSendRate);
	bsInitGame.Read(iNetModeFiringSendRate);
	bsInitGame.Read(iNetModeSendMultiplier);

	bsInitGame.Read(pNetGame->m_iLagCompensation);								// lagcomp +
	g_iLagCompensation = pNetGame->m_iLagCompensation;
	
	uint8_t byteStrLen;
	bsInitGame.Read(byteStrLen);
	if(byteStrLen)																// SetGameModeText(); +
	{
		memset(pNetGame->m_szHostName, 0, sizeof(pNetGame->m_szHostName));
		bsInitGame.Read(pNetGame->m_szHostName, byteStrLen);
	}
	pNetGame->m_szHostName[byteStrLen] = '\0';

	uint8_t byteVehicleModels[212];
	bsInitGame.Read((char*)&byteVehicleModels[0], 212);							// don't use?
	bsInitGame.Read(pNetGame->m_iVehicleFriendlyFire);							// native EnableVehicleFriendlyFire(); +

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer *pLocalPlayer = nullptr;
	if(pPlayerPool) pLocalPlayer = pPlayerPool->GetLocalPlayer();
	
	pGame->SetGravity(pNetGame->m_fGravity);

	if(pNetGame->m_bDisableEnterExits)
		pGame->DisableInteriorEnterExits();

	pNetGame->SetGameState(GAMESTATE_CONNECTED);
	if(pLocalPlayer) 
		pLocalPlayer->HandleClassSelection();

	pGame->SetWorldWeather(pNetGame->m_byteWeather);
	pGame->ToggleCJWalk(pNetGame->m_bUseCJWalk);
	
	pGame->DisableAutoAim();

	if(pChatWindow) 
		pChatWindow->AddDebugMessage("Connected to {B9C9BF}%.64s", pNetGame->m_szHostName);
}

void ServerJoin(RPCParameters *rpcParams)
{
	Log("RPC: ServerJoin");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	char szPlayerName[MAX_PLAYER_NAME+1];
	PLAYERID playerId;
	unsigned char byteNameLen = 0;
	uint8_t bIsNPC = 0;
	int pading;

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);
	bsData.Read(playerId);
	bsData.Read(pading);
	bsData.Read(bIsNPC);
	bsData.Read(byteNameLen);
	bsData.Read(szPlayerName, byteNameLen);
	szPlayerName[byteNameLen] = '\0';

	pPlayerPool->New(playerId, szPlayerName, bIsNPC);
	Log("New player: %s[%i] - NPC: %d", szPlayerName, playerId, bIsNPC);
}

void ServerQuit(RPCParameters *rpcParams)
{
	Log("RPC: ServerQuit");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	PLAYERID playerId;
	uint8_t byteReason;
	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);
	bsData.Read(playerId);
	bsData.Read(byteReason);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	pPlayerPool->Delete(playerId, byteReason);

	Log("Delete player: %i. Reason: %d", playerId, byteReason);
}

void ScmEvent(RPCParameters *rpcParams)
{
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	PLAYERID bytePlayerID;
	int iEvent;
	uint32_t dwParam1, dwParam2, dwParam3;

	bsData.Read(bytePlayerID);
	bsData.Read(iEvent);
	bsData.Read(dwParam1);
	bsData.Read(dwParam2);
	bsData.Read(dwParam3);

	ProcessIncomingEvent(bytePlayerID, iEvent, dwParam1, dwParam2, dwParam3);
}

void ClientMessage(RPCParameters *rpcParams)
{
	Log("RPC: ClientMessage");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint32_t dwStrLen;
	uint32_t dwColor;

	bsData.Read(dwColor);
	bsData.Read(dwStrLen);
	char* szMsg = (char*)malloc(dwStrLen+1);
	bsData.Read(szMsg, dwStrLen);
	szMsg[dwStrLen] = 0;

	if(pChatWindow) 
		pChatWindow->AddClientMessage(dwColor, szMsg);

	free(szMsg);
}

void Chat(RPCParameters *rpcParams)
{
	Log("RPC: Chat");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	PLAYERID playerId;
	uint8_t byteTextLen;

	if(pNetGame->GetGameState() != GAMESTATE_CONNECTED)	return;

	unsigned char szText[256];
	memset(szText, 0, 256);

	bsData.Read(playerId);
	bsData.Read(byteTextLen);
	if(byteTextLen > sizeof(szText))
		return;

	bsData.Read((char*)szText, byteTextLen);
	szText[byteTextLen] = '\0';

	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
	{
		if(playerId == pPlayerPool->GetLocalPlayerID())
		{
			CLocalPlayer *pLocalPlayer = pPlayerPool->GetLocalPlayer();
			if(pLocalPlayer) 
			{
				pChatWindow->AddChatMessage(pPlayerPool->GetLocalPlayerName(),
				pLocalPlayer->GetPlayerColor(), (char*)szText);
			}
		} 
		else 
		{
			CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(playerId);
			if(pRemotePlayer)
				pRemotePlayer->Say(szText);
		}
	}
}

void RequestClass(RPCParameters *rpcParams)
{
	Log("RPC: RequestClass");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint8_t byteRequestOutcome = 0;
	PLAYER_SPAWN_INFO SpawnInfo;

	bsData.Read(byteRequestOutcome);
	bsData.Read((char*)&SpawnInfo, sizeof(PLAYER_SPAWN_INFO));

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
	{
		CLocalPlayer *pLocalPlayer = pPlayerPool->GetLocalPlayer();
		if(pLocalPlayer)
		{
			if(byteRequestOutcome)
			{
				pLocalPlayer->SetSpawnInfo(&SpawnInfo);
				pLocalPlayer->HandleClassSelectionOutcome();
			}
		}
	}
}

void SetWeather(RPCParameters *rpcParams)
{
	Log("RPC: Weather");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	
	uint8_t byteWeather;

	bsData.Read(byteWeather);

	pNetGame->m_byteWeather = byteWeather;
	pGame->SetWorldWeather(byteWeather);
}

void RequestSpawn(RPCParameters *rpcParams)
{
	Log("RPC: RequestSpawn");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint8_t byteRequestOutcome = 0;

	bsData.Read(byteRequestOutcome);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool) 
	{
		CLocalPlayer *pLocalPlayer = pPlayerPool->GetLocalPlayer();
		if(pLocalPlayer)
		{
			if(byteRequestOutcome == 2 || (byteRequestOutcome && pLocalPlayer->m_bWaitingForSpawnRequestReply))
				pLocalPlayer->Spawn();
			else pLocalPlayer->m_bWaitingForSpawnRequestReply = false;
		}
	}
}

void SetWorldTime(RPCParameters *rpcParams)
{
	Log("RPC: SetWorldTime");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint8_t byteWorldTime;

	bsData.Read(byteWorldTime);
	
	pNetGame->m_byteWorldTime = byteWorldTime;
}

void SetPlayerTime(RPCParameters *rpcParams)
{
	Log("RPC: SetPlayerTime");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint8_t byteHour;
	uint8_t byteMinute;

	bsData.Read(byteHour);
	bsData.Read(byteMinute);

	pGame->SetWorldTime(byteHour, byteMinute);
	pNetGame->m_byteWorldTime = byteHour;
	pNetGame->m_byteWorldMinute = byteMinute;
}

void WorldPlayerAdd(RPCParameters *rpcParams)
{
	Log("RPC: WorldPlayerAdd");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	PLAYERID playerId;
	uint8_t byteFightingStyle = 4;
	uint8_t byteTeam = 255;
	unsigned int iSkin;
	VECTOR vecPos;
	float fRotation;
	uint32_t dwColor;
	bool bVisible;

	bsData.Read(playerId);
	bsData.Read(byteTeam);
	bsData.Read(iSkin);
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	bsData.Read(fRotation);
	bsData.Read(dwColor);
	bsData.Read(byteFightingStyle);
	bsData.Read(bVisible);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
	{
		CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer)
			pRemotePlayer->Spawn(byteTeam, iSkin, &vecPos, fRotation, dwColor, byteFightingStyle, bVisible);
	}
}

void WorldPlayerRemove(RPCParameters *rpcParams)
{
	Log("RPC: WorldPlayerRemove");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	PLAYERID playerId;
	
	bsData.Read(playerId);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
	{
		CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer) 
			pRemotePlayer->Remove();
	}
}

void WorldPlayerDeath(RPCParameters *rpcParams)
{
	Log("RPC: WorldPlayerDeath");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	PLAYERID playerId;

	bsData.Read(playerId);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool) 
	{
		CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer)
			pRemotePlayer->HandleDeath();
	}
}

void SetCheckpoint(RPCParameters *rpcParams)
{
	Log("RPC: SetCheckpoint");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	VECTOR vecPos, Extent;
	float fSize;

	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	bsData.Read(fSize);
	Extent.X = fSize;
	Extent.Y = fSize;
	Extent.Z = fSize;

	pGame->SetCheckpointInformation(&vecPos, &Extent);
	pGame->ToggleCheckpoints(true);
}

void DisableCheckpoint(RPCParameters *rpcParams)
{
	Log("RPC: DisableCheckpoint");
	pGame->ToggleCheckpoints(false);
}

void SetRaceCheckpoint(RPCParameters *rpcParams)
{
	Log("RPC: SetRaceCheckpoint");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	uint8_t byteType;
	VECTOR vecPos, vecNextPos;
	float fSize;

	bsData.Read(byteType);
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	bsData.Read(vecNextPos.X);
	bsData.Read(vecNextPos.Y);
	bsData.Read(vecNextPos.Z);
	bsData.Read(fSize);

	pGame->SetRaceCheckpointInformation(byteType, &vecPos, &vecNextPos, fSize);
	pGame->ToggleCheckpoints(true);
}

void DisableRaceCheckpoint(RPCParameters *rpcParams)
{
	Log("RPC: DisableRaceCheckpoint");
	pGame->ToggleRaceCheckpoints(false);
}

void WorldVehicleAdd(RPCParameters *rpcParams)
{
	Log("RPC: WorldVehicleAdd");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	NEW_VEHICLE newVehicle;

	bsData.Read((char*)&newVehicle, sizeof(NEW_VEHICLE));

	if(newVehicle.iVehicleType < 400 || newVehicle.iVehicleType > 611) return;

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(pVehiclePool)
		pVehiclePool->New(&newVehicle);
	int iVehicle = pVehiclePool->FindGtaIDFromID(newVehicle.VehicleID);
	uint16_t veh_model = pVehiclePool->GetAt(newVehicle.VehicleID)->GetModelIndex();
	for(uint16_t i = 0; i < 10; i++)
	{
		if(veh_model == moto_models[i])
			return;
	}

	if(iVehicle)
	{
		for(int i = 0; i < 14; i++)
		{
			uint32_t v = 0;

			uint32_t modslot = newVehicle.byteModSlots[i];
			if(modslot == 0)
				continue;

			modslot += 999;

			if(modslot < 1000 || modslot > 1193)
				continue;

			//pGame->RequestModel(iComponent);
			//pGame->LoadRequestedModels(false);
			(( int (*)(int,int))(g_libGTASA+0x28EB10+1))(modslot,1); //RequestModel
			(( int (*)(int,int))(g_libGTASA+0x28F2F8+1))(modslot,0); //RequestVehicleUpgrade
			(( int (*)(bool))(g_libGTASA+0x294CB4+1))(0); //LoadAllRequestedModels
			//ScriptCommand(&request_car_component, iComponent);

			int iWait = 10;
			while(!ScriptCommand(&is_component_available,modslot) && iWait){
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				iWait--;
			}
			if(!iWait){
				#ifdef BUILD_BETA
				pChatWindow->AddDebugMessage("Timeout on car component");
				#endif
				continue;
			}

			ScriptCommand(&add_car_component, iVehicle, modslot, &v);
		}

		if(newVehicle.bytePaintjob){
			newVehicle.bytePaintjob--;
			
			if(newVehicle.bytePaintjob < 0 || newVehicle.bytePaintjob > 2)
				return;

			ScriptCommand(&change_car_skin, iVehicle, newVehicle.bytePaintjob);
		}
	}
}

void WorldVehicleRemove(RPCParameters *rpcParams)
{
	Log("RPC: WorldVehicleRemove");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	VEHICLEID vehicleId;

	bsData.Read(vehicleId);

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(pVehiclePool)
		pVehiclePool->Delete(vehicleId);
}

void EnterVehicle(RPCParameters *rpcParams)
{
	Log("RPC: EnterVehicle");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	
	PLAYERID playerId;
	VEHICLEID vehicleId;
	uint8_t bytePassenger;

	bsData.Read(playerId);
	bsData.Read(vehicleId);
	bsData.Read(bytePassenger);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
	{
		CRemotePlayer *pPlayer = pPlayerPool->GetAt(playerId);
		if(pPlayer)
			pPlayer->EnterVehicle(vehicleId, bytePassenger);
	}	
}

void ExitVehicle(RPCParameters *rpcParams)
{
	Log("RPC: ExitVehicle");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	PLAYERID playerId;
	VEHICLEID vehicleId;

	bsData.Read(playerId);
	bsData.Read(vehicleId);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
	{
		CRemotePlayer *pPlayer = pPlayerPool->GetAt(playerId);
		if(pPlayer)
			pPlayer->ExitVehicle();
	}	
}

void DialogBox(RPCParameters *rpcParams)
{
	Log("RPC: DialogBox");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	uint16_t wDialogID;
	uint8_t byteDialogStyle;
	uint8_t len;
	char szBuff[4096+1];
	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	pDialogWindow->Clear();

	bsData.Read(wDialogID);
	bsData.Read(byteDialogStyle);
	pDialogWindow->m_wDialogID = wDialogID;
	pDialogWindow->m_byteDialogStyle = byteDialogStyle;

	// title
	bsData.Read(len);
	bsData.Read(szBuff, len);
	szBuff[len] = '\0';
	cp1251_to_utf8(pDialogWindow->m_utf8Title, szBuff);

	// button1
	bsData.Read(len);
	bsData.Read(szBuff, len);
	szBuff[len] = '\0';
	cp1251_to_utf8(pDialogWindow->m_utf8Button1, szBuff);

	// button2
	bsData.Read(len);
	bsData.Read(szBuff, len);
	szBuff[len] = '\0';
	cp1251_to_utf8(pDialogWindow->m_utf8Button2, szBuff);

	// info
	stringCompressor->DecodeString(szBuff, 4096, &bsData);
	pDialogWindow->SetInfo(szBuff, strlen(szBuff));

	if(wDialogID < 0)
		return;

	if( pDialogWindow->m_utf8Title[0] == '\0' && pDialogWindow->m_utf8Button1[0] == '\0' && pDialogWindow->m_utf8Button2[0] == '\0' && pDialogWindow->m_utf8Title[0] == '\0' )
		return;

	pDialogWindow->Show(true);
}

void GameModeRestart(RPCParameters *rpcParams)
{
	Log("RPC: GameModeRestart");
	pChatWindow->AddInfoMessage("The server is restarting..");
	pNetGame->ShutDownForGameRestart();
}

#define REJECT_REASON_BAD_VERSION   1
#define REJECT_REASON_BAD_NICKNAME  2
#define REJECT_REASON_BAD_MOD		3
#define REJECT_REASON_BAD_PLAYERID	4
void ConnectionRejected(RPCParameters *rpcParams)
{
	Log("RPC: ConnectionRejected");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint8_t byteRejectReason;

	bsData.Read(byteRejectReason);

	if(pChatWindow)
	{
		if(byteRejectReason == REJECT_REASON_BAD_VERSION)
			pChatWindow->AddInfoMessage("CONNECTION REJECTED: Incorrect Version.");
		else if(byteRejectReason == REJECT_REASON_BAD_NICKNAME)
		{
			pChatWindow->AddInfoMessage("CONNECTION REJECTED: Unacceptable NickName");
			pChatWindow->AddInfoMessage("Please choose another nick between and 3-20 characters");
			pChatWindow->AddInfoMessage("Please use only a-z, A-Z, 0-9");
			pChatWindow->AddInfoMessage("Use /quit to exit or press ESC and select Quit Game");
		}
		else if(byteRejectReason == REJECT_REASON_BAD_MOD)
		{
			pChatWindow->AddInfoMessage("CONNECTION REJECTED: Bad mod version.");
		}
		else if(byteRejectReason == REJECT_REASON_BAD_PLAYERID)
		{
			pChatWindow->AddInfoMessage("CONNECTION REJECTED: Unable to allocate a player slot.");
		}
	}

	pNetGame->GetRakClient()->Disconnect(500);
}

void CreatePickup(RPCParameters *rpcParams)
{
	Log("RPC: CreatePickup");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	PICKUP Pickup;
	int iIndex;

	bsData.Read(iIndex);
	bsData.Read((char*)&Pickup, sizeof(PICKUP));

	CPickupPool *pPickupPool = pNetGame->GetPickupPool();
	if(pPickupPool)
		pPickupPool->New(&Pickup, iIndex);
}

void DestroyPickup(RPCParameters *rpcParams)
{
	Log("RPC: DestroyPickup");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	int iIndex;

	bsData.Read(iIndex);

	CPickupPool *pPickupPool = pNetGame->GetPickupPool();
	if(pPickupPool) 
		pPickupPool->Destroy(iIndex);
}

void Create3DTextLabel(RPCParameters *rpcParams)
{
	Log("RPC: Create3DTextLabel");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	uint16_t wLabelId;
	uint32_t dwColor;
	VECTOR vecPos;
	float fDrawDistance;
	uint8_t byteTestLOS;
	PLAYERID playerId;
	VEHICLEID vehicleId;
	char szBuff[4096];

	bsData.Read(wLabelId);
	bsData.Read(dwColor);
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	bsData.Read(fDrawDistance);
	bsData.Read(byteTestLOS);
	bsData.Read(playerId);
	bsData.Read(vehicleId);
	stringCompressor->DecodeString(szBuff, 4096, &bsData);

	CText3DLabelsPool *pLabelPool = pNetGame->GetLabelPool();
	if(pLabelPool)
		pLabelPool->CreateTextLabel(wLabelId, szBuff, dwColor, vecPos.X, vecPos.Y, vecPos.Z, fDrawDistance, byteTestLOS, playerId, vehicleId);
}

void Delete3DTextLabel(RPCParameters *rpcParams)
{
	Log("RPC: Delete3DTextLabel");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data, (iBitLength / 8) + 1, false);

	uint16_t wLabelId;

	bsData.Read(wLabelId);

	CText3DLabelsPool *pLabelPool = pNetGame->GetLabelPool();
	if(pLabelPool)
		pLabelPool->Delete(wLabelId);
}

void Update3DTextLabel(RPCParameters *rpcParams)
{
	Log("RPC: Update3DTextLabel");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data, (iBitLength / 8) + 1, false);

	uint16_t wLabelId;

	bsData.Read(wLabelId);

	CText3DLabelsPool *pLabelPool = pNetGame->GetLabelPool();
	if(pLabelPool)
		pLabelPool->Delete(wLabelId);
}

void ChatBubble(RPCParameters *rpcParams)
{
	Log("RPC: ChatBubble");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	PLAYERID playerid;
	CHATBUBBLE_DATA data;

	bsData.Read(playerid);
	bsData.Read(data.dwColor);
	bsData.Read(data.fDrawDistance);
	bsData.Read(data.iElapseTime);
	bsData.Read(data.iTextlen);
	if(data.iTextlen > 255)
		return;

	char szBuff[0xFF];
	bsData.Read(szBuff, data.iTextlen);
	szBuff[data.iTextlen] = '\0';
	cp1251_to_utf8(data.szText, szBuff);

	if(pNetGame)
	{
		CChatBubblePool *pBubblePool = pNetGame->GetChatBubblePool();
		if(pBubblePool)
			pBubblePool->AddChatBubble(playerid, data);
	}
}

void WorldActorAdd(RPCParameters* rpcParams)
{
	Log("RPC: WorldActorAdd");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	uint16_t actorId;
	uint32_t iSkinId;
	VECTOR vecPos;
	float fRotation;
	float fHealth;
	bool bInvulnerable;

	bsData.Read(actorId);
	bsData.Read(iSkinId);
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	bsData.Read(fRotation);
	bsData.Read(fHealth);
	bsData.Read(bInvulnerable);

	CActorPool *pActorPool = pNetGame->GetActorPool();
	if(pActorPool)
		pActorPool->Spawn(actorId, iSkinId, vecPos, fRotation, fHealth, bInvulnerable);
}

void WorldActorRemove(RPCParameters* rpcParams)
{
	Log("RPC: WorldActorRemove");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	uint16_t actorId;

	bsData.Read(actorId);
	
	CActorPool *pActorPool = pNetGame->GetActorPool();
	if(pActorPool)
		pActorPool->Delete(actorId);
}

void DamageVehicle(RPCParameters *rpcParams)
{
	Log("RPC: DamageVehicle");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
   
	VEHICLEID vehicleId;
	uint32_t dwPanels;
	uint32_t dwDoors;
	uint8_t byteLights;
	uint8_t byteTire;
	
	bsData.Read(vehicleId);
	bsData.Read(dwPanels);
	bsData.Read(dwDoors);
	bsData.Read(byteLights);
	bsData.Read(byteTire);

	if(vehicleId < 0 || vehicleId > MAX_VEHICLES) return;

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(pVehiclePool)
	{
		CVehicle *pVehicle = pVehiclePool->GetAt(vehicleId);
		if(pVehicle)
			pVehicle->UpdateDamageStatus(dwPanels, dwDoors, byteLights, byteTire);
	}
}

void UpdateScoresPingsIPs(RPCParameters *rpcParams)
{
	Log("RPC: UpdateScoresPingsIPs");
	unsigned char *Data = reinterpret_cast<unsigned char*>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(Data, (iBitLength / 8) + 1, false);

	PLAYERID bytePlayerId;
	int iPlayerScore;
	uint32_t dwPlayerPing;

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	for(PLAYERID i = 0; i < (iBitLength / 8) / 9; i++)
	{
		bsData.Read(bytePlayerId);
		bsData.Read(iPlayerScore);
		bsData.Read(dwPlayerPing);

		pPlayerPool->UpdateScore(bytePlayerId, iPlayerScore);
		pPlayerPool->UpdatePing(bytePlayerId, dwPlayerPing);
		//Log("Playerid: %d score: %d ping: %d", bytePlayerId, iPlayerScore, dwPlayerPing);
	}
}

void RegisterRPCs(RakClientInterface* pRakClient)
{
	Log("Registering RPC's..");
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_InitGame, InitGame);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ServerJoin, ServerJoin);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ServerQuit, ServerQuit);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ClientMessage, ClientMessage);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_Chat, Chat);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_RequestClass, RequestClass);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_RequestSpawn, RequestSpawn);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetWeather, SetWeather);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetWorldTime, SetWorldTime);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetPlayerTime, SetPlayerTime);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldPlayerAdd, WorldPlayerAdd);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldPlayerRemove, WorldPlayerRemove);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldPlayerDeath, WorldPlayerDeath);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetCheckpoint, SetCheckpoint);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_DisableCheckpoint, DisableCheckpoint);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetRaceCheckpoint, SetRaceCheckpoint);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_DisableRaceCheckpoint, DisableRaceCheckpoint);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldVehicleAdd, WorldVehicleAdd);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldVehicleRemove, WorldVehicleRemove);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_EnterVehicle, EnterVehicle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ExitVehicle, ExitVehicle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ShowDialog, DialogBox);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_GameModeRestart, GameModeRestart);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ConnectionRejected, ConnectionRejected);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_CreatePickup, CreatePickup);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_DestroyPickup, DestroyPickup);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_Create3DTextLabel, Create3DTextLabel);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_Update3DTextLabel, Update3DTextLabel);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SCMEvent, ScmEvent);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ChatBubble, ChatBubble);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ShowActor, WorldActorAdd);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_HideActor, WorldActorRemove);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_DamageVehicle, DamageVehicle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_UpdateScoresPingsIPs, UpdateScoresPingsIPs);
}

void UnRegisterRPCs(RakClientInterface* pRakClient)
{
	Log("UnRegistering RPC's..");
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_InitGame);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ServerJoin);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ServerQuit);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ClientMessage);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_Chat);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_RequestClass);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_RequestSpawn);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetWeather);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetWorldTime);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetPlayerTime);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldPlayerAdd);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldPlayerRemove);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldPlayerDeath);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetCheckpoint);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DisableCheckpoint);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetRaceCheckpoint);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DisableRaceCheckpoint);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldVehicleAdd);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldVehicleRemove);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_EnterVehicle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ExitVehicle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ShowDialog);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_GameModeRestart);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ConnectionRejected);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_CreatePickup);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DestroyPickup);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_Create3DTextLabel);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_Update3DTextLabel);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SCMEvent);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ChatBubble);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ShowActor);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_HideActor);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DamageVehicle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_UpdateScoresPingsIPs);
}
