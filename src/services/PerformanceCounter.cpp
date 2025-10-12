#include "PerformanceCounter.hh"

#ifdef PERFORMANCE_COUNTERS

#include "RA_Log.h"

#include "IClock.hh"

namespace ra {
namespace services {

static std::string GetLabel(PerformanceCheckpoint nCheckpoint)
{
    std::string sCheckpoint;

    switch (nCheckpoint)
    {
        case PerformanceCheckpoint::RuntimeProcess: sCheckpoint = "Runtime"; break;
        case PerformanceCheckpoint::RuntimeEvents: sCheckpoint = "Events"; break;
        case PerformanceCheckpoint::OverlayManagerAdvanceFrame: sCheckpoint = "Overlay"; break;
        case PerformanceCheckpoint::MemoryBookmarksDoFrame: sCheckpoint = "Bookmarks"; break;
        case PerformanceCheckpoint::MemoryInspectorDoFrame: sCheckpoint = "Inspector"; break;
        case PerformanceCheckpoint::AssetListDoFrame: sCheckpoint = "AssetList"; break;
        case PerformanceCheckpoint::AssetEditorDoFrame: sCheckpoint = "AssetEditor"; break;
        case PerformanceCheckpoint::PointerFinderDoFrame: sCheckpoint = "PointerFinder"; break;
        case PerformanceCheckpoint::PointerInspectorDoFrame: sCheckpoint = "PointerInspector"; break;
        case PerformanceCheckpoint::FrameEvents: sCheckpoint = "FrameEvents"; break;
        default: sCheckpoint = std::to_string(ra::etoi(nCheckpoint)); break;
    }

    return sCheckpoint;
}

void PerformanceCounter::Tally(PerformanceCheckpoint nCheckpoint)
{
    const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
    m_vMeasurements.emplace_back(nCheckpoint, pClock.UpTime());
}

void PerformanceCounter::Stop()
{
    Tally(PerformanceCheckpoint::NUM_CHECKPOINTS);

    int nLapTime = 0;
    Lap& pLap = m_vLaps.at(m_nNextLap);
    for (gsl::index nIndex = 1; nIndex < gsl::narrow_cast<gsl::index>(m_vMeasurements.size()); ++nIndex)
    {
        const auto& pMeasurement = m_vMeasurements.at(nIndex - 1);
        const auto nElapsed = m_vMeasurements.at(nIndex).tWhen - pMeasurement.tWhen;
        const auto nElapsedMicroseconds = gsl::narrow_cast<int>(std::chrono::duration_cast<std::chrono::microseconds>(nElapsed).count());

        const auto nCheckpointIndex = ra::etoi(pMeasurement.nCheckpoint);
        const auto nPreviousMicroseconds = pLap.at(nCheckpointIndex);
        m_nRollingTotals.at(nCheckpointIndex) += nElapsedMicroseconds - nPreviousMicroseconds;
        pLap.at(nCheckpointIndex) = nElapsedMicroseconds;

        m_nLapTotal += nElapsedMicroseconds - nPreviousMicroseconds;
        nLapTime += nElapsedMicroseconds;
    }

    m_vMeasurements.clear();

    if (++m_nNextLap == NUM_LAPS)
        m_nNextLap = 0;

    if (++m_nTotalLaps > NUM_LAPS)
    {
        const auto nLapAverage = m_nLapTotal / NUM_LAPS;

        if (m_nTotalLaps % NUM_LAPS == 0) 
        {
            RA_LOG_INFO("Lap averages: total: %d.%03dms", nLapAverage / 1000, nLapAverage % 1000);
            for (gsl::index i = 0; i < ra::etoi(PerformanceCheckpoint::NUM_CHECKPOINTS); ++i)
            {
                const auto nCheckpointTime = pLap.at(i);
                if (nCheckpointTime != 0)
                {
                    const auto nCheckpointAverage = m_nRollingTotals.at(i) / NUM_LAPS;
                    RA_LOG_INFO(" %s: %d.%03dms", GetLabel(ra::itoe<PerformanceCheckpoint>(i)), nCheckpointAverage / 1000, nCheckpointAverage % 1000);
                }
            }
        }
        else if (nLapTime > 100) // ignore everything under 100us
        {
            if (nLapTime > 2000) // to achieve 60 fps, emulator has to render every 16ms, we don't want to use more than 2ms of that.
            {
                RA_LOG_INFO("Outstanding lap: %d.%03dms (avg: %d.%03dms)", nLapTime / 1000, nLapTime % 1000, nLapAverage / 1000, nLapAverage % 1000);

                for (gsl::index i = 0; i < ra::etoi(PerformanceCheckpoint::NUM_CHECKPOINTS); ++i)
                {
                    const auto nCheckpointTime = pLap.at(i);
                    if (nCheckpointTime != 0)
                        DumpCheckpoint(ra::itoe<PerformanceCheckpoint>(i), nCheckpointTime);
                }
            }
            else
            {
                for (gsl::index i = 1; i < ra::etoi(PerformanceCheckpoint::NUM_CHECKPOINTS); ++i)
                {
                    const auto nCheckpointTime = pLap.at(i);
                    if (nCheckpointTime > 75 && nCheckpointTime > m_nRollingTotals.at(i) * 2)
                        DumpCheckpoint(ra::itoe<PerformanceCheckpoint>(i), nCheckpointTime);
                }
            }
        }
    }
}

void PerformanceCounter::DumpCheckpoint(PerformanceCheckpoint nCheckpoint, int nCheckpointTime)
{
    const auto nCheckpointAverage = m_nRollingTotals.at(ra::etoi(nCheckpoint)) / NUM_LAPS;
    RA_LOG_INFO(" %s: %d.%03dms (avg: %d.%03dms)", GetLabel(nCheckpoint), nCheckpointTime / 1000, nCheckpointTime % 1000, nCheckpointAverage / 1000, nCheckpointAverage % 1000);
}

} // namespace services
} // namespace ra

#endif // PERFORMANCE_COUNTERS
