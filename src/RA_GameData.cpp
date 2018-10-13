#include "RA_GameData.h"

#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

GameData* g_pCurrentGameData = new GameData();

void GameData::ParseData(const rapidjson::Document& doc)
{
    m_nGameID = doc["ID"].GetUint();
    m_sGameTitle = doc["Title"].GetString();

    if (doc.HasMember("RichPresencePatch"))
    {
        std::string sRichPresence;
        if (!doc["RichPresencePatch"].IsNull())
            sRichPresence = doc["RichPresencePatch"].GetString();

        auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
        auto pRich = pLocalStorage.WriteText(ra::services::StorageItemType::RichPresence, std::to_wstring(m_nGameID));

        pRich->Write(sRichPresence);
    }

    //m_nConsoleID = doc["ConsoleID"].GetUint();
    //m_sConsoleName = doc["ConsoleName"].GetString();
    //const unsigned int nForumTopicID = doc["ForumTopicID"].GetUint();
    //const unsigned int nGameFlags = doc["Flags"].GetUint();
    //const std::string& sImageIcon = doc["ImageIcon"].GetString();
    //const std::string& sImageTitle = doc["ImageTitle"].GetString();
    //const std::string& sImageIngame = doc["ImageIngame"].GetString();
    //const std::string& sImageBoxArt = doc["ImageBoxArt"].GetString();
    //const std::string& sPublisher = doc["Publisher"].IsNull() ? "Unknown" : doc["Publisher"].GetString();
    //const std::string& sDeveloper = doc["Developer"].IsNull() ? "Unknown" : doc["Developer"].GetString();
    //const std::string& sGenre = doc["Genre"].IsNull() ? "Unknown" : doc["Genre"].GetString();
    //const std::string& sReleased = doc["Released"].IsNull() ? "Unknown" : doc["Released"].GetString();
    //const bool bIsFinal = doc["IsFinal"].GetBool();
}
