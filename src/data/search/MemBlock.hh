#ifndef SEARCH_MEMBLOCK_H
#define SEARCH_MEMBLOCK_H
#pragma once

#include "data\Types.hh"

namespace ra {
namespace data {
namespace search {

class MemBlock
{
public:
    explicit MemBlock(_In_ uint32_t nAddress, _In_ uint32_t nSize, _In_ uint32_t nMaxAddresses) noexcept :
        m_nFirstAddress(nAddress),
        m_nBytesSize(nSize),
        m_nMatchingAddresses(nMaxAddresses),
        m_nMaxAddresses(nMaxAddresses)
    {
        if (nSize > sizeof(m_vBytes))
            m_pBytes = new (std::nothrow) uint8_t[(nSize + 3) & ~3];
    }

    MemBlock(const MemBlock& other) noexcept :
        MemBlock(other.m_nFirstAddress, other.m_nBytesSize, other.m_nMaxAddresses)
    {
        if (m_nBytesSize > sizeof(m_vBytes))
            std::memcpy(m_pBytes, other.m_pBytes, m_nBytesSize);
        else
            std::memcpy(m_vBytes, other.m_vBytes, sizeof(m_vBytes));

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

    MemBlock& operator=(const MemBlock&) noexcept = delete;

    MemBlock(MemBlock&& other) noexcept :
        m_nFirstAddress(other.m_nFirstAddress),
        m_nBytesSize(other.m_nBytesSize),
        m_nMatchingAddresses(other.m_nMatchingAddresses),
        m_nMaxAddresses(other.m_nMaxAddresses)
    {
        if (other.m_nBytesSize > sizeof(m_vBytes))
        {
            m_pBytes = other.m_pBytes;
            other.m_pBytes = nullptr;
            other.m_nBytesSize = 0;
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

    MemBlock& operator=(MemBlock&&) noexcept = delete;

    ~MemBlock() noexcept
    {
        if (m_nBytesSize > sizeof(m_vBytes))
            delete[] m_pBytes;
        if (!AreAllAddressesMatching() && (m_nMatchingAddresses + 7) / 8 > sizeof(m_vAddresses))
            delete[] m_pAddresses;
    }

    uint8_t* GetBytes() noexcept { return (m_nBytesSize > sizeof(m_vBytes)) ? m_pBytes : &m_vBytes[0]; }
    const uint8_t* GetBytes() const noexcept { return (m_nBytesSize > sizeof(m_vBytes)) ? m_pBytes : &m_vBytes[0]; }

    void SetFirstAddress(ra::ByteAddress nAddress) noexcept { m_nFirstAddress = nAddress; }
    ra::ByteAddress GetFirstAddress() const noexcept { return m_nFirstAddress; }
    uint32_t GetBytesSize() const noexcept { return m_nBytesSize; }
    void SetMaxAddresses(uint32_t nMaxAddresses) noexcept { m_nMaxAddresses = nMaxAddresses; }
    uint32_t GetMaxAddresses() const noexcept { return m_nMaxAddresses; }

    bool ContainsAddress(ra::ByteAddress nAddress) const noexcept;

    void SetMatchingAddresses(std::vector<ra::ByteAddress>& vAddresses, gsl::index nFirstIndex, gsl::index nLastIndex);
    void CopyMatchingAddresses(const MemBlock& pSource);
    void ExcludeMatchingAddress(ra::ByteAddress nAddress);
    bool ContainsMatchingAddress(ra::ByteAddress nAddress) const;

    void SetMatchingAddressCount(uint32_t nCount) noexcept { m_nMatchingAddresses = nCount; }
    uint32_t GetMatchingAddressCount() const noexcept { return m_nMatchingAddresses; }
    ra::ByteAddress GetMatchingAddress(gsl::index nIndex) const noexcept;
    bool AreAllAddressesMatching() const noexcept { return m_nMatchingAddresses == m_nMaxAddresses; }

    const uint8_t* GetMatchingAddressPointer() const noexcept
    {
        if (AreAllAddressesMatching())
            return nullptr;

        const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
        return (nAddressesSize > sizeof(m_vAddresses)) ? m_pAddresses : &m_vAddresses[0];
    }

    bool HasMatchingAddress(const uint8_t* pMatchingAddresses, ra::ByteAddress nAddress) const noexcept
    {
        if (!pMatchingAddresses)
            return true;

        const auto nIndex = nAddress - m_nFirstAddress;
        if (nIndex >= m_nMaxAddresses)
            return false;

        const auto nBit = 1 << (nIndex & 7);
        return (pMatchingAddresses[nIndex >> 3] & nBit);
    }

    struct AllocatedMemory
    {
        uint32_t nReferenceCount;
        uint32_t nHash;
        uint8_t pBuffer[16];
    };

private:
    uint8_t* AllocateMatchingAddresses() noexcept;

    union // 8 bytes
    {
        uint8_t m_vBytes[8]{};
        uint8_t* m_pBytes;
    };

    union // 8 bytes
    {
        uint8_t m_vAddresses[8]{};
        uint8_t* m_pAddresses;
    };

    uint32_t m_nBytesSize;           // 4 bytes
    ra::ByteAddress m_nFirstAddress; // 4 bytes
    uint32_t m_nMatchingAddresses;   // 4 bytes
    uint32_t m_nMaxAddresses;        // 4 bytes
};
static_assert(sizeof(MemBlock) <= 32, "sizeof(MemBlock) is incorrect");

} // namespace search
} // namespace services
} // namespace ra

#endif /* !SEARCH_MEMBLOCK_H */
