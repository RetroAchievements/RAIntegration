#ifndef RA_GAMEDATA_H
#define RA_GAMEDATA_H
#pragma once

#include "RA_Defs.h"

#include "data/LuaScript.h"

class GameData
{
public:
    inline GameID GetGameID() { return m_nGameID; }
    void SetGameID(GameID nGameID) { m_nGameID = nGameID; }

    const std::string& GameTitle() { return m_sGameTitle; }
    void SetGameTitle(const std::string& str) { m_sGameTitle = str; }

    const std::string& RichPresencePatch() { return m_sRichPresencePatch; }
    void SetRichPresencePatch(const std::string& str) { m_sRichPresencePatch = str; }

#ifndef RA_UTEST
    void ParseData(const Document& doc);
#endif

    const ra::data::LuaScript& LuaScript() const { return m_LuaScript; }

private:
    GameID m_nGameID;
    std::string m_sGameTitle;
    std::string m_sRichPresencePatch;

    ra::data::LuaScript m_LuaScript;

    //unsigned int m_nConsoleID;
    //std::string m_sConsoleName;
};

extern GameData* g_pCurrentGameData;

#endif // !RA_GAMEDATA_H
