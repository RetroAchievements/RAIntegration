#include "MemBlock.hh"

#include "ra_fwd.h"

namespace ra {
namespace data {
namespace search {

MemBlock::MemBlock(const MemBlock& other) noexcept :
    m_nFirstAddress(other.m_nFirstAddress),
    m_nBytesSize(other.m_nBytesSize),
    m_nMatchingAddresses(other.m_nMatchingAddresses),
    m_nMaxAddresses(other.m_nMaxAddresses)
{
    if (IsBytesAllocated())
    {
        other.m_pAllocatedMemory->nReferenceCount++;
        m_pAllocatedMemory = other.m_pAllocatedMemory;
    }
    else
    {
        std::memcpy(m_vBytes, other.m_vBytes, sizeof(m_vBytes));
    }

    m_nMatchingAddresses = other.m_nMatchingAddresses;
    if (!AreAllAddressesMatching())
    {
        if ((m_nMatchingAddresses + 7) / 8 > sizeof(m_vAddresses))
        {
            auto* pAddresses = AllocateMatchingAddresses();
            if (pAddresses != nullptr)
                std::memcpy(pAddresses, other.m_pAddresses, (m_nMatchingAddresses + 7) / 8);
        }
        else
        {
            std::memcpy(m_vAddresses, other.m_vAddresses, sizeof(m_vAddresses));
        }
    }
}

MemBlock::MemBlock(MemBlock&& other) noexcept :
    m_nFirstAddress(other.m_nFirstAddress),
    m_nBytesSize(other.m_nBytesSize),
    m_nMatchingAddresses(other.m_nMatchingAddresses),
    m_nMaxAddresses(other.m_nMaxAddresses)
{
    if (other.IsBytesAllocated())
    {
        other.m_nBytesSize = 0;
        m_pAllocatedMemory = other.m_pAllocatedMemory;
        other.m_pAllocatedMemory = nullptr;
    }
    else
    {
        std::memcpy(m_vBytes, other.m_vBytes, sizeof(m_vBytes));
    }

    if (!AreAllAddressesMatching())
    {
        if ((m_nMatchingAddresses + 7) / 8 > sizeof(m_vAddresses))
        {
            m_pAddresses = other.m_pAddresses;
            other.m_pAddresses = nullptr;
            other.m_nMatchingAddresses = 0;
        }
        else
        {
            std::memcpy(m_vAddresses, other.m_vAddresses, sizeof(m_vAddresses));
        }
    }
}

MemBlock::~MemBlock() noexcept
{
    if (IsBytesAllocated())
    {
        m_nBytesSize = 0;
        if (--m_pAllocatedMemory->nReferenceCount == 0)
            free(m_pAllocatedMemory);
    }

    if (!AreAllAddressesMatching() && (m_nMatchingAddresses + 7) / 8 > sizeof(m_vAddresses))
        delete[] m_pAddresses;
}

void MemBlock::ShareMemory(const std::vector<MemBlock>& vBlocks, uint32_t nHash) noexcept
{
    if (IsBytesAllocated())
    {
        for (const auto& pBlock : vBlocks)
        {
            if (pBlock.m_nBytesSize == m_nBytesSize &&
                &pBlock != this &&
                pBlock.m_pAllocatedMemory->nHash == nHash)
            {
                if (memcmp(pBlock.m_pAllocatedMemory->pBytes, m_pAllocatedMemory->pBytes, m_nBytesSize) == 0)
                {
                    pBlock.m_pAllocatedMemory->nReferenceCount++;
                    auto* pAllocatedMemory = m_pAllocatedMemory;
                    m_pAllocatedMemory = pBlock.m_pAllocatedMemory;

                    if (--pAllocatedMemory->nReferenceCount == 0)
                        free(pAllocatedMemory);

                    return;
                }
            }
        }

        m_pAllocatedMemory->nHash = nHash;
    }
}

void MemBlock::SetRepeat(uint32_t nCount, uint32_t nValue) noexcept
{
    m_nBytesSize = (nCount * sizeof(uint32_t)) | 0x01000000;
    memcpy(m_vBytes, &nValue, 4);
    memcpy(&m_vBytes[4], &nValue, 4);
}

void MemBlock::OptimizeMemory(const std::vector<ra::data::search::MemBlock>& vBlocks) noexcept
{
    if (!IsBytesAllocated())
        return;

    const auto* pBytes = GetBytes();
    if (!pBytes)
        return;

    GSL_SUPPRESS_TYPE1
    const auto* pStart = reinterpret_cast<const uint32_t*>(pBytes);
    const auto* pScan = pStart;
    if (!pScan)
        return;

    GSL_SUPPRESS_TYPE1
    const auto* pStop = reinterpret_cast<const uint32_t*>(pBytes + GetBytesSize());
    //const auto* pUniqueStart = pScan;
    uint32_t nHash = *pScan++;
    uint32_t nLast = nHash;
    uint32_t nRepeat = 1;
    //bool bWasSplit = false;
    //uint32_t nAddress = GetFirstAddress();

    while (pScan < pStop)
    {
        const uint32_t nScan = *pScan++;
        if (nScan == nLast)
        {
            nRepeat++;
        }
        else
        {
            //if (nRepeat >= 64 / 4) // 64 bytes of repeated data
            //{
            //    const auto* pRepeatStart = pScan - nRepeat - 1;
            //    const auto nUniqueSize = (pRepeatStart - pUniqueStart) * sizeof(uint32_t);
            //    if (nUniqueSize)
            //    {
            //        auto& pUniqueBlock = vBlocks.emplace_back(nAddress, nUniqueSize, nUniqueSize);
            //        memcpy(pUniqueBlock.GetBytes(), pUniqueStart, nUniqueSize);
            //        pUniqueBlock.ShareMemory(vBlocks, nHash);
            //        nAddress += nUniqueSize;
            //    }

            //    auto& pRepeatBlock = vBlocks.emplace_back(nAddress, 0, 0);
            //    pRepeatBlock.SetRepeat(nRepeat, nLast);
            //    nAddress += nRepeat * sizeof(uint32_t);

            //    pUniqueStart = pScan - 1;
            //    bWasSplit = true;
            //    nHash = 0;
            //}

            // branchless version of "if (nRepeat & 1) nHash ^= nScan"
            nHash ^= (nScan * (nRepeat & 1));

            nLast = nScan;
            nRepeat = 1;
        }
    }

    //if (nRepeat >= 64 / 4) // 64 bytes of repeated data
    //{
    //    const auto* pRepeatStart = pScan - nRepeat - 1;
    //    const auto nUniqueSize = (pRepeatStart - pUniqueStart) * sizeof(uint32_t);
    //    if (nUniqueSize)
    //    {
    //        auto& pUniqueBlock = vBlocks.emplace_back(nAddress, nUniqueSize, nUniqueSize);
    //        memcpy(pUniqueBlock.GetBytes(), pUniqueStart, nUniqueSize);
    //        pUniqueBlock.ShareMemory(vBlocks, nHash);
    //        nAddress += nUniqueSize;
    //    }

    //    auto& pRepeatBlock = vBlocks.emplace_back(nAddress, 0, 0);
    //    pRepeatBlock.SetRepeat(nRepeat, nLast);
    //    nAddress += nRepeat * sizeof(uint32_t);

    //    pUniqueStart = pScan;
    //}

    //if (bWasSplit)
    //{
    //    // copy last block
    //    const auto nUniqueSize = (pScan - pUniqueStart) * sizeof(uint32_t);
    //    if (nUniqueSize)
    //    {
    //        auto& pUniqueBlock = vBlocks.emplace_back(nAddress, nUniqueSize, nUniqueSize);
    //        memcpy(pUniqueBlock.GetBytes(), pUniqueStart, nUniqueSize);
    //        pUniqueBlock.ShareMemory(vBlocks, nHash);
    //    }

    //    vBlocks.erase(this);
    //    return;
    //}

    ShareMemory(vBlocks, nHash);
}

uint8_t* MemBlock::AllocateMatchingAddresses() noexcept
{
    const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
    if (nAddressesSize > sizeof(m_vAddresses))
    {
        if (m_pAddresses == nullptr)
            m_pAddresses = new (std::nothrow) uint8_t[nAddressesSize];

        return m_pAddresses;
    }

    return &m_vAddresses[0];
}

void MemBlock::SetMatchingAddresses(std::vector<ra::ByteAddress>& vAddresses, gsl::index nFirstIndex, gsl::index nLastIndex)
{
    m_nMatchingAddresses = gsl::narrow_cast<unsigned int>(nLastIndex - nFirstIndex) + 1;

    if (m_nMatchingAddresses != m_nMaxAddresses)
    {
        const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
        unsigned char* pAddresses = AllocateMatchingAddresses();
        Expects(pAddresses != nullptr);

        memset(pAddresses, 0, nAddressesSize);
        for (gsl::index nIndex = nFirstIndex; nIndex <= nLastIndex; ++nIndex)
        {
            const auto nOffset = vAddresses.at(nIndex) - m_nFirstAddress;
            const auto nBit = 1 << (nOffset & 7);
            pAddresses[nOffset >> 3] |= nBit;
        }
    }
}

void MemBlock::CopyMatchingAddresses(const MemBlock& pSource)
{
    Expects(pSource.m_nMaxAddresses == m_nMaxAddresses);
    if (pSource.AreAllAddressesMatching())
    {
        m_nMatchingAddresses = m_nMaxAddresses;
    }
    else
    {
        const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
        unsigned char* pAddresses = AllocateMatchingAddresses();
        memcpy(pAddresses, pSource.GetMatchingAddressPointer(), nAddressesSize);
        m_nMatchingAddresses = pSource.m_nMatchingAddresses;
    }
}

void MemBlock::ExcludeMatchingAddress(ra::ByteAddress nAddress)
{
    const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
    unsigned char* pAddresses = nullptr;
    const auto nIndex = nAddress - m_nFirstAddress;
    const auto nBit = 1 << (nIndex & 7);

    if (AreAllAddressesMatching())
    {
        pAddresses = AllocateMatchingAddresses();
        Expects(pAddresses != nullptr);
        memset(pAddresses, 0xFF, nAddressesSize);
    }
    else
    {
        pAddresses = (nAddressesSize > sizeof(m_vAddresses)) ? m_pAddresses : &m_vAddresses[0];
        Expects(pAddresses != nullptr);
        if (!(pAddresses[nIndex >> 3] & nBit))
            return;
    }

    pAddresses[nIndex >> 3] &= ~nBit;
    --m_nMatchingAddresses;
}

bool MemBlock::ContainsAddress(ra::ByteAddress nAddress) const noexcept
{
    if (nAddress < m_nFirstAddress)
        return false;

    nAddress -= m_nFirstAddress;
    return (nAddress < m_nMaxAddresses);
}

bool MemBlock::ContainsMatchingAddress(ra::ByteAddress nAddress) const
{
    if (nAddress < m_nFirstAddress)
        return false;

    const auto nIndex = nAddress - m_nFirstAddress;
    if (nIndex >= m_nMaxAddresses)
        return false;

    if (AreAllAddressesMatching())
        return true;

    const uint8_t* pAddresses = GetMatchingAddressPointer();
    Expects(pAddresses != nullptr);
    const auto nBit = 1 << (nIndex & 7);
    return (pAddresses[nIndex >> 3] & nBit);
}

ra::ByteAddress MemBlock::GetMatchingAddress(gsl::index nIndex) const noexcept
{
    if (AreAllAddressesMatching())
        return m_nFirstAddress + gsl::narrow_cast<ra::ByteAddress>(nIndex);

    const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
    const uint8_t* pAddresses = (nAddressesSize > sizeof(m_vAddresses)) ? m_pAddresses : &m_vAddresses[0];
    ra::ByteAddress nAddress = m_nFirstAddress;
    const ra::ByteAddress nStop = m_nFirstAddress + m_nMaxAddresses;
    uint8_t nMask = 0x01;

    if (pAddresses != nullptr)
    {
        do
        {
            if (*pAddresses & nMask)
            {
                if (nIndex-- == 0)
                    return nAddress;
            }

            if (nMask == 0x80)
            {
                nMask = 0x01;
                pAddresses++;

                while (!*pAddresses)
                {
                    nAddress += 8;
                    if (nAddress >= nStop)
                        break;

                    pAddresses++;
                }
            }
            else
            {
                nMask <<= 1;
            }

            nAddress++;
        } while (nAddress < nStop);
    }

    return 0;
}

} // namespace search
} // namespace services
} // namespace ra
