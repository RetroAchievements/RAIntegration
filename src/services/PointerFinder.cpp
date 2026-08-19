#include "PointerFinder.hh"

#include "RA_Defs.h"

#include "context\IConsoleContext.hh"

#include "services\ServiceLocator.hh"

#include "util\Strings.hh"

#include "ui\IDesktop.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

/* General algorithm:
 * - For each of 2 or more snapshots, identify any memory that looks like a pointer
 * - For a given target address:
 *   - For each snapshot:
 *     - Examine the memory at the address pointed to by each of the captured pointers
 *       and calculate the offset from that address to the target address.
 *     - Keep any pointers that point to an address within MAX_OFFSET bytes of the target address.
 *   - Cross-reference the snapshots to find an offset to the target address that is
 *     present in all snapshots.
 *     - Examine all pointers that point to an address that is exactly that many bytes
 *       away from the target address.
 *     - If all snapshots contain a match at the same address, a pointer was successfully found.
 *     - Otherwise, repeat the process looking for the target address from the first snapshot.
 *       - If one is found, check the other snapshots to see if that address points to the
 *         target addresses from those snapshots.
 *         - If they do match, a pointer chain was found.
 *       - This process can be repeated recursively up to MAX_DEPTH
 *     - This process can be repeated for any other offsets that are present in all snapshots.
 */

namespace ra {
namespace services {

constexpr int32_t MAX_OFFSET = 8192;

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

void PointerFinder::Analyze(std::vector<PotentialPointer>& vResults, std::function<bool(size_t, size_t)> fProgress)
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

void PointerFinder::GetPointers(std::vector<PotentialPointer>& vIndirectNodes, std::function<bool(size_t, size_t)> fProgress) const
{
    AnalysisProgress pProgress;
    pProgress.fProgress = fProgress;
    pProgress.vResults = &vIndirectNodes;

    AnalysisState pAnalysisState;
    pAnalysisState.pProgress = &pProgress;

    for (const auto& pCapture : m_vCaptures)
    {
        auto& pCaptureMetrics = pAnalysisState.vCaptureMetrics.emplace_back();
        pCaptureMetrics.pCapture = &pCapture.first;
        InitializePotentialPointers(pCaptureMetrics.vPotentialPointers, pCapture.first, pCapture.second);
    }

    RemoveUnsharedOffsets(pAnalysisState.vCaptureMetrics);

    if (pProgress.fProgress)
    {
        constexpr size_t nMinProgress = 1; // std::max cannot deduce type of literal
        pProgress.nRootProgressSize = pAnalysisState.vCaptureMetrics.front().vPotentialPointers.size();
        pProgress.nMaxProgress = std::max(nMinProgress, pProgress.nRootProgressSize * (pProgress.nRootProgressSize + 1));
        pProgress.fProgress(0, pProgress.nMaxProgress);
    }

    AnalyzeState(pAnalysisState);
}

void PointerFinder::AnalyzeState(AnalysisState& pAnalysisState)
{
    for (auto& pCaptureMetrics : pAnalysisState.vCaptureMetrics)
        pCaptureMetrics.pIterator = pCaptureMetrics.vPotentialPointers.cbegin();

    const size_t nProgressStart = pAnalysisState.pProgress->nProgress;
    do
    {
        if (!FindSharedOffset(pAnalysisState.vCaptureMetrics))
            break;

        ProcessSharedOffset(pAnalysisState);
        if (pAnalysisState.pProgress->bAborted)
            return;

        for (auto& pCaptureMetrics : pAnalysisState.vCaptureMetrics)
            pCaptureMetrics.pIterator = pCaptureMetrics.pStopIterator;

        if (pAnalysisState.pProgress->fProgress)
        {
            if (pAnalysisState.nDepth < 2)
            {
                const auto nItemsProcessed = (pAnalysisState.vCaptureMetrics.front().pIterator - pAnalysisState.vCaptureMetrics.front().vPotentialPointers.cbegin());
                if (pAnalysisState.nDepth == 1)
                    pAnalysisState.pProgress->nProgress = nProgressStart + nItemsProcessed;
                else
                    pAnalysisState.pProgress->nProgress = nProgressStart + nItemsProcessed * (pAnalysisState.pProgress->nRootProgressSize + 1);
            }

            if (++pAnalysisState.pProgress->nChecksSinceLastProgressUpdated > 500)
            {
                pAnalysisState.pProgress->nChecksSinceLastProgressUpdated = 0;

                if (!pAnalysisState.pProgress->fProgress(pAnalysisState.pProgress->nProgress, pAnalysisState.pProgress->nMaxProgress))
                {
                    pAnalysisState.pProgress->bAborted = true;
                    return;
                }
            }
        }
    } while (true);

    if (pAnalysisState.pProgress->fProgress && pAnalysisState.nDepth < 2)
    {
        if (pAnalysisState.nDepth == 1)
            pAnalysisState.pProgress->nProgress = nProgressStart + pAnalysisState.pProgress->nRootProgressSize;
        else
            pAnalysisState.pProgress->nProgress++;

        if (!pAnalysisState.pProgress->fProgress(pAnalysisState.pProgress->nProgress, pAnalysisState.pProgress->nMaxProgress))
            pAnalysisState.pProgress->bAborted = true;
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

void PointerFinder::ProcessSharedOffset(AnalysisState& pAnalysisState)
{
    std::vector<std::vector<PotentialPointer>::const_iterator> vIterators;
    vIterators.reserve(pAnalysisState.vCaptureMetrics.size());
    for (const auto& pCaptureMetrics : pAnalysisState.vCaptureMetrics)
        vIterators.push_back(pCaptureMetrics.pIterator);

    const auto nOffset = vIterators.front()->vOffsets.front();
    const auto nDepth = pAnalysisState.nDepth + 1;
    auto* nOffsetFront = &pAnalysisState.pProgress->vOffsets.at(MAX_DEPTH - nDepth);
    *nOffsetFront = nOffset;

    AnalysisState pNewAnalysisState;
    pNewAnalysisState.nScore = pAnalysisState.nScore + CalculateScore(nOffset);

    if (nDepth < MAX_DEPTH)
    {
        pNewAnalysisState.nDepth = nDepth;
        pNewAnalysisState.pProgress = pAnalysisState.pProgress;

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
            // found a route. check to see if another route exists
            const auto nRootAddress = vIterators.front()->nRootAddress;
            bool bFound = false;
            for (auto& pPointer : *pAnalysisState.pProgress->vResults)
            {
                if (pPointer.nRootAddress == nRootAddress)
                {
                    // this root pointer was already captured. only keep this
                    // route if it's more efficient (lower score).
                    if (pNewAnalysisState.nScore < pPointer.nScore)
                    {
                        pPointer.nScore = pNewAnalysisState.nScore;
                        memcpy(&pPointer.vOffsets.front(), nOffsetFront, nDepth * sizeof(pPointer.vOffsets[0]));
                        pPointer.nOffsetLength = nDepth;
                    }

                    bFound = true;
                    break;
                }
            }

            // no route exists for this address, add one.
            if (!bFound)
            {
                auto& pPointer = pAnalysisState.pProgress->vResults->emplace_back();
                pPointer.nRootAddress = nRootAddress;
                memcpy(&pPointer.vOffsets.front(), nOffsetFront, nDepth * sizeof(pPointer.vOffsets[0]));
                pPointer.nOffsetLength = nDepth;
                pPointer.nScore = pNewAnalysisState.nScore;
            }
        }
        else if (nDepth < MAX_DEPTH)
        {
            // not a direct reference. check to see if there's a valid chain pointing at the current pointers
            const auto nRootAddress = vIterators.front()->nRootAddress;

            auto& vUnreachableAddresses = pAnalysisState.pProgress->vUnreachableAddresses;
            if (std::binary_search(vUnreachableAddresses.begin(), vUnreachableAddresses.end(), nRootAddress))
            {
                // this pointer was previously examined and did not share an offset with any other pointers
                // so it cannot be a valid link in the route.
            }
            else
            {
                auto& vDeadEndAddresses = pAnalysisState.pProgress->vDeadEndAddresses.at(nDepth);
                if (std::binary_search(vDeadEndAddresses.begin(), vDeadEndAddresses.end(), nRootAddress))
                {
                    // this pointer was previously exhaustively examined at this depth
                    // and no routes were found so it is not a valid link in the route.
                }
                else
                {
                    // find any pointers pointing to within MAX_OFFSET bytes of the target address
                    gsl::index nIndex = 0;
                    for (auto& pCaptureMetric : pNewAnalysisState.vCaptureMetrics)
                    {
                        pCaptureMetric.vPotentialPointers.clear();
                        InitializePotentialPointers(pCaptureMetric.vPotentialPointers, *pCaptureMetric.pCapture, vIterators.at(nIndex)->nRootAddress);
                        ++nIndex;
                    }

                    // narrow the list down to offsets that occur in all captures.
                    RemoveUnsharedOffsets(pNewAnalysisState.vCaptureMetrics);

                    if (pNewAnalysisState.vCaptureMetrics.front().vPotentialPointers.empty())
                    {
                        // if all offsets were unique, this address cannot be reached from any
                        // captured pointers. mark it as unreachable so we don't try again in the future
                        const auto pInsertAt = std::lower_bound(vUnreachableAddresses.begin(), vUnreachableAddresses.end(), nRootAddress);
                        if (pInsertAt == vUnreachableAddresses.end() || *pInsertAt != nRootAddress)
                            vUnreachableAddresses.insert(pInsertAt, nRootAddress);
                    }
                    else
                    {
                        const auto nFoundPointers = pAnalysisState.pProgress->vResults->size();

                        AnalyzeState(pNewAnalysisState);

                        if (pAnalysisState.pProgress->vResults->size() == nFoundPointers)
                        {
                            // no path to the target address was found, mark it as a dead end
                            // at this depth so we don't try to process it again.
                            const auto pInsertAt = std::lower_bound(vDeadEndAddresses.begin(), vDeadEndAddresses.end(), nRootAddress);
                            if (pInsertAt == vDeadEndAddresses.end() || *pInsertAt != nRootAddress)
                                vDeadEndAddresses.insert(pInsertAt, nRootAddress);
                        }
                    }
                }
            }
        }

        if (pAnalysisState.pProgress->bAborted)
            break;

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
