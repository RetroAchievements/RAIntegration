#include "AchievementRuntime.hh"

#include "RA_Defs.h"
#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "RA_md5factory.h"

#include "data\EmulatorContext.hh"
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

void AchievementRuntime::MonitorAchievementReset(unsigned int nId, rc_trigger_t* pTrigger) noexcept
{
    if (m_bPaused)
    {
        // If processing is paused, just queue the achievement. Once processing is unpaused, if the trigger
        // is not false, the achievement will be moved to the active list. This ensures achievements don't
        // trigger immediately upon loading the game due to uninitialized memory.
        GSL_SUPPRESS_F6 AddEntry(m_vQueuedAchievements, nId, pTrigger);
        GSL_SUPPRESS_F6 m_vQueuedAchievements.back().bPauseOnReset = true;
        RemoveEntry(m_vActiveAchievementsMonitorReset, nId);
    }
    else
    {
        GSL_SUPPRESS_F6 AddEntry(m_vActiveAchievementsMonitorReset, nId, pTrigger);
        RemoveEntry(m_vQueuedAchievements, nId);
    }

    RemoveEntry(m_vActiveAchievements, nId);
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

    // make sure to also process the achievements being monitored
    for (const auto& pAchievement : m_vActiveAchievementsMonitorReset)
    {
        rc_reset_trigger(pAchievement.pTrigger);
        AddEntry(m_vQueuedAchievements, pAchievement.nId, pAchievement.pTrigger);
        m_vQueuedAchievements.back().bPauseOnReset = true;
    }

    m_vActiveAchievementsMonitorReset.clear();

    // also reset leaderboards
    for (const auto& pLeaderboard : m_vActiveLeaderboards)
    {
        rc_reset_lboard(pLeaderboard.pLeaderboard);

        // this ensures the leaderboard won't restart until the start trigger is false for at least one frame
        pLeaderboard.pLeaderboard->submitted = 1;
    }
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
            changes.emplace_back(Change{ChangeType::AchievementTriggered, pAchievement.nId, 0U});
    }

    for (auto& pAchievement : m_vActiveAchievementsMonitorReset)
    {
        const bool bHasHits = HasHitCounts(pAchievement.pTrigger);

        const bool bResult = rc_test_trigger(pAchievement.pTrigger, rc_peek_callback, nullptr, nullptr);
        if (bResult)
            changes.emplace_back(Change{ChangeType::AchievementTriggered, pAchievement.nId, 0U});
        else if (bHasHits && !HasHitCounts(pAchievement.pTrigger))
            changes.emplace_back(Change{ChangeType::AchievementReset, pAchievement.nId, 0U});
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

            if (pIter->bPauseOnReset)
                AddEntry(m_vActiveAchievementsMonitorReset, pIter->nId, pIter->pTrigger);
            else
                AddEntry(m_vActiveAchievements, pIter->nId, pIter->pTrigger);

            pIter = m_vQueuedAchievements.erase(pIter);
        }
    }

    for (auto& pLeaderboard : m_vActiveLeaderboards)
    {
        int nValue;
        const int nResult = rc_evaluate_lboard(pLeaderboard.pLeaderboard, &nValue, rc_peek_callback, nullptr, nullptr);
        switch (nResult)
        {
            default:
            case RC_LBOARD_INACTIVE:
                break;

            case RC_LBOARD_ACTIVE:
                if (pLeaderboard.nValue != nValue)
                {
                    pLeaderboard.nValue = nValue;
                    changes.emplace_back(Change{ChangeType::LeaderboardUpdated, pLeaderboard.nId, nValue});
                }
                break;

            case RC_LBOARD_STARTED:
                pLeaderboard.nValue = nValue;
                changes.emplace_back(Change{ChangeType::LeaderboardStarted, pLeaderboard.nId, nValue});
                break;

            case RC_LBOARD_CANCELED:
                changes.emplace_back(Change{ChangeType::LeaderboardCanceled, pLeaderboard.nId, 0U});
                break;

            case RC_LBOARD_TRIGGERED:
                pLeaderboard.nValue = nValue;
                changes.emplace_back(Change{ChangeType::LeaderboardTriggered, pLeaderboard.nId, nValue});
                break;
        }
    }

    UpdateRichPresenceMemRefs();
}

void AchievementRuntime::UpdateRichPresenceMemRefs() noexcept
{
    if (m_pRichPresenceMemRefs)
    {
        // rc_update_memrefs is not exposed, create a dummy trigger with no conditions to update the memrefs
        rc_trigger_t trigger{};
        trigger.memrefs = m_pRichPresenceMemRefs;
        rc_test_trigger(&trigger, rc_peek_callback, nullptr, nullptr);
    }
}

_NODISCARD static _CONSTANT_FN ComparisonSizeToPrefix(_In_ char nSize) noexcept
{
    switch (nSize)
    {
        case RC_MEMSIZE_BIT_0:        return "M";
        case RC_MEMSIZE_BIT_1:        return "N";
        case RC_MEMSIZE_BIT_2:        return "O";
        case RC_MEMSIZE_BIT_3:        return "P";
        case RC_MEMSIZE_BIT_4:        return "Q";
        case RC_MEMSIZE_BIT_5:        return "R";
        case RC_MEMSIZE_BIT_6:        return "S";
        case RC_MEMSIZE_BIT_7:        return "T";
        case RC_MEMSIZE_LOW:          return "L";
        case RC_MEMSIZE_HIGH:         return "U";
        case RC_MEMSIZE_8_BITS:       return "H";
        case RC_MEMSIZE_24_BITS:      return "W";
        case RC_MEMSIZE_32_BITS:      return "X";
        default:
        case RC_MEMSIZE_16_BITS:      return " ";
    }
}

_NODISCARD static constexpr char ComparisonSizeFromPrefix(_In_ char cPrefix) noexcept
{
    switch (cPrefix)
    {
        case 'm': case 'M': return RC_MEMSIZE_BIT_0;
        case 'n': case 'N': return RC_MEMSIZE_BIT_1;
        case 'o': case 'O': return RC_MEMSIZE_BIT_2;
        case 'p': case 'P': return RC_MEMSIZE_BIT_3;
        case 'q': case 'Q': return RC_MEMSIZE_BIT_4;
        case 'r': case 'R': return RC_MEMSIZE_BIT_5;
        case 's': case 'S': return RC_MEMSIZE_BIT_6;
        case 't': case 'T': return RC_MEMSIZE_BIT_7;
        case 'l': case 'L': return RC_MEMSIZE_LOW;
        case 'u': case 'U': return RC_MEMSIZE_HIGH;
        case 'h': case 'H': return RC_MEMSIZE_8_BITS;
        case 'w': case 'W': return RC_MEMSIZE_24_BITS;
        case 'x': case 'X': return RC_MEMSIZE_32_BITS;
        default:
        case ' ':
            return RC_MEMSIZE_16_BITS;
    }
}

static bool IsMemoryOperand(int nOperandType) noexcept
{
    switch (nOperandType)
    {
        case RC_OPERAND_ADDRESS:
        case RC_OPERAND_DELTA:
        case RC_OPERAND_PRIOR:
            return true;

        default:
            return false;
    }
}

static void ProcessStateString(Tokenizer& pTokenizer, unsigned int nId, rc_trigger_t* pTrigger,
                               const std::string& sSalt, const std::string& sMemString)
{
    struct ConditionState
    {
        unsigned int nHits;
        unsigned int nSourceVal;
        unsigned int nSourcePrev;
        unsigned int nTargetVal;
        unsigned int nTargetPrev;
    };

    std::vector<ConditionState> vConditions;
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
            auto& cond = vConditions.emplace_back();
            cond.nHits = pTokenizer.ReadNumber();
            pTokenizer.Advance();
            cond.nSourceVal = pTokenizer.ReadNumber();
            pTokenizer.Advance();
            cond.nSourcePrev = pTokenizer.ReadNumber();
            pTokenizer.Advance();
            cond.nTargetVal = pTokenizer.ReadNumber();
            pTokenizer.Advance();
            cond.nTargetPrev = pTokenizer.ReadNumber();
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
                        const auto& condSource = vConditions.at(nCondition++);
                        pCondition->current_hits = condSource.nHits;
                        if (IsMemoryOperand(pCondition->operand1.type))
                        {
                            pCondition->operand1.value.memref->value = condSource.nSourceVal;
                            pCondition->operand1.value.memref->previous = condSource.nSourcePrev;
                        }
                        if (IsMemoryOperand(pCondition->operand2.type))
                        {
                            pCondition->operand2.value.memref->value = condSource.nTargetVal;
                            pCondition->operand2.value.memref->previous = condSource.nTargetPrev;
                        }

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

bool AchievementRuntime::LoadProgressV1(const std::string& sProgress, std::set<unsigned int>& vProcessedAchievementIds) const
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::UserContext>();

    Tokenizer pTokenizer(sProgress);

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

    return true;
}

bool AchievementRuntime::LoadProgressV2(ra::services::TextReader& pFile, std::set<unsigned int>& vProcessedAchievementIds) const
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    std::vector<rc_memref_value_t> vMemoryReferences;

    std::string sLine;
    while (pFile.GetLine(sLine))
    {
        if (sLine.empty())
            continue;

        Tokenizer tokenizer(sLine);
        const auto c = tokenizer.PeekChar();
        tokenizer.Advance();

        if (c == '$') // memory reference
        {
            rc_memref_value_t pMemRef{};

            const auto cPrefix = tokenizer.PeekChar();
            tokenizer.Advance();
            pMemRef.memref.size = ComparisonSizeFromPrefix(cPrefix);

            const auto sAddr = tokenizer.ReadTo(':');
            pMemRef.memref.address = ra::ByteAddressFromString(sAddr);

            while (tokenizer.PeekChar() != '#' && !tokenizer.EndOfString())
            {
                tokenizer.Advance();
                const auto cField = tokenizer.PeekChar();
                tokenizer.Advance(2);
                const auto nValue = tokenizer.ReadNumber();

                switch (cField)
                {
                    case 'v': pMemRef.value = nValue; break;
                    case 'd': pMemRef.previous = nValue; break;
                    case 'p': pMemRef.prior = nValue; break;
                }
            }

            if (tokenizer.PeekChar() == '#')
            {
                const auto sLineMD5 = RAGenerateMD5(sLine.substr(0, tokenizer.CurrentPosition()));
                tokenizer.Advance();
                if (tokenizer.PeekChar() == sLineMD5.at(0))
                {
                    tokenizer.Advance();
                    if (tokenizer.PeekChar() == sLineMD5.at(31))
                    {
                        // match! store it
                        vMemoryReferences.push_back(pMemRef);
                    }
                }
            }
        }
        else if (c == 'A') // achievement
        {
            const auto nId = tokenizer.ReadNumber();
            tokenizer.Advance();

            rc_trigger_t* pTrigger = nullptr;
            for (const auto& pAchievement : m_vActiveAchievements)
            {
                if (pAchievement.nId == nId)
                {
                    pTrigger = pAchievement.pTrigger;
                    break;
                }
            }
            if (pTrigger == nullptr)
            {
                for (const auto& pAchievement : m_vActiveAchievementsMonitorReset)
                {
                    if (pAchievement.nId == nId)
                    {
                        pTrigger = pAchievement.pTrigger;
                        break;
                    }
                }

                if (pTrigger == nullptr) // not active, ignore
                    continue;
            }

            const auto* pAch = pGameContext.FindAchievement(nId);
            if (pAch == nullptr)
            {
                rc_reset_trigger(pTrigger);
                continue;
            }

            const auto sChecksumMD5 = sLine.substr(tokenizer.CurrentPosition(), 32);
            tokenizer.Advance(32);

            const auto sMemString = pAch->CreateMemString();
            const auto sMemStringMD5 = RAGenerateMD5(sMemString);
            if (sMemStringMD5 != sChecksumMD5) // modified since captured
            {
                rc_reset_trigger(pTrigger);
                continue;
            }

            rc_condition_t* pCondition = nullptr;
            if (pTrigger->requirement)
            {
                pCondition = pTrigger->requirement->conditions;
                while (pCondition)
                {
                    tokenizer.Advance();
                    pCondition->current_hits = tokenizer.ReadNumber();

                    pCondition = pCondition->next;
                }
            }

            const rc_condset_t* pCondSet = pTrigger->alternative;
            while (pCondSet)
            {
                pCondition = pCondSet->conditions;
                while (pCondition)
                {
                    tokenizer.Advance();
                    pCondition->current_hits = tokenizer.ReadNumber();

                    pCondition = pCondition->next;
                }

                pCondSet = pCondSet->next;
            }

            bool bValid = (tokenizer.PeekChar() == '#');
            if (bValid)
            {
                const auto sLineMD5 = RAGenerateMD5(sLine.substr(0, tokenizer.CurrentPosition()));
                tokenizer.Advance();
                if (tokenizer.PeekChar() != sLineMD5.at(0))
                {
                    bValid = false;
                }
                else
                {
                    tokenizer.Advance();
                    bValid = (tokenizer.PeekChar() == sLineMD5.at(31));
                }

                rc_memref_value_t* pMemRef = pTrigger->memrefs;
                while (pMemRef)
                {
                    bool bFound = false;
                    for (const auto& pMemoryReference : vMemoryReferences)
                    {
                        if (pMemoryReference.memref.address == pMemRef->memref.address &&
                            pMemoryReference.memref.size == pMemRef->memref.size)
                        {
                            pMemRef->value = pMemoryReference.value;
                            pMemRef->previous = pMemoryReference.previous;
                            pMemRef->prior = pMemoryReference.prior;

                            bFound = true;
                            break;
                        }
                    }

                    if (!bFound)
                        bValid = false;

                    pMemRef = pMemRef->next;
                }
            }

            if (bValid)
                vProcessedAchievementIds.insert(nId);
            else
                rc_reset_trigger(pTrigger);
        }
    }

    return true;
}

bool AchievementRuntime::LoadProgress(const char* sLoadStateFilename) const
{
    // leaderboards aren't currently managed by the save/load progress functions, just reset them all
    for (const auto& pLeaderboard : m_vActiveLeaderboards)
        rc_reset_lboard(pLeaderboard.pLeaderboard);

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

    std::set<unsigned int> vProcessedAchievementIds;

    if (sContents.length() > 1 && sContents.at(0) == 'v')
    {
        const auto nVersion = std::stoi(sContents.substr(1));
        switch (nVersion)
        {
            case 2:
                if (!LoadProgressV2(*pFile, vProcessedAchievementIds))
                    vProcessedAchievementIds.clear();
                break;

            default:
                assert(!"Unknown persistence file version");
                return false;
        }
    }
    else
    {
        if (!LoadProgressV1(sContents, vProcessedAchievementIds))
            vProcessedAchievementIds.clear();
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

static void AddMemoryReference(std::vector<rc_memref_value_t>& vMemoryReferences, const rc_memref_value_t& pNewMemoryReference)
{
    for (const auto& pMemoryReference : vMemoryReferences)
    {
        if (pMemoryReference.memref.address == pNewMemoryReference.memref.address &&
            pMemoryReference.memref.size == pNewMemoryReference.memref.size)
        {
            return;
        }
    }

    vMemoryReferences.push_back(pNewMemoryReference);
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

    pFile->WriteLine("v2");

    // get active achievements
    std::vector<ActiveAchievement> vActiveAchievements(m_vActiveAchievements);
    for (const auto& pAchievement : m_vActiveAchievementsMonitorReset)
        vActiveAchievements.push_back(pAchievement);

    // extract memory references
    std::vector<rc_memref_value_t> vMemoryReferences;
    for (const auto& pAchievement : vActiveAchievements)
    {
        const rc_memref_value_t* pMemRef = pAchievement.pTrigger->memrefs;
        while (pMemRef)
        {
            AddMemoryReference(vMemoryReferences, *pMemRef);
            pMemRef = pMemRef->next;
        }
    }

    // write memory references
    for (const auto& pMemoryReference : vMemoryReferences)
    {
        auto sLine = ra::StringPrintf("$%s%s:v=%d;d=%d;p=%d", ComparisonSizeToPrefix(pMemoryReference.memref.size),
            ra::ByteAddressToString(pMemoryReference.memref.address), pMemoryReference.value, pMemoryReference.previous, pMemoryReference.prior);

        const auto sLineMD5 = RAGenerateMD5(sLine);
        sLine.push_back('#');
        sLine.push_back(sLineMD5.at(0));
        sLine.push_back(sLineMD5.at(31));
        pFile->WriteLine(sLine);
    }

    // write hitcounts
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    for (const auto& pAchievement : vActiveAchievements)
    {
        const auto* pAch = pGameContext.FindAchievement(pAchievement.nId);
        if (pAch == nullptr)
            continue;

        const auto sMemString = pAch->CreateMemString();
        const auto sMemStringMD5 = RAGenerateMD5(sMemString);
        auto sLine = ra::StringPrintf("A%d:%s:", pAchievement.nId, sMemStringMD5);

        const rc_condition_t* pCondition = nullptr;
        if (pAchievement.pTrigger->requirement)
        {
            pCondition = pAchievement.pTrigger->requirement->conditions;
            while (pCondition)
            {
                sLine.append(std::to_string(pCondition->current_hits));
                sLine.push_back(',');

                pCondition = pCondition->next;
            }
        }

        const rc_condset_t* pCondSet = pAchievement.pTrigger->alternative;
        while (pCondSet)
        {
            pCondition = pCondSet->conditions;
            while (pCondition)
            {
                sLine.append(std::to_string(pCondition->current_hits));
                sLine.push_back(',');

                pCondition = pCondition->next;
            }

            pCondSet = pCondSet->next;
        }

        sLine.pop_back();

        const auto sLineMD5 = RAGenerateMD5(sLine);
        sLine.push_back('#');
        sLine.push_back(sLineMD5.at(0));
        sLine.push_back(sLineMD5.at(31));
        pFile->WriteLine(sLine);
    }
}

} // namespace services
} // namespace ra

extern "C" unsigned int rc_peek_callback(unsigned int nAddress, unsigned int nBytes, _UNUSED void* pData)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    switch (nBytes)
    {
        case 1:
            return pEmulatorContext.ReadMemoryByte(nAddress);
        case 2:
            return pEmulatorContext.ReadMemory(nAddress, MemSize::SixteenBit);
        case 4:
            return pEmulatorContext.ReadMemory(nAddress, MemSize::ThirtyTwoBit);
        default:
            return 0U;
    }
}
