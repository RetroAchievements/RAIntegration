#pragma once

#include "RA_Defs.h"

class GameData
{
public:
    _NODISCARD _CONSTANT_FN GetGameID() const noexcept { return m_nGameID; }
    _CONSTANT_FN SetGameID(_In_ GameID nGameID) noexcept { m_nGameID = nGameID; }

    _NODISCARD auto& GameTitle() const noexcept { return m_sGameTitle; }
    _NORETURN auto SetGameTitle(_In_ const std::string& str) { m_sGameTitle = str; }

    _NODISCARD auto& RichPresencePatch() const noexcept { return m_sRichPresencePatch; }
    _NORETURN auto SetRichPresencePatch(_In_ const std::string& str) noexcept { m_sRichPresencePatch = str; }

    void ParseData(_In_ const Document& doc);

private:
    GameID m_nGameID = GameID{};
    std::string m_sGameTitle;
    std::string m_sRichPresencePatch;

    //unsigned int m_nConsoleID;
    //std::string m_sConsoleName;
};


extern std::unique_ptr<GameData> g_pCurrentGameData;
