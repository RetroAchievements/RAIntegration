#include "SearchResults.h"

#include "RA_MemManager.h"
#include "RA_StringUtils.h"

#include "ra_utility.h"

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

void SearchResults::Initialize(unsigned int nAddress, unsigned int nBytes, MemSize nSize)
{
    if (nSize == MemSize::Nibble_Upper)
        nSize = MemSize::Nibble_Lower;

    m_nSize = nSize;
    m_bUnfiltered = true;

    if (nBytes + nAddress > g_MemManager.TotalBankSize())
        nBytes = g_MemManager.TotalBankSize() - nAddress;

    const unsigned int nPadding = Padding(nSize);
    nBytes -= nPadding;

    m_sSummary.reserve(64);
    m_sSummary.append("Cleared: (");
    m_sSummary.append(ra::Narrow(MEMSIZE_STR.at(ra::etoi(nSize))));
    m_sSummary.append(") mode. Aware of ");
    if (nSize == MemSize::Nibble_Lower)
        m_sSummary.append(std::to_string(nBytes * 2));
    else
        m_sSummary.append(std::to_string(nBytes));
    m_sSummary.append(" RAM locations.");

    while (nBytes > 0)
    {
        const auto nBlockSize = (nBytes > MAX_BLOCK_SIZE) ? MAX_BLOCK_SIZE : nBytes;
        auto& block = AddBlock(nAddress, nBlockSize + nPadding);
        g_MemManager.ActiveBankRAMRead(block.GetBytes(), block.GetAddress(), nBlockSize + nPadding);

        nAddress += nBlockSize;
        nBytes -= nBlockSize;
    }
}

SearchResults::MemBlock& SearchResults::AddBlock(unsigned int nAddress, unsigned int nSize)
{
    m_vBlocks.emplace_back(nAddress, nSize);
    return m_vBlocks.back();
}

_Success_(return)
_NODISCARD _CONSTANT_FN Compare(_In_ unsigned int nLeft,
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
_NODISCARD inline static constexpr auto GetValue(_In_ const unsigned char* const restrict pBuffer,
                                                 _In_ unsigned int nOffset,
                                                 _In_ MemSize nSize) noexcept
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

bool SearchResults::ContainsAddress(unsigned int nAddress) const
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

bool SearchResults::ContainsNibble(unsigned int nAddress) const
{
    if (!m_bUnfiltered)
        return std::binary_search(m_vMatchingAddresses.begin(), m_vMatchingAddresses.end(), nAddress);

    // an unfiltered collection implicitly contains both nibbles of an address
    return ContainsAddress(nAddress >> 1);
}

void SearchResults::AddMatches(unsigned int nAddressBase, const unsigned char* restrict pMemory, const std::vector<unsigned int>& vMatches)
{
    const unsigned int nBlockSize = vMatches.back() - vMatches.front() + Padding(m_nSize) + 1;
    MemBlock& block = AddBlock(nAddressBase + vMatches.front(), nBlockSize);
    memcpy(block.GetBytes(), pMemory + vMatches.front(), nBlockSize);

    for (auto nMatch : vMatches)
        m_vMatchingAddresses.push_back(nAddressBase + nMatch);
}

void SearchResults::ProcessBlocks(const SearchResults& srSource, std::function<bool(unsigned int nIndex, const unsigned char* restrict pMemory, const unsigned char* restrict pPrev)> testIndexFunction)
{
    std::vector<unsigned int> vMatches;
    std::vector<unsigned char> vMemory;
    const unsigned int nPadding = Padding(m_nSize);

    for (auto& block : srSource.m_vBlocks)
    {
        if (block.GetSize() > vMemory.capacity())
            vMemory.reserve(block.GetSize());

        unsigned char* pMemory = vMemory.data();
        const unsigned char* pPrev = block.GetBytes();

        g_MemManager.ActiveBankRAMRead(pMemory, block.GetAddress(), block.GetSize());

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

void SearchResults::AddMatchesNibbles(unsigned int nAddressBase, const unsigned char* restrict pMemory, const std::vector<unsigned int>& vMatches)
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

    for (auto& block : srSource.m_vBlocks)
    {
        if (block.GetSize() > vMemory.size())
            vMemory.resize(block.GetSize());

        g_MemManager.ActiveBankRAMRead(vMemory.data(), block.GetAddress(), block.GetSize());

        for (unsigned int i = 0; i < block.GetSize() - nPadding; ++i)
        {
            const unsigned int nValue1 = vMemory.at(i);
            unsigned int nValue2 = (nTestValue > 15) ? (block.GetByte(i) & 0x0F) : nTestValue;

            if (Compare(nValue1 & 0x0F, nValue2, nCompareType))
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

            if (Compare(nValue1 >> 4, nValue2, nCompareType))
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
                          [nTestValue, nCompareType](unsigned int nIndex, const unsigned char* restrict pMemory,
                                                     [[maybe_unused]] const unsigned char* restrict)
            {
                Expects(pMemory != nullptr);
                return Compare(pMemory[nIndex], nTestValue, nCompareType);
            });
            break;

        case MemSize::SixteenBit:
            ProcessBlocks(srSource, [nTestValue, nCompareType](unsigned int nIndex, const unsigned char* restrict pMemory, [[maybe_unused]] const unsigned char* restrict)
            {
                Expects(pMemory != nullptr);
                const unsigned int nValue = pMemory[nIndex] | (pMemory[nIndex + 1] << 8);
                return Compare(nValue, nTestValue, nCompareType);
            });
            break;

        case MemSize::ThirtyTwoBit:
            ProcessBlocks(srSource, [nTestValue, nCompareType](unsigned int nIndex, const unsigned char* restrict pMemory, [[maybe_unused]] const unsigned char* restrict)
            {
                Expects(pMemory != nullptr);
                const unsigned int nValue = pMemory[nIndex] | (pMemory[nIndex + 1] << 8) |
                    (pMemory[nIndex + 2] << 16) | (pMemory[nIndex + 3] << 24);
                return Compare(nValue, nTestValue, nCompareType);
            });
            break;
    }

    m_sSummary.reserve(64);
    m_sSummary.append("Filtering for ");
    m_sSummary.append(ComparisonString(nCompareType));
    m_sSummary.append(" ");
    m_sSummary.append(std::to_string(nTestValue));
    m_sSummary.append("...");
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
                ProcessBlocks(srSource, [](unsigned int nIndex, const unsigned char* restrict pMemory, const unsigned char* restrict pPrev)
                {
                    Expects((pMemory != nullptr) && (pPrev != nullptr));
                    return pMemory[nIndex] == pPrev[nIndex];
                });
                break;

            case ComparisonType::NotEqualTo:
                ProcessBlocks(srSource, [](unsigned int nIndex, const unsigned char* restrict pMemory, const unsigned char* restrict pPrev)
                {
                    Expects((pMemory != nullptr) && (pPrev != nullptr));
                    return pMemory[nIndex] != pPrev[nIndex];
                });
                break;

            default:
                ProcessBlocks(srSource, [nCompareType](unsigned int nIndex, const unsigned char* restrict pMemory, const unsigned char* restrict pPrev)
                {
                    Expects((pMemory != nullptr) && (pPrev != nullptr));
                    return Compare(pMemory[nIndex], pPrev[nIndex], nCompareType);
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
        ProcessBlocks(srSource, [nCompareType, nSize = m_nSize](unsigned int nIndex, const unsigned char* restrict pMemory, const unsigned char* restrict pPrev)
        {
            Expects((pMemory != nullptr) && (pPrev != nullptr));
            const auto nValue = GetValue(pMemory, nIndex, nSize);
            const auto nPrevValue = GetValue(pPrev, nIndex, nSize);
            return Compare(nValue, nPrevValue, nCompareType);
        });
    }

    m_sSummary.reserve(64);
    m_sSummary.append("Filtering for ");
    m_sSummary.append(ComparisonString(nCompareType));
    m_sSummary.append(" last known value...");
}

unsigned int SearchResults::MatchingAddressCount() noexcept
{
    if (!m_bUnfiltered)
        return m_vMatchingAddresses.size();

    const unsigned int nPadding = Padding(m_nSize);
    unsigned int nCount = 0;
    for (auto& block : m_vBlocks)
        nCount += block.GetSize() - nPadding;

    if (m_nSize == MemSize::Nibble_Lower)
        nCount *= 2;

    return nCount;
}

void SearchResults::ExcludeAddress(unsigned int nAddress)
{
    if (!m_bUnfiltered)
        m_vMatchingAddresses.erase(std::remove(m_vMatchingAddresses.begin(), m_vMatchingAddresses.end(), nAddress), m_vMatchingAddresses.end());
}

void SearchResults::ExcludeMatchingAddress(unsigned int nIndex)
{
    if (!m_bUnfiltered)
        m_vMatchingAddresses.erase(m_vMatchingAddresses.begin() + nIndex);
}

bool SearchResults::GetMatchingAddress(unsigned int nIndex, _Out_ SearchResults::Result& result)
{
    result.nSize = m_nSize;

    if (m_vBlocks.empty())
        return false;

    unsigned int nPadding = 0;
    if (m_bUnfiltered)
    {
        if (m_nSize == MemSize::Nibble_Lower)
        {
            result.nAddress = (nIndex >> 1) + m_vBlocks.front().GetAddress();
            if (nIndex & 1)
                result.nSize = MemSize::Nibble_Upper;
        }
        else
        {
            result.nAddress = nIndex + m_vBlocks.front().GetAddress();
        }

        // in unfiltered mode, blocks are padded so we don't have to cross blocks to read multi-byte values
        nPadding = Padding(m_nSize);
    }
    else
    {
        if (nIndex >= m_vMatchingAddresses.size())
            return false;

        result.nAddress = m_vMatchingAddresses.at(nIndex);

        if (m_nSize == MemSize::Nibble_Lower)
        {
            if (result.nAddress & 1)
                result.nSize = MemSize::Nibble_Upper;

            result.nAddress >>= 1;
        }
    }

    size_t nBlockIndex = 0;
    gsl::not_null<MemBlock*> block{gsl::make_not_null(&m_vBlocks.at(nBlockIndex))};
    while (result.nAddress >= block->GetAddress() + block->GetSize() - nPadding)
    {
        if (++nBlockIndex == m_vBlocks.size())
            return false;
        block = gsl::make_not_null(&m_vBlocks.at(nBlockIndex));
    }

    result.nValue = GetValue(block->GetBytes(), result.nAddress - block->GetAddress(), result.nSize);
    return true;
}

} // namespace services
} // namespace ra
