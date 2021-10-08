#include "main.h"
#include "gui/gui.h"
#include "chatwindow.h"
#include "game/game.h"
#include "net/netgame.h"
#include "util/armhook.h"
#include "settings.h"
#include "cmdprocs.h"

extern CSettings *pSettings;
extern CChatWindow *pChatWindow;
extern CGame *pGame;
extern CNetGame *pNetGame;
extern CGUI *pGUI;

void cmd_quit(const char *params)
{
    QuitGame();
}

void cmd_framelimit(const char *params)
{
    if(strlen(params)) 
    {
        int framelimit = atoi(params);
        if(framelimit >= 40 && framelimit <= 144)
        {
            char fpsch[15];
            sprintf(fpsch, "\\x%x", framelimit);
            WriteMemory(g_libGTASA+0x463FE8, (uintptr_t)fpsch, 1);
            WriteMemory(g_libGTASA+0x56C1F6, (uintptr_t)fpsch, 1);
            WriteMemory(g_libGTASA+0x56C126, (uintptr_t)fpsch, 1);
            WriteMemory(g_libGTASA+0x95B074, (uintptr_t)fpsch, 1);
            WriteMemory(g_libGTASA+0x56C1A2, (uintptr_t)fpsch, 1);
            WriteMemory(g_libGTASA+0x5D1020 + 0xC, (uintptr_t)fpsch, 1);
            pChatWindow->AddInfoMessage("-> FrameLimiter: %u", framelimit);
        }
        else pChatWindow->AddInfoMessage("-> FrameLimiter: valid amounts are 20-90");
    }
}

void cmd_reconnect(const char *params) 
{
	pChatWindow->AddInfoMessage("Reconnecting...");
	pNetGame->ShutDownForGameRestart();
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	for(uint16_t playerId = 0; playerId < MAX_PLAYERS; playerId++)
	{
		CRemotePlayer *pPlayer = pPlayerPool->GetAt(playerId);
		if(pPlayer) pPlayerPool->Delete(playerId, 0);
	}
	pNetGame->SetGameState(GAMESTATE_WAIT_CONNECT);
}

void cmd_savepos(const char *params)
{
    CPlayerPed *pPlayer = pGame->FindPlayerPed();
    FILE *fileOut;
    uint32_t dwVehicleID;
    float fZAngle = 0.0f;
    
    char ff[0xFF];
    sprintf(ff, "%sSAMP/savedposition.txt", g_pszStorage);
    fileOut = fopen(ff, "a");
    if(!fileOut)
        pChatWindow->AddDebugMessage("Cant open savedposition.txt");

    if(pPlayer->IsInVehicle())
    {
        //incar savepos
        VEHICLE_TYPE *pVehicle = pPlayer->GetGtaVehicle();
        VEHICLEID m_dwGTAId = GamePool_Vehicle_GetIndex(pVehicle);
        ScriptCommand(&get_car_z_angle, m_dwGTAId, &fZAngle);
        fprintf(fileOut, "AddStaticVehicle(%u,%.4f,%.4f,%.4f,%.4f,%u,%u); // %s\n",
            pVehicle->entity.nModelIndex,
            pVehicle->entity.mat->pos.X, 
            pVehicle->entity.mat->pos.Y, 
            pVehicle->entity.mat->pos.Z, 
            fZAngle,
            pVehicle->byteColor1,
            pVehicle->byteColor2,
            params
        );
        pChatWindow->AddInfoMessage("-> InCar position saved.");
    }
    else
    {
        //onfoot savepos
        PED_TYPE *pActor = pPlayer->GetGtaActor();
        ScriptCommand(&get_actor_z_angle, pPlayer->m_dwGTAId, &fZAngle);
        fprintf(fileOut, "AddPlayerClass(%u,%.4f,%.4f,%.4f,%.4f,0,0,0,0,0,0); // %s\n", 
            pPlayer->GetModelIndex(),
            pActor->entity.mat->pos.X, 
            pActor->entity.mat->pos.Y, 
            pActor->entity.mat->pos.Z, 
            fZAngle, 
            params
        );
        pChatWindow->AddInfoMessage("-> OnFoot position saved.");
    }

    fclose(fileOut);
}

void cmd_rawsavepos(const char *params)
{
    CPlayerPed *pPlayer = pGame->FindPlayerPed();
    FILE *fileOut;
    uint32_t dwVehicleID;
    float fZAngle = 0.0f;

    if(pPlayer->IsInVehicle())
    {
        //incar savepos
        char ff[0xFF];
        sprintf(ff, "%sSAMP/rawvehicles.txt", g_pszStorage);
        fileOut = fopen(ff, "a");
        if(!fileOut)
            pChatWindow->AddDebugMessage("Cant open rawvehicles.txt");

        VEHICLE_TYPE *pVehicle = pPlayer->GetGtaVehicle();
        VEHICLEID m_dwGTAId = GamePool_Vehicle_GetIndex(pVehicle);
        ScriptCommand(&get_car_z_angle, m_dwGTAId, &fZAngle);
        fprintf(fileOut, "%u,%.4f,%.4f,%.4f,%.4f,%u,%u ; // %s\n",
            pVehicle->entity.nModelIndex,
            pVehicle->entity.mat->pos.X, 
            pVehicle->entity.mat->pos.Y, 
            pVehicle->entity.mat->pos.Z, 
            fZAngle,
            pVehicle->byteColor1,
            pVehicle->byteColor2,
            params
        );
        pChatWindow->AddInfoMessage("-> InCar position saved.");
    }
    else
    {
        //onfoot savepos
        char ff[0xFF];
        sprintf(ff, "%sSAMP/rawfoot.txt", g_pszStorage);
        fileOut = fopen(ff, "a");
        if(!fileOut)
            pChatWindow->AddDebugMessage("Cant open rawfoot.txt");

        PED_TYPE *pActor = pPlayer->GetGtaActor();
        ScriptCommand(&get_actor_z_angle, pPlayer->m_dwGTAId, &fZAngle);
        fprintf(fileOut, "%u,%.4f,%.4f,%.4f,%.4f ; // %s\n", 
            pPlayer->GetModelIndex(),
            pActor->entity.mat->pos.X, 
            pActor->entity.mat->pos.Y, 
            pActor->entity.mat->pos.Z, 
            fZAngle, 
            params
        );
        pChatWindow->AddInfoMessage("-> OnFoot position saved.");
    }

    fclose(fileOut);
}

void cmd_interior(const char *params)
{
    uint32_t dwRet;
    ScriptCommand(&get_active_interior, &dwRet);
    pChatWindow->AddDebugMessage("Current Interior: %u", dwRet);
}

void cmd_rcon(const char *params)
{
    if(!params) return;
    uint8_t bytePacketID = ID_RCON_COMMAND;
    RakNet::BitStream bsCommand;
    bsCommand.Write(bytePacketID);
    uint32_t dwCmdLen = (uint32_t)strlen(params);
    bsCommand.Write(dwCmdLen);
    bsCommand.Write(params, dwCmdLen);
    pNetGame->GetRakClient()->Send(&bsCommand, HIGH_PRIORITY, RELIABLE, 0);
}

void cmd_dl(const char *params)
{
    pGUI->bShowDebugLabels = !pGUI->bShowDebugLabels;
}

/*void cmd_headmove(const char *params)
{
    pNetGame->m_bHeadMove = !pNetGame->m_bHeadMove;
}*/

void SetupCommands()
{
    pChatWindow->AddCmdProc("q", cmd_quit);
    pChatWindow->AddCmdProc("quit", cmd_quit);
    pChatWindow->AddCmdProc("fpslimit", cmd_framelimit);
    pChatWindow->AddCmdProc("save", cmd_savepos);
    pChatWindow->AddCmdProc("rs", cmd_rawsavepos);
    pChatWindow->AddCmdProc("interior", cmd_interior);
    pChatWindow->AddCmdProc("rec", cmd_reconnect);
    pChatWindow->AddCmdProc("rcon", cmd_rcon);
    pChatWindow->AddCmdProc("dl", cmd_dl);
    //pChatWindow->AddCmdProc("headmove", cmd_headmove);
    Log("Commands initialized.");
}
