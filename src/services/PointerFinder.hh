#ifndef RA_SERVICES_POINTERFINDER_HH
#define RA_SERVICES_POINTERFINDER_HH
#pragma once

#include "data\Types.hh"

#include "util\GSL.hh"

#include "SearchResults.h"

namespace ra {
namespace services {

class PointerFinder
{
public:
    GSL_SUPPRESS_F6 PointerFinder() = default;
    virtual ~PointerFinder() = default;

    PointerFinder(const PointerFinder&) noexcept = delete;
    PointerFinder& operator=(const PointerFinder&) noexcept = delete;
    PointerFinder(PointerFinder&&) noexcept = delete;
    PointerFinder& operator=(PointerFinder&&) noexcept = delete;

    typedef struct PointerValue
    {
        PointerValue(ra::data::ByteAddress nAddress, uint32_t nValue, ra::data::ByteAddress nValueAsAddress) noexcept
            : nAddress(nAddress), nValue(nValue), nValueAsAddress(nValueAsAddress)
        {
        }
        
        ra::data::ByteAddress nAddress;
        uint32_t nValue;
        ra::data::ByteAddress nValueAsAddress;
    } PointerValue;

private:
    typedef std::pair<const PointerValue*, const PointerValue*> PointerAddressRange;

public:
    class Capture
    {
    public:
        void Initialize(const SearchResults& pSearchResults);
        size_t GetCapturedPointerCount() const noexcept { return m_vPointerValues.size(); }

        const PointerValue* GetValue(ra::data::ByteAddress nAddress) const;
        ra::data::ByteAddress GetValueAsAddress(ra::data::ByteAddress nAddress) const;

    private:
        friend class PointerFinder;
        PointerAddressRange NarrowSearch(ra::data::ByteAddress nTargetAddress) const;

        std::vector<PointerValue> m_vPointerValues;
    };

    void AddCapture(const Capture& pCapture, ra::data::ByteAddress nTargetAddress);

    static constexpr uint32_t MAX_DEPTH = 16;

    typedef struct PotentialPointer
    {
        ra::data::ByteAddress nRootAddress = 0;
        uint32_t nScore = 0;
        uint32_t nOffsetLength = 0;
        std::array<int32_t, MAX_DEPTH> vOffsets{};
    } PotentialPointer;

    void Analyze(std::vector<PotentialPointer>& vResults, std::function<bool(size_t, size_t)> fProgress);

private:
    typedef struct Node
    {
        ra::data::ByteAddress nAddress = 0;        // the address of the pointer
        int32_t nOffset = 0;                       // the offset to add to the pointer to reach the value
        uint32_t nScore = 0;                       // the composite distance to reach the final value
        ra::data::ByteAddress nNextAddress = 0;    // the address of the next pointer node
    } Node;

    typedef struct CaptureMetrics
    {
        std::vector<PotentialPointer> vPotentialPointers;
        std::vector<PotentialPointer>::const_iterator pIterator;
        std::vector<PotentialPointer>::const_iterator pStopIterator;
        const Capture* pCapture = nullptr;
    } CaptureMetrics;

    typedef struct AnalysisProgress
    {
        size_t nProgress = 0;
        size_t nRootProgressSize = 0;
        size_t nMaxProgress = 0;
        size_t nChecksSinceLastProgressUpdated = 0;
        bool bAborted = false;
        std::function<bool(size_t, size_t)> fProgress;

        std::vector<PotentialPointer>* vResults = nullptr;

        std::array<int32_t, MAX_DEPTH> vOffsets{};

        // addresses that cannot be reached by a common offset from any pointers across captures.
        std::vector<ra::data::ByteAddress> vUnreachableAddresses;

        // addresses that could not be found within MAX_DEPTH traversals.
        // index of outer array is the depth at which the address could not be resolved.
        std::array<std::vector<ra::data::ByteAddress>, MAX_DEPTH> vDeadEndAddresses;
    } AnalysisProgress;

    typedef struct AnalysisState
    {
        std::vector<CaptureMetrics> vCaptureMetrics;
        uint32_t nScore = 0;
        uint32_t nDepth = 0;

        AnalysisProgress* pProgress = nullptr;
    } AnalysisState;

    void GetPointers(std::vector<PotentialPointer>& vIndirectNodes, std::function<bool(size_t, size_t)> fProgress) const;
    static void AnalyzeState(AnalysisState& pAnalysisState);
    static void SortPointers(std::vector<PotentialPointer>& vPointers);
    static void InitializePotentialPointers(std::vector<PotentialPointer>& vPointers, const Capture& pCapture, ra::data::ByteAddress nTargetAddres);
    static void RemoveUnsharedOffsets(std::vector<CaptureMetrics>& vCaptureMetrics);
    static bool FindSharedOffset(std::vector<CaptureMetrics>& vCaptureMetrics) noexcept;
    static void ProcessSharedOffset(AnalysisState& pAnalysisState);
    static bool PointersMatch(std::vector<std::vector<PotentialPointer>::const_iterator>& vIterators) noexcept;

    std::vector<std::pair<const Capture&, ra::data::ByteAddress>> m_vCaptures;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_FRAME_EVENT_QUEUE_HH
