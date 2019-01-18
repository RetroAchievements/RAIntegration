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

void AchievementRuntime::ActivateAchievement(unsigned int nId, rc_trigger_t* pTrigger) noexcept
{
    if (m_bPaused)
    {
        // If processing is paused, just queue the achievement. Once processing is unpaused, if the trigger
        // is not false, the achievement will be moved to the active list. This ensures achievements don't
        // trigger immediately upon loading the game due to uninitialized memory.
        GSL_SUPPRESS_F6 AddEntry(m_vQueuedAchievements, nId, pTrigger);
        RemoveEntry(m_vActiveAchievements, nId);
    }
    else
    {
        GSL_SUPPRESS_F6 AddEntry(m_vActiveAchievements, nId, pTrigger);
        RemoveEntry(m_vQueuedAchievements, nId);
    }

    RemoveEntry(m_vActiveAchievementsMonitorReset, nId);
}

void AchievementRuntime::ResetActiveAchievements()
{
    // Reset any active achievements and move them to the pending queue, where they'll stay as long as the
    // trigger is active. This ensures they don't trigger while we're resetting.
    for (const auto& pAchievement : m_vActiveAchievements)
    {
        rc_reset_trigger(pAchievement.pTrigger);
        AddEntry(m_vQueuedAchievements, pAchievement.nId, pAchievement.pTrigger);
    }

    m_vActiveAchievements.clear();
}

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

_Use_decl_annotations_ void AchievementRuntime::Process(std::vector<Change>& changes)
{
    if (m_bPaused)
        return;

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

    auto pIter = m_vQueuedAchievements.begin();
    while (pIter != m_vQueuedAchievements.end())
    {
        const bool bResult = rc_test_trigger(pIter->pTrigger, rc_peek_callback, nullptr, nullptr);
        if (bResult)
        {
            // trigger is active, ignore the achievement for now. reset it so it can't pause itself.
            rc_reset_trigger(pIter->pTrigger);
            ++pIter;
        }
        else
        {
            // trigger is not active, reset the achievement and allow it to be triggered on future frames
            rc_reset_trigger(pIter->pTrigger);
            AddEntry(m_vActiveAchievements, pIter->nId, pIter->pTrigger);
            pIter = m_vQueuedAchievements.erase(pIter);
        }
    }
}

static void ProcessStateString(Tokenizer& pTokenizer, unsigned int nId, rc_trigger_t* pTrigger,
                               const std::string& sSalt, const std::string& sMemString)
{
    std::vector<rc_condition_t> vConditions;
    const size_t nStart = pTokenizer.CurrentPosition();

    // each group appears as an entry for the current nId
    while (!pTokenizer.EndOfString())
    {
        const size_t nPosition = pTokenizer.CurrentPosition();
        const unsigned int nGroupId = pTokenizer.ReadNumber();
        if (nGroupId != nId || pTokenizer.PeekChar() != ':')
        {
            pTokenizer.Seek(nPosition);
            break;
        }
        pTokenizer.Advance();

        const unsigned int nNumCond = pTokenizer.ReadNumber();
        pTokenizer.Advance();
        vConditions.reserve(vConditions.size() + nNumCond);

        for (size_t i = 0; i < nNumCond; ++i)
        {
            // Parse next condition state
            rc_condition_t& cond = vConditions.emplace_back();
            cond.current_hits = pTokenizer.ReadNumber();
            pTokenizer.Advance();
            cond.operand1.value = pTokenizer.ReadNumber();
            pTokenizer.Advance();
            cond.operand1.previous = pTokenizer.ReadNumber();
            pTokenizer.Advance();
            cond.operand2.value = pTokenizer.ReadNumber();
            pTokenizer.Advance();
            cond.operand2.previous = pTokenizer.ReadNumber();
            pTokenizer.Advance();
        }
    }

    const size_t nEnd = pTokenizer.CurrentPosition();

    // read the given md5s
    std::string sGivenMD5Progress = pTokenizer.ReadTo(':');
    pTokenizer.Advance();
    std::string sGivenMD5Achievement = pTokenizer.ReadTo(':');
    pTokenizer.Advance();

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
            const char* pStart = pTokenizer.GetPointer(nStart);
            std::string sModifiedProgressString =
                ra::StringPrintf("%s%.*s%s%u", sSalt, nEnd - nStart, pStart, sSalt, nId);
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
}

bool AchievementRuntime::LoadProgress(const char* sLoadStateFilename) const
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

    Tokenizer pTokenizer(sContents);

    while (!pTokenizer.EndOfString())
    {
        const unsigned int nId = pTokenizer.PeekNumber();
        vProcessedAchievementIds.insert(nId);

        rc_trigger_t* pTrigger = nullptr;
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
            ProcessStateString(pTokenizer, nId, nullptr, sEmpty, sEmpty);
        }
        else
        {
            const auto* pAchievement = pGameContext.FindAchievement(nId);
            std::string sMemString = pAchievement ? pAchievement->CreateMemString() : "";
            ProcessStateString(pTokenizer, nId, pTrigger, pUserContext.GetUsername(), sMemString);
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

static std::string CreateStateString(unsigned int nId, rc_trigger_t* pTrigger, const std::string& sSalt,
                                     const std::string& sMemString)
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
            oss << pCondition->current_hits << ':' << pCondition->operand1.value << ':' << pCondition->operand1.previous
                << ':' << pCondition->operand2.value << ':' << pCondition->operand2.previous << ':';

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

void AchievementRuntime::SaveProgress(const char* sSaveStateFilename) const
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
        std::string sProgress =
            CreateStateString(pAchievement.nId, pAchievement.pTrigger, pUserContext.GetUsername(), sMemString);
        pFile->Write(sProgress);
    }

    for (const auto& pAchievement : m_vActiveAchievementsMonitorReset)
    {
        std::string sMemString = pGameContext.FindAchievement(pAchievement.nId)->CreateMemString();
        std::string sProgress =
            CreateStateString(pAchievement.nId, pAchievement.pTrigger, pUserContext.GetUsername(), sMemString);
        pFile->Write(sProgress);
    }
}

} // namespace services
} // namespace ra
