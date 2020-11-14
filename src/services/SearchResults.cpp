#include "SearchResults.h"

#include "RA_StringUtils.h"

#include "ra_utility.h"

#include "data\context\EmulatorContext.hh"

#include "services\ServiceLocator.hh"

#include <algorithm>

namespace ra {
namespace services {

namespace impl {

_NODISCARD static constexpr bool CompareValues(_In_ unsigned int nLeft, _In_ unsigned int nRight, _In_ ComparisonType nCompareType) noexcept
{
    switch (nCompareType)
    {
        case ComparisonType::Equals:
            return nLeft == nRight;
        case ComparisonType::LessThan:
            return nLeft < nRight;
        case ComparisonType::LessThanOrEqual:
            return nLeft <= nRight;
        case ComparisonType::GreaterThan:
            return nLeft > nRight;
        case ComparisonType::GreaterThanOrEqual:
            return nLeft >= nRight;
        case ComparisonType::NotEqualTo:
            return nLeft != nRight;
        default:
            return false;
    }
}

class SearchImpl
{
public:
    SearchImpl() noexcept = default;
    virtual ~SearchImpl() noexcept = default;
    SearchImpl(const SearchImpl&) noexcept = delete;
    SearchImpl& operator=(const SearchImpl&) noexcept = delete;
    SearchImpl(SearchImpl&&) noexcept = delete;
    SearchImpl& operator=(SearchImpl&&) noexcept = delete;

    // Gets the number of bytes after an address that are required to hold the data at the address
    virtual unsigned int GetPadding() const noexcept { return 0U; }

    // Gets the size of values handled by this implementation
    virtual MemSize GetMemSize() const noexcept { return MemSize::EightBit; }

    // Determines if the specified address exists in the collection of addresses.
    virtual bool ContainsAddress(const std::vector<ra::ByteAddress>& vAddresses, ra::ByteAddress nAddress) const
    {
        return std::binary_search(vAddresses.begin(), vAddresses.end(), nAddress);
    }

    virtual bool IsAddressValid(_UNUSED ra::ByteAddress nAddress) const noexcept
    {
        return true;
    }

    virtual size_t AdjustInitialAddressCount(size_t nBytes) const noexcept
    {
        return nBytes;
    }

    // populates a vector of addresses that match the specified filter when applied to a previous search result
    virtual void ApplyFilter(SearchResults& srNew, const SearchResults& srPrevious) const
    {
        unsigned int nLargestBlock = 0U;
        for (auto& block : srPrevious.m_vBlocks)
        {
            if (block.GetSize() > nLargestBlock)
                nLargestBlock = block.GetSize();
        }

        std::vector<unsigned char> vMemory(nLargestBlock);
        std::vector<ra::ByteAddress> vMatches;
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();

        for (auto& block : srPrevious.m_vBlocks)
        {
            pEmulatorContext.ReadMemory(block.GetAddress(), vMemory.data(), block.GetSize());

            const auto nStop = block.GetSize() - GetPadding();

            switch (srNew.GetFilterType())
            {
                default:
                case SearchFilterType::Constant:
                    for (unsigned int i = 0; i < nStop; ++i)
                    {
                        const unsigned int nValue1 = BuildValue(&vMemory.at(i));
                        const unsigned int nAddress = block.GetAddress() + i;
                        ApplyFilter(vMatches, srPrevious, nAddress, srNew, nValue1, srNew.GetFilterValue());
                    }
                    break;

                case SearchFilterType::InitialValue:
                case SearchFilterType::LastKnownValue:
                case SearchFilterType::LastKnownValuePlus:
                case SearchFilterType::LastKnownValueMinus:
                    for (unsigned int i = 0; i < nStop; ++i)
                    {
                        const unsigned int nValue1 = BuildValue(&vMemory.at(i));
                        const unsigned int nValue2 = BuildValue(block.GetBytes() + i);
                        const unsigned int nAddress = block.GetAddress() + i;
                        ApplyFilter(vMatches, srPrevious, nAddress, srNew, nValue1, nValue2);
                    }
                    break;
            }

            if (!vMatches.empty())
            {
                AddBlock(srNew, vMatches, vMemory, block.GetAddress());
                srNew.m_vMatchingAddresses.insert(srNew.m_vMatchingAddresses.end(), vMatches.begin(), vMatches.end());
                vMatches.clear();
            }
        }
    }

    // gets the nIndex'th search result
    virtual bool GetMatchingAddress(const SearchResults& srResults, gsl::index nIndex, _Out_ SearchResults::Result& result) const
    {
        result.nSize = GetMemSize();

        if (srResults.m_vMatchingAddresses.empty())
        {
            if (!HasFirstAddress(srResults))
                return false;

            result.nAddress = GetFirstAddress(srResults) + gsl::narrow_cast<ra::ByteAddress>(nIndex);
        }
        else
        {
            if (ra::to_unsigned(nIndex) >= srResults.m_vMatchingAddresses.size())
                return false;

            result.nAddress = srResults.m_vMatchingAddresses.at(nIndex);
        }

        return GetValue(srResults, result);
    }

    // gets the value associated with the address and size in the search result structure
    bool GetValue(const SearchResults& srResults, SearchResults::Result& result) const noexcept
    {
        for (const auto& block : srResults.m_vBlocks)
        {
            if (GetValue(block, result))
                return true;
        }

        result.nValue = 0;
        return false;
    }

protected:
    static const std::vector<ra::ByteAddress>& GetMatchingAddresses(const SearchResults& srResults) noexcept
    {
        return srResults.m_vMatchingAddresses;
    }

    static std::vector<ra::ByteAddress>& GetMatchingAddresses(SearchResults& srResults) noexcept
    {
        return srResults.m_vMatchingAddresses;
    }

    static bool HasFirstAddress(const SearchResults& srResults) noexcept
    {
        return !srResults.m_vBlocks.empty();
    }

    static ra::ByteAddress GetFirstAddress(const SearchResults& srResults) noexcept
    {
        return srResults.m_vBlocks.front().GetAddress();
    }

    static const std::vector<impl::MemBlock>& GetBlocks(const SearchResults& srResults) noexcept
    {
        return srResults.m_vBlocks;
    }

    virtual bool GetValue(const impl::MemBlock& block, SearchResults::Result& result) const noexcept
    {
        if (result.nAddress < block.GetAddress())
            return false;

        const unsigned int nOffset = result.nAddress - block.GetAddress();
        if (nOffset >= block.GetSize() - GetPadding())
            return false;

        result.nValue = BuildValue(block.GetBytes() + nOffset);
        return true;
    }

    virtual unsigned int BuildValue(const unsigned char* ptr) const noexcept
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        return ptr[0];
    }

    virtual void ApplyFilter(std::vector<ra::ByteAddress>& vMatches, const SearchResults& srPrevious,
        ra::ByteAddress nAddress, const SearchResults& srFilter,
        unsigned int nValue1, unsigned int nValue2) const
    {
        switch (srFilter.GetFilterType())
        {
            case SearchFilterType::LastKnownValuePlus:
                nValue2 += srFilter.GetFilterValue();
                break;

            case SearchFilterType::LastKnownValueMinus:
                nValue2 -= srFilter.GetFilterValue();
                break;

            default:
                break;
        }

        if (CompareValues(nValue1, nValue2, srFilter.GetFilterComparison()))
            AddMatch(vMatches, srPrevious, nAddress);
    }

    static void AddMatch(std::vector<ra::ByteAddress>& vMatches, const SearchResults& srPrevious, ra::ByteAddress nAddress)
    {
        if (srPrevious.m_nFilterType == SearchFilterType::None ||
            std::binary_search(srPrevious.m_vMatchingAddresses.begin(), srPrevious.m_vMatchingAddresses.end(), nAddress))
        {
            vMatches.push_back(nAddress);
        }
    }

    static impl::MemBlock& AddBlock(SearchResults& srNew, ra::ByteAddress nAddress, unsigned int nSize)
    {
        return srNew.m_vBlocks.emplace_back(nAddress, nSize);
    }

    virtual void AddBlock(SearchResults& srNew, std::vector<ra::ByteAddress>& vMatches,
        std::vector<unsigned char>& vMemory, ra::ByteAddress nFirstMemoryAddress) const
    {
        const unsigned int nBlockSize = vMatches.back() - vMatches.front() + GetPadding() + 1;
        MemBlock& block = AddBlock(srNew, vMatches.front(), nBlockSize);
        memcpy(block.GetBytes(), &vMemory.at(vMatches.front() - nFirstMemoryAddress), nBlockSize);
    }
};

class FourBitSearchImpl : public SearchImpl
{
    MemSize GetMemSize() const noexcept override { return MemSize::Nibble_Lower; }

    bool ContainsAddress(const std::vector<ra::ByteAddress>& vAddresses, ra::ByteAddress nAddress) const override
    {
        nAddress <<= 1;
        const auto iter = std::lower_bound(vAddresses.begin(), vAddresses.end(), nAddress);
        if (iter == vAddresses.end())
            return false;

        return (*iter == nAddress || *iter == (nAddress | 1));
    }

    size_t AdjustInitialAddressCount(size_t nCount) const noexcept override
    {
        return nCount * 2;
    }

    void ApplyFilter(std::vector<ra::ByteAddress>& vMatches, const SearchResults& srPrevious,
        ra::ByteAddress nAddress, const SearchResults& srFilter,
        unsigned int nValue1, unsigned int nValue2) const override
    {
        const unsigned int nValue1a = (nValue1 & 0x0F);
        const unsigned int nValue1b = ((nValue1 >> 4) & 0x0F);
        unsigned int nValue2a = (nValue2 & 0x0F);
        unsigned int nValue2b = ((nValue2 >> 4) & 0x0F);

        switch (srFilter.GetFilterType())
        {
            case SearchFilterType::LastKnownValuePlus:
                nValue2a += srFilter.GetFilterValue();
                nValue2b += srFilter.GetFilterValue();
                break;

            case SearchFilterType::LastKnownValueMinus:
                nValue2a -= srFilter.GetFilterValue();
                nValue2b -= srFilter.GetFilterValue();
                break;

            case SearchFilterType::Constant:
                nValue2b = nValue2a;
                break;

            default:
                break;
        }

        if (CompareValues(nValue1a, nValue2a, srFilter.GetFilterComparison()))
            AddMatch(vMatches, srPrevious, nAddress << 1);
        if (CompareValues(nValue1b, nValue2b, srFilter.GetFilterComparison()))
            AddMatch(vMatches, srPrevious, (nAddress << 1) | 1);
    }

    bool GetMatchingAddress(const SearchResults& srResults, gsl::index nIndex, _Out_ SearchResults::Result& result) const override
    {
        result.nSize = GetMemSize();

        const auto& vMatchingAddresses = GetMatchingAddresses(srResults);
        if (vMatchingAddresses.empty())
        {
            if (!HasFirstAddress(srResults))
                return false;

            result.nAddress = (GetFirstAddress(srResults) << 1) + gsl::narrow_cast<ra::ByteAddress>(nIndex);
        }
        else
        {
            if (ra::to_unsigned(nIndex) >= vMatchingAddresses.size())
                return false;

            result.nAddress = vMatchingAddresses.at(nIndex);
        }

        if (result.nAddress & 0x01)
            result.nSize = MemSize::Nibble_Upper;

        result.nAddress >>= 1;
        return SearchImpl::GetValue(srResults, result);
    }

protected:
    void AddBlock(SearchResults& srNew, std::vector<ra::ByteAddress>& vMatches,
        std::vector<unsigned char>& vMemory, ra::ByteAddress nFirstMemoryAddress) const override
    {
        const ra::ByteAddress nFirstAddress = (vMatches.front() >> 1);
        const unsigned int nBlockSize = (vMatches.back() >> 1) - nFirstAddress + 1;
        MemBlock& block = SearchImpl::AddBlock(srNew, nFirstAddress, nBlockSize);
        memcpy(block.GetBytes(), &vMemory.at(nFirstAddress - nFirstMemoryAddress), nBlockSize);
    }

    bool GetValue(const impl::MemBlock& block, SearchResults::Result& result) const noexcept override
    {
        if (!SearchImpl::GetValue(block, result))
            return false;

        if (result.nSize == MemSize::Nibble_Lower)
            result.nValue &= 0x0F;
        else
            result.nValue = (result.nValue >> 4) & 0x0F;

        return true;
    }
};

class EightBitSearchImpl : public SearchImpl
{

};

class SixteenBitSearchImpl : public SearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::SixteenBit; }

    unsigned int GetPadding() const noexcept override { return 1U; }

    unsigned int BuildValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        return (ptr[1] << 8) | ptr[0];
    }
};

class ThirtyTwoBitSearchImpl : public SearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::ThirtyTwoBit; }

    unsigned int GetPadding() const noexcept override { return 3U; }

    unsigned int BuildValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        return (ptr[3] << 24) | (ptr[2] << 16) | (ptr[1] << 8) | ptr[0];
    }
};

class ThirtyTwoBitAlignedSearchImpl : public ThirtyTwoBitSearchImpl
{
public:
    bool IsAddressValid(ra::ByteAddress nAddress) const noexcept override
    {
        return ((nAddress & 3) == 0);
    }

    size_t AdjustInitialAddressCount(size_t nCount) const noexcept override
    {
        return (nCount + GetPadding()) / 4; 
    }

    bool GetMatchingAddress(const SearchResults& srResults, gsl::index nIndex, _Out_ SearchResults::Result& result) const override
    {
        result.nSize = GetMemSize();

        if (srResults.GetFilterType() == SearchFilterType::None)
        {
            if (!HasFirstAddress(srResults))
                return false;

            result.nAddress = ((GetFirstAddress(srResults) + 3) & ~3) + gsl::narrow_cast<ra::ByteAddress>(nIndex * 4);
            return GetValue(srResults, result);
        }

        return ThirtyTwoBitSearchImpl::GetMatchingAddress(srResults, nIndex, result);
    }

    void ApplyFilter(std::vector<ra::ByteAddress>& vMatches, const SearchResults& srPrevious,
        ra::ByteAddress nAddress, const SearchResults& srFilter,
        unsigned int nValue1, unsigned int nValue2) const override
    {
        if (IsAddressValid(nAddress))
        {
            ThirtyTwoBitSearchImpl::ApplyFilter(vMatches, srPrevious, nAddress,
                srFilter, nValue1, nValue2);
        }
    }
};

class SixteenBitAlignedSearchImpl : public SixteenBitSearchImpl
{
public:
    bool IsAddressValid(ra::ByteAddress nAddress) const noexcept override
    {
        return ((nAddress & 1) == 0);
    }

    size_t AdjustInitialAddressCount(size_t nCount) const noexcept override
    {
        return (nCount + GetPadding()) / 2;
    }

    bool GetMatchingAddress(const SearchResults& srResults, gsl::index nIndex, _Out_ SearchResults::Result& result) const override
    {
        result.nSize = GetMemSize();

        if (srResults.GetFilterType() == SearchFilterType::None)
        {
            if (!HasFirstAddress(srResults))
                return false;

            result.nAddress = ((GetFirstAddress(srResults) + 1) & ~1) + gsl::narrow_cast<ra::ByteAddress>(nIndex * 2);
            return GetValue(srResults, result);
        }

        return SixteenBitSearchImpl::GetMatchingAddress(srResults, nIndex, result);
    }

    void ApplyFilter(std::vector<ra::ByteAddress>& vMatches, const SearchResults& srPrevious,
        ra::ByteAddress nAddress, const SearchResults& srFilter,
        unsigned int nValue1, unsigned int nValue2) const override
    {
        if (IsAddressValid(nAddress))
        {
            SixteenBitSearchImpl::ApplyFilter(vMatches, srPrevious, nAddress,
                srFilter, nValue1, nValue2);
        }
    }
};

class AsciiTextSearchImpl : public SearchImpl
{
public:
    // indicate that search results can be very wide
    MemSize GetMemSize() const noexcept override { return MemSize::Text; }

    // populates a vector of addresses that match the specified filter when applied to a previous search result
    void ApplyFilter(SearchResults& srNew, const SearchResults& srPrevious) const override
    {
        std::vector<unsigned char> vSearchText;
        GetSearchText(vSearchText, srNew.GetFilterString());

        // can't match 0 characters!
        if (vSearchText.empty())
            return;

        unsigned int nLargestBlock = 0U;
        for (auto& block : GetBlocks(srPrevious))
        {
            if (block.GetSize() > nLargestBlock)
                nLargestBlock = block.GetSize();
        }

        std::vector<unsigned char> vMemory(nLargestBlock);
        std::vector<ra::ByteAddress> vMatches;
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();

        for (auto& block : GetBlocks(srPrevious))
        {
            pEmulatorContext.ReadMemory(block.GetAddress(), vMemory.data(), block.GetSize());

            const auto nStop = block.GetSize() - 1;

            switch (srNew.GetFilterType())
            {
                default:
                case SearchFilterType::Constant:
                    for (unsigned int i = 0; i < nStop; ++i)
                    {
                        if (CompareMemory(&vMemory.at(i), &vSearchText.at(0), vSearchText.size(), srNew.GetFilterComparison()))
                        {
                            const unsigned int nAddress = block.GetAddress() + i;
                            AddMatch(vMatches, srPrevious, nAddress);
                        }
                    }
                    break;

                case SearchFilterType::InitialValue:
                case SearchFilterType::LastKnownValue:
                    for (unsigned int i = 0; i < nStop; ++i)
                    {
                        if (CompareMemory(&vMemory.at(i), block.GetBytes() + i, vSearchText.size(), srNew.GetFilterComparison()))
                        {
                            const unsigned int nAddress = block.GetAddress() + i;
                            AddMatch(vMatches, srPrevious, nAddress);
                        }
                    }
                    break;
            }

            if (!vMatches.empty())
            {
                auto& vMatchingAddresses = GetMatchingAddresses(srNew);
                vMatchingAddresses.insert(vMatchingAddresses.end(), vMatches.begin(), vMatches.end());

                // adjust the last entry to account for the length of the string to ensure
                // the block contains the whole string
                vMatches.back() += gsl::narrow_cast<ra::ByteAddress>(vSearchText.size() - 1);
                AddBlock(srNew, vMatches, vMemory, block.GetAddress());

                vMatches.clear();
            }
        }
    }

    virtual void GetSearchText(std::vector<unsigned char>& vText, const std::wstring& sText) const
    {
        for (const auto c : sText)
            vText.push_back(gsl::narrow_cast<unsigned char>(c));

        // NOTE: excludes null terminator
    }

    virtual bool CompareMemory(const unsigned char* pLeft,
        const unsigned char* pRight, size_t nCount, ComparisonType nCompareType) const
    {
        Expects(pLeft != nullptr);
        Expects(pRight != nullptr);

        do
        {
            // not exact match, compare the current character normally
            if (*pLeft != *pRight)
                return CompareValues(*pLeft, *pRight, nCompareType);

            // both strings terminated; exact match
            if (*pLeft == '\0')
                break;

            // proceed to next character
            ++pLeft;
            ++pRight;
        } while (--nCount);

        // exact match
        return (nCompareType == ComparisonType::Equals ||
            nCompareType == ComparisonType::GreaterThanOrEqual ||
            nCompareType == ComparisonType::LessThanOrEqual);
    }
};

static FourBitSearchImpl s_pFourBitSearchImpl;
static EightBitSearchImpl s_pEightBitSearchImpl;
static SixteenBitSearchImpl s_pSixteenBitSearchImpl;
static ThirtyTwoBitSearchImpl s_pThirtyTwoBitSearchImpl;
static SixteenBitAlignedSearchImpl s_pSixteenBitAlignedSearchImpl;
static ThirtyTwoBitAlignedSearchImpl s_pThirtyTwoBitAlignedSearchImpl;
static AsciiTextSearchImpl s_pAsciiTextSearchImpl;

} // namespace impl

_CONSTANT_VAR MAX_BLOCK_SIZE = 256U * 1024; // 256K

void SearchResults::Initialize(ra::ByteAddress nAddress, size_t nBytes, SearchType nType)
{
    m_nType = nType;

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (nBytes + nAddress > pEmulatorContext.TotalMemorySize())
        nBytes = pEmulatorContext.TotalMemorySize() - nAddress;

    switch (nType)
    {
        case SearchType::FourBit:
            m_pImpl = &ra::services::impl::s_pFourBitSearchImpl;
            break;
        default:
        case SearchType::EightBit:
            m_pImpl = &ra::services::impl::s_pEightBitSearchImpl;
            break;
        case SearchType::SixteenBit:
            m_pImpl = &ra::services::impl::s_pSixteenBitSearchImpl;
            break;
        case SearchType::ThirtyTwoBit:
            m_pImpl = &ra::services::impl::s_pThirtyTwoBitSearchImpl;
            break;
        case SearchType::SixteenBitAligned:
            m_pImpl = &ra::services::impl::s_pSixteenBitAlignedSearchImpl;
            break;
        case SearchType::ThirtyTwoBitAligned:
            m_pImpl = &ra::services::impl::s_pThirtyTwoBitAlignedSearchImpl;
            break;
        case SearchType::AsciiText:
            m_pImpl = &ra::services::impl::s_pAsciiTextSearchImpl;
            break;
    }

    const unsigned int nPadding = m_pImpl->GetPadding();
    if (nPadding >= nBytes)
        nBytes = 0;
    else if (nBytes > nPadding)
        nBytes -= nPadding;

    while (nBytes > 0)
    {
        const auto nBlockSize = gsl::narrow_cast<unsigned int>((nBytes > MAX_BLOCK_SIZE) ? MAX_BLOCK_SIZE : nBytes);
        auto& block = m_vBlocks.emplace_back(nAddress, nBlockSize + nPadding);
        pEmulatorContext.ReadMemory(block.GetAddress(), block.GetBytes(), gsl::narrow_cast<size_t>(block.GetSize()));

        nAddress += nBlockSize;
        nBytes -= nBlockSize;
    }
}

bool SearchResults::Result::Compare(unsigned int nPreviousValue, ComparisonType nCompareType) const noexcept
{
    return impl::CompareValues(nValue, nPreviousValue, nCompareType);
}

bool SearchResults::ContainsAddress(ra::ByteAddress nAddress) const
{
    if (m_nFilterType != SearchFilterType::None)
        return m_pImpl->ContainsAddress(m_vMatchingAddresses, nAddress);

    if (!m_pImpl->IsAddressValid(nAddress))
        return false;

    const unsigned int nPadding = m_pImpl->GetPadding();
    for (const auto& block : m_vBlocks)
    {
        if (block.GetAddress() > nAddress)
            return false;
        if (nAddress < block.GetAddress() + block.GetSize() - nPadding)
            return true;
    }

    return false;
}

_Use_decl_annotations_
void SearchResults::Initialize(const SearchResults& srSource, ComparisonType nCompareType,
    SearchFilterType nFilterType, unsigned int nFilterValue)
{
    m_nType = srSource.m_nType;
    m_pImpl = srSource.m_pImpl;
    m_nCompareType = nCompareType;
    m_nFilterType = nFilterType;
    m_nFilterValue = nFilterValue;

    m_pImpl->ApplyFilter(*this, srSource);
}

_Use_decl_annotations_
void SearchResults::Initialize(const SearchResults& srMemory, const SearchResults& srAddresses,
    ComparisonType nCompareType, SearchFilterType nFilterType, unsigned int nFilterValue)
{
    if (&srMemory == &srAddresses)
    {
        Initialize(srMemory, nCompareType, nFilterType, nFilterValue);
        return;
    }

    // create a new merged SearchResults object that has the matching addresses from srAddresses,
    // and the memory blocks from srMemory.
    SearchResults srMerge;
    srMerge.MergeSearchResults(srMemory, srAddresses);

    // then do a standard comparison against the merged SearchResults
    Initialize(srMerge, nCompareType, nFilterType, nFilterValue);
}

void SearchResults::MergeSearchResults(const SearchResults& srMemory, const SearchResults& srAddresses)
{
    m_vMatchingAddresses = srAddresses.m_vMatchingAddresses;
    m_vBlocks.reserve(srAddresses.m_vBlocks.size());
    m_nType = srAddresses.m_nType;
    m_pImpl = srAddresses.m_pImpl;
    m_nCompareType = srAddresses.m_nCompareType;
    m_nFilterType = srAddresses.m_nFilterType;
    m_nFilterValue = srAddresses.m_nFilterValue;

    for (const auto pSrcBlock : srAddresses.m_vBlocks)
    {
        unsigned int nSize = pSrcBlock.GetSize();
        ra::ByteAddress nAddress = pSrcBlock.GetAddress();
        auto& pNewBlock = m_vBlocks.emplace_back(nAddress, nSize);
        unsigned char* pWrite = pNewBlock.GetBytes();

        for (const auto pMemBlock : srMemory.m_vBlocks)
        {
            if (nAddress >= pMemBlock.GetAddress() && nAddress < pMemBlock.GetAddress() + pMemBlock.GetSize())
            {
                const auto nOffset = nAddress - pMemBlock.GetAddress();
                const auto nAvailable = pMemBlock.GetSize() - nOffset;
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
void SearchResults::Initialize(const SearchResults& srSource, ComparisonType nCompareType,
    SearchFilterType nFilterType, const std::wstring& sFilterValue)
{
    m_nType = srSource.m_nType;
    m_pImpl = srSource.m_pImpl;
    m_nCompareType = nCompareType;
    m_nFilterType = nFilterType;
    m_sFilterValue = sFilterValue;

    m_pImpl->ApplyFilter(*this, srSource);
}

_Use_decl_annotations_
void SearchResults::Initialize(const SearchResults& srMemory, const SearchResults& srAddresses,
    ComparisonType nCompareType, SearchFilterType nFilterType, const std::wstring& sFilterValue)
{
    if (&srMemory == &srAddresses)
    {
        Initialize(srMemory, nCompareType, nFilterType, sFilterValue);
        return;
    }

    // create a new merged SearchResults object that has the matching addresses from srAddresses,
    // and the memory blocks from srMemory.
    SearchResults srMerge;
    srMerge.MergeSearchResults(srMemory, srAddresses);

    // then do a standard comparison against the merged SearchResults
    Initialize(srMerge, nCompareType, nFilterType, sFilterValue);
}

size_t SearchResults::MatchingAddressCount() const noexcept
{
    if (m_nFilterType != SearchFilterType::None)
        return m_vMatchingAddresses.size();

    if (m_pImpl == nullptr)
        return 0;

    const unsigned int nPadding = m_pImpl->GetPadding();
    size_t nCount = 0;
    for (auto& block : m_vBlocks)
        nCount += gsl::narrow_cast<size_t>(block.GetSize()) - nPadding;

    return m_pImpl->AdjustInitialAddressCount(nCount);
}

void SearchResults::ExcludeAddress(ra::ByteAddress nAddress)
{
    if (m_nFilterType != SearchFilterType::None)
    {
        for (auto iter = m_vMatchingAddresses.begin(); iter != m_vMatchingAddresses.end(); ++iter)
        {
            if (*iter == nAddress)
            {
                m_vMatchingAddresses.erase(iter);
                break;
            }
        }
    }
}

void SearchResults::ExcludeMatchingAddress(gsl::index nIndex)
{
    if (m_nFilterType != SearchFilterType::None)
        m_vMatchingAddresses.erase(m_vMatchingAddresses.begin() + nIndex);
}

bool SearchResults::GetMatchingAddress(gsl::index nIndex, _Out_ SearchResults::Result& result) const
{
    if (m_pImpl == nullptr)
    {
        memset(&result, 0, sizeof(SearchResults::Result));
        return false;
    }

    return m_pImpl->GetMatchingAddress(*this, nIndex, result);
}

bool SearchResults::GetValue(ra::ByteAddress nAddress, MemSize nSize, _Out_ unsigned int& nValue) const noexcept
{
    if (m_pImpl == nullptr)
    {
        nValue = 0U;
        return false;
    }

    Result result{ nAddress, 0, nSize };
    const auto ret = m_pImpl->GetValue(*this, result);
    nValue = result.nValue;
    return ret;
}

bool SearchResults::GetBytes(ra::ByteAddress nAddress, unsigned char* pBuffer, size_t nCount) const noexcept
{
    if (m_pImpl != nullptr)
    {
        const unsigned int nPadding = m_pImpl->GetPadding();
        for (auto& block : m_vBlocks)
        {
            if (block.GetAddress() > nAddress)
                break;

            const int nRemaining = ra::to_signed(block.GetAddress() + block.GetSize() - nAddress);
            if (nRemaining < 0)
                continue;

            if (ra::to_unsigned(nRemaining) >= nCount)
            {
                memcpy(pBuffer, block.GetBytes() + (nAddress - block.GetAddress()), nCount);
                return true;
            }

            memcpy(pBuffer, block.GetBytes() + (nAddress - block.GetAddress()), nRemaining);
            nCount -= nRemaining;
            pBuffer += nRemaining;
            nAddress += nRemaining;
        }
    }

    memset(pBuffer, gsl::narrow_cast<int>(nCount), 0);
    return false;
}

MemSize SearchResults::GetSize() const noexcept
{
    return m_pImpl ? m_pImpl->GetMemSize() : MemSize::EightBit;
}

} // namespace services
} // namespace ra
