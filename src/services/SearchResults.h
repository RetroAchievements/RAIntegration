#ifndef SEARCHRESULTS_H
#define SEARCHRESULTS_H
#pragma once

#include "RA_Condition.h" // MemSize, ComparisonType

namespace ra {
namespace services {

enum class SearchType
{
    FourBit,
    EightBit,
    SixteenBit,
    ThirtyTwoBit,
};

enum class SearchFilterType
{
    None,
    Constant,
    LastKnownValue,
    LastKnownValuePlus,
    LastKnownValueMinus,
};

namespace impl {

class MemBlock
{
public:
    explicit MemBlock(_In_ unsigned int nAddress, _In_ unsigned int nSize) noexcept :
        m_nAddress(nAddress),
        m_nSize(nSize)
    {
        if (nSize > sizeof(m_vBytes))
            m_pBytes = new (std::nothrow) unsigned char[nSize];
    }

    MemBlock(const MemBlock& other) noexcept : MemBlock(other.m_nAddress, other.m_nSize)
    {
        if (m_nSize > sizeof(m_vBytes))
            std::memcpy(m_pBytes, other.m_pBytes, m_nSize);
        else
            std::memcpy(m_vBytes, other.m_vBytes, sizeof(m_vBytes));
    }

    MemBlock& operator=(const MemBlock&) noexcept = delete;

    MemBlock(MemBlock&& other) noexcept :
        m_nAddress(other.m_nAddress),
        m_nSize(other.m_nSize)
    {
        if (other.m_nSize > sizeof(m_vBytes))
        {
            m_pBytes = other.m_pBytes;
            other.m_pBytes = nullptr;
            other.m_nSize = 0;
        }
        else
        {
            std::memcpy(m_vBytes, other.m_vBytes, sizeof(m_vBytes));
        }
    }

    MemBlock& operator=(MemBlock&&) noexcept = delete;

    ~MemBlock() noexcept
    {
        if (m_nSize > sizeof(m_vBytes))
            delete[] m_pBytes;
    }

    unsigned char* GetBytes() noexcept { return (m_nSize > sizeof(m_vBytes)) ? m_pBytes : &m_vBytes[0]; }
    const unsigned char* GetBytes() const noexcept { return (m_nSize > sizeof(m_vBytes)) ? m_pBytes : &m_vBytes[0]; }

    unsigned char GetByte(std::size_t nIndex) noexcept { return GetBytes()[nIndex]; }
    unsigned char GetByte(std::size_t nIndex) const noexcept { return GetBytes()[nIndex]; }

    ra::ByteAddress GetAddress() const noexcept { return m_nAddress; }
    unsigned int GetSize() const noexcept { return m_nSize; }

private:
    union // 8 bytes
    {
        unsigned char m_vBytes[8]{};
        unsigned char* m_pBytes;
    };

    ra::ByteAddress m_nAddress; // 4 bytes
    unsigned int m_nSize;       // 4 bytes
};
static_assert(sizeof(MemBlock) == 16, "sizeof(MemBlock) is incorrect");

class SearchImpl;

} // namespace impl

class SearchResults
{
public:
    /// <summary>
    /// Initializes an unfiltered result set.
    /// </summary>
    /// <param name="nAddress">The address to start reading from.</param>
    /// <param name="nBytes">The number of bytes to read.</param>
    /// <param name="nType">Type of search to initialize.</param>
    void Initialize(ra::ByteAddress nAddress, size_t nBytes, SearchType nType);

    /// <summary>
    /// Initializes a result set by comparing against the another result set.
    /// </summary>
    /// <param name="srSource">The result set to filter.</param>
    /// <param name="nCompareType">Type of comparison to apply.</param>
    /// <param name="nFilterType">Type of filter to apply.</param>
    /// <param name="nFilterValue">Parameter for filter being applied.</param>
    void Initialize(_In_ const SearchResults& srSource, _In_ ComparisonType nCompareType,
        _In_ SearchFilterType nFilterType, _In_ unsigned int nFilterValue);

    /// <summary>
    /// Gets the number of matching addresses.
    /// </summary>
    size_t MatchingAddressCount() const noexcept;

    struct Result
    {
        ra::ByteAddress nAddress{};
        unsigned int nValue{};
        MemSize nSize{};

        bool Compare(unsigned int nPreviousValue, ComparisonType nCompareType) const noexcept;
    };

    /// <summary>
    /// Gets the nIndex'th matching address.
    /// </summary>
    /// <param name="result">The result.</param>
    /// <returns><c>true</c> if result was populated, <c>false</c> if the index was invalid.</returns>
    bool GetMatchingAddress(gsl::index nIndex, _Out_ Result& result) const;

    /// <summary>
    /// Gets the value at the specified address.
    /// </summary>
    bool GetValue(ra::ByteAddress nAddress, MemSize nSize, _Out_ unsigned int& nValue) const;

    /// <summary>
    /// Gets the type of search performed.
    /// </summary>
    SearchType GetSearchType() const noexcept { return m_nType; }

    /// <summary>
    /// Gets the size of the matching items.
    /// </summary>
    MemSize GetSize() const noexcept;

    /// <summary>
    /// Gets the type of filter that was applied to generate this search result.
    /// </summary>
    SearchFilterType GetFilterType() const noexcept { return m_nFilterType; }

    /// <summary>
    /// Gets the filter parameter that was applied to generate this search result.
    /// </summary>
    unsigned int GetFilterValue() const noexcept { return m_nFilterValue; }

    /// <summary>
    /// Gets the filter comparison that was applied to generate this search result.
    /// </summary>
    ComparisonType GetFilterComparison() const noexcept { return m_nCompareType; }

    /// <summary>
    /// Determines whether the specified address appears in the matching address list.
    /// </summary>
    /// <param name="nAddress">The n address.</param>
    /// <returns><c>true</c> if the specified address is in the matching address list; otherwise,
    /// <c>false</c>.</returns>
    bool ContainsAddress(ra::ByteAddress nAddress) const;

    /// <summary>
    /// Removes an address from the matching address list.
    /// </summary>
    /// <param name="nAddress">The address to remove.</param>
    void ExcludeAddress(ra::ByteAddress nAddress);

    /// <summary>
    /// Removes an address from the matching address list.
    /// </summary>
    /// <param name="nAddress">The index of the address to remove.</param>
    void ExcludeMatchingAddress(gsl::index nIndex);

private:
    std::vector<impl::MemBlock> m_vBlocks;
    SearchType m_nType = SearchType::EightBit;

    friend class impl::SearchImpl;
    impl::SearchImpl* m_pImpl = nullptr;

    std::vector<ra::ByteAddress> m_vMatchingAddresses;
    ComparisonType m_nCompareType = ComparisonType::Equals;
    SearchFilterType m_nFilterType = SearchFilterType::None;
    unsigned int m_nFilterValue = 0U;
};

} // namespace services
} // namespace ra

#endif /* !SEARCHRESULTS_H */
