#ifndef RA_GAMEDATA_H
#define RA_GAMEDATA_H
#pragma once

#include "RA_Defs.h"

class GameData
{
public:
    inline ra::GameID GetGameID() { return m_nGameID; }
    void SetGameID(ra::GameID nGameID) { m_nGameID = nGameID; }

    const std::string& GameTitle() { return m_sGameTitle; }
    void SetGameTitle(const std::string& str) { m_sGameTitle = str; }

#ifndef RA_UTEST
    void ParseData(const rapidjson::Document& doc);
#endif

private:
    ra::GameID m_nGameID;
    std::string m_sGameTitle;
};

extern GameData* g_pCurrentGameData;

#endif // !RA_GAMEDATA_H
