#ifndef SEARCHRESULTS_H
#define SEARCHRESULTS_H
#pragma once

#include "RA_Condition.h" // MemSize, ComparisonType

namespace ra {
namespace services {

class SearchResults
{
public:
    /// <summary>
    /// Initializes an unfiltered result set.
    /// </summary>
    /// <param name="nAddress">The address to start reading from.</param>
    /// <param name="nBytes">The number of bytes to read.</param>
    /// <param name="nSize">Size of the entries.</param>
    void Initialize(unsigned int nAddress, unsigned int nBytes, MemSize nSize);

    /// <summary>
    /// Initializes a result set by comparing against the previous result set.
    /// </summary>
    /// <param name="srSource">The result set to filter.</param>
    /// <param name="nCompareType">Type of comparison to apply.</param>
    void Initialize(_In_ const SearchResults& srSource, _In_ ComparisonType nCompareType);

    /// <summary>
    /// Initializes a result set by comparing against a provided value.
    /// </summary>
    /// <param name="srSource">The result set to filter.</param>
    /// <param name="nCompareType">Type of comparison to apply.</param>
    /// <param name="nTestValue">The value to compare against.</param>
    void Initialize(const SearchResults& srSource, ComparisonType nCompareType, unsigned int nTestValue);

    /// <summary>
    /// Gets the number of matching addresses.
    /// </summary>
    unsigned int MatchingAddressCount() noexcept;

    /// <summary>
    /// Gets a summary of the results
    /// </summary>
    const std::string& Summary() const noexcept { return m_sSummary; }

    struct Result
    {
        unsigned int nAddress{};
        unsigned int nValue{};
        MemSize nSize{};
    };

    /// <summary>
    /// Gets the nIndex matching address
    /// </summary>
    /// <param name="result">The result.</param>
    /// <returns><c>true</c> if result was populated, <c>false</c> if the index was invalid.</returns>
    bool GetMatchingAddress(unsigned int nIndex, _Out_ Result& result);

    /// <summary>
    /// Determines whether the specified address appears in the matching address list.
    /// </summary>
    /// <param name="nAddress">The n address.</param>
    /// <returns><c>true</c> if the specified address is in the matching address list; otherwise,
    /// <c>false</c>.</returns>
    bool ContainsAddress(unsigned int nAddress) const;

    /// <summary>
    /// Removes an address from the matching address list.
    /// </summary>
    /// <param name="nAddress">The address to remove.</param>
    void ExcludeAddress(unsigned int nAddress);

    /// <summary>
    /// Removes an address from the matching address list.
    /// </summary>
    /// <param name="nAddress">The index of the address to remove.</param>
    void ExcludeMatchingAddress(unsigned int nIndex);

protected:
    class MemBlock
    {
    public:
        explicit MemBlock(_In_ unsigned int nAddress, _In_ unsigned int nSize) noexcept :
            nAddress{nAddress},
            nSize{nSize}
        {
            if (nSize > sizeof(m_vBytes))
                m_pBytes = new (std::nothrow) unsigned char[nSize];
        }

        MemBlock(const MemBlock& other) noexcept : MemBlock(other.nAddress, other.nSize)
        {
            if (nSize > sizeof(m_vBytes))
                std::memcpy(m_pBytes, other.m_pBytes, nSize);
        }
        MemBlock& operator=(const MemBlock&) noexcept = delete;

        MemBlock(MemBlock&& other) noexcept : nAddress{other.nAddress}, nSize{other.nSize}
        {
            if (other.nSize > sizeof(m_vBytes))
            {
                m_pBytes       = other.m_pBytes;
                other.m_pBytes = nullptr;
                other.nSize    = 0;
            }
            else
            {
                std::memcpy(m_vBytes, other.m_vBytes, sizeof(m_vBytes));
            }
        }
        MemBlock& operator=(MemBlock&&) noexcept = delete;
        ~MemBlock() noexcept
        {
            if (nSize > sizeof(m_vBytes))
                delete[] m_pBytes;
        }

        unsigned char* GetBytes() noexcept { return (nSize > sizeof(m_vBytes)) ? m_pBytes : &m_vBytes[0]; }
        const unsigned char* GetBytes() const noexcept { return (nSize > sizeof(m_vBytes)) ? m_pBytes : &m_vBytes[0]; }
        unsigned char GetByte(std::size_t nIndex) noexcept { return GetBytes()[nIndex]; }
        unsigned char GetByte(std::size_t nIndex) const noexcept { return GetBytes()[nIndex]; }
        unsigned int GetAddress() const noexcept { return nAddress; }
        unsigned int GetSize() const noexcept { return nSize; }

    private:
        union // 8 bytes
        {
            unsigned char m_vBytes[8]{};
            unsigned char* m_pBytes;
        };

        unsigned int nAddress; // 4 bytes
        unsigned int nSize;    // 4 bytes
    };
    static_assert(sizeof(MemBlock) == 16, "sizeof(MemBlock) is incorrect");

    MemBlock& AddBlock(unsigned int nAddress, unsigned int nSize);

private:
    void ProcessBlocks(
        const SearchResults& srSource,
        std::function<bool(unsigned int, const unsigned char* restrict, const unsigned char* restrict)>
            testIndexFunction);
    void ProcessBlocksNibbles(const SearchResults& srSource, unsigned int nTestValue, ComparisonType nCompareType);
    void AddMatches(unsigned int nAddressBase, const unsigned char* restrict pMemory,
                    const std::vector<unsigned int>& vMatches);
    void AddMatchesNibbles(unsigned int nAddressBase, const unsigned char* restrict pMemory,
                           const std::vector<unsigned int>& vMatches);
    bool ContainsNibble(unsigned int nAddress) const;

    std::string m_sSummary;
    std::vector<MemBlock> m_vBlocks;
    MemSize m_nSize = MemSize::EightBit;

    std::vector<unsigned int> m_vMatchingAddresses;
    bool m_bUnfiltered = false;
};

} // namespace services
} // namespace ra

#endif /* !SEARCHRESULTS_H */
