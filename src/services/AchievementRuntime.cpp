#include "AchievementRuntime.hh"

#include "RA_Log.h"
#include "RA_MemManager.h"
#include "RA_StringUtils.h"

#include "RA_md5factory.h"

#include "data\GameContext.hh"
#include "data\UserContext.hh"

#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {

static constexpr bool HasHitCounts(const rc_condset_t* pCondSet) noexcept
{
    rc_condition_t* condition = pCondSet->conditions;
    while (condition != nullptr)
    {
        if (condition->current_hits)
            return true;

        condition = condition->next;
    }

    return false;
}

static constexpr bool HasHitCounts(const rc_trigger_t* pTrigger) noexcept
{
    if (HasHitCounts(pTrigger->requirement))
        return true;

    rc_condset_t* pAlternate = pTrigger->alternative;
    while (pAlternate != nullptr)
    {
        if (HasHitCounts(pAlternate))
            return true;

        pAlternate = pAlternate->next;
    }

    return false;
}

void AchievementRuntime::Process(std::vector<Change>& changes) const noexcept
{
    for (auto& pAchievement : m_vActiveAchievements)
    {
        const bool bResult = rc_test_trigger(pAchievement.pTrigger, rc_peek_callback, nullptr, nullptr);
        if (bResult)
            changes.emplace_back(Change{ChangeType::AchievementTriggered, pAchievement.nId});
    }

    for (auto& pAchievement : m_vActiveAchievementsMonitorReset)
    {
        const bool bHasHits = HasHitCounts(pAchievement.pTrigger);

        const bool bResult = rc_test_trigger(pAchievement.pTrigger, rc_peek_callback, nullptr, nullptr);
        if (bResult)
            changes.emplace_back(Change{ChangeType::AchievementTriggered, pAchievement.nId});
        else if (bHasHits && !HasHitCounts(pAchievement.pTrigger))
            changes.emplace_back(Change{ChangeType::AchievementReset, pAchievement.nId});
    }
}

static const char* ProcessStateString(const char* pIter, unsigned int nId, rc_trigger_t* pTrigger, const std::string& sSalt, const std::string& sMemString)
{
    std::vector<rc_condition_t> vConditions;
    const char* pStart = pIter;

    // each group appears as an entry for the current nId
    while (*pIter)
    {
        char* pEnd;
        const unsigned int nGroupId = strtoul(pIter, &pEnd, 10);
        if (nGroupId != nId)
            break;
        pIter = pEnd + 1;

        const unsigned int nNumCond = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;
        vConditions.reserve(vConditions.size() + nNumCond);

        for (size_t i = 0; i < nNumCond; ++i)
        {
            // Parse next condition state
            const unsigned int nHits = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;
            const unsigned int nSourceVal = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;
            const unsigned int nSourcePrev = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;
            const unsigned int nTargetVal = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;
            const unsigned int nTargetPrev = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;

            rc_condition_t& cond = vConditions.emplace_back();
            cond.current_hits = nHits;
            cond.operand1.value = nSourceVal;
            cond.operand1.previous = nSourcePrev;
            cond.operand2.value = nTargetVal;
            cond.operand2.previous = nTargetPrev;
        }
    }

    const char* pConditionEnd = pIter;

    // read the given md5s
    const char* pMd5End = pIter;
    while (*pMd5End && *pMd5End != ':')
        ++pMd5End;
    std::string sGivenMD5Progress(pIter, pMd5End - pIter);
    if (*pMd5End == ':')
        pIter = ++pMd5End;

    while (*pMd5End && *pMd5End != ':')
        ++pMd5End;
    std::string sGivenMD5Achievement(pIter, pMd5End - pIter);
    if (*pMd5End == ':')
        ++pMd5End;
    pIter = pMd5End;

    if (!pTrigger)
    {
        // achievement wasn't found, ignore it. still had to parse to advance pIter
    }
    else
    {
        // recalculate the current achievement checksum
        std::string sMD5Achievement = RAGenerateMD5(sMemString);
        if (sGivenMD5Achievement != sMD5Achievement)
        {
            // achievement has been modified since data was captured, can't apply, just reset
            rc_reset_trigger(pTrigger);
        }
        else
        {
            // regenerate the md5 and see if it sticks
            std::string sModifiedProgressString = ra::StringPrintf("%s%.*s%s%u", sSalt, pConditionEnd - pStart, pStart, sSalt, nId);
            std::string sMD5Progress = RAGenerateMD5(sModifiedProgressString);
            if (sMD5Progress != sGivenMD5Progress)
            {
                // state checksum fail - assume user tried to modify it and ignore, just reset
                rc_reset_trigger(pTrigger);
            }
            else
            {
                // compatible - merge
                size_t nCondition = 0;

                rc_condset_t* pGroup = pTrigger->requirement;
                while (pGroup != nullptr)
                {
                    rc_condition_t* pCondition = pGroup->conditions;
                    while (pCondition != nullptr)
                    {
                        const rc_condition_t& condSource = vConditions.at(nCondition++);
                        pCondition->current_hits = condSource.current_hits;
                        pCondition->operand1.value = condSource.operand1.value;
                        pCondition->operand1.previous = condSource.operand1.previous;
                        pCondition->operand2.value = condSource.operand2.value;
                        pCondition->operand2.previous = condSource.operand2.previous;

                        pCondition = pCondition->next;
                    }

                    if (pGroup == pTrigger->requirement)
                        pGroup = pTrigger->alternative;
                    else
                        pGroup = pGroup->next;
                }
            }
        }
    }

    return pIter;
}

bool AchievementRuntime::LoadProgress(const char* sLoadStateFilename) const noexcept
{
    if (sLoadStateFilename == nullptr)
        return false;

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::UserContext>();
    if (!pUserContext.IsLoggedIn())
        return false;

    std::wstring sAchievementStateFile = ra::Widen(sLoadStateFilename) + L".rap";
    auto pFile = ra::services::ServiceLocator::Get<ra::services::IFileSystem>().OpenTextFile(sAchievementStateFile);
    if (pFile == nullptr)
        return false;

    std::string sContents;
    if (!pFile->GetLine(sContents))
        return false;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    std::set<unsigned int> vProcessedAchievementIds;

    const char* pIter = sContents.c_str();
    while (*pIter)
    {
        char* pUnused;
        const unsigned int nId = strtoul(pIter, &pUnused, 10);
        vProcessedAchievementIds.insert(nId);

        rc_trigger_t *pTrigger = nullptr;
        for (const auto& pActiveAchievement : m_vActiveAchievements)
        {
            if (pActiveAchievement.nId == nId)
            {
                pTrigger = pActiveAchievement.pTrigger;
                break;
            }
        }

        if (pTrigger == nullptr)
        {
            for (const auto& pActiveAchievement : m_vActiveAchievementsMonitorReset)
            {
                if (pActiveAchievement.nId == nId)
                {
                    pTrigger = pActiveAchievement.pTrigger;
                    break;
                }
            }
        }

        if (pTrigger == nullptr)
        {
            // achievement not active, still have to process state string to skip over it
            std::string sEmpty;
            pIter = ProcessStateString(pIter, nId, nullptr, sEmpty, sEmpty);
        }
        else
        {
            auto* pAchievement = pGameContext.FindAchievement(nId);
            std::string sMemString = pAchievement ? pAchievement->CreateMemString() : "";
            pIter = ProcessStateString(pIter, nId, pTrigger, pUserContext.GetUsername(), sMemString);
        }
    }

    // reset any active achievements that weren't in the file
    for (auto& pActiveAchievement : m_vActiveAchievements)
    {
        if (vProcessedAchievementIds.find(pActiveAchievement.nId) == vProcessedAchievementIds.end())
            rc_reset_trigger(pActiveAchievement.pTrigger);
    }

    for (auto& pActiveAchievement : m_vActiveAchievementsMonitorReset)
    {
        if (vProcessedAchievementIds.find(pActiveAchievement.nId) == vProcessedAchievementIds.end())
            rc_reset_trigger(pActiveAchievement.pTrigger);
    }

    return true;
}

static std::string CreateStateString(unsigned int nId, rc_trigger_t* pTrigger, const std::string& sSalt, const std::string& sMemString)
{
    // build the progress string
    std::ostringstream oss;

    rc_condset_t* pGroup = pTrigger->requirement;
    while (pGroup != nullptr)
    {
        size_t nGroups = 0;
        rc_condition_t* pCondition = pGroup->conditions;
        while (pCondition)
        {
            ++nGroups;
            pCondition = pCondition->next;
        }
        oss << nId << ':' << nGroups << ':';

        pCondition = pGroup->conditions;
        while (pCondition != nullptr)
        {
            oss << pCondition->current_hits << ':'
                << pCondition->operand1.value << ':' << pCondition->operand1.previous << ':'
                << pCondition->operand2.value << ':' << pCondition->operand2.previous << ':';

            pCondition = pCondition->next;
        }

        if (pGroup == pTrigger->requirement)
            pGroup = pTrigger->alternative;
        else
            pGroup = pGroup->next;
    }

    std::string sProgressString = oss.str();

    // Checksum the progress string (including the salt value)
    std::string sModifiedProgressString = ra::StringPrintf("%s%s%s%u", sSalt, sProgressString, sSalt, nId);
    std::string sMD5Progress = RAGenerateMD5(sModifiedProgressString);

    sProgressString.append(sMD5Progress);
    sProgressString.push_back(':');

    // Also checksum the achievement string itself
    std::string sMD5Achievement = RAGenerateMD5(sMemString);
    sProgressString.append(sMD5Achievement);
    sProgressString.push_back(':');

    return sProgressString;
}

void AchievementRuntime::SaveProgress(const char* sSaveStateFilename) const noexcept
{
    if (sSaveStateFilename == nullptr)
        return;

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::UserContext>();
    if (!pUserContext.IsLoggedIn())
        return;

    std::wstring sAchievementStateFile = ra::Widen(sSaveStateFilename) + L".rap";
    auto pFile = ra::services::ServiceLocator::Get<ra::services::IFileSystem>().CreateTextFile(sAchievementStateFile);
    if (pFile == nullptr)
    {
        RA_LOG_WARN("Could not write %s", sAchievementStateFile);
        return;
    }

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    for (const auto& pAchievement : m_vActiveAchievements)
    {
        std::string sMemString = pGameContext.FindAchievement(pAchievement.nId)->CreateMemString();
        std::string sProgress = CreateStateString(pAchievement.nId, pAchievement.pTrigger, pUserContext.GetUsername(), sMemString);
        pFile->Write(sProgress);
    }

    for (const auto& pAchievement : m_vActiveAchievementsMonitorReset)
    {
        std::string sMemString = pGameContext.FindAchievement(pAchievement.nId)->CreateMemString();
        std::string sProgress = CreateStateString(pAchievement.nId, pAchievement.pTrigger, pUserContext.GetUsername(), sMemString);
        pFile->Write(sProgress);
    }
}

} // namespace services
} // namespace ra
