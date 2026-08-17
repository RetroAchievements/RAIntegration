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

constexpr uint32_t MAX_OFFSET = 1024;

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
                    vPointerValues.emplace_back(pResult.nAddress, static_cast<uint32_t>(nPointerAddress));
            }

            return true;
        });
}

uint32_t PointerFinder::Capture::GetValue(ra::data::ByteAddress nAddress) const
{
    const auto pLowerBound = std::lower_bound(m_vPointerValues.begin(), m_vPointerValues.end(), nAddress,
        [](const PointerValue& pPointerValue, uint32_t nAddress)
        {
            return pPointerValue.nAddress < nAddress;
        });
    return (pLowerBound < m_vPointerValues.end() && pLowerBound->nAddress == nAddress) ? pLowerBound->nValue : 0;
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

void PointerFinder::Analyze(std::vector<PotentialPointer>& vResults)
{
    if (m_vCaptures.empty())
        return;

    std::vector<ra::data::ByteAddress> vRootNodes;
    GetRootNodes(vRootNodes);

    GetDirectPointers(vResults, vRootNodes);

    std::sort(vResults.begin(), vResults.end(),
        [](const PotentialPointer& a, const PotentialPointer& b)
        {
            if (a.nScore == b.nScore)
                return a.nRootAddress < b.nRootAddress;

            return a.nScore < b.nScore;
        });
}

void PointerFinder::GetRootNodes(std::vector<ra::data::ByteAddress>& vRootNodes) const
{
    const auto& pFirstCapture = m_vCaptures.front().first;

    vRootNodes.reserve(pFirstCapture.m_vPointerValues.size());
    for (const auto& pPointerValue : pFirstCapture.m_vPointerValues)
        vRootNodes.emplace_back(pPointerValue.nAddress);

    for (size_t i = 1; i < m_vCaptures.size(); ++i)
    {
        auto pRootNodeAddress = vRootNodes.begin();
        const auto& vCapturePointers = m_vCaptures.at(i).first.m_vPointerValues;
        for (const auto& pCapturePointer : vCapturePointers)
        {
            if (*pRootNodeAddress < pCapturePointer.nAddress)
            {
                auto pNodeIterStart = pRootNodeAddress;
                do
                {
                    ++pRootNodeAddress;
                } while (pRootNodeAddress < vRootNodes.end() && *pRootNodeAddress < pCapturePointer.nAddress);

                pRootNodeAddress = vRootNodes.erase(pNodeIterStart, pRootNodeAddress);
                if (pRootNodeAddress == vRootNodes.end())
                    break;
            }

            if (*pRootNodeAddress == pCapturePointer.nAddress)
            {
                ++pRootNodeAddress;
                if (pRootNodeAddress == vRootNodes.end())
                    break;
            }
        }
    }
}

void PointerFinder::GetDirectPointers(std::vector<PotentialPointer>& vDirectNodes, const std::vector<ra::data::ByteAddress>& vRootNodes) const
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    for (const auto pRootNodeAddress : vRootNodes)
    {
        int32_t nFirstOffset = 0;
        bool bFirst = true;
        bool bMatch = true;
        for (const auto& pCapture : m_vCaptures)
        {
            const auto nValue = pCapture.first.GetValue(pRootNodeAddress);
            const auto nPointedAtAddress = pConsoleContext.ByteAddressFromRealAddress(nValue);
            const auto nOffset = ra::to_signed(pCapture.second) - ra::to_signed(nPointedAtAddress);

            if (bFirst)
            {
                nFirstOffset = nOffset;
                bFirst = false;
            }
            else if (nOffset != nFirstOffset)
            {
                bMatch = false;
                break;
            }
        }

        if (bMatch)
        {
            auto& pDirectNode = vDirectNodes.emplace_back();
            pDirectNode.nRootAddress = pRootNodeAddress;
            pDirectNode.vOffsets.push_back(nFirstOffset);

            // treat negative offsets as being farther away
            pDirectNode.nScore = ra::to_unsigned((nFirstOffset < 0) ? (-nFirstOffset * 8) : nFirstOffset);
        }
    }
}



} // namespace services
} // namespace ra
