#include "main.h"
#include "game/game.h"
#include "game/RW/RenderWare.h"
#include "net/netgame.h"
#include "gui/gui.h"
#include "chatwindow.h"
#include "spawnscreen.h"
#include "playertags.h"
#include "dialog.h"
#include "keyboard.h"
#include "extrakeyboard.h"
#include "settings.h"
#include "debug.h"
#include "util/armhook.h"
#include "game/snapshothelper.h"
#include "scoreboard.h"
#include "cmdprocs.h"
#include "deathmessage.h"
#include "checkfilehash.h"
#include "game/materialtext.h"
#include "game/objectmaterial.h"
#include "game/audiostream.h"
#include "passengerbutton.h"

// voice
#include "voice/Plugin.h"

extern bool m_bAudioStreamPaused;
uintptr_t g_libGTASA = 0;
uintptr_t g_libSAMP = 0;
const char* g_pszStorage = nullptr;

CGame *pGame = nullptr;
CNetGame *pNetGame = nullptr;
CDialogWindow *pDialogWindow = nullptr;
CChatWindow *pChatWindow = nullptr;
CSpawnScreen *pSpawnScreen = nullptr;
CPlayerTags *pPlayerTags = nullptr;
CGUI *pGUI = nullptr;
CKeyBoard *pKeyBoard = nullptr;
CExtraKeyBoard *pExtraKeyBoard = nullptr;
CDebug *pDebug = nullptr;
CSettings *pSettings = nullptr;
CSnapShotHelper *pSnapShotHelper = nullptr;
CScoreBoard *pScoreBoard = nullptr;
CDeathMessage *pDeathMessage = nullptr;
CMaterialText *pMaterialText = nullptr;
CPassengerButton *pPassengerButton = nullptr;
CAudioStream *pAudioStream = nullptr;

#define UseSettings
const auto encryptedAddress = cryptor::create("samp.lostliferoleplay.com");

const auto encryptedPassword = cryptor::create("");
unsigned short usPort = 7777;

bool bGameInited = false;
bool bNetworkInited = false;
bool bQuitGame = false; 

uint16_t dwStartQuitTick = 0;

void InitHookStuff();
void InstallSpecialHooks();
void InitRenderWareFunctions();
void ApplyInGamePatches();
void ApplyPatches_level0();
void InitInGame();
void ProcessSAMPGraphic();
void ProcessSAMPGraphicFrame();
void InitNetwork();
void ProcessSAMPNetwork();
/*void InitInGame();
void MainLoop();*/
void QuitGame();

using namespace std;

string response;
size_t writeCallback(char* buf, size_t size, size_t nmemb, void* up) 
{
    for (int c = 0; c < size * nmemb; c++)
        response.push_back(buf[c]);

    return size * nmemb;
}

void InitSAMP()
{
	Log("SAMP library loaded! Build time: " __DATE__ " " __TIME__);

	Log("Initializing SA-MP..");

	if(!g_pszStorage)
	{
		Log("Error: storage path not found!");
		std::terminate();
		return;
	}

	Log("Storage: %s", g_pszStorage);

	pSettings = new CSettings();

	Log("Checking samp files..");
	if(!FileCheckSum())
	{
		Log("SOME FILES HAVE BEEN MODIFIED. YOU NEED REINSTALL SAMP!");
		//std::terminate();
		//return;
	}

	const char *szKey = cryptor::create("F9AOS0S8N8UJSSD8AKGN23A82KLSD1YA").decrypt();
	std::string strKey = szKey;
	const char *szText = cryptor::create("pRWk8fBbhRniAadB7k4J3YGcpk9Mu5kL").decrypt();
	std::string strText = szText;
	std::string encode = encrypt_key(strText, strKey);
	std::string decode = decrypt_key(encode, strKey);
	
   	Log("Decrypted: %s -> Encrypted: %s", decode.c_str(), encode.c_str());
	Log("Key: %s", strKey.c_str());
}

void InitInMenu()
{
	pGame = new CGame();
	pGame->InitInMenu();

	if(pSettings->Get().bDebug) pDebug = new CDebug();
	
	// voice
	Plugin::OnPluginLoad();
	Plugin::OnSampLoad();

	pGUI = new CGUI();
	pDialogWindow = new CDialogWindow();
	pExtraKeyBoard = new CExtraKeyBoard();
	pKeyBoard = new CKeyBoard();
	pPlayerTags = new CPlayerTags();
	pChatWindow = new CChatWindow();
	pSpawnScreen = new CSpawnScreen();
	pSnapShotHelper = new CSnapShotHelper();
	pScoreBoard = new CScoreBoard();
	pDeathMessage = new CDeathMessage();
	pMaterialText = new CMaterialText();
	pAudioStream = new CAudioStream();
	pPassengerButton = new CPassengerButton();
}

void InitInGame()
{
	if(!bGameInited)
	{
		pGame->InitInGame();
		pGame->SetMaxStats();
		pAudioStream->Initialize();
		InitObjectMaterial();
		SetupCommands();
		
		// voice
		LogVoice("[dbg:samp:load] : module loading...");

		for(const auto& loadCallback : Samp::loadCallbacks)
		{
			if(loadCallback != nullptr)
				loadCallback();
		}

		Samp::loadStatus = true;

		LogVoice("[dbg:samp:load] : module loaded");

		bGameInited = true;
	}
}

void ProcessSAMPGraphic()
{
	if (pNetGame)
	{
		CTextDrawPool *pTextDrawPool = pNetGame->GetTextDrawPool();
		if(pTextDrawPool) pTextDrawPool->Draw();
	}
}

void ProcessSAMPGraphicFrame()
{
	if(pGUI) pGUI->Render();
}

void InitNetwork()
{
	if(!bNetworkInited && pSettings->Get().iFontOutline)
	{
		#ifdef UseSettings
		pNetGame = new CNetGame( 
			encryptedAddress.decrypt(),
			usPort, 
			pSettings->Get().szNickName,
			encryptedPassword.decrypt()
			);
	/*	pNetGame = new CNetGame( 
			pSettings->Get().szHost,
			pSettings->Get().iPort, 
			pSettings->Get().szNickName,
			pSettings->Get().szPassword
			);*/
		#else
		CURL *curl = curl_easy_init();
		uint8_t httpCode;
		if(curl != nullptr)
		{
			char szUrl[256];
			const auto szApiKey = cryptor::create("");
			sprintf(szUrl, cryptor::create("").decrypt(), szApiKey.decrypt());

			char szPostFields[32];
			sprintf(szPostFields, cryptor::create("hash_id=%s").decrypt(), pSettings->Get().szAuthKey);

			curl_easy_setopt(curl, CURLOPT_USERAGENT, cryptor::create("").decrypt());
			curl_easy_setopt(curl, CURLOPT_URL, szUrl);
			curl_easy_setopt(curl, CURLOPT_POST, 1);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, szPostFields);
			curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);

			CURLcode res = curl_easy_perform(curl);
			if(res != CURLE_OK) {
				Log("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			}
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
			curl_easy_cleanup(curl);
		}

		if(httpCode == 200)
		{
			Log("%s", response.c_str());

			uint8_t byteWithPassword;
			std::vector<std::string> getStringLine;
			std::stringstream ssLine(response);
			std::string tmpLine;
			while(std::getline(ssLine, tmpLine, '/'))
			{
				// push back the string to getStringLine
				if(!tmpLine[0])
				{
					byteWithPassword++;
					getStringLine.push_back(tmpLine);
				}
			}

			if(byteWithPassword == 3)
			{
				pNetGame = new CNetGame( 
					getStringLine[0].c_str(),
					getStringLine[1], 
					pSettings->Get().szNickName,
					getStringLine[2].c_str());
			}
			else
			{
				pNetGame = new CNetGame( 
					getStringLine[0].c_str(),
					getStringLine[1], 
					pSettings->Get().szNickName,
					"");
			}
		}
		else exit(0);
		#endif

		bNetworkInited = true;
	}
}

void ProcessSAMPNetwork()
{
	if(pNetGame) pNetGame->Process();
	if(pAudioStream) pAudioStream->Process();

	if(bQuitGame)
	{
		if((GetTickCount() - dwStartQuitTick) > 1000) 
		{
			if(pNetGame)
			{
				delete pNetGame;
				pNetGame = NULL;
			}
			
			exit(0);
		}
		return;
	}
}

/*void InitInGame()
{
	static bool bGameInited = false;
	static bool bNetworkInited = false;

	if(!bGameInited)
	{
		pGame->InitInGame();
		pGame->SetMaxStats();

		if(pDebug && !pSettings->Get().bOnline)
		{
			pDebug->SpawnLocalPlayer();
		}

		bGameInited = true;
		return;
	}

	if(!bNetworkInited && pSettings->Get().bOnline)
	{
		#ifdef UseSettings
		pNetGame = new CNetGame( 
			pSettings->Get().szHost,
			pSettings->Get().iPort, 
			pSettings->Get().szNickName,
			pSettings->Get().szPassword);
		#else
		pNetGame = new CNetGame( 
			encryptedAddress.decrypt(),
			usPort, 
			pSettings->Get().szNickName,
			encryptedPassword.decrypt());
		#endif
		
		bNetworkInited = true;
		return;
	}
}

void MainLoop()
{
	InitInGame();

	if(pDebug) pDebug->Process();
	if(pNetGame) pNetGame->Process();
}*/

void QuitGame() 
{
	if(pNetGame && pNetGame->GetGameState() == GAMESTATE_CONNECTED)
		pNetGame->GetRakClient()->Disconnect(500);
	
	bQuitGame = true;
	dwStartQuitTick = GetTickCount();
}

void handler(int signum, siginfo_t *info, void* contextPtr)
{
	ucontext* context = (ucontext_t*)contextPtr;

	if(info->si_signo == SIGSEGV)
	{
		Log("SIGSEGV | Fault address: 0x%X", info->si_addr);        
		Log("libGTASA base address: 0x%X", g_libGTASA);
		Log("libSAMP base address: 0x%X", g_libSAMP);      

		Log("register states:");        
		Log("r0: 0x%X, r1: 0x%X, r2: 0x%X, r3: 0x%X", context->uc_mcontext.arm_r0, context->uc_mcontext.arm_r1, context->uc_mcontext.arm_r2, context->uc_mcontext.arm_r3);        
		Log("r4: 0x%x, r5: 0x%x, r6: 0x%x, r7: 0x%x", context->uc_mcontext.arm_r4, context->uc_mcontext.arm_r5, context->uc_mcontext.arm_r6, context->uc_mcontext.arm_r7);        
		Log("r8: 0x%x, r9: 0x%x, sl: 0x%x, fp: 0x%x", context->uc_mcontext.arm_r8, context->uc_mcontext.arm_r9, context->uc_mcontext.arm_r10, context->uc_mcontext.arm_fp);        
		Log("ip: 0x%x, sp: 0x%x, lr: 0x%x, pc: 0x%x", context->uc_mcontext.arm_ip, context->uc_mcontext.arm_sp, context->uc_mcontext.arm_lr, context->uc_mcontext.arm_pc);         
		
		Log("backtrace (libGTASA | libSAMP):");        
		Log("1: 0x%X (0x%X | 0x%X)", context->uc_mcontext.arm_pc, context->uc_mcontext.arm_pc - g_libGTASA, context->uc_mcontext.arm_pc - g_libSAMP);        
		Log("2: 0x%X (0x%X | 0x%X)", context->uc_mcontext.arm_lr, context->uc_mcontext.arm_lr - g_libGTASA, context->uc_mcontext.arm_lr - g_libSAMP);         
		
		exit(0);//QuitGame();
	}

	return;
}

void *Init(void *p)
{
	ApplyPatches_level0();

	pthread_exit(0);
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	g_libGTASA = FindLibrary("libGTASA.so");
	if(g_libGTASA == 0)
	{
		Log("ERROR: libGTASA.so address not found!");
		return 0;
	}

	g_pszStorage = (const char*)(g_libGTASA+0x63C4B8);

	Log("libGTASA.so image base address: 0x%X", g_libGTASA);

	g_libSAMP = FindLibrary("libSAMP.so");
	Log("libSAMP.so image base address: 0x%X", g_libSAMP);

	srand(time(0));
	
	Log("Loading Bass Library");
	LoadBassLibrary();

	InitHookStuff();
	InitRenderWareFunctions();
	InstallSpecialHooks();

	pthread_t thread;
	pthread_create(&thread, 0, Init, 0);

	struct sigaction act;
	act.sa_sigaction = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &act, 0);

	return JNI_VERSION_1_4;
}

void Log(const char *fmt, ...)
{	
	char buffer[0xFF];
	static FILE* flLog = nullptr;

	if(flLog == nullptr && g_pszStorage != nullptr)
	{
		sprintf(buffer, "", g_pszStorage);

		flLog = fopen(buffer, "w");
	}

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	__android_log_write(ANDROID_LOG_INFO, "AXL", buffer);

	if(pDebug) pDebug->AddMessage(buffer);

	if(flLog == nullptr) return;
	fprintf(flLog, "%s\n", buffer);
	fflush(flLog);

	return;
}

void LogVoice(const char *fmt, ...)
{	
	char buffer[0xFF];
	static FILE* flLog = nullptr;

	if(flLog == nullptr && g_pszStorage != nullptr)
	{
		sprintf(buffer, "%sSAMP/%s", g_pszStorage, SV::kLogFileName);
		flLog = fopen(buffer, "w");
	}

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	__android_log_write(ANDROID_LOG_INFO, "AXL", buffer);

	if(pDebug) pDebug->AddMessage(buffer);

	if(flLog == nullptr) return;
	fprintf(flLog, "%s\n", buffer);
	fflush(flLog);

	return;
}

uint32_t GetTickCount()
{
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return (tv.tv_sec*1000+tv.tv_usec/1000);
}
int (*BASS_Init) (uint32_t, uint32_t, uint32_t);
int (*BASS_Free) (void);
int (*BASS_SetConfigPtr) (uint32_t, const char *);
int (*BASS_SetConfig) (uint32_t, uint32_t);
int (*BASS_ChannelStop) (uint32_t);
int (*BASS_StreamCreateURL) (char *, uint32_t, uint32_t, uint32_t);
int (*BASS_StreamCreate) (uint32_t, uint32_t, uint32_t, STREAMPROC *, void *);
int (*BASS_ChannelPlay) (uint32_t, bool);
int (*BASS_ChannelPause) (uint32_t);
int *BASS_ChannelGetTags;
int *BASS_ChannelSetSync;
int *BASS_StreamGetFilePosition;
int (*BASS_StreamFree) (uint32_t);
int (*BASS_ErrorGetCode) (void);
int (*BASS_Set3DFactors) (float, float, float);
int (*BASS_Set3DPosition) (const BASS_3DVECTOR *, const BASS_3DVECTOR *, const BASS_3DVECTOR *, const BASS_3DVECTOR *);
int (*BASS_Apply3D) (void);
int (*BASS_ChannelSetFX) (uint32_t, HFX);
int (*BASS_ChannelRemoveFX) (uint32_t, HFX);
int (*BASS_FXSetParameters) (HFX, const void *);
int (*BASS_IsStarted) (void);
int (*BASS_RecordGetDeviceInfo) (uint32_t, BASS_DEVICEINFO *);
int (*BASS_RecordInit) (int);
int (*BASS_RecordGetDevice) (void);
int (*BASS_RecordFree) (void);
int (*BASS_RecordStart) (uint32_t, uint32_t, uint32_t, RECORDPROC *, void *);
int (*BASS_ChannelSetAttribute) (uint32_t, uint32_t, float);
int (*BASS_ChannelGetData) (uint32_t, void *, uint32_t);
int (*BASS_RecordSetInput) (int, uint32_t, float);
int (*BASS_StreamPutData) (uint32_t, const void *, uint32_t);
int (*BASS_ChannelSetPosition) (uint32_t, uint64_t, uint32_t);
int (*BASS_ChannelIsActive) (uint32_t);
int (*BASS_ChannelSlideAttribute) (uint32_t, uint32_t, float, uint32_t);
int (*BASS_ChannelSet3DAttributes) (uint32_t, int, float, float, int, int, float);
int (*BASS_ChannelSet3DPosition) (uint32_t, const BASS_3DVECTOR *, const BASS_3DVECTOR *, const BASS_3DVECTOR *);
int (*BASS_SetVolume) (float);

void LoadBassLibrary()
{
	void *v0 = dlopen("/data/data/com.rockstargames.gtasa/lib/libbass.so", 1);
	if(!v0) Log("%s", dlerror());
		
	BASS_Init = (int (*)(uint32_t, uint32_t, uint32_t))dlsym(v0, "BASS_Init");
	BASS_Free = (int (*)(void))dlsym(v0, "BASS_Free");
	BASS_SetConfigPtr = (int (*)(uint32_t, const char *))dlsym(v0, "BASS_SetConfigPtr");
	BASS_SetConfig = (int (*)(uint32_t, uint32_t))dlsym(v0, "BASS_SetConfig");
	BASS_ChannelStop = (int (*)(uint32_t))dlsym(v0, "BASS_ChannelStop");
	BASS_StreamCreateURL = (int (*)(char *, uint32_t, uint32_t, uint32_t))dlsym(v0, "BASS_StreamCreateURL");
	BASS_StreamCreate = (int (*)(uint32_t, uint32_t, uint32_t, STREAMPROC *, void *))dlsym(v0, "BASS_StreamCreate");
	BASS_ChannelPlay = (int (*)(uint32_t, bool))dlsym(v0, "BASS_ChannelPlay");
	BASS_ChannelPause = (int (*)(uint32_t))dlsym(v0, "BASS_ChannelPause");
	BASS_ChannelGetTags = (int *)dlsym(v0, "BASS_ChannelGetTags");
	BASS_ChannelSetSync = (int *)dlsym(v0, "BASS_ChannelSetSync");
	BASS_StreamGetFilePosition = (int *)dlsym(v0, "BASS_StreamGetFilePosition");
	BASS_StreamFree = (int (*)(uint32_t))dlsym(v0, "BASS_StreamFree");
	BASS_ErrorGetCode = (int (*)(void))dlsym(v0, "BASS_ErrorGetCode");
	BASS_Set3DFactors = (int (*)(float, float, float))dlsym(v0, "BASS_Set3DFactors");
	BASS_Set3DPosition = (int (*)(const BASS_3DVECTOR *, const BASS_3DVECTOR *, const BASS_3DVECTOR *, const BASS_3DVECTOR *))dlsym(v0, "BASS_Set3DPosition");
	BASS_Apply3D = (int (*)(void))dlsym(v0, "BASS_Apply3D");
	BASS_ChannelSetFX = (int (*)(uint32_t, HFX))dlsym(v0, "BASS_ChannelSetFX");
	BASS_ChannelRemoveFX = (int (*)(uint32_t, HFX))dlsym(v0, "BASS_ChannelRemoveFX");
	BASS_FXSetParameters = (int (*)(HFX, const void *))dlsym(v0, "BASS_FXSetParameters");
	BASS_IsStarted = (int (*)(void))dlsym(v0, "BASS_IsStarted");
	BASS_RecordGetDeviceInfo = (int (*)(uint32_t, BASS_DEVICEINFO *))dlsym(v0, "BASS_RecordGetDeviceInfo");
	BASS_RecordInit = (int (*)(int))dlsym(v0, "BASS_RecordInit");
	BASS_RecordGetDevice = (int (*)(void))dlsym(v0, "BASS_RecordGetDevice");
	BASS_RecordFree = (int (*)(void))dlsym(v0, "BASS_RecordFree");
	BASS_RecordStart = (int (*)(uint32_t, uint32_t, uint32_t, RECORDPROC *, void *))dlsym(v0, "BASS_RecordStart");
	BASS_ChannelSetAttribute = (int (*)(uint32_t, uint32_t, float))dlsym(v0, "BASS_ChannelSetAttribute");
	BASS_ChannelGetData = (int (*)(uint32_t, void *, uint32_t))dlsym(v0, "BASS_ChannelGetData");
	BASS_RecordSetInput = (int (*)(int, uint32_t, float))dlsym(v0, "BASS_RecordSetInput");
	BASS_StreamPutData = (int (*)(uint32_t, const void *, uint32_t))dlsym(v0, "BASS_StreamPutData");
	BASS_ChannelSetPosition = (int (*)(uint32_t, uint64_t, uint32_t))dlsym(v0, "BASS_ChannelSetPosition");
	BASS_ChannelIsActive = (int (*)(uint32_t))dlsym(v0, "BASS_ChannelIsActive");
	BASS_ChannelSlideAttribute = (int (*)(uint32_t, uint32_t, float, uint32_t))dlsym(v0, "BASS_ChannelSlideAttribute");
	BASS_ChannelSet3DAttributes = (int (*)(uint32_t, int, float, float, int, int, float))dlsym(v0, "BASS_ChannelSet3DAttributes");
	BASS_ChannelSet3DPosition = (int (*)(uint32_t, const BASS_3DVECTOR *, const BASS_3DVECTOR *, const BASS_3DVECTOR *))dlsym(v0, "BASS_ChannelSet3DPosition");
	BASS_SetVolume = (int (*)(float))dlsym(v0, "BASS_SetVolume");
}
