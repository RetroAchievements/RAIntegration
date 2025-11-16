#include "SessionTracker.hh"

#include "Exports.hh"
#include "RA_md5factory.h"
#include "util\Log.hh"
#include "util\Strings.hh"

#include "data\context\GameContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IClock.hh"
#include "services\IConfiguration.hh"
#include "services\IFileSystem.hh"
#include "services\ILocalStorage.hh"
#include "services\IThreadPool.hh"

#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace data {
namespace context {

constexpr int SERVER_PING_FREQUENCY = 2 * 60; // seconds between server pings

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
            ra::Tokenizer pTokenizer(sLine);

            const auto nGameId = pTokenizer.ReadNumber();
            if (!pTokenizer.Consume(':'))
                continue;

            const auto nSessionStart = pTokenizer.ReadNumber();
            if (!pTokenizer.Consume(':'))
                continue;

            const auto nSessionLength = pTokenizer.ReadNumber();
            if (!pTokenizer.Consume(':'))
                continue;


            const BYTE* pLine;
            GSL_SUPPRESS_TYPE1{ pLine = reinterpret_cast<const BYTE*>(sLine.c_str()); }
            const auto md5 = RAGenerateMD5(pLine, pTokenizer.CurrentPosition());
            if (pTokenizer.Consume(md5.front()) && pTokenizer.Consume(md5.back()))
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
    pIter->TotalPlayTime += tSessionDuration;
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

    // update session duration
    const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
    const auto tSessionDuration = std::chrono::duration_cast<std::chrono::seconds>(pClock.UpTime() - m_tpSessionStart);

    // ignore sessions less than one minute
    if (m_nCurrentGameId != 0 && tSessionDuration > std::chrono::seconds(60))
        WriteSessionStats(tSessionDuration);

    // schedule next ping
    auto& pThreadPool = ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>();
    pThreadPool.ScheduleAsync(std::chrono::seconds(SERVER_PING_FREQUENCY), [this, tSessionStart]() { UpdateSession(tSessionStart); });

    // check memory security
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
        pEmulatorContext.IsMemoryInsecure();
    }
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
            tPlaytime += pGameStats.TotalPlayTime;
            break;
        }
    }

    return tPlaytime;
}

} // namespace context
} // namespace data
} // namespace ra
