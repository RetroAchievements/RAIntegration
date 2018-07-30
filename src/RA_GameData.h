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

    const std::string& RichPresencePatch() { return m_sRichPresencePatch; }
    void SetRichPresencePatch(const std::string& str) { m_sRichPresencePatch = str; }

    void ParseData(const Document& doc);

private:
    ra::GameID m_nGameID{};
    std::string m_sGameTitle;
    std::string m_sRichPresencePatch;

    //unsigned int m_nConsoleID;
    //std::string m_sConsoleName;
};



extern std::unique_ptr<GameData> g_pCurrentGameData;


#endif // !RA_GAMEDATA_H
