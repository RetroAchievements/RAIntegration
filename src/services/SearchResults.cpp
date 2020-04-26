#include "SearchResults.h"

#include "RA_StringUtils.h"

#include "ra_utility.h"

#include "data/EmulatorContext.hh"

#include "services/ServiceLocator.hh"

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

    // populates a vector of addresses that match the specified filter when applied to a previous search result
    void ApplyFilter(SearchResults& srNew, const SearchResults& srPrevious,
        ComparisonType nCompareType, SearchFilterType nFilterType, unsigned int nFilterValue) const
    {
        unsigned int nLargestBlock = 0U;
        for (auto& block : srPrevious.m_vBlocks)
        {
            if (block.GetSize() > nLargestBlock)
                nLargestBlock = block.GetSize();
        }

        std::vector<unsigned char> vMemory(nLargestBlock);
        std::vector<ra::ByteAddress> vMatches;
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();

        for (auto& block : srPrevious.m_vBlocks)
        {
            pEmulatorContext.ReadMemory(block.GetAddress(), vMemory.data(), block.GetSize());

            const auto nStop = block.GetSize() - GetPadding();

            switch (nFilterType)
            {
                default:
                case SearchFilterType::Constant:
                    for (unsigned int i = 0; i < nStop; ++i)
                    {
                        const unsigned int nValue1 = BuildValue(&vMemory.at(i));
                        const unsigned int nAddress = block.GetAddress() + i;
                        ApplyFilter(vMatches, srPrevious, nAddress, nFilterType, nFilterValue, nValue1, nFilterValue, nCompareType);
                    }
                    break;

                case SearchFilterType::LastKnownValue:
                case SearchFilterType::LastKnownValuePlus:
                case SearchFilterType::LastKnownValueMinus:
                    for (unsigned int i = 0; i < nStop; ++i)
                    {
                        const unsigned int nValue1 = BuildValue(&vMemory.at(i));
                        const unsigned int nValue2 = BuildValue(block.GetBytes() + i);
                        const unsigned int nAddress = block.GetAddress() + i;
                        ApplyFilter(vMatches, srPrevious, nAddress, nFilterType, nFilterValue, nValue1, nValue2, nCompareType);
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
            if (srResults.m_vBlocks.empty())
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

    static ra::ByteAddress GetFirstAddress(const SearchResults& srResults) noexcept
    {
        return srResults.m_vBlocks.front().GetAddress();
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
        ra::ByteAddress nAddress, _UNUSED SearchFilterType nFilterType, unsigned int nFilterValue,
        unsigned int nValue1, unsigned int nValue2, ComparisonType nCompareType) const
    {
        switch (nFilterType)
        {
            case SearchFilterType::LastKnownValuePlus:
                nValue2 += nFilterValue;
                break;

            case SearchFilterType::LastKnownValueMinus:
                nValue2 -= nFilterValue;
                break;

            default:
                break;
        }

        if (CompareValues(nValue1, nValue2, nCompareType))
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

    void ApplyFilter(std::vector<ra::ByteAddress>& vMatches, const SearchResults& srPrevious,
        ra::ByteAddress nAddress, SearchFilterType nFilterType, unsigned int nFilterValue,
        unsigned int nValue1, unsigned int nValue2, ComparisonType nCompareType) const override
    {
        const unsigned int nValue1a = (nValue1 & 0x0F);
        const unsigned int nValue1b = ((nValue1 >> 4) & 0x0F);
        unsigned int nValue2a = (nValue2 & 0x0F);
        unsigned int nValue2b = ((nValue2 >> 4) & 0x0F);

        switch (nFilterType)
        {
            case SearchFilterType::LastKnownValuePlus:
                nValue2a += nFilterValue;
                nValue2b += nFilterValue;
                break;

            case SearchFilterType::LastKnownValueMinus:
                nValue2a -= nFilterValue;
                nValue2b -= nFilterValue;
                break;

            case SearchFilterType::Constant:
                nValue2b = nValue2a;
                break;

            default:
                break;
        }

        if (CompareValues(nValue1a, nValue2a, nCompareType))
            AddMatch(vMatches, srPrevious, nAddress << 1);
        if (CompareValues(nValue1b, nValue2b, nCompareType))
            AddMatch(vMatches, srPrevious, (nAddress << 1) | 1);
    }

    bool GetMatchingAddress(const SearchResults& srResults, gsl::index nIndex, _Out_ SearchResults::Result& result) const override
    {
        result.nSize = GetMemSize();

        const auto& vMatchingAddresses = GetMatchingAddresses(srResults);
        if (vMatchingAddresses.empty())
        {
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

static FourBitSearchImpl s_pFourBitSearchImpl;
static EightBitSearchImpl s_pEightBitSearchImpl;
static SixteenBitSearchImpl s_pSixteenBitSearchImpl;
static ThirtyTwoBitSearchImpl s_pThirtyTwoBitSearchImpl;

} // namespace impl

_CONSTANT_VAR MAX_BLOCK_SIZE = 256U * 1024; // 256K

void SearchResults::Initialize(ra::ByteAddress nAddress, size_t nBytes, SearchType nType)
{
    m_nType = nType;

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
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

    m_pImpl->ApplyFilter(*this, srSource, nCompareType, nFilterType, nFilterValue);
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

    if (m_nType == SearchType::FourBit)
        nCount *= 2;

    return nCount;
}

void SearchResults::ExcludeAddress(ra::ByteAddress nAddress)
{
    if (m_nFilterType != SearchFilterType::None)
        m_vMatchingAddresses.erase(std::remove(m_vMatchingAddresses.begin(), m_vMatchingAddresses.end(), nAddress), m_vMatchingAddresses.end());
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

MemSize SearchResults::GetSize() const noexcept
{
    return m_pImpl ? m_pImpl->GetMemSize() : MemSize::EightBit;
}

} // namespace services
} // namespace ra
