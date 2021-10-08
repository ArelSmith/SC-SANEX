#include <pthread.h>

#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "entity.h"
#include "game/audiostream.h"

extern CGame *pGame;
extern CNetGame *pNetGame;

char m_szAudioStreamUrl[UNKNOWN_SIZE];
VECTOR m_fAudioStream;
float m_fAudioStreamRadius;
bool m_bAudioStreamUsePos;
bool m_bAudioStreamStop;
bool m_bAudioStreamPaused;
bool m_bAudioStreamOutStream;
pthread_t m_bAudioStreamThreadWorked;
uintptr_t bassStream;

void *audioStreamThread(void *p)
{
	m_bAudioStreamThreadWorked = 1;
	if(bassStream)
	{
		BASS_ChannelStop(bassStream);
		bassStream = 0;
	}
	bassStream = BASS_StreamCreateURL(m_szAudioStreamUrl, 0, 9699328, 0);
	BASS_ChannelPlay(bassStream, 1);
	
	while(!m_bAudioStreamStop)
		usleep(2000);
	
	BASS_ChannelStop(bassStream);
	bassStream = 0;
	m_bAudioStreamThreadWorked = 0;
	pthread_exit(0);
}

void CAudioStream::Process() 
{
	if(m_bAudioStreamUsePos)
	{
		CPlayerPed *pPlayer = pGame->FindPlayerPed();
		if(pPlayer)
		{
			PED_TYPE *pActor = pPlayer->GetGtaActor();
			VECTOR fS;

			fS.X = (pActor->entity.mat->pos.X - m_fAudioStream.X) * (pActor->entity.mat->pos.X - m_fAudioStream.X);
			fS.Y = (pActor->entity.mat->pos.Y - m_fAudioStream.Y) * (pActor->entity.mat->pos.Y - m_fAudioStream.Y);
			fS.Z = (pActor->entity.mat->pos.Z - m_fAudioStream.Z) * (pActor->entity.mat->pos.Z - m_fAudioStream.Z);
			if((float)sqrt(fS.X + fS.Y + fS.Z) <= m_fAudioStreamRadius)
			{
				if(m_bAudioStreamOutStream || m_bAudioStreamPaused)
				{
					pthread_create(&m_bAudioStreamThreadWorked, 0, audioStreamThread, 0);//Play(m_szAudioStreamUrl, m_fAudioStream.X, m_fAudioStream.Y, m_fAudioStream.Z, m_fAudioStreamRadius, m_bAudioStreamUsePos);
					m_bAudioStreamOutStream = false;
					m_bAudioStreamPaused = false;
				}
			}
			else
			{
				if(!m_bAudioStreamOutStream)
				{
					Stop(true);
					m_bAudioStreamOutStream = true;
					m_bAudioStreamPaused = false;
				}
			}
		}
	}
	else
	{
		if(m_bAudioStreamPaused)
		{
			m_bAudioStreamPaused = false;
			pthread_create(&m_bAudioStreamThreadWorked, 0, audioStreamThread, 0);//Play(m_szAudioStreamUrl, m_fAudioStream.X, m_fAudioStream.Y, m_fAudioStream.Z, m_fAudioStreamRadius, m_bAudioStreamUsePos);
		}
	}
	//Paused + Use Pos
}

CAudioStream::CAudioStream() 
{
	m_bAudioStreamPaused = false;
	m_bAudioStreamOutStream = false;
	m_bAudioStreamUsePos = false;
	Log("Audiostream initialized.");
}

CAudioStream::~CAudioStream() { }

void CAudioStream::Initialize()
{
	bassStream = 0;
	
	BASS_SetConfigPtr(16, "SA-MP/0.3");
	
}

void CAudioStream::Play(char const *szURL, float X, float Y, float Z, float fRadius, bool bUsePos)
{
	if(m_bAudioStreamThreadWorked)
	{
		m_bAudioStreamStop = 1;
		do
			usleep(2000);
		while(m_bAudioStreamThreadWorked);
		BASS_StreamFree(bassStream);
		bassStream = 0;
	}
	
	memset(m_szAudioStreamUrl, 0, sizeof(m_szAudioStreamUrl));
	strncpy(m_szAudioStreamUrl, szURL, sizeof(m_szAudioStreamUrl));
	m_fAudioStream.X = X;
	m_fAudioStream.Y = Y;
	m_fAudioStream.Z = Z;
	m_fAudioStreamRadius = fRadius;
	m_bAudioStreamUsePos = bUsePos;
	m_bAudioStreamStop = 0;
	pthread_create(&m_bAudioStreamThreadWorked, 0, audioStreamThread, 0);
}

void CAudioStream::Stop(bool bStop, bool bPaused)
{
	if(m_bAudioStreamThreadWorked)
	{
		if(bPaused) m_bAudioStreamPaused = true;
		m_bAudioStreamStop = 1;
		if(bStop)
		{
			do
				usleep(2000);
			while(m_bAudioStreamThreadWorked);
		}
		BASS_StreamFree(bassStream);
		bassStream = 0;
	}
}
