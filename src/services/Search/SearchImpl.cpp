#include "SearchImpl.hh"

#include "util\TypeCasts.hh"

#include <algorithm>

namespace ra {
namespace services {
namespace search {

bool SearchImpl::ContainsAddress(const SearchResults& srResults, ra::ByteAddress nAddress) const
{
    if (nAddress & (GetStride() - 1))
        return false;

    nAddress = ConvertFromRealAddress(nAddress);
    for (const auto& pBlock : srResults.m_vBlocks)
    {
        if (pBlock.ContainsAddress(nAddress))
            return pBlock.ContainsMatchingAddress(nAddress);
    }

    return false;
}

bool SearchImpl::ExcludeResult(SearchResults& srResults, const SearchResult& pResult) const
{
    if (pResult.nAddress & (GetStride() - 1))
        return false;

    return ExcludeAddress(srResults, ConvertFromRealAddress(pResult.nAddress));
}

bool SearchImpl::ValidateFilterValue(SearchResults& srNew) const
{
    // convert the filter string into a value
    if (!srNew.GetFilterString().empty())
    {
        const wchar_t* pStart = srNew.GetFilterString().c_str();
        wchar_t* pEnd;

        // try decimal first
        srNew.m_nFilterValue = std::wcstoul(pStart, &pEnd, 10);
        if (pEnd && *pEnd)
        {
            // decimal parse failed, try hex
            srNew.m_nFilterValue = std::wcstoul(pStart, &pEnd, 16);
            if (*pEnd)
            {
                // hex parse failed
                return false;
            }
        }
    }
    else if (srNew.GetFilterType() == SearchFilterType::Constant)
    {
        // constant cannot be empty string
        return false;
    }

    return true;
}

void SearchImpl::ApplyFilter(SearchResults& srNew, const SearchResults& srPrevious, std::function<void(ra::ByteAddress,uint8_t*,size_t)> pReadMemory) const
{
    uint32_t nLargestBlock = 0U;
    for (auto& block : srPrevious.m_vBlocks)
    {
        if (block.GetBytesSize() > nLargestBlock)
            nLargestBlock = block.GetBytesSize();
    }

    std::vector<uint8_t> vMemory(nLargestBlock);
    std::vector<ra::ByteAddress> vMatches;

    uint32_t nAdjustment = 0;
    switch (srNew.GetFilterType())
    {
        case SearchFilterType::LastKnownValuePlus:
            nAdjustment = srNew.GetFilterValue();
            break;
        case SearchFilterType::LastKnownValueMinus:
            nAdjustment = ra::to_unsigned(-ra::to_signed(srNew.GetFilterValue()));
            break;
        default:
            break;
    }

    for (auto& block : srPrevious.m_vBlocks)
    {
        const auto nRealAddress = ConvertToRealAddress(block.GetFirstAddress());
        pReadMemory(nRealAddress, vMemory.data(), block.GetBytesSize());

        const auto nStop = block.GetBytesSize() - GetPadding();

        switch (srNew.GetFilterType())
        {
            case SearchFilterType::Constant:
                ApplyConstantFilter(vMemory.data(), vMemory.data() + nStop, block,
                    srNew.GetFilterComparison(), srNew.GetFilterValue(), vMatches);
                break;

            default:
                // if an entire block is unchanged, everything in it is equal to the LastKnownValue
                // or InitialValue and we don't have to check every address.
                if (memcmp(vMemory.data(), block.GetBytes(), block.GetBytesSize()) == 0)
                {
                    switch (srNew.GetFilterComparison())
                    {
                        case ComparisonType::Equals:
                        case ComparisonType::GreaterThanOrEqual:
                        case ComparisonType::LessThanOrEqual:
                            if (nAdjustment == 0)
                            {
                                // entire block matches, copy the old block
                                srNew.m_vBlocks.emplace_back(block);
                                continue;
                            }
                            else if (srNew.GetFilterComparison() == ComparisonType::Equals)
                            {
                                // entire block matches, so adjustment won't match. discard the block
                                continue;
                            }
                            break;

                        default:
                            if (nAdjustment == 0)
                            {
                                // entire block matches, discard it
                                continue;
                            }
                            break;
                    }

                    // have to check individual addresses to see if adjustment matches
                }

                // per-address comparison
                ApplyCompareFilter(vMemory.data(), vMemory.data() + nStop, block,
                    srNew.GetFilterComparison(), nAdjustment, vMatches);
                break;
        }

        if (!vMatches.empty())
        {
            AddBlocks(srNew, vMatches, vMemory, block.GetFirstAddress(), GetPadding());
            vMatches.clear();
        }
    }
}

bool SearchImpl::GetMatchingAddress(const SearchResults& srResults, gsl::index nIndex, _Out_ SearchResult& result) const noexcept
{
    result.nSize = GetMemSize();

    for (const auto& pBlock : srResults.m_vBlocks)
    {
        if (nIndex < gsl::narrow_cast<gsl::index>(pBlock.GetMatchingAddressCount()))
        {
            result.nAddress = pBlock.GetMatchingAddress(nIndex);
            return GetValueFromMemBlock(pBlock, result);
        }

        nIndex -= pBlock.GetMatchingAddressCount();
    }

    return false;
}

bool SearchImpl::GetValueAtVirtualAddress(const SearchResults& srResults, SearchResult& result) const noexcept
{
    for (const auto& block : srResults.m_vBlocks)
    {
        if (GetValueFromMemBlock(block, result))
            return true;
    }

    result.nValue = 0;
    result.nAddress = ConvertToRealAddress(result.nAddress);
    return false;
}

std::wstring SearchImpl::GetFormattedValue(const SearchResults&, const SearchResult& pResult) const
{
    return L"0x" + ra::data::MemSizeFormat(pResult.nValue, pResult.nSize, MemFormat::Hex);
}

std::wstring SearchImpl::GetFormattedValue(const SearchResults& pResults, ra::ByteAddress nAddress, MemSize nSize) const
{
    SearchResult pResult{ ConvertFromRealAddress(nAddress), 0, nSize };
    if (GetValueAtVirtualAddress(pResults, pResult))
        return GetFormattedValue(pResults, pResult);

    return L"";
}

bool SearchImpl::UpdateValue(const SearchResults& pResults, SearchResult& pResult,
    _Out_ std::wstring* sFormattedValue, const ra::data::context::EmulatorContext& pEmulatorContext) const
{
    const uint32_t nPreviousValue = pResult.nValue;
    pResult.nValue = pEmulatorContext.ReadMemory(pResult.nAddress, pResult.nSize);

    if (sFormattedValue)
        *sFormattedValue = GetFormattedValue(pResults, pResult);

    return (pResult.nValue != nPreviousValue);
}

bool SearchImpl::MatchesFilter(const SearchResults& pResults, const SearchResults& pPreviousResults,
    SearchResult& pResult) const
{
    uint32_t nPreviousValue = 0;
    if (pResults.GetFilterType() == SearchFilterType::Constant)
    {
        nPreviousValue = pResults.GetFilterValue();
    }
    else
    {
        SearchResult pPreviousResult{ pResult };
        pPreviousResult.nAddress = ConvertFromRealAddress(pResult.nAddress);
        GetValueAtVirtualAddress(pPreviousResults, pPreviousResult);

        switch (pResults.GetFilterType())
        {
            case SearchFilterType::LastKnownValue:
            case SearchFilterType::InitialValue:
                nPreviousValue = pPreviousResult.nValue;
                break;

            case SearchFilterType::LastKnownValuePlus:
                nPreviousValue = pPreviousResult.nValue + pResults.GetFilterValue();
                break;

            case SearchFilterType::LastKnownValueMinus:
                nPreviousValue = pPreviousResult.nValue - pResults.GetFilterValue();
                break;

            default:
                return false;
        }
    }

    return CompareValues(pResult.nValue, nPreviousValue, pResults.GetFilterComparison());
}

void SearchImpl::ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
    const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
    std::vector<ra::ByteAddress>& vMatches) const
{
    const auto nBlockAddress = pPreviousBlock.GetFirstAddress();
    const auto nStride = GetStride();
    const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();

    for (const auto* pScan = pBytes; pScan < pBytesStop; pScan += nStride)
    {
        const uint32_t nValue1 = BuildValue(pScan);
        if (CompareValues(nValue1, nConstantValue, nComparison))
        {
            const ra::ByteAddress nAddress = nBlockAddress +
                ConvertFromRealAddress(gsl::narrow_cast<uint32_t>(pScan - pBytes));
            if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                vMatches.push_back(nAddress);
        }
    }
}

void SearchImpl::ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
    const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
    std::vector<ra::ByteAddress>& vMatches) const
{
    const auto* pBlockBytes = pPreviousBlock.GetBytes();
    const auto nBlockAddress = pPreviousBlock.GetFirstAddress();
    const auto nStride = GetStride();
    const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();

    for (const auto* pScan = pBytes; pScan < pBytesStop; pScan += nStride, pBlockBytes += nStride)
    {
        const uint32_t nValue1 = BuildValue(pScan);
        const uint32_t nValue2 = BuildValue(pBlockBytes) + nAdjustment;
        if (CompareValues(nValue1, nValue2, nComparison))
        {
            const ra::ByteAddress nAddress = nBlockAddress +
                ConvertFromRealAddress(gsl::narrow_cast<uint32_t>(pScan - pBytes));
            if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                vMatches.push_back(nAddress);
        }
    }
}

bool SearchImpl::GetValueFromMemBlock(const MemBlock& block, SearchResult& result) const noexcept
{
    if (result.nAddress < block.GetFirstAddress())
        return false;

    const uint32_t nOffset = result.nAddress - block.GetFirstAddress();
    if (nOffset >= block.GetBytesSize() - GetPadding())
        return false;

    result.nValue = BuildValue(block.GetBytes() + nOffset);
    return true;
}

uint32_t SearchImpl::BuildValue(const uint8_t* ptr) const noexcept
{
    return ptr ? ptr[0] : 0;
}

void SearchImpl::AddBlocks(SearchResults& srNew, std::vector<ra::ByteAddress>& vMatches,
    std::vector<uint8_t>& vMemory, ra::ByteAddress nPreviousBlockFirstAddress, uint32_t nPadding) const
{
    const gsl::index nStopIndex = gsl::narrow_cast<gsl::index>(vMatches.size()) - 1;
    gsl::index nFirstIndex = 0;
    gsl::index nLastIndex = 0;
    do
    {
        const auto nFirstMatchingAddress = vMatches.at(nFirstIndex);
        while (nLastIndex < nStopIndex && vMatches.at(nLastIndex + 1) - nFirstMatchingAddress < 64)
            nLastIndex++;

        // determine how many bytes we need to capture
        const auto nFirstRealAddress = ConvertToRealAddress(nFirstMatchingAddress);
        const auto nLastRealAddress = ConvertToRealAddress(vMatches.at(nLastIndex));
        const uint32_t nBlockSize = nLastRealAddress - nFirstRealAddress + 1 + nPadding;

        // determine the maximum number of addresses that can be associated to the captured bytes
        const auto nMaxAddresses = GetAddressCountForBytes(nBlockSize);

        // determine the address of the first captured byte
        const auto nFirstAddress = ConvertFromRealAddress(nFirstRealAddress);

        // allocate the new block
        MemBlock& block = AddBlock(srNew, nFirstAddress, nBlockSize, nMaxAddresses);

        // capture the subset of data that corresponds to the subset of matches
        const auto nOffset = nFirstRealAddress - ConvertToRealAddress(nPreviousBlockFirstAddress);
        memcpy(block.GetBytes(), &vMemory.at(nOffset), nBlockSize);

        // capture the matched addresses
        block.SetMatchingAddresses(vMatches, nFirstIndex, nLastIndex);

        nFirstIndex = nLastIndex + 1;
    } while (nFirstIndex < gsl::narrow_cast<gsl::index>(vMatches.size()));
}

} // namespace search
} // namespace services
} // namespace ra
