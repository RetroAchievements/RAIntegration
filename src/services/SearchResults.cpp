#include "SearchResults.h"

#include "RA_Defs.h"
#include "ra_utility.h"

#include "data\context\EmulatorContext.hh"

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
#include "search\SearchImpl_double32_be.hh"
#include "search\SearchImpl_float.hh"
#include "search\SearchImpl_float_be.hh"
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
static search::FloatBESearchImpl s_pFloatBESearchImpl;
static search::Double32SearchImpl s_pDouble32SearchImpl;
static search::Double32BESearchImpl s_pDouble32BESearchImpl;
static search::MBF32SearchImpl s_pMBF32SearchImpl;
static search::MBF32LESearchImpl s_pMBF32LESearchImpl;
} // namespace search

_CONSTANT_VAR MAX_BLOCK_SIZE = 256U * 1024; // 256K

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
        case SearchType::FloatBigEndian:
            return &ra::services::search::s_pFloatBESearchImpl;
        case SearchType::Double32:
            return &ra::services::search::s_pDouble32SearchImpl;
        case SearchType::Double32BigEndian:
            return &ra::services::search::s_pDouble32BESearchImpl;
        case SearchType::MBF32:
            return &ra::services::search::s_pMBF32SearchImpl;
        case SearchType::MBF32LE:
            return &ra::services::search::s_pMBF32LESearchImpl;
    }
}

void SearchResults::Initialize(ra::ByteAddress nAddress, size_t nBytes, SearchType nType)
{
    m_nType = nType;
    m_pImpl = GetSearchImpl(nType);

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    const auto nTotalMemorySize = pEmulatorContext.TotalMemorySize();
    if (nAddress > nTotalMemorySize)
        nAddress = 0;
    if (nBytes + nAddress > nTotalMemorySize)
        nBytes = nTotalMemorySize - nAddress;

    const unsigned int nPadding = m_pImpl->GetPadding();
    if (nPadding >= nBytes)
        nBytes = 0;
    else if (nBytes > nPadding)
        nBytes -= nPadding;

    while (nBytes > 0)
    {
        const auto nBlockSize = gsl::narrow_cast<unsigned int>((nBytes > MAX_BLOCK_SIZE) ? MAX_BLOCK_SIZE : nBytes);
        const auto nMaxAddresses = m_pImpl->GetAddressCountForBytes(nBlockSize + nPadding);
        const auto nBlockAddress = m_pImpl->ConvertFromRealAddress(nAddress);
        auto& block = m_vBlocks.emplace_back(nBlockAddress, nBlockSize + nPadding, nMaxAddresses);
        pEmulatorContext.ReadMemory(nAddress, block.GetBytes(), gsl::narrow_cast<size_t>(block.GetBytesSize()));

        nAddress += nBlockSize;
        nBytes -= nBlockSize;
    }
}

_Use_decl_annotations_
void SearchResults::Initialize(const std::vector<SearchResult>& vResults, SearchType nType)
{
    m_nType = nType;
    m_pImpl = GetSearchImpl(nType);

    const auto bAligned = m_pImpl->ConvertToRealAddress(m_pImpl->ConvertFromRealAddress(7)) != 7;
    const auto nSize = ra::data::MemSizeBytes(m_pImpl->GetMemSize());
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

    std::vector<ra::ByteAddress> vAddresses;
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
}

bool SearchResults::ContainsAddress(ra::ByteAddress nAddress) const
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
        ra::ByteAddress nAddress = m_pImpl->ConvertToRealAddress(pNewBlock.GetFirstAddress());
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
bool SearchResults::Initialize(const SearchResults& srFirst, std::function<void(ra::ByteAddress,uint8_t*,size_t)> pReadMemory,
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
    return true;
}

_Use_decl_annotations_
bool SearchResults::Initialize(const SearchResults& srSource, ComparisonType nCompareType,
    SearchFilterType nFilterType, const std::wstring& sFilterValue)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    auto pReadMemory = [&pEmulatorContext](ra::ByteAddress nAddress, uint8_t* pBuffer, size_t nBufferSize) {
        pEmulatorContext.ReadMemory(nAddress, pBuffer, nBufferSize);
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

bool SearchResults::GetBytes(ra::ByteAddress nAddress, unsigned char* pBuffer, size_t nCount) const noexcept
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

std::wstring SearchResults::GetFormattedValue(ra::ByteAddress nAddress, MemSize nSize) const
{
    return m_pImpl ? m_pImpl->GetFormattedValue(*this, nAddress, nSize) : L"";
}

bool SearchResults::UpdateValue(SearchResult& pResult,
    _Out_ std::wstring* sFormattedValue, const ra::data::context::EmulatorContext& pEmulatorContext) const
{
    if (m_pImpl)
        return m_pImpl->UpdateValue(*this, pResult, sFormattedValue, pEmulatorContext);

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

MemSize SearchResults::GetSize() const noexcept
{
    return m_pImpl ? m_pImpl->GetMemSize() : MemSize::EightBit;
}

} // namespace services
} // namespace ra
