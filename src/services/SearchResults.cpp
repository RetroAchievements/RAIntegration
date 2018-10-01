#include "SearchResults.h"

#include "RA_MemManager.h"

namespace ra {
namespace services {

const unsigned int MAX_BLOCK_SIZE = 256 * 1024; // 256K

static unsigned int Padding(ComparisonVariableSize size)
{
    switch (size)
    {
        case ThirtyTwoBit:
            return 3;
        case SixteenBit:
            return 1;
        default:
            return 0;
    }
}

void SearchResults::Initialize(unsigned int nAddress, unsigned int nBytes, ComparisonVariableSize nSize)
{
    if (nSize == Nibble_Upper)
        nSize = Nibble_Lower;

    m_nSize = nSize;
    m_bUnfiltered = true;

    if (nBytes + nAddress > g_MemManager.TotalBankSize())
        nBytes = g_MemManager.TotalBankSize() - nAddress;

    unsigned int nPadding = Padding(nSize);
    nBytes -= nPadding;

    m_sSummary.reserve(64);
    m_sSummary.append("Cleared: (");
    m_sSummary.append(COMPARISONVARIABLESIZE_STR[nSize]);
    m_sSummary.append(") mode. Aware of ");
    if (nSize == Nibble_Lower)
        m_sSummary.append(std::to_string(nBytes * 2));
    else
        m_sSummary.append(std::to_string(nBytes));
    m_sSummary.append(" RAM locations.");

    while (nBytes > 0)
    {
        auto nBlockSize = (nBytes > MAX_BLOCK_SIZE) ? MAX_BLOCK_SIZE : nBytes;
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

static bool Compare(unsigned int nLeft, unsigned int nRight, ComparisonType nCompareType)
{
    switch (nCompareType)
    {
        case Equals:
            return nLeft == nRight;
        case LessThan:
            return nLeft < nRight;
        case LessThanOrEqual:
            return nLeft <= nRight;
        case GreaterThan:
            return nLeft > nRight;
        case GreaterThanOrEqual:
            return nLeft >= nRight;
        case NotEqualTo:
            return nLeft != nRight;
        default:
            return false;
    }
}

static const char* ComparisonString(ComparisonType nCompareType)
{
    switch (nCompareType)
    {
        case Equals:
            return "EQUAL";
        case LessThan:
            return "LESS THAN";
        case LessThanOrEqual:
            return "LESS THAN/EQUAL";
        case GreaterThan:
            return "GREATER THAN";
        case GreaterThanOrEqual:
            return "GREATER THAN/EQUAL";
        case NotEqualTo:
            return "NOT EQUAL";
        default:
            return "?";
    }
}

static unsigned int GetValue(const unsigned char* pBuffer, unsigned int nOffset, ComparisonVariableSize nSize)
{
    switch (nSize)
    {
        case EightBit:
            return pBuffer[nOffset];

        case SixteenBit:
            return pBuffer[nOffset] | (pBuffer[nOffset + 1] << 8);

        case ThirtyTwoBit:
            return pBuffer[nOffset] | (pBuffer[nOffset + 1] << 8) |
                (pBuffer[nOffset + 2] << 16) | (pBuffer[nOffset + 3] << 24);

        case Nibble_Upper:
            return pBuffer[nOffset] >> 4;

        case Nibble_Lower:
            return pBuffer[nOffset] & 0x0F;

        default:
            return 0;
    }
}

bool SearchResults::ContainsAddress(unsigned int nAddress) const
{
    if (!m_bUnfiltered)
    {
        if (m_nSize != Nibble_Lower)
            return std::binary_search(m_vMatchingAddresses.begin(), m_vMatchingAddresses.end(), nAddress);

        nAddress <<= 1;
        const auto iter = std::lower_bound(m_vMatchingAddresses.begin(), m_vMatchingAddresses.end(), nAddress);
        return (iter != m_vMatchingAddresses.end() && (*iter == nAddress || *iter == (nAddress | 1)));
    }

    unsigned int nPadding = Padding(m_nSize);
    for (const auto& block : m_vBlocks)
    {
        if (block.GetAddress() > nAddress)
            return false;
        if (nAddress < block.GetAddress() + block.GetSize() - nPadding)
            return true;
    }

    return false;
}

void SearchResults::AddMatches(unsigned int nAddressBase, const unsigned char pMemory[], const std::vector<unsigned int>& vMatches)
{
    unsigned int nBlockSize = vMatches.back() - vMatches.front() + Padding(m_nSize) + 1;
    MemBlock& block = AddBlock(nAddressBase + vMatches.front(), nBlockSize);
    memcpy(block.GetBytes(), pMemory + vMatches.front(), nBlockSize);

    for (auto nMatch : vMatches)
        m_vMatchingAddresses.push_back(nAddressBase + nMatch);
}

void SearchResults::ProcessBlocks(const SearchResults& srSource, std::function<bool(unsigned int nIndex, const unsigned char pMemory[], const unsigned char pPrev[])> testIndexFunction)
{
    std::vector<unsigned int> vMatches;
    std::vector<unsigned char> vMemory;
    unsigned int nPadding = Padding(m_nSize);

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

            unsigned int nAddress = block.GetAddress() + i;
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

void SearchResults::AddMatchesNibbles(unsigned int nAddressBase, const unsigned char pMemory[], const std::vector<unsigned int>& vMatches)
{
    unsigned int nBlockSize = (vMatches.back() >> 1) - (vMatches.front() >> 1) + Padding(m_nSize) + 1;
    auto& block = AddBlock((nAddressBase + vMatches.front()) >> 1, nBlockSize);
    memcpy(block.GetBytes(), pMemory + (vMatches.front() >> 1), nBlockSize);

    for (auto nMatch : vMatches)
        m_vMatchingAddresses.push_back(nAddressBase + nMatch);
}

void SearchResults::ProcessBlocksNibbles(const SearchResults& srSource, unsigned int nTestValue, ComparisonType nCompareType)
{
    std::vector<unsigned int> vMatches;
    std::vector<unsigned char> vMemory;
    unsigned int nPadding = Padding(m_nSize);

    for (auto& block : srSource.m_vBlocks)
    {
        if (block.GetSize() > vMemory.capacity())
            vMemory.reserve(block.GetSize());

        unsigned char* pMemory = vMemory.data();
        const unsigned char* pPrev = block.GetBytes();

        g_MemManager.ActiveBankRAMRead(pMemory, block.GetAddress(), block.GetSize());

        for (unsigned int i = 0; i < block.GetSize() - nPadding; ++i)
        {
            unsigned int nValue1 = pMemory[i];
            unsigned int nValue2 = (nTestValue > 15) ? (pPrev[i] & 0x0F) : nTestValue;

            if (Compare(nValue1 & 0x0F, nValue2, nCompareType))
            {
                unsigned int nAddress = (block.GetAddress() + i) << 1;
                if (srSource.ContainsAddress(nAddress >> 1))
                {
                    if (!vMatches.empty() && (i - (vMatches.back() >> 1)) > 16)
                    {
                        AddMatchesNibbles(block.GetAddress() << 1, pMemory, vMatches);
                        vMatches.clear();
                    }

                    vMatches.push_back(i << 1);
                }
            }

            if (nTestValue > 15)
                nValue2 = pPrev[i] >> 4;

            if (Compare(nValue1 >> 4, nValue2, nCompareType))
            {
                unsigned int nAddress = ((block.GetAddress() + i) << 1) | 1;
                if (srSource.ContainsAddress(nAddress >> 1))
                {
                    if (!vMatches.empty() && (i - (vMatches.back() >> 1)) > 16)
                    {
                        AddMatchesNibbles(block.GetAddress() << 1, pMemory, vMatches);
                        vMatches.clear();
                    }

                    vMatches.push_back((i << 1) | 1);
                }
            }
        }

        if (!vMatches.empty())
        {
            AddMatchesNibbles(block.GetAddress() << 1, pMemory, vMatches);
            vMatches.clear();
        }
    }
}


void SearchResults::Initialize(const SearchResults& srSource, ComparisonType nCompareType, unsigned int nTestValue)
{
    m_nSize = srSource.m_nSize;

    switch (m_nSize)
    {
        case Nibble_Lower:
            ProcessBlocksNibbles(srSource, nTestValue & 0x0F, nCompareType);
            break;

        case EightBit:
            ProcessBlocks(srSource, [nTestValue, nCompareType](unsigned int nIndex, const unsigned char pMemory[], [[maybe_unused]] const unsigned char[])
            {
                return Compare(pMemory[nIndex], nTestValue, nCompareType);
            });
            break;

        case SixteenBit:
            ProcessBlocks(srSource, [nTestValue, nCompareType](unsigned int nIndex, const unsigned char pMemory[], [[maybe_unused]] const unsigned char[])
            {
                unsigned int nValue = pMemory[nIndex] | (pMemory[nIndex + 1] << 8);
                return Compare(nValue, nTestValue, nCompareType);
            });
            break;

        case ThirtyTwoBit:
            ProcessBlocks(srSource, [nTestValue, nCompareType](unsigned int nIndex, const unsigned char pMemory[], [[maybe_unused]] const unsigned char[])
            {
                unsigned int nValue = pMemory[nIndex] | (pMemory[nIndex + 1] << 8) |
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

void SearchResults::Initialize(const SearchResults& srSource, ComparisonType nCompareType)
{
    m_nSize = srSource.m_nSize;

    if (m_nSize == EightBit)
    {
        // efficient comparisons for 8-bit
        switch (nCompareType)
        {
            case Equals:
                ProcessBlocks(srSource, [](unsigned int nIndex, const unsigned char pMemory[], const unsigned char pPrev[])
                {
                    return pMemory[nIndex] == pPrev[nIndex];
                });
                break;

            case NotEqualTo:
                ProcessBlocks(srSource, [](unsigned int nIndex, const unsigned char pMemory[], const unsigned char pPrev[])
                {
                    return pMemory[nIndex] != pPrev[nIndex];
                });
                break;

            default:
                ProcessBlocks(srSource, [nCompareType](unsigned int nIndex, const unsigned char pMemory[], const unsigned char pPrev[])
                {
                    return Compare(pMemory[nIndex], pPrev[nIndex], nCompareType);
                });
                break;
        }
    }
    else if (m_nSize == Nibble_Lower)
    {
        // special logic for nibbles
        ProcessBlocksNibbles(srSource, 0xFFFF, nCompareType);
    }
    else
    {
        // generic handling for 16-bit and 32-bit
        ProcessBlocks(srSource, [nCompareType, nSize = m_nSize](unsigned int nIndex, const unsigned char pMemory[], const unsigned char pPrev[])
        {
            auto nValue = GetValue(pMemory, nIndex, nSize);
            auto nPrevValue = GetValue(pPrev, nIndex, nSize);
            return Compare(nValue, nPrevValue, nCompareType);
        });
    }

    m_sSummary.reserve(64);
    m_sSummary.append("Filtering for ");
    m_sSummary.append(ComparisonString(nCompareType));
    m_sSummary.append(" last known value...");
}

unsigned int SearchResults::MatchingAddressCount()
{
    if (!m_bUnfiltered)
        return m_vMatchingAddresses.size();

    unsigned int nPadding = Padding(m_nSize);
    unsigned int nCount = 0;
    for (auto& block : m_vBlocks)
        nCount += block.GetSize() - nPadding;

    if (m_nSize == Nibble_Lower)
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
        if (m_nSize == Nibble_Lower)
        {
            result.nAddress = (nIndex >> 1) + m_vBlocks[0].GetAddress();
            if (nIndex & 1)
                result.nSize = Nibble_Upper;
        }
        else
        {
            result.nAddress = nIndex + m_vBlocks[0].GetAddress();
        }

        // in unfiltered mode, blocks are padded so we don't have to cross blocks to read multi-byte values
        nPadding = Padding(m_nSize);
    }
    else
    {
        if (nIndex >= m_vMatchingAddresses.size())
            return false;

        result.nAddress = m_vMatchingAddresses[nIndex];

        if (m_nSize == Nibble_Lower)
        {
            if (result.nAddress & 1)
                result.nSize = Nibble_Upper;

            result.nAddress >>= 1;
        }
    }

    size_t nBlockIndex = 0;

    MemBlock* block = &m_vBlocks[nBlockIndex];
    while (result.nAddress >= block->GetAddress() + block->GetSize() - nPadding)
    {
        if (++nBlockIndex == m_vBlocks.size())
            return false;

        block = &m_vBlocks[nBlockIndex];
    }

    result.nValue = GetValue(block->GetBytes(), result.nAddress - block->GetAddress(), result.nSize);
    return true;
}

} // namespace services
} // namespace ra
