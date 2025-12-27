#ifndef SEARCH_MEMBLOCK_H
#define SEARCH_MEMBLOCK_H
#pragma once

#include "data\Memory.hh"

namespace ra {
namespace data {
namespace search {

class MemBlock
{
private:
    struct AllocatedMemory
    {
        uint32_t nReferenceCount;
        uint32_t nHash;
        uint8_t pBytes[16];
    };

public:
    explicit MemBlock(_In_ uint32_t nAddress, _In_ uint32_t nSize, _In_ uint32_t nMaxAddresses) :
        m_nFirstAddress(nAddress),
        m_nBytesSize(nSize),
        m_nMatchingAddresses(nMaxAddresses),
        m_nMaxAddresses(nMaxAddresses)
    {
        if (IsBytesAllocated())
        {
            const auto nAllocSize = sizeof(AllocatedMemory) - sizeof(m_pAllocatedMemory->pBytes) +
                                    ((gsl::narrow_cast<size_t>(nSize) + 3) & ~3);
            m_pAllocatedMemory = static_cast<AllocatedMemory*>(malloc(nAllocSize));
            Expects(m_pAllocatedMemory != nullptr);
            m_pAllocatedMemory->nReferenceCount = 1;
            m_pAllocatedMemory->nHash = 0;
        }
    }

    MemBlock(const MemBlock& other) noexcept;
    MemBlock& operator=(const MemBlock&) noexcept = delete;
    MemBlock(MemBlock&& other) noexcept;
    MemBlock& operator=(MemBlock&&) noexcept = delete;

    ~MemBlock() noexcept;

    uint8_t* GetBytes() noexcept
    {
        return IsBytesAllocated() ? &m_pAllocatedMemory->pBytes[0] : &m_vBytes[0];
    }
    const uint8_t* GetBytes() const noexcept
    {
        return IsBytesAllocated() ? &m_pAllocatedMemory->pBytes[0] : &m_vBytes[0];
    }

    void SetFirstAddress(ra::data::ByteAddress nAddress) noexcept { m_nFirstAddress = nAddress; }
    ra::data::ByteAddress GetFirstAddress() const noexcept { return m_nFirstAddress; }
    uint32_t GetBytesSize() const noexcept { return m_nBytesSize & 0x00FFFFFF; }
    bool IsBytesAllocated() const noexcept { return GetBytesSize() > sizeof(m_vBytes); }
    void SetMaxAddresses(uint32_t nMaxAddresses) noexcept { m_nMaxAddresses = nMaxAddresses; }
    uint32_t GetMaxAddresses() const noexcept { return m_nMaxAddresses; }

    bool ContainsAddress(ra::data::ByteAddress nAddress) const noexcept;

    void SetMatchingAddresses(std::vector<ra::data::ByteAddress>& vAddresses, gsl::index nFirstIndex, gsl::index nLastIndex);
    void CopyMatchingAddresses(const MemBlock& pSource);
    void ExcludeMatchingAddress(ra::data::ByteAddress nAddress);
    bool ContainsMatchingAddress(ra::data::ByteAddress nAddress) const;

    void SetMatchingAddressCount(uint32_t nCount) noexcept { m_nMatchingAddresses = nCount; }
    uint32_t GetMatchingAddressCount() const noexcept { return m_nMatchingAddresses; }
    ra::data::ByteAddress GetMatchingAddress(gsl::index nIndex) const noexcept;
    bool AreAllAddressesMatching() const noexcept { return m_nMatchingAddresses == m_nMaxAddresses; }

    const uint8_t* GetMatchingAddressPointer() const noexcept
    {
        if (AreAllAddressesMatching())
            return nullptr;

        const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
        return (nAddressesSize > sizeof(m_vAddresses)) ? m_pAddresses : &m_vAddresses[0];
    }

    bool HasMatchingAddress(const uint8_t* pMatchingAddresses, ra::data::ByteAddress nAddress) const noexcept
    {
        if (!pMatchingAddresses)
            return true;

        const auto nIndex = nAddress - m_nFirstAddress;
        if (nIndex >= m_nMaxAddresses)
            return false;

        const auto nBit = 1 << (nIndex & 7);
        return (pMatchingAddresses[nIndex >> 3] & nBit);
    }

    void ShareMemory(const std::vector<MemBlock>& vBlocks, uint32_t nHash) noexcept;
    void OptimizeMemory(const std::vector<ra::data::search::MemBlock>& vBlocks) noexcept;

private:
    uint8_t* AllocateMatchingAddresses() noexcept;
    void SetRepeat(uint32_t nCount, uint32_t nValue) noexcept;

    ra::data::ByteAddress m_nFirstAddress; // 4 bytes
    uint32_t m_nBytesSize;           // 4 bytes

    union                            // 8 bytes
    {
        uint8_t m_vBytes[8]{};
        AllocatedMemory* m_pAllocatedMemory;
    };

    union // 8 bytes
    {
        uint8_t m_vAddresses[8]{};
        uint8_t* m_pAddresses;
    };

    uint32_t m_nMatchingAddresses;   // 4 bytes
    uint32_t m_nMaxAddresses;        // 4 bytes
};
static_assert(sizeof(MemBlock) <= 32, "sizeof(MemBlock) is incorrect");

} // namespace search
} // namespace services
} // namespace ra

#endif /* !SEARCH_MEMBLOCK_H */
