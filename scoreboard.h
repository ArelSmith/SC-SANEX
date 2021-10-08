#pragma once

#include <iostream>

typedef struct _PLAYER_SCORE_INFO
{
    uint32_t dwID;
    char *szName;
    int iScore;
    int32_t dwPing;
    uint32_t dwColor;
    int iState;
} PLAYER_SCORE_INFO;

class CScoreBoard
{
public:
    int m_iOffset;
    bool m_bSorted;
    bool m_bToggle;
    uint32_t m_tickProcessingUpdate;

    CScoreBoard();
    ~CScoreBoard();

    void ProcessUpdating();
    void Render();
    void Toggle();

private:
	float               m_fScoreBoardSizeX;
	float               m_fScoreBoardSizeY;

    PLAYER_SCORE_INFO   *m_pPlayers;
    uint16_t            m_pPlayerCount;

    int                 m_iLastSelectedItem;
    int                 m_iSelectedItem;
};
