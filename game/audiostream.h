#pragma once

#define UNKNOWN_SIZE 256

class CAudioStream
{
public:
	CAudioStream();
	~CAudioStream();
	
	void Initialize();
	void Play(char const *szURL, float X, float Y, float Z, float fRadius, bool bUsePos);
	void Stop(bool bStop, bool bPaused = false);
	void Process();
};
