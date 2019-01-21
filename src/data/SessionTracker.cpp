#include "SessionTracker.hh"

#include "Exports.hh"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_MemBookmark.h"
#include "RA_Dlg_Memory.h"
#include "RA_Log.h"
#include "RA_md5factory.h"
#include "RA_StringUtils.h"

#include "api\Ping.hh"
#include "api\StartSession.hh"

#include "data\GameContext.hh"

#include "services\IClock.hh"
#include "services\IFileSystem.hh"
#include "services\ILocalStorage.hh"
#include "services\IThreadPool.hh"

namespace ra {
namespace data {

const int SERVER_PING_FREQUENCY = 2 * 60; // seconds between server pings

void SessionTracker::Initialize(const std::string& sUsername)
{
    m_sUsername = ra::Widen(sUsername);

    LoadSessions();
    SortSessions();
}

void SessionTracker::LoadSessions()
{
    m_vGameStats.clear();

    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pStatsFile = pLocalStorage.ReadText(ra::services::StorageItemType::SessionStats, m_sUsername);

    if (pStatsFile != nullptr)
    {
        // line format: <gameid>:<sessionstart>:<sessionlength>:<checksum>
        std::string sLine;
        while (pStatsFile->GetLine(sLine))
        {
            auto nIndex = sLine.find(':');
            if (nIndex == std::string::npos)
                continue;

            const auto nGameId = std::stoul(sLine);

            auto nIndex2 = sLine.find(':', ++nIndex);
            if (nIndex2 == std::string::npos)
                continue;

            const auto nSessionStart = std::stoul(&sLine.at(nIndex));

            nIndex = sLine.find(':', ++nIndex2);
            if (nIndex == std::string::npos)
                continue;
            const auto nSessionLength = std::stoul(&sLine.at(nIndex2));

            std::string md5;
            GSL_SUPPRESS_TYPE1 md5 = RAGenerateMD5(reinterpret_cast<const unsigned char*>(sLine.c_str()), nIndex + 1);
            if (sLine.at(nIndex + 1) == md5.front() && sLine.at(nIndex + 2) == md5.back())
                AddSession(nGameId, nSessionStart, std::chrono::seconds(nSessionLength));
        }

        m_nFileWritePosition = pStatsFile->GetPosition();
    }
}

void SessionTracker::AddSession(unsigned int nGameId, time_t tSessionStart, std::chrono::seconds tSessionDuration)
{
    auto pIter = m_vGameStats.begin();
    while (pIter != m_vGameStats.end() && pIter->GameId != nGameId)
        ++pIter;

    if (pIter == m_vGameStats.end())
    {
        m_vGameStats.emplace_back();
        pIter = m_vGameStats.end() - 1;
        pIter->GameId = nGameId;
    }

    pIter->LastSessionStart = std::chrono::system_clock::from_time_t(tSessionStart);
    pIter->TotalPlaytime += tSessionDuration;
}

void SessionTracker::SortSessions()
{
    // move most recently played items to the front of the list
    std::sort(m_vGameStats.begin(), m_vGameStats.end(), [](const GameStats& left, const GameStats& right)
    {
        return (right.LastSessionStart < left.LastSessionStart);
    });
}

void SessionTracker::BeginSession(unsigned int nGameId)
{
    if (m_nCurrentGameId != 0)
        EndSession();

    RA_LOG_INFO("Starting new session for game %u", nGameId);

    m_nCurrentGameId = nGameId;

    const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
    m_tpSessionStart = pClock.UpTime();
    m_tSessionStart = std::chrono::system_clock::to_time_t(pClock.Now());

    auto& pThreadPool = ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>();
    pThreadPool.ScheduleAsync(std::chrono::seconds(30), [this, tSessionStart = m_tSessionStart]() { UpdateSession(tSessionStart); });

    if (nGameId != 0)
    {
        ra::api::StartSession::Request request;
        request.GameId = nGameId;
        request.CallAsync([](const ra::api::StartSession::Response&) {});

        // make sure a persisted play entry exists for the game and is first in the list
        AddSession(nGameId, m_tSessionStart, std::chrono::seconds(0));
        if (m_vGameStats.begin()->GameId != nGameId)
            SortSessions();
    }
}

void SessionTracker::EndSession()
{
    if (m_nCurrentGameId != 0)
    {
        // update session duration
        const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
        const auto tSessionDuration = std::chrono::duration_cast<std::chrono::seconds>(pClock.UpTime() - m_tpSessionStart);
        RA_LOG_INFO("Ending session for game %u (%zu seconds)", m_nCurrentGameId, static_cast<unsigned int>(tSessionDuration.count()));

        // write session
        m_nFileWritePosition = WriteSessionStats(tSessionDuration);

        // update the persisted play duration
        AddSession(m_nCurrentGameId, m_tSessionStart, tSessionDuration);
    }

    m_nCurrentGameId = 0;
    m_tSessionStart = 0;
}

void SessionTracker::UpdateSession(time_t tSessionStart)
{
    // make sure we're still tracking the same game
    if (tSessionStart != m_tSessionStart)
        return;

    // ping the server (this also updates the player's current activity string)
    ra::api::Ping::Request request;
    request.GameId = m_nCurrentGameId;
    request.CurrentActivity = GetCurrentActivity();
    request.CallAsync([](const ra::api::Ping::Response&) {});

    // update session duration
    const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
    const auto tSessionDuration = std::chrono::duration_cast<std::chrono::seconds>(pClock.UpTime() - m_tpSessionStart);

    // ignore sessions less than one minute
    if (m_nCurrentGameId != 0 && tSessionDuration > std::chrono::seconds(60))
        WriteSessionStats(tSessionDuration);

    // schedule next ping
    auto& pThreadPool = ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>();
    pThreadPool.ScheduleAsync(std::chrono::seconds(SERVER_PING_FREQUENCY), [this, tSessionStart]() { UpdateSession(tSessionStart); });
}

std::streampos SessionTracker::WriteSessionStats(std::chrono::seconds tSessionDuration) const
{
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pStatsFile = pLocalStorage.AppendText(ra::services::StorageItemType::SessionStats, m_sUsername);

    auto nSessionDuration = tSessionDuration.count();
    auto sLine = ra::StringPrintf("%u:%ll:%ll:", m_nCurrentGameId, m_tSessionStart, nSessionDuration);
    const auto sMD5 = RAGenerateMD5(sLine);
    sLine.push_back(sMD5.front());
    sLine.push_back(sMD5.back());

    pStatsFile->SetPosition(m_nFileWritePosition);
    pStatsFile->WriteLine(sLine);

    return pStatsFile->GetPosition();
}

std::chrono::seconds SessionTracker::GetTotalPlaytime(unsigned int nGameId) const
{
    std::chrono::seconds tPlaytime(0);

    // get the current session duration
    if (m_nCurrentGameId == nGameId)
    {
        const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
        auto tSessionDuration = std::chrono::duration_cast<std::chrono::seconds>(pClock.UpTime() - m_tpSessionStart);

        tPlaytime += tSessionDuration;
    }

    // add any prior session durations
    for (const auto& pGameStats : m_vGameStats)
    {
        if (pGameStats.GameId == nGameId)
        {
            tPlaytime += pGameStats.TotalPlaytime;
            break;
        }
    }

    return tPlaytime;
}

bool SessionTracker::IsInspectingMemory() const noexcept
{
#ifdef RA_UTEST
    return false;
#else
    return (g_MemoryDialog.IsActive() || g_AchievementEditorDialog.IsActive() || g_MemBookmarkDialog.IsActive());
#endif
}

static bool HasCoreAchievements(const ra::data::GameContext& pGameContext)
{
    bool bResult = false;
    pGameContext.EnumerateAchievements([&bResult](const Achievement& pAchievement) noexcept
    {
        if (pAchievement.Category() == ra::etoi(AchievementSet::Type::Core))
        {
            bResult = true;
            return false;
        }

        return true;
    });

    return bResult;
}

std::wstring SessionTracker::GetCurrentActivity() const
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    if (IsInspectingMemory())
    {
        if (!HasCoreAchievements(pGameContext))
            return L"Developing Achievements";

        if (_RA_HardcoreModeIsActive())
            return L"Inspecting Memory in Hardcore mode";

        if (pGameContext.ActiveAchievementType() == AchievementSet::Type::Core)
            return L"Fixing Achievements";

        return L"Developing Achievements";
    }

    if (pGameContext.HasRichPresence())
    {
        const auto sRPResponse = pGameContext.GetRichPresenceDisplayString();
        if (!sRPResponse.empty())
            return sRPResponse;
    }

    if (HasCoreAchievements(pGameContext))
        return L"Earning Achievements";

    return L"Playing " + pGameContext.GameTitle();
}

} // namespace data
} // namespace ra
