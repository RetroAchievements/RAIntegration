#ifndef SEARCHIMPL_ASCIITEXT_H
#define SEARCHIMPL_ASCIITEXT_H
#pragma once

#include "SearchImpl.hh"

#include "util\Strings.hh"

namespace ra {
namespace services {
namespace search {

class AsciiTextSearchImpl : public SearchImpl
{
public:
    // indicate that search results can be very wide
    ra::data::Memory::Size GetMemSize() const noexcept override { return ra::data::Memory::Size::Text; }

    bool ValidateFilterValue(SearchResults& srNew) const noexcept override
    {
        if (srNew.GetFilterString().empty())
        {
            // can't match 0 characters!
            if (srNew.GetFilterType() == SearchFilterType::Constant)
                return false;
        }

        return true;
    }

    // populates a vector of addresses that match the specified filter when applied to a previous search result
    void ApplyFilter(SearchResults& srNew, const SearchResults& srPrevious,  std::function<void(ra::data::ByteAddress,uint8_t*,size_t)> pReadMemory) const override
    {
        size_t nCompareLength = 16; // maximum length for generic search
        std::vector<unsigned char> vSearchText;
        GetSearchText(vSearchText, srNew.GetFilterString());

        if (!vSearchText.empty())
            nCompareLength = vSearchText.size();

        unsigned int nLargestBlock = 0U;
        for (auto& block : GetBlocks(srPrevious))
        {
            if (block.GetBytesSize() > nLargestBlock)
                nLargestBlock = block.GetBytesSize();
        }

        std::vector<unsigned char> vMemory(nLargestBlock);
        std::vector<ra::data::ByteAddress> vMatches;

        for (auto& block : GetBlocks(srPrevious))
        {
            pReadMemory(block.GetFirstAddress(), vMemory.data(), block.GetBytesSize());

            const auto nStop = block.GetBytesSize() - 1;
            const auto nFilterComparison = srNew.GetFilterComparison();
            const auto* pMatchingAddresses = block.GetMatchingAddressPointer();

            switch (srNew.GetFilterType())
            {
                default:
                case SearchFilterType::Constant:
                    for (unsigned int i = 0; i < nStop; ++i)
                    {
                        if (CompareMemory(&vMemory.at(i), &vSearchText.at(0), nCompareLength, nFilterComparison))
                        {
                            const unsigned int nAddress = block.GetFirstAddress() + i;
                            if (block.HasMatchingAddress(pMatchingAddresses, nAddress))
                                vMatches.push_back(nAddress);
                        }
                    }
                    break;

                case SearchFilterType::InitialValue:
                case SearchFilterType::LastKnownValue:
                    for (unsigned int i = 0; i < nStop; ++i)
                    {
                        if (CompareMemory(&vMemory.at(i), block.GetBytes() + i, nCompareLength, nFilterComparison))
                        {
                            const unsigned int nAddress = block.GetFirstAddress() + i;
                            if (block.HasMatchingAddress(pMatchingAddresses, nAddress))
                                vMatches.push_back(nAddress);
                        }
                    }
                    break;
            }

            if (!vMatches.empty())
            {
                // adjust the block size to account for the length of the string to ensure
                // the block contains the whole string
                AddBlocks(srNew, vMatches, vMemory, block.GetFirstAddress(), gsl::narrow_cast<unsigned int>(nCompareLength - 1));
                vMatches.clear();
            }
        }
    }

    virtual void GetSearchText(std::vector<unsigned char>& vText, const std::wstring& sText) const
    {
        for (const auto c : sText)
            vText.push_back(gsl::narrow_cast<unsigned char>(c));

        // NOTE: excludes null terminator
    }

    virtual bool CompareMemory(const unsigned char* pLeft,
        const unsigned char* pRight, size_t nCount, ComparisonType nCompareType) const
    {
        Expects(pLeft != nullptr);
        Expects(pRight != nullptr);

        do
        {
            // not exact match, compare the current character normally
            if (*pLeft != *pRight)
                return CompareValues(*pLeft, *pRight, nCompareType);

            // both strings terminated; exact match
            if (*pLeft == '\0')
                break;

            // proceed to next character
            ++pLeft;
            ++pRight;
        } while (--nCount);

        // exact match
        return (nCompareType == ComparisonType::Equals ||
            nCompareType == ComparisonType::GreaterThanOrEqual ||
            nCompareType == ComparisonType::LessThanOrEqual);
    }

    static void GetASCIIText(std::wstring& sText, const unsigned char* pBytes, size_t nBytes)
    {
        sText.clear();
        for (size_t i = 0; i < nBytes; ++i)
        {
            wchar_t c = gsl::narrow_cast<wchar_t>(*pBytes++);
            if (c == 0)
                break;

            if (c < 0x20 || c > 0x7E)
                c = L'\xFFFD';

            sText.push_back(c);
        }
    }

    std::wstring GetFormattedValue(const SearchResults& pResults, const SearchResult& pResult) const override
    {
        return GetFormattedValue(pResults, pResult.nAddress, GetMemSize());
    }

    std::wstring GetFormattedValue(const SearchResults& pResults, ra::data::ByteAddress nAddress, ra::data::Memory::Size) const override
    {
        std::array<unsigned char, 16> pBuffer = {0};
        pResults.GetBytes(nAddress, &pBuffer.at(0), pBuffer.size());

        std::wstring sText;
        GetASCIIText(sText, pBuffer.data(), pBuffer.size());

        return sText;
    }

    bool UpdateValue(const SearchResults&, SearchResult& pResult,
        _Out_ std::wstring* sFormattedValue, const ra::context::IEmulatorMemoryContext& pMemoryContext) const override
    {
        std::array<unsigned char, 16> pBuffer;
        pMemoryContext.ReadMemory(pResult.nAddress, &pBuffer.at(0), pBuffer.size());

        std::wstring sText;
        GetASCIIText(sText, pBuffer.data(), pBuffer.size());

        const unsigned int nPreviousValue = pResult.nValue;
        pResult.nValue = ra::StringHash(sText);

        if (sFormattedValue)
            sFormattedValue->swap(sText);

        return (pResult.nValue != nPreviousValue);
    }

    bool MatchesFilter(const SearchResults& pResults, const SearchResults& pPreviousResults,
        SearchResult& pResult) const override
    {
        std::wstring sPreviousValue;
        if (pResults.GetFilterType() == SearchFilterType::Constant)
            sPreviousValue = pResults.GetFilterString();
        else
            sPreviousValue = GetFormattedValue(pPreviousResults, pResult);

        const std::wstring sValue = GetFormattedValue(pResults, pResult);
        const int nResult = wcscmp(sValue.c_str(), sPreviousValue.c_str());

        switch (pResults.GetFilterComparison())
        {
            case ComparisonType::Equals:
                return (nResult == 0);

            case ComparisonType::NotEqualTo:
                return (nResult != 0);

            case ComparisonType::GreaterThan:
                return (nResult > 0);

            case ComparisonType::LessThan:
                return (nResult < 0);

            case ComparisonType::GreaterThanOrEqual:
                return (nResult >= 0);

            case ComparisonType::LessThanOrEqual:
                return (nResult <= 0);

            default:
                return false;
        }
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_ASCIITEXT_H
