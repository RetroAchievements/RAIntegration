#ifndef RA_USER_H
#define RA_USER_H
#pragma once

#include "ra_fwd.h"

#include "data\UserContext.hh"

#include "services\ServiceLocator.hh"

//////////////////////////////////////////////////////////////////////////
//	RAUser

typedef struct
{
    char m_Sender[256];
    char m_sDateSent[256];
    char m_sTitle[256];
    char m_sMessage[256];
    BOOL m_bRead;
} RAMessage;

class RAUser
{
public:
    RAUser(const std::string& sUsername);

public:
    unsigned int GetScore() const { return m_nScore; }
    void SetScore(unsigned int nScore) { m_nScore = nScore; }

    void SetUsername(const std::string& sUser) { m_sUsername = sUser; }
    const std::string& Username() const { return m_sUsername; }

    void UpdateActivity(const std::string& sAct) { m_sActivity = sAct; }
    const std::string& Activity() const { return m_sActivity; }

private:
    /*const*/ std::string m_sUsername;
    std::string m_sActivity;
    unsigned int m_nScore;
};

class LocalRAUser
{
public:
    const std::string& Username() const
    {
        return ra::services::ServiceLocator::Get<ra::data::UserContext>().GetUsername();
    }

    void RequestFriendList();
    void OnFriendListResponse(const rapidjson::Document& doc);

    RAUser* AddFriend(const std::string& sFriend, unsigned int nScore);
    RAUser* FindFriend(const std::string& sName);

    RAUser* GetFriendByIter(size_t nOffs) { return nOffs < m_aFriends.size() ? m_aFriends[nOffs] : nullptr; }
    const size_t NumFriends() const { return m_aFriends.size(); }

    const std::string& Token() const
    {
        return ra::services::ServiceLocator::Get<ra::data::UserContext>().GetApiToken();
    }

private:
    std::vector<RAUser*> m_aFriends;
    std::vector<RAMessage> m_aMessages;
};

class RAUsers
{
public:
    static LocalRAUser& LocalUser() { return ms_LocalUser; }
    static BOOL DatabaseContainsUser(const std::string& sUser);

    static void RegisterUser(const std::string& sUsername, RAUser* pUser) { UserDatabase[sUsername] = pUser; }

    static RAUser* GetUser(const std::string& sUser);

private:
    static LocalRAUser ms_LocalUser;
    static std::map<std::string, RAUser*> UserDatabase;
};

#endif // !RA_USER_H
