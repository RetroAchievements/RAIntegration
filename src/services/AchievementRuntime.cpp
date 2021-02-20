#include "AchievementRuntime.hh"

#include "RA_Defs.h"
#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "RA_md5factory.h"

#include "data\context\EmulatorContext.hh"
#include "data\context\GameContext.hh"
#include "data\context\UserContext.hh"

#include "services\IFileSystem.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

#include <rcheevos\src\rhash\md5.h>

namespace ra {
namespace services {

void AchievementRuntime::EnsureInitialized() noexcept
{
    if (!m_bInitialized)
    {
        rc_runtime_init(&m_pRuntime);
        m_bInitialized = true;
    }
}

void AchievementRuntime::ResetRuntime() noexcept
{
    if (m_bInitialized)
    {
        rc_runtime_destroy(&m_pRuntime);
        m_bInitialized = false;
    }
}

int AchievementRuntime::ActivateAchievement(ra::AchievementID nId, const std::string& sTrigger)
{
    std::lock_guard<std::mutex> pLock(m_pMutex);
    EnsureInitialized();

    if (m_bPaused)
    {
        // If processing is paused, just queue the achievement. Once processing is unpaused, if the trigger
        // is not false, the achievement will be moved to the active list. This ensures achievements don't
        // trigger immediately upon loading the game due to uninitialized memory.
        m_vQueuedAchievements.insert_or_assign(nId, sTrigger);
        rc_runtime_deactivate_achievement(&m_pRuntime, nId);
        return RC_OK;
    }

    return rc_runtime_activate_achievement(&m_pRuntime, nId, sTrigger.c_str(), nullptr, 0);
}

rc_trigger_t* AchievementRuntime::GetAchievementTrigger(ra::AchievementID nId, const std::string& sTrigger)
{
    if (!m_bInitialized)
        return nullptr;

    unsigned char md5[16]{};
    md5_state_t state{};
    const md5_byte_t* bytes;
    GSL_SUPPRESS_TYPE1 bytes = reinterpret_cast<const md5_byte_t*>(sTrigger.c_str());

    md5_init(&state);
    md5_append(&state, bytes, gsl::narrow_cast<int>(sTrigger.length()));
    md5_finish(&state, md5);

    std::lock_guard<std::mutex> pLock(m_pMutex);
    for (size_t i = 0; i < m_pRuntime.trigger_count; ++i)
    {
        if (m_pRuntime.triggers[i].id == nId && memcmp(m_pRuntime.triggers[i].md5, md5, sizeof(md5)) == 0)
        {
            if (m_pRuntime.triggers[i].trigger)
                return m_pRuntime.triggers[i].trigger;

            return static_cast<rc_trigger_t*>(m_pRuntime.triggers[i].buffer);
        }
    }

    return nullptr;
}

void AchievementRuntime::DeactivateAchievement(ra::AchievementID nId)
{
    // NOTE: rc_runtime_deactivate_achievement will either reset the trigger or free it
    // we want to keep it so the user can examine the hit counts after an incorrect trigger
    // so we're just going to detach it (memory is still held by m_pRuntime.triggers[i].buffer
    DetachAchievementTrigger(nId);
}

rc_trigger_t* AchievementRuntime::DetachAchievementTrigger(ra::AchievementID nId)
{
    if (!m_bInitialized)
        return nullptr;

    std::lock_guard<std::mutex> pLock(m_pMutex);
    for (unsigned i = 0; i < m_pRuntime.trigger_count; ++i)
    {
        if (m_pRuntime.triggers[i].id == nId)
        {
            auto* pTrigger = m_pRuntime.triggers[i].trigger;
            if (pTrigger)
            {
                m_pRuntime.triggers[i].trigger = nullptr;
                return pTrigger;
            }
        }
    }

    return nullptr;
}

void AchievementRuntime::ReleaseAchievementTrigger(ra::AchievementID nId, _In_ rc_trigger_t* pTrigger)
{
    if (!m_bInitialized)
        return;

    std::lock_guard<std::mutex> pLock(m_pMutex);

#pragma warning(push)
#pragma warning(disable:6001) // Using uninitialized memory 'pTrigger'

    for (unsigned i = 0; i < m_pRuntime.trigger_count; ++i)
    {
        if (m_pRuntime.triggers[i].id == nId && static_cast<rc_trigger_t*>(m_pRuntime.triggers[i].buffer) == pTrigger)
        {
            // this duplicates logic from rc_runtime_deactivate_trigger_by_index
            if (!m_pRuntime.triggers[i].owns_memrefs)
            {
                free(pTrigger);

                if (--m_pRuntime.trigger_count > i)
                    memcpy(&m_pRuntime.triggers[i], &m_pRuntime.triggers[m_pRuntime.trigger_count], sizeof(rc_runtime_trigger_t));

            }
        }
    }

#pragma warning(pop)
}

void AchievementRuntime::UpdateAchievementId(ra::AchievementID nOldId, ra::AchievementID nNewId)
{
    if (!m_bInitialized)
        return;

    std::lock_guard<std::mutex> pLock(m_pMutex);
    for (unsigned i = 0; i < m_pRuntime.trigger_count; ++i)
    {
        if (m_pRuntime.triggers[i].id == nOldId)
            m_pRuntime.triggers[i].id = nNewId;
    }
}

int AchievementRuntime::ActivateLeaderboard(unsigned int nId, const std::string& sDefinition)
{
    std::lock_guard<std::mutex> pLock(m_pMutex);
    EnsureInitialized();

    return rc_runtime_activate_lboard(&m_pRuntime, nId, sDefinition.c_str(), nullptr, 0);
}

void AchievementRuntime::ActivateRichPresence(const std::string& sScript)
{
    std::lock_guard<std::mutex> pLock(m_pMutex);
    EnsureInitialized();

    if (sScript.empty())
    {
        m_nRichPresenceParseResult = RC_OK;
        if (m_pRuntime.richpresence != nullptr)
            m_pRuntime.richpresence->richpresence = nullptr;
    }
    else
    {
        m_nRichPresenceParseResult = rc_runtime_activate_richpresence(&m_pRuntime, sScript.c_str(), nullptr, 0);
    }
}

std::wstring AchievementRuntime::GetRichPresenceDisplayString() const
{
    if (m_bInitialized)
    {
        std::lock_guard<std::mutex> pLock(m_pMutex);

        if (m_nRichPresenceParseResult != RC_OK)
            return ra::StringPrintf(L"Parse error %d: %s\n", m_nRichPresenceParseResult, rc_error_str(m_nRichPresenceParseResult));

        char sRichPresence[256];
        if (rc_runtime_get_richpresence(&m_pRuntime, sRichPresence, sizeof(sRichPresence), rc_peek_callback, nullptr, nullptr) > 0)
            return ra::Widen(sRichPresence);
    }

    return std::wstring(L"No Rich Presence defined.");
}

void AchievementRuntime::SetPaused(bool bValue) 
{ 
    if (!bValue && m_bPaused && !m_vQueuedAchievements.empty())
    {
        m_bPaused = false;

        for (const auto pPair : m_vQueuedAchievements)
            ActivateAchievement(pPair.first, pPair.second);

        m_vQueuedAchievements.clear();
    }

    m_bPaused = bValue; 
}

static std::vector<AchievementRuntime::Change>* g_pChanges;

static void map_event_to_change(const rc_runtime_event_t* pRuntimeEvent)
{
    auto nChangeType = AchievementRuntime::ChangeType::None;

    switch (pRuntimeEvent->type)
    {
        case RC_RUNTIME_EVENT_ACHIEVEMENT_TRIGGERED:
            nChangeType = AchievementRuntime::ChangeType::AchievementTriggered;
            break;
        case RC_RUNTIME_EVENT_ACHIEVEMENT_RESET:
            nChangeType = AchievementRuntime::ChangeType::AchievementReset;
            break;
        case RC_RUNTIME_EVENT_ACHIEVEMENT_DISABLED:
            nChangeType = AchievementRuntime::ChangeType::AchievementDisabled;
            break;
        case RC_RUNTIME_EVENT_ACHIEVEMENT_PRIMED:
            nChangeType = AchievementRuntime::ChangeType::AchievementPrimed;
            break;
        case RC_RUNTIME_EVENT_ACHIEVEMENT_ACTIVATED:
            nChangeType = AchievementRuntime::ChangeType::AchievementActivated;
            break;
        case RC_RUNTIME_EVENT_LBOARD_STARTED:
            nChangeType = AchievementRuntime::ChangeType::LeaderboardStarted;
            break;
        case RC_RUNTIME_EVENT_LBOARD_CANCELED:
            nChangeType = AchievementRuntime::ChangeType::LeaderboardCanceled;
            break;
        case RC_RUNTIME_EVENT_LBOARD_UPDATED:
            nChangeType = AchievementRuntime::ChangeType::LeaderboardUpdated;
            break;
        case RC_RUNTIME_EVENT_LBOARD_TRIGGERED:
            nChangeType = AchievementRuntime::ChangeType::LeaderboardTriggered;
            break;
        case RC_RUNTIME_EVENT_LBOARD_DISABLED:
            nChangeType = AchievementRuntime::ChangeType::LeaderboardDisabled;
            break;
        default:
            // unsupported
            return;
    }

    g_pChanges->emplace_back(AchievementRuntime::Change{ nChangeType, pRuntimeEvent->id, pRuntimeEvent->value });
}

_Use_decl_annotations_ void AchievementRuntime::Process(std::vector<Change>& changes)
{
    if (!m_bInitialized || m_bPaused)
        return;

    std::lock_guard<std::mutex> pLock(m_pMutex);
    g_pChanges = &changes;
    rc_runtime_do_frame(&m_pRuntime, map_event_to_change, rc_peek_callback, nullptr, nullptr);
    g_pChanges = nullptr;
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
                            pCondition->operand1.value.memref->value.value = condSource.nSourceVal;
                            pCondition->operand1.value.memref->value.prior = condSource.nSourcePrev;
                            pCondition->operand1.value.memref->value.changed = 0;
                        }
                        if (IsMemoryOperand(pCondition->operand2.type))
                        {
                            pCondition->operand2.value.memref->value.value = condSource.nTargetVal;
                            pCondition->operand2.value.memref->value.prior = condSource.nTargetPrev;
                            pCondition->operand2.value.memref->value.changed = 0;
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

bool AchievementRuntime::LoadProgressV1(const std::string& sProgress, std::set<unsigned int>& vProcessedAchievementIds)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();

    Tokenizer pTokenizer(sProgress);

    while (!pTokenizer.EndOfString())
    {
        const unsigned int nId = pTokenizer.PeekNumber();
        vProcessedAchievementIds.insert(nId);

        rc_trigger_t* pTrigger = rc_runtime_get_achievement(&m_pRuntime, nId);
        if (pTrigger == nullptr)
        {
            // achievement not active, still have to process state string to skip over it
            std::string sEmpty;
            ProcessStateString(pTokenizer, nId, nullptr, sEmpty, sEmpty);
        }
        else
        {
            const auto* pAchievement = pGameContext.Assets().FindAchievement(nId);
            std::string sMemString = pAchievement ? pAchievement->GetTrigger() : "";
            ProcessStateString(pTokenizer, nId, pTrigger, pUserContext.GetUsername(), sMemString);
            pTrigger->state = RC_TRIGGER_STATE_ACTIVE;
        }
    }

    return true;
}

bool AchievementRuntime::LoadProgressV2(ra::services::TextReader& pFile, std::set<unsigned int>& vProcessedAchievementIds)
{
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
            rc_memref_t pMemRef{};

            const auto cPrefix = tokenizer.PeekChar();
            tokenizer.Advance();
            pMemRef.value.size = ComparisonSizeFromPrefix(cPrefix);

            const auto sAddr = tokenizer.ReadTo(':');
            pMemRef.address = ra::ByteAddressFromString(sAddr);

            unsigned delta = 0;
            while (tokenizer.PeekChar() != '#' && !tokenizer.EndOfString())
            {
                tokenizer.Advance();
                const auto cField = tokenizer.PeekChar();
                tokenizer.Advance(2);
                const auto nValue = tokenizer.ReadNumber();

                switch (cField)
                {
                    case 'v': pMemRef.value.value = nValue; break;
                    case 'd': delta = nValue; break;
                    case 'p': pMemRef.value.prior = nValue; break;
                }
            }
            pMemRef.value.changed = (pMemRef.value.value != delta);

            if (tokenizer.PeekChar() == '#')
            {
                const auto sLineMD5 = RAGenerateMD5(sLine.substr(0, tokenizer.CurrentPosition()));
                tokenizer.Advance();
                if (tokenizer.PeekChar() == sLineMD5.at(0))
                {
                    tokenizer.Advance();
                    if (tokenizer.PeekChar() == sLineMD5.at(31))
                    {
                        // match! attempt to store it
                        rc_memref_t* pMemoryReference = m_pRuntime.memrefs;
                        while (pMemoryReference)
                        {
                            if (pMemoryReference->address == pMemRef.address &&
                                pMemoryReference->value.size == pMemRef.value.size)
                            {
                                pMemoryReference->value.value = pMemRef.value.value;
                                pMemoryReference->value.changed = pMemRef.value.changed;
                                pMemoryReference->value.prior = pMemRef.value.prior;
                                break;
                            }

                            pMemoryReference = pMemoryReference->next;
                        }
                    }
                }
            }
        }
        else if (c == 'A') // achievement
        {
            const auto nId = tokenizer.ReadNumber();
            tokenizer.Advance();

            rc_runtime_trigger_t* pRuntimeTrigger = nullptr;
            for (size_t i = 0; i < m_pRuntime.trigger_count; ++i)
            {
                if (m_pRuntime.triggers[i].id == nId && m_pRuntime.triggers[i].trigger)
                {
                    pRuntimeTrigger = &m_pRuntime.triggers[i];
                    break;
                }
            }

            if (pRuntimeTrigger == nullptr) // not active, ignore
                continue;
            auto* pTrigger = pRuntimeTrigger->trigger;

            const auto sChecksumMD5 = sLine.substr(tokenizer.CurrentPosition(), 32);
            tokenizer.Advance(32);

            const auto sMemStringMD5 = RAFormatMD5(pRuntimeTrigger->md5);
            if (sMemStringMD5 != sChecksumMD5) // modified since captured
            {
                rc_reset_trigger(pTrigger);
                continue;
            }

            pTrigger->has_hits = 0;

            rc_condition_t* pCondition = nullptr;
            if (pTrigger->requirement)
            {
                pCondition = pTrigger->requirement->conditions;
                while (pCondition)
                {
                    tokenizer.Advance();
                    pCondition->current_hits = tokenizer.ReadNumber();
                    pTrigger->has_hits |= (pCondition->current_hits != 0);

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
                    pTrigger->has_hits |= (pCondition->current_hits != 0);

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
            }

            if (bValid)
            {
                pTrigger->state = RC_TRIGGER_STATE_ACTIVE;
                vProcessedAchievementIds.insert(nId);
            }
            else
            {
                rc_reset_trigger(pTrigger);
            }
        }
    }

    return true;
}

bool AchievementRuntime::LoadProgressFromFile(const char* sLoadStateFilename)
{
    if (!m_bInitialized)
        return true;

    std::lock_guard<std::mutex> pLock(m_pMutex);

    // clear out any captured hits for active achievements so they get re-evaluated from the runtime
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()); ++nIndex)
    {
        auto* pAchievement = dynamic_cast<ra::data::models::AchievementModel*>(pGameContext.Assets().GetItemAt(nIndex));
        if (pAchievement != nullptr && pAchievement->IsActive())
            pAchievement->ResetCapturedHits();
    }

    // reset the runtime state, then apply state from file
    rc_runtime_reset(&m_pRuntime);

    if (sLoadStateFilename == nullptr)
        return false;

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    if (!pUserContext.IsLoggedIn())
        return false;

    std::wstring sAchievementStateFile = ra::Widen(sLoadStateFilename) + L".rap";
    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    auto pFile = pFileSystem.OpenTextFile(sAchievementStateFile);
    if (pFile == nullptr)
        return false;

    std::string sContents;
    if (!pFile->GetLine(sContents))
        return false;

    std::set<unsigned int> vProcessedAchievementIds;

    if (sContents == "RAP")
    {
        const auto nSize = pFile->GetSize();
        std::vector<unsigned char> pBuffer;
        pBuffer.resize(nSize);
        pFile->SetPosition({ 0 });
        pFile->GetBytes(&pBuffer.front(), nSize);

        if (rc_runtime_deserialize_progress(&m_pRuntime, &pBuffer.front(), nullptr) == RC_OK)
        {
            RA_LOG_INFO("Runtime state loaded from %s", sLoadStateFilename);
        }
    }
    else if (sContents == "v2")
    {
        const auto nVersion = std::stoi(sContents.substr(1));
        switch (nVersion)
        {
            case 2:
                if (LoadProgressV2(*pFile, vProcessedAchievementIds))
                {
                    RA_LOG_INFO("Runtime state (v2) loaded from %s", sLoadStateFilename);
                }
                else
                {
                    vProcessedAchievementIds.clear();
                }
                break;

            default:
                assert(!"Unknown persistence file version");
                return false;
        }
    }
    else
    {
        if (LoadProgressV1(sContents, vProcessedAchievementIds))
        {
            RA_LOG_INFO("Runtime state (v1) loaded from %s", sLoadStateFilename);
        }
        else
        {
            vProcessedAchievementIds.clear();
        }
    }

    return true;
}

bool AchievementRuntime::LoadProgressFromBuffer(const char* pBuffer)
{
    if (!m_bInitialized)
        return true;

    std::lock_guard<std::mutex> pLock(m_pMutex);

    // reset the runtime state, then apply state from file
    rc_runtime_reset(&m_pRuntime);

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    if (!pUserContext.IsLoggedIn())
        return false;

    const unsigned char* pBytes;
    GSL_SUPPRESS_TYPE1 pBytes = reinterpret_cast<const unsigned char*>(pBuffer);
    if (rc_runtime_deserialize_progress(&m_pRuntime, pBytes, nullptr) == RC_OK)
    {
        RA_LOG_INFO("Runtime state loaded from buffer");
    }

    return true;
}

void AchievementRuntime::SaveProgressToFile(const char* sSaveStateFilename) const
{
    if (sSaveStateFilename == nullptr)
        return;

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    if (!pUserContext.IsLoggedIn())
        return;

    std::wstring sAchievementStateFile = ra::Widen(sSaveStateFilename) + L".rap";
    auto pFile = ra::services::ServiceLocator::Get<ra::services::IFileSystem>().CreateTextFile(sAchievementStateFile);
    if (pFile == nullptr)
    {
        RA_LOG_WARN("Could not write %s", sAchievementStateFile);
        return;
    }

    std::lock_guard<std::mutex> pLock(m_pMutex);

    const auto nSize = rc_runtime_progress_size(&m_pRuntime, nullptr);
    std::string sSerialized;
    sSerialized.resize(nSize);
    rc_runtime_serialize_progress(sSerialized.data(), &m_pRuntime, nullptr);
    pFile->Write(sSerialized);

    RA_LOG_INFO("Runtime state written to %s", sSaveStateFilename);
}

int AchievementRuntime::SaveProgressToBuffer(char* pBuffer, int nBufferSize) const
{
    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    if (!pUserContext.IsLoggedIn())
        return 0;

    std::lock_guard<std::mutex> pLock(m_pMutex);

    const auto nSize = rc_runtime_progress_size(&m_pRuntime, nullptr);
    if (nSize <= nBufferSize)
    {
        rc_runtime_serialize_progress(pBuffer, &m_pRuntime, nullptr);
        RA_LOG_INFO("Runtime state written to buffer (%d/%d bytes)", nSize, nBufferSize);
    }
    else
    {
        RA_LOG_WARN("Runtime state not written to buffer (%d/%d bytes)", nSize, nBufferSize);
    }

    return nSize;
}

void AchievementRuntime::OnTotalMemorySizeChanged()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.RemoveNotifyTarget(*this);

    auto& pThreadPool = ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>();
    pThreadPool.ScheduleAsync(std::chrono::milliseconds(100), [this]()
    {
        DetectUnsupportedAchievements();
    });
}

void AchievementRuntime::InvalidateAddress(ra::ByteAddress nAddress) noexcept
{
    // this should only be called from DetectUnsupportedAchievements or indirectly via Process,
    // both of which aquire the lock, so we shouldn't try to acquire it here.
    // we can also avoid checking m_bInitialized
    rc_runtime_invalidate_address(&m_pRuntime, nAddress);
}

#pragma warning(push)
#pragma warning(disable : 5045)

size_t AchievementRuntime::DetectUnsupportedAchievements()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    if (pEmulatorContext.TotalMemorySize() == 0)
    {
        // no memory registered yet, wait
        pEmulatorContext.AddNotifyTarget(*this);
        return 0;
    }

    std::lock_guard<std::mutex> pLock(m_pMutex);

    const rc_memref_t* memref = m_pRuntime.memrefs;
    while (memref)
    {
        // ignore indirect memory references as their value is calculated from other memory and can easily point at
        // invalid addresses.
        if (!memref->value.is_indirect)
        {
            // attempt to read one byte from the memory address (assume multi-byte reads won't span regions). if the
            // read fails, EmulatorContext will call InvalidateAddress, which will flag all associated achievements.
            if (!pEmulatorContext.IsValidAddress(memref->address))
                InvalidateAddress(memref->address);
        }

        memref = memref->next;
    }

    size_t nDisabled = 0;
    const auto* pTrigger = m_pRuntime.triggers;
    const auto* pStop = pTrigger + m_pRuntime.trigger_count;
    while (pTrigger < pStop)
    {
        if (pTrigger->invalid_memref)
        {
            // before rc_runtime_do_frame called
            ++nDisabled;
        }
        else if (pTrigger->trigger && pTrigger->trigger->state == RC_TRIGGER_STATE_DISABLED)
        {
            // after rc_runtime_do_frame called
            ++nDisabled;
        }

        ++pTrigger;
    }

    return nDisabled;
}

#pragma warning(pop)

} // namespace services
} // namespace ra

extern "C" unsigned int rc_peek_callback(unsigned int nAddress, unsigned int nBytes, _UNUSED void* pData)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
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
