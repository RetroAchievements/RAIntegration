#ifndef RA_DATA_SESSIONTRACKER_HH
#define RA_DATA_SESSIONTRACKER_HH
#pragma once

#include <string>

namespace ra {
namespace data {
namespace context {

class SessionTracker
{
public:
    SessionTracker() noexcept = default;
    virtual ~SessionTracker() noexcept = default;
    SessionTracker(const SessionTracker&) noexcept = delete;
    SessionTracker& operator=(const SessionTracker&) noexcept = delete;
    SessionTracker(SessionTracker&&) noexcept = delete;
    SessionTracker& operator=(SessionTracker&&) noexcept = delete;
    
    /// <summary>
    /// Initializes the session data for the specified user.
    /// </summary>
    void Initialize(const std::string& sUsername);

    /// <summary>
    /// Begins a new session for the specified game.
    /// </summary>
    void BeginSession(unsigned int nGameId);
    
    /// <summary>
    /// Ends the current session.
    /// </summary>
    void EndSession();
    
    /// <summary>
    /// Gets the total captured playtime for the specified game.
    /// </summary>
    std::chrono::seconds GetTotalPlaytime(unsigned int nGameId) const;
    
    /// <summary>
    /// Determines whether any previous session data exists.
    /// </summary>
    bool HasSessionData() const noexcept { return !m_vGameStats.empty(); }

    struct GameStats
    {
        unsigned int GameId{};
        std::chrono::seconds TotalPlayTime{};
        std::chrono::system_clock::time_point LastSessionStart;
    };

    /// <summary>
    /// Gets the previous session data.
    /// </summary>
    const std::vector<GameStats>& SessionData() const noexcept { return m_vGameStats; }

protected:
    virtual void LoadSessions();
    void AddSession(unsigned int nGameId, time_t tSessionStart, std::chrono::seconds tSessionDuration);

    void UpdateSession(time_t tSessionStart);
    std::streampos WriteSessionStats(std::chrono::seconds tSessionDuration) const;

    std::wstring m_sUsername;
    unsigned int m_nCurrentGameId = 0;

private:
    void SortSessions();

    std::chrono::steady_clock::time_point m_tpSessionStart{};
    time_t m_tSessionStart{};

    std::vector<GameStats> m_vGameStats;

    std::streamoff m_nFileWritePosition{};
};

} // namespace context
} // namespace data
} // namespace ra

#endif // !RA_DATA_SESSIONTRACKER_HH
