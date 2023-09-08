#ifndef SEARCHRESULTS_H
#define SEARCHRESULTS_H
#pragma once

#include "data\Types.hh"

#include "data\context\EmulatorContext.hh"

namespace ra {
namespace services {

enum class SearchType
{
    FourBit,
    EightBit,
    SixteenBit,
    TwentyFourBit,
    ThirtyTwoBit,
    SixteenBitAligned,
    ThirtyTwoBitAligned,
    SixteenBitBigEndian,
    ThirtyTwoBitBigEndian,
    SixteenBitBigEndianAligned,
    ThirtyTwoBitBigEndianAligned,
    Float,
    FloatBigEndian,
    MBF32,
    MBF32LE,
    AsciiText,
    BitCount,
};

enum class SearchFilterType
{
    None,
    Constant,
    LastKnownValue,
    LastKnownValuePlus,
    LastKnownValueMinus,
    InitialValue,
};

namespace impl {

class MemBlock
{
public:
    explicit MemBlock(_In_ unsigned int nAddress, _In_ unsigned int nSize, _In_ unsigned int nMaxAddresses) noexcept :
        m_nFirstAddress(nAddress),
        m_nBytesSize(nSize),
        m_nMatchingAddresses(nMaxAddresses),
        m_nMaxAddresses(nMaxAddresses)
    {
        if (nSize > sizeof(m_vBytes))
            m_pBytes = new (std::nothrow) uint8_t[nSize];
    }

    MemBlock(const MemBlock& other) noexcept : MemBlock(other.m_nFirstAddress, other.m_nBytesSize, other.m_nMaxAddresses)
    {
        if (m_nBytesSize > sizeof(m_vBytes))
            std::memcpy(m_pBytes, other.m_pBytes, m_nBytesSize);
        else
            std::memcpy(m_vBytes, other.m_vBytes, sizeof(m_vBytes));

        m_nMatchingAddresses = other.m_nMatchingAddresses;
        if (!AreAllAddressesMatching())
        {
            if ((m_nMatchingAddresses + 7) / 8 > sizeof(m_vAddresses))
                std::memcpy(m_pAddresses, other.m_pAddresses, (m_nMatchingAddresses + 7) / 8);
            else
                std::memcpy(m_vAddresses, other.m_vAddresses, sizeof(m_vAddresses));
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

    ra::ByteAddress GetFirstAddress() const noexcept { return m_nFirstAddress; }
    unsigned int GetBytesSize() const noexcept { return m_nBytesSize; }
    unsigned int GetMaxAddresses() const noexcept { return m_nMaxAddresses; }

    bool ContainsAddress(ra::ByteAddress nAddress) const noexcept;

    void SetMatchingAddresses(std::vector<ra::ByteAddress>& vAddresses, gsl::index nFirstIndex, gsl::index nLastIndex);
    void CopyMatchingAddresses(const MemBlock& pSource);
    void ExcludeMatchingAddress(ra::ByteAddress nAddress);
    bool ContainsMatchingAddress(ra::ByteAddress nAddress) const;

    unsigned int GetMatchingAddressCount() const noexcept { return m_nMatchingAddresses; }
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

    unsigned int m_nBytesSize;         // 4 bytes
    ra::ByteAddress m_nFirstAddress;   // 4 bytes
    unsigned int m_nMatchingAddresses; // 4 bytes
    unsigned int m_nMaxAddresses;      // 4 bytes
};
static_assert(sizeof(MemBlock) <= 32, "sizeof(MemBlock) is incorrect");

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
    /// Initializes a result set by comparing current memory against another result set.
    /// </summary>
    /// <param name="srSource">The result set to filter.</param>
    /// <param name="nCompareType">Type of comparison to apply.</param>
    /// <param name="nFilterType">Type of filter to apply.</param>
    /// <param name="sFilterValue">Parameter for filter being applied.</param>
    /// <returns><c>true</c> if initialization was successful, <c>false</c> if the filter value was not supported</returns>
    bool Initialize(_In_ const SearchResults& srSource, _In_ ComparisonType nCompareType,
        _In_ SearchFilterType nFilterType, _In_ const std::wstring& sFilterValue);

    /// <summary>
    /// Initializes a result set by comparing current memory against another result set using the address filter from a third set.
    /// </summary>
    /// <param name="srSource">The result set to filter.</param>
    /// <param name="srAddresses">The result set specifying which addresses to examine.</param>
    /// <param name="nCompareType">Type of comparison to apply.</param>
    /// <param name="nFilterType">Type of filter to apply.</param>
    /// <param name="sFilterValue">Parameter for filter being applied.</param>
    /// <returns><c>true</c> if initialization was successful, <c>false</c> if the filter value was not supported</returns>
    bool Initialize(_In_ const SearchResults& srSource, _In_ const SearchResults& srAddresses,
        _In_ ComparisonType nCompareType, _In_ SearchFilterType nFilterType, _In_ const std::wstring& sFilterValue);

    /// <summary>
    /// Initializes a result set by comparing provided memory against another result set.
    /// </summary>
    /// <param name="srFirst">The result set to filter.</param>
    /// <param name="pReadMemory">A function that provides current values of memory.</param>
    /// <param name="nCompareType">Type of comparison to apply.</param>
    /// <param name="nFilterType">Type of filter to apply.</param>
    /// <param name="sFilterValue">Parameter for filter being applied.</param>
    /// <returns><c>true</c> if initialization was successful, <c>false</c> if the filter value was not supported</returns>
    bool Initialize(_In_ const SearchResults& srFirst, _In_ std::function<void(ra::ByteAddress,uint8_t*,size_t)> pReadMemory,
        _In_ ComparisonType nCompareType, _In_ SearchFilterType nFilterType, _In_ const std::wstring& sFilterValue);

    /// <summary>
    /// Gets the number of matching addresses.
    /// </summary>
    size_t MatchingAddressCount() const noexcept;

    struct Result
    {
        ra::ByteAddress nAddress{};
        unsigned int nValue{};
        MemSize nSize{};
    };

    /// <summary>
    /// Gets the nIndex'th matching address.
    /// </summary>
    /// <param name="result">The result.</param>
    /// <returns><c>true</c> if result was populated, <c>false</c> if the index was invalid.</returns>
    bool GetMatchingAddress(gsl::index nIndex, _Out_ Result& result) const noexcept;

    /// <summary>
    /// Gets and item from the results matching a result from another set of results.
    /// </summary>
    /// <param name="pSrcResult">The result to find a match for.</param>
    /// <param name="result">The result.</param>
    /// <returns><c>true</c> if result was populated, <c>false</c> if the index was invalid.</returns>
    bool GetMatchingAddress(const SearchResults::Result& pSrcResult, _Out_ Result& result) const noexcept;

    /// <summary>
    /// Gets the raw bytes at the specified address.
    /// </summary>
    bool GetBytes(ra::ByteAddress nAddress, unsigned char* pBuffer, size_t nCount) const noexcept;

    /// <summary>
    /// Gets a formatted value for the specified address.
    /// </summary>
    /// <param name="nAddress">Address of the start of the value.</param>
    /// <param name="nSize">Size of the value.</param>
    /// <returns>Formatted value.</returns>
    std::wstring GetFormattedValue(ra::ByteAddress nAddress, MemSize nSize) const;

    /// <summary>
    /// Updates the current value of the provided result.
    /// </summary>
    /// <param name="pResult">The result to update.</param>
    /// <param name="pPreviousResult">The previous value of the result.</param>
    /// <param name="sFormattedValue">Pointer to a string to populate with a textual representation of the value. [optional]</param>
    /// <returns><c>true</c> if the value changed, <c>false</c> if not.</returns>
    bool UpdateValue(SearchResults::Result& pResult, _Out_ std::wstring* sFormattedValue,
        const ra::data::context::EmulatorContext& pEmulatorContext) const;

    /// <summary>
    /// Determines if a value compared to its previous value matches the filter.
    /// </summary>
    /// <param name="pPreviousResults">The previous results to compare against.</param>
    /// <param name="pResult">The result to test.</param>
    /// <returns><c>true</c> if the value still matches the filter, <c>false</c> if not.</returns>
    bool MatchesFilter(const SearchResults& pPreviousResults, SearchResults::Result& pResult) const;

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
    /// Gets the filter parameter that was applied to generate this search result.
    /// </summary>
    const std::wstring& GetFilterString() const noexcept { return m_sFilterValue; }

    /// <summary>
    /// Gets the filter comparison that was applied to generate this search result.
    /// </summary>
    ComparisonType GetFilterComparison() const noexcept { return m_nCompareType; }

    /// <summary>
    /// Determines whether the specified address appears in the matching address list.
    /// </summary>
    /// <param name="nAddress">The n address.</param>
    /// <returns><c>true</c> if the specified address is in the matching address list; otherwise
    /// <c>false</c>.</returns>
    bool ContainsAddress(ra::ByteAddress nAddress) const;

    /// <summary>
    /// Removes an entry from the matching address list.
    /// </summary>
    /// <param name="pResult">The result to remove.</param>
    /// <returns><c>true</c> if the specified address was removed from the matching address list;
    /// otherwise <c>false</c>.</returns>
    bool ExcludeResult(const SearchResults::Result& pResult);

private:
    void MergeSearchResults(const SearchResults& srMemory, const SearchResults& srAddresses);

    std::vector<impl::MemBlock> m_vBlocks;
    SearchType m_nType = SearchType::EightBit;

    friend class impl::SearchImpl;
    impl::SearchImpl* m_pImpl = nullptr;

    ComparisonType m_nCompareType = ComparisonType::Equals;
    SearchFilterType m_nFilterType = SearchFilterType::None;
    unsigned int m_nFilterValue = 0U;
    std::wstring m_sFilterValue;
};

} // namespace services
} // namespace ra

#endif /* !SEARCHRESULTS_H */
