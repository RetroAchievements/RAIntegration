#ifndef RA_GAMEDATA_H
#define RA_GAMEDATA_H
#pragma once

#define RAPIDJSON_NOMEMBERITERATORCLASS
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>  

#ifndef _MEMORY_
#include <memory>  
#endif // !_MEMORY_

#include "ra_fwd.h"

class GameData
{
public:
    inline ra::GameID GetGameID() { return m_nGameID; }
    void SetGameID(ra::GameID nGameID) { m_nGameID = nGameID; }

    const std::string& GameTitle() { return m_sGameTitle; }
    void SetGameTitle(const std::string& str) { m_sGameTitle = str; }

    const std::string& RichPresencePatch() { return m_sRichPresencePatch; }
    void SetRichPresencePatch(const std::string& str) { m_sRichPresencePatch = str; }

    void ParseData(const rapidjson::Document& doc);

private:
    ra::GameID m_nGameID{};
    std::string m_sGameTitle;
    std::string m_sRichPresencePatch;
};

extern std::unique_ptr<GameData> g_pCurrentGameData;

#endif // !RA_GAMEDATA_H
