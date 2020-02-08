#include "SearchResults.h"

#include "RA_StringUtils.h"

#include "ra_utility.h"

#include "data/EmulatorContext.hh"

#include "services/ServiceLocator.hh"

#include <algorithm>

namespace ra {
namespace services {

_CONSTANT_VAR MAX_BLOCK_SIZE = 256U * 1024; // 256K

_NODISCARD inline static constexpr auto Padding(_In_ MemSize size) noexcept
{
    switch (size)
    {
        case MemSize::ThirtyTwoBit:
            return 3U;
        case MemSize::SixteenBit:
            return 1U;
        default:
            return 0U;
    }
}

void SearchResults::Initialize(ra::ByteAddress nAddress, size_t nBytes, MemSize nSize)
{
    if (nSize == MemSize::Nibble_Upper)
        nSize = MemSize::Nibble_Lower;

    m_nSize = nSize;
    m_bUnfiltered = true;

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    if (nBytes + nAddress > pEmulatorContext.TotalMemorySize())
        nBytes = pEmulatorContext.TotalMemorySize() - nAddress;

    const unsigned int nPadding = Padding(nSize);
    if (nPadding >= nBytes)
        nBytes = 0;
    else if (nBytes > nPadding)
        nBytes -= nPadding;

    while (nBytes > 0)
    {
        const auto nBlockSize = gsl::narrow_cast<unsigned int>((nBytes > MAX_BLOCK_SIZE) ? MAX_BLOCK_SIZE : nBytes);
        auto& block = AddBlock(nAddress, nBlockSize + nPadding);
        pEmulatorContext.ReadMemory(block.GetAddress(), block.GetBytes(), gsl::narrow_cast<size_t>(block.GetSize()));

        nAddress += nBlockSize;
        nBytes -= nBlockSize;
    }
}

SearchResults::MemBlock& SearchResults::AddBlock(ra::ByteAddress nAddress, unsigned int nSize)
{
    m_vBlocks.emplace_back(nAddress, nSize);
    return m_vBlocks.back();
}

_Success_(return)
_NODISCARD _CONSTANT_FN CompareValues(_In_ unsigned int nLeft,
                                      _In_ unsigned int nRight,
                                      _In_ ComparisonType nCompareType) noexcept
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

bool SearchResults::Result::Compare(unsigned int nPreviousValue, ComparisonType nCompareType) const noexcept
{
    return CompareValues(nValue, nPreviousValue, nCompareType);
}

_NODISCARD _CONSTANT_FN ComparisonString(_In_ ComparisonType nCompareType) noexcept
{
    switch (nCompareType)
    {
        case ComparisonType::Equals:
            return "EQUAL";
        case ComparisonType::LessThan:
            return "LESS THAN";
        case ComparisonType::LessThanOrEqual:
            return "LESS THAN/EQUAL";
        case ComparisonType::GreaterThan:
            return "GREATER THAN";
        case ComparisonType::GreaterThanOrEqual:
            return "GREATER THAN/EQUAL";
        case ComparisonType::NotEqualTo:
            return "NOT EQUAL";
        default:
            return "?";
    }
}

_Success_(return)
_NODISCARD inline static constexpr auto BuildValue(_In_ const unsigned char* const restrict pBuffer,
                                                   _In_ gsl::index nOffset, _In_ MemSize nSize) noexcept
{
    Expects(pBuffer != nullptr);
    auto ret{ 0U };
    switch (nSize)
    {
        case MemSize::EightBit:
            ret = pBuffer[nOffset];
            break;
        case MemSize::SixteenBit:
            ret = pBuffer[nOffset] | (pBuffer[nOffset + 1U] << 8U);
            break;
        case MemSize::ThirtyTwoBit:
            ret = (pBuffer[nOffset] | (pBuffer[nOffset + 1] << 8U) |
                (pBuffer[nOffset + 2] << 16U) | (pBuffer[nOffset + 3] << 24U));
            break;
        case MemSize::Nibble_Upper:
            ret = pBuffer[nOffset] >> 4U;
            break;
        case MemSize::Nibble_Lower:
            ret = pBuffer[nOffset] & 0x0FU;
    }

    return ret;
}

bool SearchResults::ContainsAddress(ra::ByteAddress nAddress) const
{
    if (!m_bUnfiltered)
    {
        if (m_nSize != MemSize::Nibble_Lower)
            return std::binary_search(m_vMatchingAddresses.begin(), m_vMatchingAddresses.end(), nAddress);

        nAddress <<= 1;
        const auto iter = std::lower_bound(m_vMatchingAddresses.begin(), m_vMatchingAddresses.end(), nAddress);
        return (iter != m_vMatchingAddresses.end() && (*iter == nAddress || *iter == (nAddress | 1)));
    }

    const unsigned int nPadding = Padding(m_nSize);
    for (const auto& block : m_vBlocks)
    {
        if (block.GetAddress() > nAddress)
            return false;
        if (nAddress < block.GetAddress() + block.GetSize() - nPadding)
            return true;
    }

    return false;
}

bool SearchResults::ContainsNibble(ra::ByteAddress nAddress) const
{
    if (!m_bUnfiltered)
        return std::binary_search(m_vMatchingAddresses.begin(), m_vMatchingAddresses.end(), nAddress);

    // an unfiltered collection implicitly contains both nibbles of an address
    return ContainsAddress(nAddress >> 1);
}

void SearchResults::AddMatches(ra::ByteAddress nAddressBase, const unsigned char* restrict pMemory, const std::vector<ra::ByteAddress>& vMatches)
{
    const unsigned int nBlockSize = vMatches.back() - vMatches.front() + Padding(m_nSize) + 1;
    MemBlock& block = AddBlock(nAddressBase + vMatches.front(), nBlockSize);
    memcpy(block.GetBytes(), pMemory + vMatches.front(), nBlockSize);

    for (auto nMatch : vMatches)
        m_vMatchingAddresses.push_back(nAddressBase + nMatch);
}

void SearchResults::ProcessBlocks(const SearchResults& srSource, std::function<bool(gsl::index nIndex, const unsigned char* restrict pMemory, const unsigned char* restrict pPrev)> testIndexFunction)
{
    std::vector<unsigned int> vMatches;
    std::vector<unsigned char> vMemory;
    const unsigned int nPadding = Padding(m_nSize);
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();

    for (auto& block : srSource.m_vBlocks)
    {
        if (block.GetSize() > vMemory.capacity())
            vMemory.reserve(block.GetSize());

        unsigned char* pMemory = vMemory.data();
        const unsigned char* pPrev = block.GetBytes();

        pEmulatorContext.ReadMemory(block.GetAddress(), pMemory, block.GetSize());

        for (unsigned int i = 0; i < block.GetSize() - nPadding; ++i)
        {
            if (!testIndexFunction(i, pMemory, pPrev))
                continue;

            const unsigned int nAddress = block.GetAddress() + i;
            if (!srSource.ContainsAddress(nAddress))
                continue;

            if (!vMatches.empty() && (i - vMatches.back()) > 16)
            {
                AddMatches(block.GetAddress(), pMemory, vMatches);
                vMatches.clear();
            }

            vMatches.push_back(i);
        }

        if (!vMatches.empty())
        {
            AddMatches(block.GetAddress(), pMemory, vMatches);
            vMatches.clear();
        }
    }
}

void SearchResults::AddMatchesNibbles(ra::ByteAddress nAddressBase, const unsigned char* restrict pMemory, const std::vector<ra::ByteAddress>& vMatches)
{
    const unsigned int nBlockSize = (vMatches.back() >> 1) - (vMatches.front() >> 1) + Padding(m_nSize) + 1;
    auto& block = AddBlock((nAddressBase + vMatches.front()) >> 1, nBlockSize);
    memcpy(block.GetBytes(), pMemory + (vMatches.front() >> 1), nBlockSize);

    for (auto nMatch : vMatches)
        m_vMatchingAddresses.push_back(nAddressBase + nMatch);
}

void SearchResults::ProcessBlocksNibbles(const SearchResults& srSource, unsigned int nTestValue, ComparisonType nCompareType)
{
    std::vector<unsigned int> vMatches;
    std::vector<unsigned char> vMemory;
    const unsigned int nPadding = Padding(m_nSize);
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();

    for (auto& block : srSource.m_vBlocks)
    {
        if (block.GetSize() > vMemory.size())
            vMemory.resize(block.GetSize());

        pEmulatorContext.ReadMemory(block.GetAddress(), vMemory.data(), block.GetSize());

        for (unsigned int i = 0; i < block.GetSize() - nPadding; ++i)
        {
            const unsigned int nValue1 = vMemory.at(i);
            unsigned int nValue2 = (nTestValue > 15) ? (block.GetByte(i) & 0x0F) : nTestValue;

            if (CompareValues(nValue1 & 0x0F, nValue2, nCompareType))
            {
                const unsigned int nAddress = (block.GetAddress() + i) << 1;
                if (srSource.ContainsNibble(nAddress))
                {
                    if (!vMatches.empty() && (i - (vMatches.back() >> 1)) > 16)
                    {
                        AddMatchesNibbles(block.GetAddress() << 1, vMemory.data(), vMatches);
                        vMatches.clear();
                    }

                    vMatches.push_back(i << 1);
                }
            }

            if (nTestValue > 15)
                nValue2 = block.GetByte(i) >> 4;

            if (CompareValues(nValue1 >> 4, nValue2, nCompareType))
            {
                const unsigned int nAddress = ((block.GetAddress() + i) << 1) | 1;
                if (srSource.ContainsNibble(nAddress))
                {
                    if (!vMatches.empty() && (i - (vMatches.back() >> 1)) > 16)
                    {
                        AddMatchesNibbles(block.GetAddress() << 1, vMemory.data(), vMatches);
                        vMatches.clear();
                    }

                    vMatches.push_back((i << 1) | 1);
                }
            }
        }

        if (!vMatches.empty())
        {
            AddMatchesNibbles(block.GetAddress() << 1, vMemory.data(), vMatches);
            vMatches.clear();
        }
    }
}


void SearchResults::Initialize(const SearchResults& srSource, ComparisonType nCompareType, unsigned int nTestValue)
{
    m_nSize = srSource.m_nSize;

    switch (m_nSize)
    {
        case MemSize::Nibble_Lower:
            ProcessBlocksNibbles(srSource, nTestValue & 0x0F, nCompareType);
            break;

        case MemSize::EightBit:
            ProcessBlocks(srSource,
                          [nTestValue, nCompareType](gsl::index nIndex, const unsigned char* restrict pMemory,
                                                     [[maybe_unused]] const unsigned char* restrict)
            {
                Expects(pMemory != nullptr);
                return CompareValues(pMemory[nIndex], nTestValue, nCompareType);
            });
            break;

        case MemSize::SixteenBit:
            ProcessBlocks(srSource, [nTestValue, nCompareType](gsl::index nIndex, const unsigned char* restrict pMemory, [[maybe_unused]] const unsigned char* restrict)
            {
                Expects(pMemory != nullptr);
                const unsigned int nValue = pMemory[nIndex] | (pMemory[nIndex + 1] << 8);
                return CompareValues(nValue, nTestValue, nCompareType);
            });
            break;

        case MemSize::ThirtyTwoBit:
            ProcessBlocks(srSource, [nTestValue, nCompareType](gsl::index nIndex, const unsigned char* restrict pMemory, [[maybe_unused]] const unsigned char* restrict)
            {
                Expects(pMemory != nullptr);
                const unsigned int nValue = pMemory[nIndex] | (pMemory[nIndex + 1] << 8) |
                    (pMemory[nIndex + 2] << 16) | (pMemory[nIndex + 3] << 24);
                return CompareValues(nValue, nTestValue, nCompareType);
            });
            break;
    }
}

_Use_decl_annotations_
void SearchResults::Initialize(const SearchResults& srSource, ComparisonType nCompareType)
{
    m_nSize = srSource.m_nSize;

    if (m_nSize == MemSize::EightBit)
    {
        // efficient comparisons for 8-bit
        switch (nCompareType)
        {
            case ComparisonType::Equals:
                ProcessBlocks(srSource, [](gsl::index nIndex, const unsigned char* restrict pMemory, const unsigned char* restrict pPrev)
                {
                    Expects((pMemory != nullptr) && (pPrev != nullptr));
                    return pMemory[nIndex] == pPrev[nIndex];
                });
                break;

            case ComparisonType::NotEqualTo:
                ProcessBlocks(srSource, [](gsl::index nIndex, const unsigned char* restrict pMemory, const unsigned char* restrict pPrev)
                {
                    Expects((pMemory != nullptr) && (pPrev != nullptr));
                    return pMemory[nIndex] != pPrev[nIndex];
                });
                break;

            default:
                ProcessBlocks(srSource, [nCompareType](gsl::index nIndex, const unsigned char* restrict pMemory, const unsigned char* restrict pPrev)
                {
                    Expects((pMemory != nullptr) && (pPrev != nullptr));
                    return CompareValues(pMemory[nIndex], pPrev[nIndex], nCompareType);
                });
                break;
        }
    }
    else if (m_nSize == MemSize::Nibble_Lower)
    {
        // special logic for nibbles
        ProcessBlocksNibbles(srSource, 0xFFFF, nCompareType);
    }
    else
    {
        // generic handling for 16-bit and 32-bit
        ProcessBlocks(srSource, [nCompareType, nSize = m_nSize](gsl::index nIndex, const unsigned char* restrict pMemory, const unsigned char* restrict pPrev)
        {
            Expects((pMemory != nullptr) && (pPrev != nullptr));
            const auto nValue = BuildValue(pMemory, nIndex, nSize);
            const auto nPrevValue = BuildValue(pPrev, nIndex, nSize);
            return CompareValues(nValue, nPrevValue, nCompareType);
        });
    }
}

size_t SearchResults::MatchingAddressCount() const noexcept
{
    if (!m_bUnfiltered)
        return m_vMatchingAddresses.size();

    const unsigned int nPadding = Padding(m_nSize);
    size_t nCount = 0;
    for (auto& block : m_vBlocks)
        nCount += gsl::narrow_cast<size_t>(block.GetSize()) - nPadding;

    if (m_nSize == MemSize::Nibble_Lower)
        nCount *= 2;

    return nCount;
}

void SearchResults::IterateMatchingAddresses(std::function<void(ra::ByteAddress)> pHandler) const
{
    if (!m_bUnfiltered)
    {
        for (const auto nAddress : m_vMatchingAddresses)
            pHandler(nAddress);
    }
    else if (m_nSize == MemSize::Nibble_Lower)
    {
        for (auto& block : m_vBlocks)
        {
            auto nAddress = block.GetAddress() << 1;
            for (unsigned int i = 0; i < block.GetSize(); ++i)
            {
                pHandler(nAddress++); // low nibble
                pHandler(nAddress++); // high nibble
            }
        }
    }
    else
    {
        const unsigned int nPadding = Padding(m_nSize);
        for (auto& block : m_vBlocks)
        {
            auto nAddress = block.GetAddress();
            for (unsigned int i = 0; i < block.GetSize() - nPadding; ++i)
                pHandler(nAddress++);
        }
    }
}

void SearchResults::ExcludeAddress(ra::ByteAddress nAddress)
{
    if (!m_bUnfiltered)
        m_vMatchingAddresses.erase(std::remove(m_vMatchingAddresses.begin(), m_vMatchingAddresses.end(), nAddress), m_vMatchingAddresses.end());
}

void SearchResults::ExcludeMatchingAddress(gsl::index nIndex)
{
    if (!m_bUnfiltered)
        m_vMatchingAddresses.erase(m_vMatchingAddresses.begin() + nIndex);
}

bool SearchResults::GetMatchingAddress(gsl::index nIndex, _Out_ SearchResults::Result& result) const
{
    result.nSize = m_nSize;

    if (m_vBlocks.empty())
        return false;

    unsigned int nPadding = 0;
    if (m_bUnfiltered)
    {
        if (m_nSize == MemSize::Nibble_Lower)
        {
            result.nAddress = m_vBlocks.front().GetAddress() + gsl::narrow_cast<ra::ByteAddress>(nIndex >> 1);
            if (nIndex & 1)
                result.nSize = MemSize::Nibble_Upper;
        }
        else
        {
            result.nAddress = m_vBlocks.front().GetAddress() + gsl::narrow_cast<ra::ByteAddress>(nIndex);
        }

        // in unfiltered mode, blocks are padded so we don't have to cross blocks to read multi-byte values
        nPadding = Padding(m_nSize);
    }
    else
    {
        if (ra::to_unsigned(nIndex) >= m_vMatchingAddresses.size())
            return false;

        result.nAddress = m_vMatchingAddresses.at(nIndex);

        if (m_nSize == MemSize::Nibble_Lower)
        {
            if (result.nAddress & 1)
                result.nSize = MemSize::Nibble_Upper;

            result.nAddress >>= 1;
        }
    }

    return GetValue(result.nAddress, result.nSize, result.nValue);
}

bool SearchResults::GetValue(ra::ByteAddress nAddress, MemSize nSize, _Out_ unsigned int& nValue) const
{
    if (m_vBlocks.empty())
    {
        nValue = 0;
        return false;
    }

    size_t nBlockIndex = 0;
    gsl::not_null<const MemBlock*> block{gsl::make_not_null(&m_vBlocks.at(nBlockIndex))};

    const unsigned int nPadding = (m_bUnfiltered) ? Padding(m_nSize) : 0;
    while (nAddress >= block->GetAddress() + block->GetSize() - nPadding)
    {
        if (++nBlockIndex == m_vBlocks.size())
        {
            nValue = 0;
            return false;
        }

        block = gsl::make_not_null(&m_vBlocks.at(nBlockIndex));
    }

    if (nAddress < block->GetAddress())
    {
        nValue = 0;
        return false;
    }

    nValue = BuildValue(block->GetBytes(), nAddress - block->GetAddress(), nSize);
    return true;
}

} // namespace services
} // namespace ra
