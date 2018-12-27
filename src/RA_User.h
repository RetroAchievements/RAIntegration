#ifndef RA_USER_H
#define RA_USER_H
#pragma once

#include "ra_fwd.h"

#include "data\UserContext.hh"

#include "services\ServiceLocator.hh"

//////////////////////////////////////////////////////////////////////////
// RAUser

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
    explicit RAUser(const std::string& sUsername);
    ~RAUser() noexcept = default;
    RAUser(const RAUser&) = delete;
    RAUser& operator=(const RAUser&) = delete;
    RAUser(RAUser&&) noexcept = default;
    RAUser& operator=(RAUser&&) noexcept = default;

public:
    _NODISCARD inline auto GetScore() const noexcept { return m_nScore; }
    void SetScore(unsigned int nScore) noexcept { m_nScore = nScore; }

    void SetUsername(const std::string& sUser) { m_sUsername = sUser; }
    _NODISCARD inline auto& Username() const noexcept { return m_sUsername; }

    void UpdateActivity(const std::string& sAct) { m_sActivity = sAct; }
    _NODISCARD inline auto& Activity() const noexcept { return m_sActivity; }

private:
    std::string m_sUsername;
    std::string m_sActivity;
    unsigned int m_nScore{};

    // friends to give map capabillity to non-map standard containers
    friend inline auto operator==(const RAUser& a, const RAUser& b) noexcept;
    friend inline auto operator==(const std::string& key, const RAUser& value) noexcept;
    friend inline auto operator==(const RAUser& value, const std::string& key) noexcept;
    friend inline auto operator<(const RAUser& a, const RAUser& b) noexcept;
    friend inline auto operator<(const std::string& key, const RAUser& value) noexcept;
    friend inline auto operator<(const RAUser& value, const std::string& key) noexcept;
};

_NODISCARD inline auto operator==(const RAUser& a, const RAUser& b) noexcept
{
    return (a.m_sUsername == b.m_sUsername);
}
_NODISCARD inline auto operator==(const std::string& key, const RAUser& value) noexcept
{
    return (key == value.m_sUsername);
}
_NODISCARD inline auto operator==(const RAUser& value, const std::string& key) noexcept
{
    return (value.m_sUsername == key);
}
_NODISCARD inline auto operator<(const RAUser& a, const RAUser& b) noexcept { return (a.m_sUsername < b.m_sUsername); }
_NODISCARD inline auto operator<(const std::string& key, const RAUser& value) noexcept
{
    return (key < value.m_sUsername);
}
_NODISCARD inline auto operator<(const RAUser& value, const std::string& key) noexcept
{
    return (value.m_sUsername < key);
}

class LocalRAUser
{
public:
    _NODISCARD inline auto& Username() const
    {
        return ra::services::ServiceLocator::Get<ra::data::UserContext>().GetUsername();
    }

    void RequestFriendList();
    void OnFriendListResponse(const rapidjson::Document& doc);

    void AddFriend(const std::string& sFriend, unsigned int nScore);
    _NODISCARD const RAUser& FindFriend(const std::string& sName) const;
    inline auto& GetFriendByIter(size_t nOffs) const
    {
        Expects(nOffs >= 0 && nOffs < m_aFriends.size());
        return m_aFriends.at(nOffs);
    }
    _NODISCARD inline auto FriendExists(const std::string& sKey) const
    {
        return (std::binary_search(m_aFriends.cbegin(), m_aFriends.cend(), sKey));
    }

    _NODISCARD inline auto NumFriends() const noexcept { return m_aFriends.size(); }

    _NODISCARD inline auto& Token() const
    {
        return ra::services::ServiceLocator::Get<ra::data::UserContext>().GetApiToken();
    }

private:
    std::vector<RAUser> m_aFriends;
    std::vector<RAMessage> m_aMessages;
};

class RAUsers
{
public:
    _NODISCARD inline static auto& LocalUser() noexcept { return ms_LocalUser; }
    _NODISCARD static BOOL DatabaseContainsUser(const std::string& sUser);

    static RAUser& GetUser(const std::string& sUser);

private:
    inline static void Sort() { std::sort(UserDatabase.begin(), UserDatabase.end()); }
    inline static void RegisterUser(RAUser&& user)
    {
        UserDatabase.push_back(std::move(user));
        Sort(); // sort after adding
    }

    // always check for end iterator, const-ness dependant on calling function
    _NODISCARD inline static auto FindUser(const std::string& sUser)
    {
        return std::find(UserDatabase.begin(), UserDatabase.end(), sUser);
    }

    static LocalRAUser ms_LocalUser;
    static std::vector<RAUser> UserDatabase;

    // access register user
    friend class RAUser;
};

#endif // !RA_USER_H
