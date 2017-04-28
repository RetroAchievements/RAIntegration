#include "RA_GameData.h"

GameData* g_pCurrentGameData = new GameData();

void GameData::ParseData(const Document& doc)
{
	m_nGameID = doc["ID"].GetUint();
	m_sGameTitle = doc["Title"].GetString();
	m_sRichPresencePatch = doc["RichPresencePatch"].IsNull() ? "" : doc["RichPresencePatch"].GetString();

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