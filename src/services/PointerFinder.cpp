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
constexpr uint32_t MAX_DEPTH = 4;

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

void PointerFinder::InitializeBestRoutes(std::vector<PotentialPointer>& vPointers, const Capture& pCapture, ra::data::ByteAddress nTargetAddress)
{
    vPointers.reserve(pCapture.m_vPointerValues.size());

    for (const auto& pValue : pCapture.m_vPointerValues)
    {
        const int32_t nOffset = ra::to_signed(nTargetAddress) - ra::to_signed(pValue.nValueAsAddress);
        if (nOffset < MAX_OFFSET && nOffset > -MAX_OFFSET)
        {
            auto& pPointer = vPointers.emplace_back();
            pPointer.nRootAddress = pValue.nAddress;

            pPointer.vOffsets.push_back(nOffset);
            pPointer.nScore = CalculateScore(nOffset);
        }
    }

    std::sort(vPointers.begin(), vPointers.end(),
        [](const PotentialPointer& a, const PotentialPointer& b)
        {
            return a.vOffsets.front() < b.vOffsets.front();
        });
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
        InitializeBestRoutes(pCaptureMetrics.vBestRoutes, pCapture.first, pCapture.second);
    }

    if (pAnalysisState.fProgress)
    {
        pAnalysisState.nRootProgressSize = pAnalysisState.vCaptureMetrics.front().vBestRoutes.size();
        pAnalysisState.nMaxProgress = pAnalysisState.nRootProgressSize * (pAnalysisState.nRootProgressSize + 1);
        pAnalysisState.fProgress(0, pAnalysisState.nMaxProgress);
    }

    GetPointers(vIndirectNodes, pAnalysisState);
}

void PointerFinder::GetPointers(std::vector<PotentialPointer>& vIndirectNodes, AnalysisState& pAnalysisState)
{
    for (auto& pCaptureMetrics : pAnalysisState.vCaptureMetrics)
        pCaptureMetrics.pIterator = pCaptureMetrics.vBestRoutes.cbegin();

    const size_t nProgressStart = pAnalysisState.nProgress;
    do
    {
        if (!FindSharedOffset(pAnalysisState.vCaptureMetrics))
            break;

        ProcessSharedOffset(vIndirectNodes, pAnalysisState);

        for (auto& pCaptureMetrics : pAnalysisState.vCaptureMetrics)
            pCaptureMetrics.pIterator = pCaptureMetrics.pStopIterator;

        if (pAnalysisState.fProgress && pAnalysisState.vOffsets.size() < 2)
        {
            if (pAnalysisState.vOffsets.size() == 1)
                pAnalysisState.nProgress = nProgressStart + (pAnalysisState.vCaptureMetrics.front().pIterator - pAnalysisState.vCaptureMetrics.front().vBestRoutes.cbegin());
            else
                pAnalysisState.nProgress++;

            pAnalysisState.fProgress(pAnalysisState.nProgress, pAnalysisState.nMaxProgress);
        }
    } while (true);

    if (pAnalysisState.fProgress && pAnalysisState.vOffsets.size() < 2)
    {
        if (pAnalysisState.vOffsets.size() == 1)
            pAnalysisState.nProgress = nProgressStart + pAnalysisState.nRootProgressSize;
        else
            pAnalysisState.nProgress++;

        pAnalysisState.fProgress(pAnalysisState.nProgress, pAnalysisState.nMaxProgress);
    }
}

bool PointerFinder::FindSharedOffset(std::vector<CaptureMetrics>& vCaptureMetrics) noexcept
{
    int32_t nMaxOffset = std::numeric_limits<int>::min();
    for (auto& pCaptureMetric : vCaptureMetrics)
    {
        if (pCaptureMetric.pIterator == pCaptureMetric.vBestRoutes.cend())
            return false;

        nMaxOffset = std::max(nMaxOffset, pCaptureMetric.pIterator->vOffsets.front());
    }

    bool bRepeat;
    do
    {
        bRepeat = false;
        for (auto& pCaptureMetrics : vCaptureMetrics)
        {
            const auto pStop = pCaptureMetrics.vBestRoutes.cend();

            auto pIter = pCaptureMetrics.pIterator;
            if (pIter->vOffsets.front() < nMaxOffset)
            {
                do {
                    if (++pIter == pStop)
                        return false;
                } while (pIter->vOffsets.front() < nMaxOffset);
            }

            if (pIter->vOffsets.front() > nMaxOffset)
            {
                nMaxOffset = pIter->vOffsets.front();
                bRepeat = true;
                break;
            }

            pCaptureMetrics.pIterator = pIter;

            do {
                ++pIter;
            } while (pIter < pStop && pIter->vOffsets.front() == nMaxOffset);

            pCaptureMetrics.pStopIterator = pIter;
        }
    } while (bRepeat);

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

    pNewAnalysisState.vOffsets.push_back(nOffset);
    for (const auto nChildOffset : pAnalysisState.vOffsets)
        pNewAnalysisState.vOffsets.push_back(nChildOffset);

    if (pNewAnalysisState.vOffsets.size() < MAX_DEPTH)
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
                pPointer.nScore = pNewAnalysisState.nScore;
            }
        }
        else if (!pNewAnalysisState.vCaptureMetrics.empty())
        {
            gsl::index nIndex = 0;
            for (auto& pCaptureMetric : pNewAnalysisState.vCaptureMetrics)
            {
                pCaptureMetric.vBestRoutes.clear();
                InitializeBestRoutes(pCaptureMetric.vBestRoutes, *pCaptureMetric.pCapture, vIterators.at(nIndex)->nRootAddress);
                ++nIndex;
            }

            pNewAnalysisState.nProgress = pAnalysisState.nProgress;
            GetPointers(vIndirectNodes, pNewAnalysisState);
            pAnalysisState.nProgress = pNewAnalysisState.nProgress;
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
