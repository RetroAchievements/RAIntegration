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

private:
    typedef struct PointerValue
    {
        PointerValue(ra::data::ByteAddress nAddress, uint32_t nValue)
            : nAddress(nAddress), nValue(nValue)
        {
        }

        ra::data::ByteAddress nAddress;
        uint32_t nValue;
    } PointerValue;
    typedef std::pair<const PointerValue*, const PointerValue*> PointerAddressRange;

public:
    class Capture
    {
    public:
        void Initialize(const SearchResults& pSearchResults);

        uint32_t GetValue(ra::data::ByteAddress nAddress) const;

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

    void Analyze(std::vector<PotentialPointer>& vResults);

private:
    typedef struct Node
    {
        ra::data::ByteAddress nAddress = 0;        // the address of the pointer
        int32_t nOffset = 0;                       // the offset to add to the pointer to reach the value
        uint32_t nScore = 0;                       // the composite distance to reach the final value
        ra::data::ByteAddress nNextAddress = 0;    // the address of the next pointer node
    } Node;

    void GetRootNodes(std::vector<ra::data::ByteAddress>& vRootNodes) const;
    void GetDirectPointers(std::vector<PotentialPointer>& vDirectNodes, const std::vector<ra::data::ByteAddress>& vRootNodes) const;

    std::vector<std::pair<const Capture&, ra::data::ByteAddress>> m_vCaptures;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_FRAME_EVENT_QUEUE_HH
