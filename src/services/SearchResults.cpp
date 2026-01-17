#include "SearchResults.h"

#include "RA_Defs.h"
#include "util\Log.hh"

#include "context\IEmulatorMemoryContext.hh"

#include "services\ServiceLocator.hh"

#include "search\SearchImpl_4bit.hh"
#include "search\SearchImpl_8bit.hh"
#include "search\SearchImpl_16bit.hh"
#include "search\SearchImpl_16bit_aligned.hh"
#include "search\SearchImpl_16bit_be.hh"
#include "search\SearchImpl_16bit_be_aligned.hh"
#include "search\SearchImpl_24bit.hh"
#include "search\SearchImpl_32bit.hh"
#include "search\SearchImpl_32bit_aligned.hh"
#include "search\SearchImpl_32bit_be.hh"
#include "search\SearchImpl_32bit_be_aligned.hh"
#include "search\SearchImpl_asciitext.hh"
#include "search\SearchImpl_bitcount.hh"
#include "search\SearchImpl_double32.hh"
#include "search\SearchImpl_double32_aligned.hh"
#include "search\SearchImpl_double32_be.hh"
#include "search\SearchImpl_double32_be_aligned.hh"
#include "search\SearchImpl_float.hh"
#include "search\SearchImpl_float_aligned.hh"
#include "search\SearchImpl_float_be.hh"
#include "search\SearchImpl_float_be_aligned.hh"
#include "search\SearchImpl_mbf32.hh"
#include "search\SearchImpl_mbf32_le.hh"

namespace ra {
namespace services {

namespace search {
static search::FourBitSearchImpl s_pFourBitSearchImpl;
static search::EightBitSearchImpl s_pEightBitSearchImpl;
static search::SixteenBitSearchImpl s_pSixteenBitSearchImpl;
static search::TwentyFourBitSearchImpl s_pTwentyFourBitSearchImpl;
static search::ThirtyTwoBitSearchImpl s_pThirtyTwoBitSearchImpl;
static search::SixteenBitAlignedSearchImpl s_pSixteenBitAlignedSearchImpl;
static search::ThirtyTwoBitAlignedSearchImpl s_pThirtyTwoBitAlignedSearchImpl;
static search::SixteenBitBigEndianSearchImpl s_pSixteenBitBigEndianSearchImpl;
static search::ThirtyTwoBitBigEndianSearchImpl s_pThirtyTwoBitBigEndianSearchImpl;
static search::SixteenBitBigEndianAlignedSearchImpl s_pSixteenBitBigEndianAlignedSearchImpl;
static search::ThirtyTwoBitBigEndianAlignedSearchImpl s_pThirtyTwoBitBigEndianAlignedSearchImpl;
static search::BitCountSearchImpl s_pBitCountSearchImpl;
static search::AsciiTextSearchImpl s_pAsciiTextSearchImpl;
static search::FloatSearchImpl s_pFloatSearchImpl;
static search::FloatAlignedSearchImpl s_pFloatAlignedSearchImpl;
static search::FloatBESearchImpl s_pFloatBESearchImpl;
static search::FloatBEAlignedSearchImpl s_pFloatBEAlignedSearchImpl;
static search::Double32SearchImpl s_pDouble32SearchImpl;
static search::Double32AlignedSearchImpl s_pDouble32AlignedSearchImpl;
static search::Double32BESearchImpl s_pDouble32BESearchImpl;
static search::Double32BEAlignedSearchImpl s_pDouble32BEAlignedSearchImpl;
static search::MBF32SearchImpl s_pMBF32SearchImpl;
static search::MBF32LESearchImpl s_pMBF32LESearchImpl;
} // namespace search

static search::SearchImpl* GetSearchImpl(SearchType nType) noexcept
{
    switch (nType)
    {
        case SearchType::FourBit:
            return &ra::services::search::s_pFourBitSearchImpl;
        default:
        case SearchType::EightBit:
            return &ra::services::search::s_pEightBitSearchImpl;
        case SearchType::SixteenBit:
            return &ra::services::search::s_pSixteenBitSearchImpl;
        case SearchType::TwentyFourBit:
            return &ra::services::search::s_pTwentyFourBitSearchImpl;
        case SearchType::ThirtyTwoBit:
            return &ra::services::search::s_pThirtyTwoBitSearchImpl;
        case SearchType::SixteenBitAligned:
            return &ra::services::search::s_pSixteenBitAlignedSearchImpl;
        case SearchType::ThirtyTwoBitAligned:
            return &ra::services::search::s_pThirtyTwoBitAlignedSearchImpl;
        case SearchType::SixteenBitBigEndian:
            return &ra::services::search::s_pSixteenBitBigEndianSearchImpl;
        case SearchType::ThirtyTwoBitBigEndian:
            return &ra::services::search::s_pThirtyTwoBitBigEndianSearchImpl;
        case SearchType::SixteenBitBigEndianAligned:
            return &ra::services::search::s_pSixteenBitBigEndianAlignedSearchImpl;
        case SearchType::ThirtyTwoBitBigEndianAligned:
            return &ra::services::search::s_pThirtyTwoBitBigEndianAlignedSearchImpl;
        case SearchType::BitCount:
            return &ra::services::search::s_pBitCountSearchImpl;
        case SearchType::AsciiText:
            return &ra::services::search::s_pAsciiTextSearchImpl;
        case SearchType::Float:
            return &ra::services::search::s_pFloatSearchImpl;
        case SearchType::FloatAligned:
            return &ra::services::search::s_pFloatAlignedSearchImpl;
        case SearchType::FloatBigEndian:
            return &ra::services::search::s_pFloatBESearchImpl;
        case SearchType::FloatBigEndianAligned:
            return &ra::services::search::s_pFloatBEAlignedSearchImpl;
        case SearchType::Double32:
            return &ra::services::search::s_pDouble32SearchImpl;
        case SearchType::Double32Aligned:
            return &ra::services::search::s_pDouble32AlignedSearchImpl;
        case SearchType::Double32BigEndian:
            return &ra::services::search::s_pDouble32BESearchImpl;
        case SearchType::Double32BigEndianAligned:
            return &ra::services::search::s_pDouble32BEAlignedSearchImpl;
        case SearchType::MBF32:
            return &ra::services::search::s_pMBF32SearchImpl;
        case SearchType::MBF32LE:
            return &ra::services::search::s_pMBF32LESearchImpl;
    }
}

#ifndef RA_UTEST
static size_t CalcSize(const std::vector<ra::data::CapturedMemoryBlock>& vBlocks)
{
    std::set<const uint8_t*> vAllocatedMemoryBlocks;

    size_t nTotalSize = vBlocks.size() * sizeof(ra::data::CapturedMemoryBlock);
    for (const auto& pBlock : vBlocks)
    {
        if (pBlock.GetBytesSize() > 8) // IsBytesAllocated
        {
            const auto* pBytes = pBlock.GetBytes();
            if (vAllocatedMemoryBlocks.find(pBytes) == vAllocatedMemoryBlocks.end())
            {
                vAllocatedMemoryBlocks.insert(pBytes);

                // 8 = sizeof(MemBlock::AllocatedMemory) - sizeof(MemBlock::AllocatedMemory::pBytes)
                nTotalSize += gsl::narrow_cast<size_t>(((pBlock.GetBytesSize() + 3) & ~3) + 8);
            }
        }

        if (pBlock.GetMatchingAddressCount() > 8)
            nTotalSize += pBlock.GetMatchingAddressCount();
    }

    return nTotalSize;
}
#endif

void SearchResults::Initialize(ra::data::ByteAddress nAddress, size_t nBytes, SearchType nType)
{
    m_nType = nType;
    m_pImpl = GetSearchImpl(nType);

    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    const auto nTotalMemorySize = pMemoryContext.TotalMemorySize();
    if (nAddress > nTotalMemorySize)
        nAddress = 0;
    if (nBytes + nAddress > nTotalMemorySize)
        nBytes = nTotalMemorySize - nAddress;

    const auto nPadding = m_pImpl->GetPadding();
    if (nPadding >= nBytes)
        nBytes = 0;

    pMemoryContext.CaptureMemory(m_vBlocks, nAddress, gsl::narrow_cast<uint32_t>(nBytes), nPadding);

    for (auto& pBlock : m_vBlocks)
    {
        pBlock.SetFirstAddress(m_pImpl->ConvertFromRealAddress(pBlock.GetFirstAddress()));

        const auto nAdjustedAddressCount = m_pImpl->GetAddressCountForBytes(pBlock.GetBytesSize());
        pBlock.SetAddressCount(nAdjustedAddressCount);
    }

    RA_LOG_INFO("Allocated %zu bytes for initial search", CalcSize(m_vBlocks));
}

_Use_decl_annotations_
void SearchResults::Initialize(const std::vector<SearchResult>& vResults, SearchType nType)
{
    m_nType = nType;
    m_pImpl = GetSearchImpl(nType);

    const auto bAligned = m_pImpl->ConvertToRealAddress(m_pImpl->ConvertFromRealAddress(7)) != 7;
    const auto nSize = ra::data::Memory::SizeBytes(m_pImpl->GetMemSize());
    bool bNeedsReversed = false;
    switch (nType)
    {
        case SearchType::SixteenBitBigEndian:
        case SearchType::SixteenBitBigEndianAligned:
        case SearchType::ThirtyTwoBitBigEndian:
        case SearchType::ThirtyTwoBitBigEndianAligned:
            bNeedsReversed = true;
            break;
    }

    auto nFirstAddress = vResults.front().nAddress;
    auto nLastAddressPlusOne = vResults.back().nAddress + nSize;
    if (bAligned)
    {
        nFirstAddress = m_pImpl->ConvertToRealAddress(m_pImpl->ConvertFromRealAddress(nFirstAddress));
        nLastAddressPlusOne = m_pImpl->ConvertToRealAddress(m_pImpl->ConvertFromRealAddress(nLastAddressPlusOne));
    }

    const auto nMemorySize = gsl::narrow_cast<size_t>(nLastAddressPlusOne) - nFirstAddress;
    std::vector<unsigned char> vMemory;
    vMemory.resize(nMemorySize);

    std::vector<ra::data::ByteAddress> vAddresses;
    vAddresses.reserve(vResults.size());

    for (const auto& pResult : vResults)
    {
        if (pResult.nAddress < nFirstAddress || pResult.nAddress + nSize > nLastAddressPlusOne)
            continue;

        const auto nVirtualAddress = m_pImpl->ConvertFromRealAddress(pResult.nAddress);

        // ignore unaligned values for aligned types
        if (bAligned && m_pImpl->ConvertToRealAddress(nVirtualAddress) != pResult.nAddress)
            continue;

        vAddresses.push_back(nVirtualAddress);

        auto nOffset = pResult.nAddress - nFirstAddress;        
        auto nValue = pResult.nValue;
        if (bNeedsReversed)
        {
            nValue = (((nValue & 0xFF000000) >> 24) |
                      ((nValue & 0x00FF0000) >> 8) |
                      ((nValue & 0x0000FF00) << 8) |
                      ((nValue & 0x000000FF) << 24));
        }

        switch (nSize)
        {
            case 4:
                vMemory.at(nOffset++) = nValue & 0xFF;
                nValue >>= 8;
                _FALLTHROUGH;
            case 3:
                vMemory.at(nOffset++) = nValue & 0xFF;
                nValue >>= 8;
                _FALLTHROUGH;
            case 2:
                vMemory.at(nOffset++) = nValue & 0xFF;
                nValue >>= 8;
                _FALLTHROUGH;
            default:
                vMemory.at(nOffset) = nValue & 0xFF;
                break;
        }
    }

    m_pImpl->AddBlocks(*this, vAddresses, vMemory, nFirstAddress, m_pImpl->GetPadding());
    RA_LOG_INFO("Allocated %zu bytes for initial search", CalcSize(m_vBlocks));
}

bool SearchResults::ContainsAddress(ra::data::ByteAddress nAddress) const
{
    if (m_pImpl)
        return m_pImpl->ContainsAddress(*this, nAddress);

    return false;
}

void SearchResults::MergeSearchResults(const SearchResults& srMemory, const SearchResults& srAddresses)
{
    m_vBlocks.reserve(srAddresses.m_vBlocks.size());
    m_nType = srAddresses.m_nType;
    m_pImpl = srAddresses.m_pImpl;
    m_nCompareType = srAddresses.m_nCompareType;
    m_nFilterType = srAddresses.m_nFilterType;
    m_nFilterValue = srAddresses.m_nFilterValue;

    for (const auto& pSrcBlock : srAddresses.m_vBlocks)
    {
        // copy the block from the srAddresses collection, then update the memory from the srMemory collection.
        // this creates a new block with the memory from the srMemory collection and the addresses from the
        // srAddresses collection.
        auto& pNewBlock = m_vBlocks.emplace_back(pSrcBlock);
        unsigned int nSize = pNewBlock.GetBytesSize();
        ra::data::ByteAddress nAddress = m_pImpl->ConvertToRealAddress(pNewBlock.GetFirstAddress());
        unsigned char* pWrite = pNewBlock.GetBytes();

        for (const auto& pMemBlock : srMemory.m_vBlocks)
        {
            const auto nBlockFirstAddress = m_pImpl->ConvertToRealAddress(pMemBlock.GetFirstAddress());
            if (nAddress >= nBlockFirstAddress && nAddress < nBlockFirstAddress + pMemBlock.GetBytesSize())
            {
                const auto nOffset = nAddress - nBlockFirstAddress;
                const auto nAvailable = pMemBlock.GetBytesSize() - nOffset;
                if (nAvailable >= nSize)
                {
                    memcpy(pWrite, pMemBlock.GetBytes() + nOffset, nSize);
                    break;
                }
                else
                {
                    memcpy(pWrite, pMemBlock.GetBytes() + nOffset, nAvailable);
                    nSize -= nAvailable;
                    pWrite += nAvailable;
                    nAddress += nAvailable;
                }
            }
        }
    }
}

_Use_decl_annotations_
bool SearchResults::Initialize(const SearchResults& srFirst, std::function<void(ra::data::ByteAddress,uint8_t*,size_t)> pReadMemory,
    ComparisonType nCompareType, SearchFilterType nFilterType, const std::wstring& sFilterValue)
{
    m_nType = srFirst.m_nType;
    m_pImpl = srFirst.m_pImpl;
    m_nCompareType = nCompareType;
    m_nFilterType = nFilterType;
    m_sFilterValue = sFilterValue;

    if (!m_pImpl->ValidateFilterValue(*this))
        return false;

    m_pImpl->ApplyFilter(*this, srFirst, pReadMemory);

    RA_LOG_INFO("Allocated %zu bytes for filtered search", CalcSize(m_vBlocks));

    return true;
}

_Use_decl_annotations_
bool SearchResults::Initialize(const SearchResults& srSource, ComparisonType nCompareType,
    SearchFilterType nFilterType, const std::wstring& sFilterValue)
{
    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    auto pReadMemory = [&pMemoryContext](ra::data::ByteAddress nAddress, uint8_t* pBuffer, size_t nBufferSize) {
        pMemoryContext.ReadMemory(nAddress, pBuffer, nBufferSize);
    };

    return Initialize(srSource, pReadMemory, nCompareType, nFilterType, sFilterValue);
}

_Use_decl_annotations_
bool SearchResults::Initialize(const SearchResults& srMemory, const SearchResults& srAddresses,
    ComparisonType nCompareType, SearchFilterType nFilterType, const std::wstring& sFilterValue)
{
    if (&srMemory == &srAddresses)
        return Initialize(srMemory, nCompareType, nFilterType, sFilterValue);

    // create a new merged SearchResults object that has the matching addresses from srAddresses,
    // and the memory blocks from srMemory.
    SearchResults srMerge;
    srMerge.MergeSearchResults(srMemory, srAddresses);

    // then do a standard comparison against the merged SearchResults
    return Initialize(srMerge, nCompareType, nFilterType, sFilterValue);
}

size_t SearchResults::MatchingAddressCount() const noexcept
{
    size_t nCount = 0;
    for (const auto& pBlock : m_vBlocks)
        nCount += pBlock.GetMatchingAddressCount();

    return nCount;
}

bool SearchResults::ExcludeResult(const SearchResult& pResult)
{
    if (m_nFilterType != SearchFilterType::None && m_pImpl != nullptr)
        return m_pImpl->ExcludeResult(*this, pResult);

    return false;
}

bool SearchResults::GetMatchingAddress(gsl::index nIndex, _Out_ SearchResult& result) const noexcept
{
    if (m_pImpl == nullptr)
    {
        memset(&result, 0, sizeof(SearchResult));
        return false;
    }

    return m_pImpl->GetMatchingAddress(*this, nIndex, result);
}

bool SearchResults::GetMatchingAddress(const SearchResult& pSrcResult, _Out_ SearchResult& result) const noexcept
{
    memcpy(&result, &pSrcResult, sizeof(SearchResult));

    if (m_pImpl == nullptr)
        return false;

    return m_pImpl->GetValueAtVirtualAddress(*this, result);
}

bool SearchResults::GetBytes(ra::data::ByteAddress nAddress, unsigned char* pBuffer, size_t nCount) const noexcept
{
    if (m_pImpl != nullptr)
    {
        for (auto& block : m_vBlocks)
        {
            const auto nFirstAddress = m_pImpl->ConvertToRealAddress(block.GetFirstAddress());
            if (nFirstAddress > nAddress)
                break;

            const int nRemaining = ra::to_signed(nFirstAddress + block.GetBytesSize() - nAddress);
            if (nRemaining < 0)
                continue;

            if (ra::to_unsigned(nRemaining) >= nCount)
            {
                memcpy(pBuffer, block.GetBytes() + (nAddress - nFirstAddress), nCount);
                return true;
            }

            memcpy(pBuffer, block.GetBytes() + (nAddress - nFirstAddress), nRemaining);
            nCount -= nRemaining;
            pBuffer += nRemaining;
            nAddress += nRemaining;
        }
    }

    memset(pBuffer, gsl::narrow_cast<int>(nCount), 0);
    return false;
}

std::wstring SearchResults::GetFormattedValue(ra::data::ByteAddress nAddress, ra::data::Memory::Size nSize) const
{
    return m_pImpl ? m_pImpl->GetFormattedValue(*this, nAddress, nSize) : L"";
}

bool SearchResults::UpdateValue(SearchResult& pResult,
    _Out_ std::wstring* sFormattedValue, const ra::context::IEmulatorMemoryContext& pMemoryContext) const
{
    if (m_pImpl)
        return m_pImpl->UpdateValue(*this, pResult, sFormattedValue, pMemoryContext);

    if (sFormattedValue)
        sFormattedValue->clear();

    return false;
}

bool SearchResults::MatchesFilter(const SearchResults& pPreviousResults, SearchResult& pResult) const
{
    if (m_pImpl)
        return m_pImpl->MatchesFilter(*this, pPreviousResults, pResult);

    return true;
}

ra::data::Memory::Size SearchResults::GetSize() const noexcept
{
    return m_pImpl ? m_pImpl->GetMemSize() : ra::data::Memory::Size::EightBit;
}

SearchType GetAlignedSearchType(SearchType searchType) noexcept
{
    switch (searchType)
    {
        case SearchType::SixteenBit: return SearchType::SixteenBitAligned;
        case SearchType::SixteenBitBigEndian: return SearchType::SixteenBitBigEndianAligned;
        case SearchType::ThirtyTwoBit: return SearchType::ThirtyTwoBitAligned;
        case SearchType::ThirtyTwoBitBigEndian: return SearchType::ThirtyTwoBitBigEndianAligned;
        case SearchType::Float: return SearchType::FloatAligned;
        case SearchType::FloatBigEndian: return SearchType::FloatBigEndianAligned;
        case SearchType::Double32: return SearchType::Double32Aligned;
        case SearchType::Double32BigEndian: return SearchType::Double32BigEndianAligned;
        default: return SearchType::None;
    }
}


} // namespace services
} // namespace ra
