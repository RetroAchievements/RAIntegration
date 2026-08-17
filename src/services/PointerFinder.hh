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

        const PointerValue* GetValue(ra::data::ByteAddress nAddress) const;
        ra::data::ByteAddress GetValueAsAddress(ra::data::ByteAddress nAddress) const;

    private:
        friend class PointerFinder;
        PointerAddressRange NarrowSearch(ra::data::ByteAddress nTargetAddress) const;

        std::vector<PointerValue> m_vPointerValues;
    };

    void AddCapture(const Capture& pCapture, ra::data::ByteAddress nTargetAddress);

    typedef struct PotentialPointer
    {
        ra::data::ByteAddress nRootAddress = 0;
        uint32_t nScore = 0;
        std::vector<int32_t> vOffsets;
    } PotentialPointer;

    void Analyze(std::vector<PotentialPointer>& vResults, std::function<void(size_t, size_t)> fProgress);

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
        std::vector<PotentialPointer> vBestRoutes;
        std::vector<PotentialPointer>::const_iterator pIterator;
        std::vector<PotentialPointer>::const_iterator pStopIterator;
        const Capture* pCapture = nullptr;
    } CaptureMetrics;

    typedef struct AnalysisState
    {
        std::vector<CaptureMetrics> vCaptureMetrics;
        std::vector<int32_t> vOffsets;
        uint32_t nScore = 0;
        size_t nProgress = 0;
        size_t nRootProgressSize = 0;
        size_t nMaxProgress = 0;
        std::function<void(size_t, size_t)> fProgress;
        std::vector<PotentialPointer>* vResults = nullptr;
    } AnalysisState;

    void GetPointers(std::vector<PotentialPointer>& vIndirectNodes, std::function<void(size_t, size_t)> fProgress) const;
    static void GetPointers(std::vector<PotentialPointer>& vIndirectNodes, AnalysisState& pAnalysisState);
    static void SortPointers(std::vector<PotentialPointer>& vPointers);
    static void InitializeBestRoutes(std::vector<PotentialPointer>& vPointers, const Capture& pCapture, ra::data::ByteAddress nTargetAddres);
    static bool FindSharedOffset(std::vector<CaptureMetrics>& vCaptureMetrics) noexcept;
    static void ProcessSharedOffset(std::vector<PotentialPointer>& vIndirectNodes, AnalysisState& pAnalysisState);
    static bool PointersMatch(std::vector<std::vector<PotentialPointer>::const_iterator>& vIterators) noexcept;

    std::vector<std::pair<const Capture&, ra::data::ByteAddress>> m_vCaptures;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_FRAME_EVENT_QUEUE_HH
