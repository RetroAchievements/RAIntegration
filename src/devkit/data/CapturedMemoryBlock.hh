#ifndef RA_DATA_CAPTUREDMEMORYBLOCK_HH
#define RA_DATA_CAPTUREDMEMORYBLOCK_HH
#pragma once

#include "Memory.hh"

#include "util/GSL.hh"

namespace ra {
namespace data {

class CapturedMemoryBlock
{
private:
    struct AllocatedMemory
    {
        uint32_t nReferenceCount;
        uint32_t nHash;
        uint8_t pBytes[16]; // this will actually be sized by the allocation code.
    };

public:
    explicit CapturedMemoryBlock(uint32_t nAddress, uint32_t nSize, uint32_t nMaxAddresses) :
        m_nFirstAddress(nAddress),
        m_nBytesSize(nSize),
        m_nMatchingAddressCount(nMaxAddresses),
        m_nAddressCount(nMaxAddresses)
    {
        if (IsBytesAllocated()) // not actually allocated yet, but about to be.
        {
            const auto nAllocSize = sizeof(AllocatedMemory) - sizeof(m_pAllocatedMemory->pBytes) +
                                    ((gsl::narrow_cast<size_t>(nSize) + 3) & ~3);
            m_pAllocatedMemory = static_cast<AllocatedMemory*>(malloc(nAllocSize));
            Expects(m_pAllocatedMemory != nullptr);
            m_pAllocatedMemory->nReferenceCount = 1;
            m_pAllocatedMemory->nHash = 0;
        }
    }

    ~CapturedMemoryBlock() noexcept;

    CapturedMemoryBlock(const CapturedMemoryBlock& other) noexcept;
    CapturedMemoryBlock& operator=(const CapturedMemoryBlock&) noexcept = delete;
    CapturedMemoryBlock(CapturedMemoryBlock&& other) noexcept;
    CapturedMemoryBlock& operator=(CapturedMemoryBlock&&) noexcept = delete;

    /// <summary>
    /// Gets the raw bytes that were captured.
    /// </summary>
    uint8_t* GetBytes() noexcept
    {
        return IsBytesAllocated() ? &m_pAllocatedMemory->pBytes[0] : &m_vBytes[0];
    }

    /// <summary>
    /// Gets the raw bytes that were captured.
    /// </summary>
    const uint8_t* GetBytes() const noexcept
    {
        return IsBytesAllocated() ? &m_pAllocatedMemory->pBytes[0] : &m_vBytes[0];
    }

    /// <summary>
    /// Gets the number of captured bytes.
    /// </summary>
    uint32_t GetBytesSize() const noexcept { return m_nBytesSize; }

    /// <summary>
    /// Gets the first address of the captured memory.
    /// </summary>
    /// <remarks>
    /// May be virtualized as addresses represent indices into the captured memory.
    /// <remarks>
    ByteAddress GetFirstAddress() const noexcept { return m_nFirstAddress; }

    /// <summary>
    /// Sets the first address of the captured memory.
    /// </summary>
    void SetFirstAddress(ByteAddress nAddress) noexcept { m_nFirstAddress = nAddress; }

    /// <summary>
    /// Gets the number of addresses associated to the captured memory.
    /// </summary>
    /// <remarks>
    /// This is specifed by the capturing code to indicate how many whole records are availble
    /// in the captured memory depending on the captured size. For example, 75 bytes of memory
    /// holds 18 whole 32-bit records, so the caller would set this to 18.
    /// <remarks>
    uint32_t GetAddressCount() const noexcept { return m_nAddressCount; }

    /// <summary>
    /// Sets the number of addresses associated to the captured memory.
    /// </summary>
    void SetAddressCount(uint32_t nAddressCount) noexcept
    { 
        m_nAddressCount = nAddressCount; 
        m_nMatchingAddressCount = nAddressCount;
    }

    /// <summary>
    /// Determines whether the specified address is valid within this CapturedMemoryBlock.
    /// </summary>
    bool ContainsAddress(ByteAddress nAddress) const noexcept;

    /// <summary>
    /// Gets the number of addresses associated to the captured memory that have been flagged as matching.
    /// </summary>
    uint32_t GetMatchingAddressCount() const noexcept { return m_nMatchingAddressCount; }

    /// <summary>
    /// Gets the Nth matching address.
    /// </summary>
    ByteAddress GetMatchingAddress(gsl::index nIndex) const noexcept;

    /// <summary>
    /// Determines whether the specified address is marked as matching.
    /// </summary>
    bool ContainsMatchingAddress(ByteAddress nAddress) const;

    /// <summary>
    /// Specifies which addresses are matching from a slice of an address list.
    /// </summary>
    void SetMatchingAddresses(std::vector<ByteAddress>& vAddresses, gsl::index nFirstIndex, gsl::index nLastIndex);

    /// <summary>
    /// Marks an address as no longer matching.
    /// </summary>
    void ExcludeMatchingAddress(ByteAddress nAddress);

    /// <summary>
    /// Copies the matching addresses from another CapturedMemoryBlock.
    /// </summary>
    void CopyMatchingAddresses(const CapturedMemoryBlock& pSource);

    /// <summary>
    /// Gets the raw pointer to the matching address data to pass to <see cref="HasMatchingAddress" />.
    /// </summary>
    /// <remarks>
    /// Exposed for performance. Do not call unnecessarily.
    /// </remarks>
    const uint8_t* GetMatchingAddressPointer() const noexcept
    {
        if (AreAllAddressesMatching())
            return nullptr;

        const auto nAddressesSize = (m_nAddressCount + 7) / 8;
        return (nAddressesSize > sizeof(m_vAddresses)) ? m_pAddresses : &m_vAddresses[0];
    }

    /// <summary>
    /// Determines whether the specified address is marked as matching.
    /// </summary>
    /// <remarks>
    /// Exposed for performance. Call <see cref="ContainsMatchingAddress" /> for general use.
    /// </remarks>
    bool HasMatchingAddress(const uint8_t* pMatchingAddresses, ByteAddress nAddress) const noexcept
    {
        if (!pMatchingAddresses)
            return true;

        const auto nIndex = nAddress - m_nFirstAddress;
        if (nIndex >= m_nAddressCount)
            return false;

        const auto nBit = 1 << (nIndex & 7);
        return (pMatchingAddresses[nIndex >> 3] & nBit);
    }

    /// <summary>
    /// Attempts to minimize memory allocations by detecting and sharing large blocks of repeated data.
    /// </summary>
    /// <param name="vBlocks">A collection of previously captured memory blocks that could be shared.</param>
    void OptimizeMemory(const std::vector<CapturedMemoryBlock>& vBlocks) noexcept;

private:
    bool IsBytesAllocated() const noexcept { return GetBytesSize() > sizeof(m_vBytes); }

    void ShareMemory(const std::vector<CapturedMemoryBlock>& vBlocks, uint32_t nHash) noexcept;
    //void SetRepeat(uint32_t nCount, uint32_t nValue) noexcept;

    uint8_t* AllocateMatchingAddresses() noexcept;
    bool AreAllAddressesMatching() const noexcept { return m_nMatchingAddressCount == m_nAddressCount; }

    ByteAddress m_nFirstAddress;     // 4 bytes
    uint32_t m_nBytesSize;           // 4 bytes

    union                            // 8 bytes
    {
        uint8_t m_vBytes[8]{};
        AllocatedMemory* m_pAllocatedMemory;
    };

    union                            // 8 bytes
    {
        uint8_t m_vAddresses[8]{};
        uint8_t* m_pAddresses;
    };

    uint32_t m_nMatchingAddressCount;// 4 bytes
    uint32_t m_nAddressCount;        // 4 bytes
};
static_assert(sizeof(CapturedMemoryBlock) <= 32, "sizeof(CapturedMemoryBlock) is incorrect");

} // namespace data
} // namespace ra

#endif /* !RA_DATA_CAPTUREDMEMORYBLOCK_HH */
