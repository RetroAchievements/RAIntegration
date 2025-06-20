#ifndef SEARCHIMPL_FLOAT_H
#define SEARCHIMPL_FLOAT_H
#pragma once

#include "SearchImpl_32bit.hh"

#include <rcheevos\src\rcheevos\rc_internal.h>

namespace ra {
namespace services {
namespace search {

class FloatSearchImpl : public ThirtyTwoBitSearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::Float; }

    std::wstring GetFormattedValue(const SearchResults&, const SearchResult& pResult) const override
    {
        return ra::data::MemSizeFormat(pResult.nValue, pResult.nSize, MemFormat::Dec);
    }

    bool ValidateFilterValue(SearchResults& srNew) const noexcept override
    {
        // convert the filter string into a value
        if (!srNew.GetFilterString().empty())
        {
            const wchar_t* pStart = srNew.GetFilterString().c_str();
            wchar_t* pEnd;

            const float fFilterValue = std::wcstof(pStart, &pEnd);
            if (pEnd && *pEnd)
            {
                // parse failed
                return false;
            }

            SetFilterValue(srNew, DeconstructFloatValue(fFilterValue));
        }
        else if (srNew.GetFilterType() == SearchFilterType::Constant)
        {
            // constant cannot be empty string
            return false;
        }

        return true;
    }

    GSL_SUPPRESS_TYPE1
    bool MatchesFilter(const SearchResults& pResults, const SearchResults& pPreviousResults,
        SearchResult& pResult) const noexcept override
    {
        float fPreviousValue = 0.0;
        if (pResults.GetFilterType() == SearchFilterType::Constant)
        {
            const auto nFilterValue = pResults.GetFilterValue();
            const auto* pFilterValue = reinterpret_cast<const uint8_t*>(&nFilterValue);
            fPreviousValue = BuildFloatValue(pFilterValue);
        }
        else
        {
            SearchResult pPreviousResult{ pResult };
            GetValueAtVirtualAddress(pPreviousResults, pPreviousResult);
            const auto* pPreviousValue = reinterpret_cast<const uint8_t*>(&pPreviousResult.nValue);
            fPreviousValue = BuildFloatValue(pPreviousValue);
        }

        const auto* pValue = reinterpret_cast<const uint8_t*>(&pResult.nValue);
        const auto fValue = BuildFloatValue(pValue);
        return CompareValues(fValue, fPreviousValue, pResults.GetFilterComparison());
    }

protected:
    virtual float BuildFloatValue(const uint8_t* ptr) const noexcept
    {
        if (!ptr)
            return 0.0f;

        GSL_SUPPRESS_TYPE1 return *reinterpret_cast<const float*>(ptr);
    }

    float ConvertFloatValue(const uint8_t* ptr, uint8_t size) const noexcept
    {
        if (!ptr)
            return 0.0f;

        rc_typed_value_t value;
        value.type = RC_VALUE_TYPE_UNSIGNED;
        GSL_SUPPRESS_TYPE1 value.value.u32 = *reinterpret_cast<const uint32_t*>(ptr);
        rc_transform_memref_value(&value, size);

        return value.value.f32;
    }

    virtual uint32_t DeconstructFloatValue(float fValue) const noexcept
    {
        return ra::data::FloatToU32(fValue, MemSize::Float);
    }

    GSL_SUPPRESS_TYPE1
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        if (nComparison == ComparisonType::Equals || nComparison == ComparisonType::NotEqualTo)
        {
            // for direct equality, we can just compare the raw bytes without converting
            ThirtyTwoBitSearchImpl::ApplyConstantFilter(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nConstantValue, vMatches);
            return;
        }

        const auto nBlockAddress = pPreviousBlock.GetFirstAddress();
        const auto nStride = GetStride();
        const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();

        const auto* pConstantValue = reinterpret_cast<const uint8_t*>(&nConstantValue);
        const float fConstantValue = BuildFloatValue(pConstantValue);

        for (const auto* pScan = pBytes; pScan < pBytesStop; pScan += nStride)
        {
            const float fValue1 = BuildFloatValue(pScan);
            if (CompareValues(fValue1, fConstantValue, nComparison))
            {
                const ra::ByteAddress nAddress = nBlockAddress +
                    ConvertFromRealAddress(gsl::narrow_cast<uint32_t>(pScan - pBytes));
                if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                    vMatches.push_back(nAddress);
            }
        }
    }

    GSL_SUPPRESS_TYPE1
    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        if (nComparison == ComparisonType::Equals || nComparison == ComparisonType::NotEqualTo)
        {
            // for direct equality, we can just compare the raw bytes without converting
            ThirtyTwoBitSearchImpl::ApplyCompareFilter(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nAdjustment, vMatches);
            return;
        }

        const auto* pBlockBytes = pPreviousBlock.GetBytes();
        const auto nBlockAddress = pPreviousBlock.GetFirstAddress();
        const auto nStride = GetStride();
        const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();

        const auto* pAdjustment = reinterpret_cast<const uint8_t*>(&nAdjustment);
        const float fAdjustment = BuildFloatValue(pAdjustment);

        for (const auto* pScan = pBytes; pScan < pBytesStop; pScan += nStride, pBlockBytes += nStride)
        {
            const float fValue1 = BuildFloatValue(pScan);
            const float fValue2 = BuildFloatValue(pBlockBytes) + fAdjustment;
            if (CompareValues(fValue1, fValue2, nComparison))
            {
                const ra::ByteAddress nAddress = nBlockAddress +
                    ConvertFromRealAddress(gsl::narrow_cast<uint32_t>(pScan - pBytes));
                if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                    vMatches.push_back(nAddress);
            }
        }
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_FLOAT_H
