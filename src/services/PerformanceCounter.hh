#ifndef RA_SERVICES_PERFORMANCECOUNTER_H
#define RA_SERVICES_PERFORMANCECOUNTER_H
#pragma once

#ifndef RA_UTEST
//#define PERFORMANCE_COUNTERS
#endif

enum class PerformanceCheckpoint
{
    RuntimeProcess = 0,
    RuntimeEvents,
    OverlayManagerAdvanceFrame,
    MemoryBookmarksDoFrame,
    MemoryDialogInvalidate,

    NUM_CHECKPOINTS
};

#ifdef PERFORMANCE_COUNTERS

#include "services/ServiceLocator.hh"

namespace ra {
namespace services {

class PerformanceCounter
{
public:
    void Tally(PerformanceCheckpoint nCheckpoint);

    void Stop();

private:
    void DumpCheckpoint(PerformanceCheckpoint nCheckpoint, int nCheckpointTime);

    struct Measurement
    {
        Measurement(PerformanceCheckpoint nCheckpoint, std::chrono::steady_clock::time_point tWhen)
        {
            this->nCheckpoint = nCheckpoint;
            this->tWhen = tWhen;
        }

        PerformanceCheckpoint nCheckpoint;
        std::chrono::steady_clock::time_point tWhen;
    };

    std::vector<Measurement> m_vMeasurements;

    size_t m_nTotalLaps = 0;
    
    using Lap = std::array<int, static_cast<size_t>(PerformanceCheckpoint::NUM_CHECKPOINTS)>;
    Lap m_nRollingTotals;
    int m_nLapTotal;

    static constexpr size_t NUM_LAPS = 3600; // once per minute, log overall averages
    std::array<Lap, NUM_LAPS> m_vLaps;
    gsl::index m_nNextLap = 0;
};

} // namespace services
} // namespace ra

#define TALLY_PERFORMANCE(checkpoint) ra::services::ServiceLocator::GetMutable<ra::services::PerformanceCounter>().Tally(checkpoint)
#define CHECK_PERFORMANCE() ra::services::ServiceLocator::GetMutable<ra::services::PerformanceCounter>().Stop()

#else // PERFORMANCE_COUNTERS

#define TALLY_PERFORMANCE(checkpoint) 
#define CHECK_PERFORMANCE()

#endif // PERFORMANCE_COUNTERS

#endif !RA_SERVICES_PERFORMANCECOUNTER_H
