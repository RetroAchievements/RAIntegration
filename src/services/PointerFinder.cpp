#include "PointerFinder.hh"

#include "RA_Defs.h"

#include "context\IConsoleContext.hh"

#include "services\ServiceLocator.hh"

#include "util\Strings.hh"

#include "ui\IDesktop.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace services {

constexpr int32_t MAX_OFFSET = 1024;

void PointerFinder::Capture::Initialize(const SearchResults& pSearchResults)
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    m_vPointerValues.reserve(pSearchResults.MatchingAddressCount());
    m_vPointerValues.clear();

    pSearchResults.EnumerateMatches(
        [&pConsoleContext, &vPointerValues = m_vPointerValues]
            (const ra::services::SearchResult& pResult)
        {
            if (pResult.nValue != 0) // ignore null values
            {
                const auto nPointerAddress = pConsoleContext.ByteAddressFromRealAddress(pResult.nValue);
                if (nPointerAddress != 0xFFFFFFFF) // ignore non-pointer values
                    vPointerValues.emplace_back(pResult.nAddress, pResult.nValue, nPointerAddress);
            }

            return true;
        });
}

const PointerFinder::PointerValue* PointerFinder::Capture::GetValue(ra::data::ByteAddress nAddress) const
{
    const auto pLowerBound = std::lower_bound(m_vPointerValues.begin(), m_vPointerValues.end(), nAddress,
        [](const PointerValue& pPointerValue, uint32_t nAddress)
        {
            return pPointerValue.nAddress < nAddress;
        });
    return (pLowerBound < m_vPointerValues.end() && pLowerBound->nAddress == nAddress) ? &*pLowerBound : nullptr;
}

ra::data::ByteAddress PointerFinder::Capture::GetValueAsAddress(ra::data::ByteAddress nAddress) const
{
    const auto pLowerBound = std::lower_bound(m_vPointerValues.begin(), m_vPointerValues.end(), nAddress,
        [](const PointerValue& pPointerValue, uint32_t nAddress)
        {
            return pPointerValue.nAddress < nAddress;
        });
    return (pLowerBound < m_vPointerValues.end() && pLowerBound->nAddress == nAddress) ? pLowerBound->nValueAsAddress : 0;
}

PointerFinder::PointerAddressRange PointerFinder::Capture::NarrowSearch(ra::data::ByteAddress nTargetAddress) const
{
    if (m_vPointerValues.empty())
        return PointerAddressRange(m_vPointerValues.data(), m_vPointerValues.data());

    const auto pSearchIter = std::lower_bound(m_vPointerValues.begin(), m_vPointerValues.end(), nTargetAddress,
        [](const PointerValue& pPointerValue, uint32_t nAddress)
        {
            return pPointerValue.nAddress < nAddress;
        });
    auto pEnd = pSearchIter;
    auto pStart = pSearchIter;

    uint32_t nMaxOffset = MAX_OFFSET;
    for (int i = 0; i < 4; ++i, nMaxOffset *= 2)
    {
        while (pEnd < m_vPointerValues.end() && pEnd->nAddress - nTargetAddress <= nMaxOffset)
            ++pEnd;

        while (pStart > m_vPointerValues.begin() && nTargetAddress - (pStart - 1)->nAddress <= nMaxOffset)
            --pStart;

        const auto nMatches = gsl::narrow_cast<size_t>(pEnd - pStart);
        if (nMatches >= 8 || nMatches == m_vPointerValues.size())
            break;
    }

    return PointerAddressRange(m_vPointerValues.data() + (pStart - m_vPointerValues.begin()),
                               m_vPointerValues.data() + (pEnd - m_vPointerValues.begin()));
}

void PointerFinder::AddCapture(const Capture& pCapture, ra::data::ByteAddress nTargetAddress)
{
    m_vCaptures.emplace_back(pCapture, nTargetAddress);
}

void PointerFinder::Analyze(std::vector<PotentialPointer>& vResults, std::function<void(size_t, size_t)> fProgress)
{
    if (m_vCaptures.empty())
        return;

    GetPointers(vResults, fProgress);

    SortPointers(vResults);
}

void PointerFinder::SortPointers(std::vector<PotentialPointer>& vPointers)
{
    std::sort(vPointers.begin(), vPointers.end(),
        [](const PotentialPointer& a, const PotentialPointer& b)
        {
            if (a.nScore == b.nScore)
                return a.nRootAddress < b.nRootAddress;

            return a.nScore < b.nScore;
        });
}

static uint32_t CalculateScore(int32_t nOffset) noexcept
{
    // treat negative offsets as being farther away
    return ra::to_unsigned((nOffset < 0) ? (-nOffset * 8) : nOffset);
}

void PointerFinder::InitializePotentialPointers(std::vector<PotentialPointer>& vPointers, const Capture& pCapture, ra::data::ByteAddress nTargetAddress)
{
    vPointers.reserve(pCapture.m_vPointerValues.size());

    for (const auto& pValue : pCapture.m_vPointerValues)
    {
        const int32_t nOffset = ra::to_signed(nTargetAddress) - ra::to_signed(pValue.nValueAsAddress);
        if (nOffset < MAX_OFFSET && nOffset > -MAX_OFFSET)
        {
            auto& pPointer = vPointers.emplace_back();
            pPointer.nRootAddress = pValue.nAddress;

            pPointer.vOffsets.at(pPointer.nOffsetLength++) = nOffset;
            pPointer.nScore = CalculateScore(nOffset);
        }
    }

    std::sort(vPointers.begin(), vPointers.end(),
        [](const PotentialPointer& a, const PotentialPointer& b)
        {
            return a.vOffsets.front() < b.vOffsets.front();
        });
}

void PointerFinder::RemoveUnsharedOffsets(std::vector<CaptureMetrics>& vCaptureMetrics)
{
    // determine which offsets are shared across all captures
    std::vector<int32_t> vSharedOffsets;

    int32_t nPrevOffset = std::numeric_limits<int>::min();
    for (const auto& pPointer : vCaptureMetrics.front().vPotentialPointers)
    {
        const auto nOffset = pPointer.vOffsets.front();
        if (nOffset != nPrevOffset)
        {
            vSharedOffsets.push_back(nOffset);
            nPrevOffset = nOffset;
        }
    }

    for (auto pCaptureMetrics = vCaptureMetrics.begin() + 1; pCaptureMetrics < vCaptureMetrics.end(); ++pCaptureMetrics)
    {
        vSharedOffsets.erase(std::remove_if(vSharedOffsets.begin(), vSharedOffsets.end(),
            [&pCaptureMetrics](const int32_t nOffset) noexcept
            {
                for (const auto& pPointer : pCaptureMetrics->vPotentialPointers)
                {
                    if (pPointer.vOffsets.front() == nOffset)
                        return false;
                }

                return true;
            }),
            vSharedOffsets.end());
    }

    // remove any potential pointers at offsets not shared by all captures
    if (vSharedOffsets.empty())
    {
        for (auto& pCaptureMetrics : vCaptureMetrics)
            pCaptureMetrics.vPotentialPointers.clear();
    }
    else
    {
        for (auto& pCaptureMetrics : vCaptureMetrics)
        {
            pCaptureMetrics.vPotentialPointers.erase(std::remove_if(pCaptureMetrics.vPotentialPointers.begin(), pCaptureMetrics.vPotentialPointers.end(),
                [&vSharedOffsets](const PotentialPointer& a)
                {
                    return !std::binary_search(vSharedOffsets.begin(), vSharedOffsets.end(), a.vOffsets.front());
                }),
                pCaptureMetrics.vPotentialPointers.end());
        }
    }
}

void PointerFinder::GetPointers(std::vector<PotentialPointer>& vIndirectNodes, std::function<void(size_t, size_t)> fProgress) const
{
    AnalysisState pAnalysisState;
    pAnalysisState.fProgress = fProgress;
    pAnalysisState.vResults = &vIndirectNodes;

    for (const auto& pCapture : m_vCaptures)
    {
        auto& pCaptureMetrics = pAnalysisState.vCaptureMetrics.emplace_back();
        pCaptureMetrics.pCapture = &pCapture.first;
        InitializePotentialPointers(pCaptureMetrics.vPotentialPointers, pCapture.first, pCapture.second);
    }

    RemoveUnsharedOffsets(pAnalysisState.vCaptureMetrics);

    if (pAnalysisState.fProgress)
    {
        constexpr size_t nMinProgress = 1; // std::max cannot deduce type of literal
        pAnalysisState.nRootProgressSize = pAnalysisState.vCaptureMetrics.front().vPotentialPointers.size();
        pAnalysisState.nMaxProgress = std::max(nMinProgress, pAnalysisState.nRootProgressSize * (pAnalysisState.nRootProgressSize + 1));
        pAnalysisState.fProgress(0, pAnalysisState.nMaxProgress);
    }

    GetPointers(vIndirectNodes, pAnalysisState);
}

void PointerFinder::GetPointers(std::vector<PotentialPointer>& vIndirectNodes, AnalysisState& pAnalysisState)
{
    for (auto& pCaptureMetrics : pAnalysisState.vCaptureMetrics)
        pCaptureMetrics.pIterator = pCaptureMetrics.vPotentialPointers.cbegin();

    const size_t nProgressStart = pAnalysisState.nProgress;
    do
    {
        if (!FindSharedOffset(pAnalysisState.vCaptureMetrics))
            break;

        ProcessSharedOffset(vIndirectNodes, pAnalysisState);

        for (auto& pCaptureMetrics : pAnalysisState.vCaptureMetrics)
            pCaptureMetrics.pIterator = pCaptureMetrics.pStopIterator;

        if (pAnalysisState.fProgress && pAnalysisState.nOffsetLength < 2)
        {
            const auto nItemsProcessed = (pAnalysisState.vCaptureMetrics.front().pIterator - pAnalysisState.vCaptureMetrics.front().vPotentialPointers.cbegin());
            if (pAnalysisState.nOffsetLength == 1)
                pAnalysisState.nProgress = nProgressStart + nItemsProcessed;
            else
                pAnalysisState.nProgress = nProgressStart + nItemsProcessed * (pAnalysisState.nRootProgressSize + 1);

            pAnalysisState.fProgress(pAnalysisState.nProgress, pAnalysisState.nMaxProgress);
        }
    } while (true);

    if (pAnalysisState.fProgress && pAnalysisState.nOffsetLength < 2)
    {
        if (pAnalysisState.nOffsetLength == 1)
            pAnalysisState.nProgress = nProgressStart + pAnalysisState.nRootProgressSize;
        else
            pAnalysisState.nProgress++;

        pAnalysisState.fProgress(pAnalysisState.nProgress, pAnalysisState.nMaxProgress);
    }
}

bool PointerFinder::FindSharedOffset(std::vector<CaptureMetrics>& vCaptureMetrics) noexcept
{
    // assert: metrics arrays only contain shared offsets per RemoveUnsharedOffsets
    //         and they're in incremental order, so all we have to do is update the
    //         stop iterators.
    if (vCaptureMetrics.front().pIterator == vCaptureMetrics.front().vPotentialPointers.cend())
        return false;

    const auto nOffset = vCaptureMetrics.front().pIterator->vOffsets.front();

    for (auto& pCaptureMetric : vCaptureMetrics)
    {
        auto pIter = pCaptureMetric.pIterator;
        while (pIter < pCaptureMetric.vPotentialPointers.end() && pIter->vOffsets.front() == nOffset)
            ++pIter;
        pCaptureMetric.pStopIterator = pIter;
    }

    return true;
}

void PointerFinder::ProcessSharedOffset(std::vector<PotentialPointer>& vIndirectNodes, AnalysisState& pAnalysisState)
{
    std::vector<std::vector<PotentialPointer>::const_iterator> vIterators;
    vIterators.reserve(pAnalysisState.vCaptureMetrics.size());
    for (const auto& pCaptureMetrics : pAnalysisState.vCaptureMetrics)
        vIterators.push_back(pCaptureMetrics.pIterator);

    const auto nOffset = vIterators.front()->vOffsets.front();

    AnalysisState pNewAnalysisState;
    pNewAnalysisState.nScore = pAnalysisState.nScore + CalculateScore(nOffset);

    pNewAnalysisState.vOffsets.at(0) = nOffset;
    memcpy(&pNewAnalysisState.vOffsets.at(1), &pAnalysisState.vOffsets.at(0), pAnalysisState.nOffsetLength * sizeof(pAnalysisState.vOffsets.at(0)));
    pNewAnalysisState.nOffsetLength = pAnalysisState.nOffsetLength + 1;

    if (pNewAnalysisState.nOffsetLength < MAX_DEPTH)
    {
        pNewAnalysisState.fProgress = pAnalysisState.fProgress;
        pNewAnalysisState.nRootProgressSize = pAnalysisState.nRootProgressSize;
        pNewAnalysisState.nMaxProgress = pAnalysisState.nMaxProgress;
        pNewAnalysisState.vResults = pAnalysisState.vResults;

        gsl::index nIndex = 0;
        for (const auto& pCaptureMetric : pAnalysisState.vCaptureMetrics)
        {
            auto& pNewCaptureMetric = pNewAnalysisState.vCaptureMetrics.emplace_back();
            pNewCaptureMetric.pCapture = pCaptureMetric.pCapture;
            ++nIndex;
        }
    }

    do
    {
        if (PointersMatch(vIterators))
        {
            const auto nRootAddress = vIterators.front()->nRootAddress;
            bool bFound = false;
            for (auto& pPointer : vIndirectNodes)
            {
                if (pPointer.nRootAddress == nRootAddress)
                {
                    // this root pointer was already captured. only keep this
                    // route if it's more efficient (lower score).
                    if (pNewAnalysisState.nScore < pPointer.nScore)
                    {
                        pPointer.vOffsets = pNewAnalysisState.vOffsets;
                        pPointer.nOffsetLength = pNewAnalysisState.nOffsetLength;
                        pPointer.nScore = pNewAnalysisState.nScore;
                    }

                    bFound = true;
                    break;
                }
            }
            if (!bFound)
            {
                auto& pPointer = vIndirectNodes.emplace_back();
                pPointer.nRootAddress = nRootAddress;
                pPointer.vOffsets = pNewAnalysisState.vOffsets;
                pPointer.nOffsetLength = pNewAnalysisState.nOffsetLength;
                pPointer.nScore = pNewAnalysisState.nScore;
            }
        }
        else if (!pNewAnalysisState.vCaptureMetrics.empty())
        {
            const auto nRootAddress = vIterators.front()->nRootAddress;
            if (!std::binary_search(pAnalysisState.vDeadEndAddresses.begin(), pAnalysisState.vDeadEndAddresses.end(), nRootAddress))
            {
                gsl::index nIndex = 0;
                for (auto& pCaptureMetric : pNewAnalysisState.vCaptureMetrics)
                {
                    pCaptureMetric.vPotentialPointers.clear();
                    InitializePotentialPointers(pCaptureMetric.vPotentialPointers, *pCaptureMetric.pCapture, vIterators.at(nIndex)->nRootAddress);
                    ++nIndex;
                }

                RemoveUnsharedOffsets(pNewAnalysisState.vCaptureMetrics);

                const auto nFoundPointers = pAnalysisState.vResults->size();

                pNewAnalysisState.nProgress = pAnalysisState.nProgress;
                GetPointers(vIndirectNodes, pNewAnalysisState);
                pAnalysisState.nProgress = pNewAnalysisState.nProgress;

                if (pAnalysisState.vResults->size() == nFoundPointers)
                {
                    const auto pInsertAt = std::lower_bound(pAnalysisState.vDeadEndAddresses.begin(), pAnalysisState.vDeadEndAddresses.end(), nRootAddress);
                    if (pInsertAt == pAnalysisState.vDeadEndAddresses.end() || *pInsertAt != nRootAddress)
                        pAnalysisState.vDeadEndAddresses.insert(pInsertAt, nRootAddress);
                }
            }
            else
            {
                static int nUnused = 0;
                nUnused++;
            }
        }

        // advance to the next combination
        gsl::index nIndex = 0;
        do
        {
            const auto& pCaptureMetrics = pAnalysisState.vCaptureMetrics.at(nIndex);
            auto pIter = vIterators.at(nIndex);
            if (++pIter < pCaptureMetrics.pStopIterator)
            {
                vIterators.at(nIndex) = pIter;
                break;
            }

            vIterators.at(nIndex) = pCaptureMetrics.pIterator;
            if (++nIndex == gsl::narrow_cast<gsl::index>(vIterators.size()))
                return;

        } while (true);
    } while (true);
}

bool PointerFinder::PointersMatch(std::vector<std::vector<PotentialPointer>::const_iterator>& vIterators) noexcept
{
    auto pIter = vIterators.begin();
    const ra::data::ByteAddress nAddress = (*pIter)->nRootAddress;

    ++pIter;
    for (; pIter < vIterators.end(); ++pIter)
    {
        const ra::data::ByteAddress nOtherAddress = (*pIter)->nRootAddress;
        if (nOtherAddress != nAddress)
            return false;
    }

    return true;
}

} // namespace services
} // namespace ra
