#include "RA_Achievement.h"

#include "RA_md5factory.h"

#include "RA_Defs.h"

#include "data\context\GameContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#ifndef RA_UTEST
#include "RA_ImageFactory.h"
#endif

#ifndef RA_UTEST
#include "RA_Core.h"
#else
// duplicate of code in RA_Core, but RA_Core needs to be cleaned up before it can be pulled into the unit test build
GSL_SUPPRESS_F23
void _ReadStringTil(std::string& value, char nChar, const char* restrict& pSource)
{
    Expects(pSource != nullptr);
    const char* pStartString = pSource;

    while (*pSource != '\0' && *pSource != nChar)
        pSource++;

    value.assign(pStartString, pSource - pStartString);
    pSource++;
}
#endif

//////////////////////////////////////////////////////////////////////////

Achievement::Achievement() noexcept
{
    m_vConditions.AddGroup();
}

Achievement::~Achievement() noexcept
{
    if (m_bActive)
    {
        if (ra::services::ServiceLocator::Exists<ra::services::AchievementRuntime>())
        {
            auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
            pRuntime.DeactivateAchievement(ID());
        }
    }
}

static void MergeHits(rc_condset_t& pGroup, rc_condset_t& pOldGroup) noexcept
{
    // for each condition in pGroup, attempt to find the matching condition in pOldGroup
    // if found, merge the hits, delta, and prior values into the new condition to maintain state
    auto* pFirstOldCondition = pOldGroup.conditions;
    for (auto* pCondition = pGroup.conditions; pCondition; pCondition = pCondition->next)
    {
        for (auto* pOldCondition = pFirstOldCondition; pOldCondition; pOldCondition = pOldCondition->next)
        {
            if (pOldCondition->operand1.type != pCondition->operand1.type ||
                pOldCondition->operand2.type != pCondition->operand2.type ||
                pOldCondition->oper != pCondition->oper ||
                pOldCondition->type != pCondition->type ||
                pOldCondition->required_hits != pCondition->required_hits)
            {
                continue;
            }

            switch (pOldCondition->operand1.type)
            {
                case RC_OPERAND_ADDRESS:
                case RC_OPERAND_DELTA:
                case RC_OPERAND_PRIOR:
                    if (pOldCondition->operand1.value.memref->memref.address != pCondition->operand1.value.memref->memref.address ||
                        pOldCondition->operand1.size != pCondition->operand1.size)
                    {
                        continue;
                    }

                    pCondition->operand1.value.memref->previous = pOldCondition->operand1.value.memref->previous;
                    pCondition->operand1.value.memref->prior = pOldCondition->operand1.value.memref->prior;
                    break;

                default:
                case RC_OPERAND_CONST:
                    if (pOldCondition->operand1.value.num != pCondition->operand1.value.num)
                        continue;

                    break;

                case RC_OPERAND_FP:
                    if (pOldCondition->operand1.value.dbl != pCondition->operand1.value.dbl)
                        continue;

                    break;

                case RC_OPERAND_LUA:
                    if (pOldCondition->operand1.value.luafunc != pCondition->operand1.value.luafunc)
                        continue;

                    break;
            }

            pCondition->current_hits = pOldCondition->current_hits;

            // if we matched the first condition, ignore it on future scans
            if (pOldCondition == pFirstOldCondition)
                pFirstOldCondition = pOldCondition->next;

            break;
        }
    }
}

void Achievement::RebuildTrigger()
{
    // store the original trigger for later
    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    auto* pOldTrigger = pRuntime.DetachAchievementTrigger(ID());

    // perform validation - if the new trigger will result in a parse error, the changes will be lost.
    // attempt to detect and correct for these to minimize the impact to the user.
    unsigned int nMeasuredTarget = 0U;
    for (auto& pGroup : m_vConditions)
    {
        for (size_t i = 0; i < pGroup.Count(); ++i)
        {
            auto& pCond = pGroup.GetAt(i);

            if (pCond.GetConditionType() == Condition::Type::Measured)
            {
                unsigned int nCondTarget = pCond.RequiredHits();
                if (nCondTarget == 0 && pCond.CompTarget().GetType() != CompVariable::Type::ValueComparison)
                {
                    // RC_INVALID_MEASURED_TARGET
                    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
                        L"Removing Measured flag",
                        L"A Measured condition requires a non-zero hit count target, or a constant value for the right operand.");

                    pCond.SetConditionType(Condition::Type::Standard);
                }
                else
                {
                    if (nCondTarget == 0)
                        nCondTarget = pCond.CompTarget().GetValue();

                    if (nMeasuredTarget == 0U)
                    {
                        nMeasuredTarget = nCondTarget;
                    }
                    else if (nMeasuredTarget != nCondTarget)
                    {
                        // RC_MULTIPLE_MEASURED
                        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
                            L"Removing Measured flag",
                            ra::StringPrintf(L"Another Measured condition already exists with a target of %u.", nMeasuredTarget));

                        pCond.SetConditionType(Condition::Type::Standard);
                    }
                }
            }
        }
    }

    // convert the UI definition into a string
    m_vConditions.Serialize(m_sTrigger);

    // if achievement was previously active, reactivate it
    if (pOldTrigger != nullptr)
    {
        const auto nResult = pRuntime.ActivateAchievement(ID(), m_sTrigger);
        if (nResult != RC_OK)
        {
            // parse error occurred
            RA_LOG_WARN("rc_parse_trigger returned %d for achievement %u", nResult, ID());

            ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
                ra::StringPrintf(L"Unable to rebuild achievement: %s", Title()),
                ra::StringPrintf(L"Parse error %d\n%s", nResult, rc_error_str(nResult)));
        }

        m_pTrigger = pRuntime.GetAchievementTrigger(ID());

        // if the trigger hasn't actually changed, it will be recycled.
        // otherwise, attempt to copy over the hit counts.
        if (m_pTrigger != pOldTrigger)
        {
            m_pTrigger->state = pOldTrigger->state;

            if (m_pTrigger->requirement && pOldTrigger->requirement)
                MergeHits(*m_pTrigger->requirement, *pOldTrigger->requirement);

            auto* pAltGroup = m_pTrigger->alternative;
            auto* pOldAltGroup = pOldTrigger->alternative;
            while (pAltGroup && pOldAltGroup)
            {
                MergeHits(*pAltGroup, *pOldAltGroup);
                pAltGroup = pAltGroup->next;
                pOldAltGroup = pOldAltGroup->next;
            }

            // relese the memory associated with the previous trigger
            pRuntime.ReleaseAchievementTrigger(ID(), pOldTrigger);
        }
    }
    else
    {
        m_pTrigger = nullptr;
    }

    // mark the conditions as dirty so they will get repainted
    SetDirtyFlag(DirtyFlags::Conditions);
}

static constexpr MemSize GetCompVariableSize(char nOperandSize) noexcept
{
    switch (nOperandSize)
    {
        case RC_MEMSIZE_BIT_0:
            return MemSize::Bit_0;
        case RC_MEMSIZE_BIT_1:
            return MemSize::Bit_1;
        case RC_MEMSIZE_BIT_2:
            return MemSize::Bit_2;
        case RC_MEMSIZE_BIT_3:
            return MemSize::Bit_3;
        case RC_MEMSIZE_BIT_4:
            return MemSize::Bit_4;
        case RC_MEMSIZE_BIT_5:
            return MemSize::Bit_5;
        case RC_MEMSIZE_BIT_6:
            return MemSize::Bit_6;
        case RC_MEMSIZE_BIT_7:
            return MemSize::Bit_7;
        case RC_MEMSIZE_LOW:
            return MemSize::Nibble_Lower;
        case RC_MEMSIZE_HIGH:
            return MemSize::Nibble_Upper;
        case RC_MEMSIZE_8_BITS:
            return MemSize::EightBit;
        case RC_MEMSIZE_16_BITS:
            return MemSize::SixteenBit;
        case RC_MEMSIZE_24_BITS:
            return MemSize::TwentyFourBit;
        case RC_MEMSIZE_32_BITS:
            return MemSize::ThirtyTwoBit;
        case RC_MEMSIZE_BITCOUNT:
            return MemSize::BitCount;
        default:
            ASSERT(!"Unsupported operand size");
            return MemSize::EightBit;
    }
}

inline static constexpr void SetOperand(CompVariable& var, const rc_operand_t& operand) noexcept
{
    switch (operand.type)
    {
        case RC_OPERAND_ADDRESS:
            var.Set(GetCompVariableSize(operand.size), CompVariable::Type::Address, operand.value.memref->memref.address);
            break;

        case RC_OPERAND_DELTA:
            var.Set(GetCompVariableSize(operand.size), CompVariable::Type::DeltaMem, operand.value.memref->memref.address);
            break;

        case RC_OPERAND_PRIOR:
            var.Set(GetCompVariableSize(operand.size), CompVariable::Type::PriorMem, operand.value.memref->memref.address);
            break;

        case RC_OPERAND_CONST:
            var.Set(MemSize::ThirtyTwoBit, CompVariable::Type::ValueComparison, operand.value.num);
            break;

        case RC_OPERAND_FP:
            ASSERT(!"Floating point operand not supported");
            var.Set(MemSize::ThirtyTwoBit, CompVariable::Type::ValueComparison, 0U);
            break;

        case RC_OPERAND_LUA:
            ASSERT(!"Lua operand not supported");
            var.Set(MemSize::ThirtyTwoBit, CompVariable::Type::ValueComparison, 0U);
            break;
    }
}

static void MakeConditionGroup(ConditionSet& vConditions, rc_condset_t* pCondSet)
{
    vConditions.AddGroup();
    if (!pCondSet)
        return;

    ConditionGroup& group = vConditions.GetGroup(vConditions.GroupCount() - 1);

    rc_condition_t* pCondition = pCondSet->conditions;
    while (pCondition != nullptr)
    {
        Condition cond;
        SetOperand(cond.CompSource(), pCondition->operand1);
        SetOperand(cond.CompTarget(), pCondition->operand2);

        switch (pCondition->oper)
        {
            default:
                ASSERT(!"Unsupported operator");
                _FALLTHROUGH;
            case RC_OPERATOR_NONE:
            case RC_OPERATOR_EQ:
                cond.SetCompareType(ComparisonType::Equals);
                break;
            case RC_OPERATOR_NE:
                cond.SetCompareType(ComparisonType::NotEqualTo);
                break;
            case RC_OPERATOR_LT:
                cond.SetCompareType(ComparisonType::LessThan);
                break;
            case RC_OPERATOR_LE:
                cond.SetCompareType(ComparisonType::LessThanOrEqual);
                break;
            case RC_OPERATOR_GT:
                cond.SetCompareType(ComparisonType::GreaterThan);
                break;
            case RC_OPERATOR_GE:
                cond.SetCompareType(ComparisonType::GreaterThanOrEqual);
                break;
        }

        switch (pCondition->type)
        {
            default:
                ASSERT(!"Unsupported condition type");
                _FALLTHROUGH;
            case RC_CONDITION_STANDARD:
                cond.SetConditionType(Condition::Type::Standard);
                break;
            case RC_CONDITION_RESET_IF:
                cond.SetConditionType(Condition::Type::ResetIf);
                break;
            case RC_CONDITION_PAUSE_IF:
                cond.SetConditionType(Condition::Type::PauseIf);
                break;
            case RC_CONDITION_ADD_SOURCE:
                cond.SetConditionType(Condition::Type::AddSource);
                break;
            case RC_CONDITION_SUB_SOURCE:
                cond.SetConditionType(Condition::Type::SubSource);
                break;
            case RC_CONDITION_ADD_HITS:
                cond.SetConditionType(Condition::Type::AddHits);
                break;
            case RC_CONDITION_AND_NEXT:
                cond.SetConditionType(Condition::Type::AndNext);
                break;
            case RC_CONDITION_OR_NEXT:
                cond.SetConditionType(Condition::Type::OrNext);
                break;
            case RC_CONDITION_MEASURED:
                cond.SetConditionType(Condition::Type::Measured);
                break;
            case RC_CONDITION_MEASURED_IF:
                cond.SetConditionType(Condition::Type::MeasuredIf);
                break;
            case RC_CONDITION_ADD_ADDRESS:
                cond.SetConditionType(Condition::Type::AddAddress);
                break;
        }

        cond.SetRequiredHits(pCondition->required_hits);

        group.Add(cond);
        pCondition = pCondition->next;
    }
}

void Achievement::SetTrigger(const std::string& sTrigger)
{
    m_sTrigger = sTrigger;
    m_pAchievementModel->SetTrigger(sTrigger);
    m_pTrigger = nullptr;
    m_vConditions.Clear();
}

static rc_trigger_t* GetRawTrigger(ra::AchievementID nId, const std::string& sTrigger)
{
    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    rc_trigger_t* pTrigger = pRuntime.GetAchievementTrigger(nId);
    if (pTrigger == nullptr)
        pTrigger = pRuntime.GetAchievementTrigger(nId, sTrigger);

    return pTrigger;
}

void Achievement::GenerateConditions()
{
    if (!m_vConditions.EmptyGroup())
        return;

    std::vector<unsigned char> pBuffer;

    rc_trigger_t* pTrigger = m_pTrigger;
    if (pTrigger == nullptr)
    {
        pTrigger = m_pTrigger = GetRawTrigger(ID(), m_sTrigger);

        if (pTrigger == nullptr)
        {
            const int nSize = rc_trigger_size(m_sTrigger.c_str());
            if (nSize < 0)
            {
                // parse error occurred
                RA_LOG_WARN("rc_parse_trigger returned %d for achievement %u", nSize, ID());

                ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
                    ra::StringPrintf(L"Unable to activate achievement: %s", Title()),
                    ra::StringPrintf(L"Parse error %d\n%s", nSize, rc_error_str(nSize)));

                return;
            }

            pBuffer.resize(nSize);
            pTrigger = rc_parse_trigger(&pBuffer.front(), m_sTrigger.c_str(), nullptr, 0);
        }
    }

    // wrap rc_trigger_t in a ConditionSet for the UI
    MakeConditionGroup(m_vConditions, pTrigger->requirement);
    rc_condset_t* alternative = pTrigger->alternative;
    while (alternative != nullptr)
    {
        MakeConditionGroup(m_vConditions, alternative);
        alternative = alternative->next;
    }
}

static constexpr rc_condition_t* GetTriggerCondition(rc_trigger_t* pTrigger, size_t nGroup, size_t nIndex) noexcept
{
    rc_condset_t* pGroup = pTrigger->requirement;
    if (nGroup > 0)
    {
        pGroup = pTrigger->alternative;
        --nGroup;

        while (pGroup && nGroup > 0)
        {
            pGroup = pGroup->next;
            --nGroup;
        }
    }

    if (!pGroup)
        return nullptr;

    rc_condition_t* pCondition = pGroup->conditions;
    while (pCondition && nIndex > 0)
    {
        --nIndex;
        pCondition = pCondition->next;
    }

    return pCondition;
}

unsigned int Achievement::GetConditionHitCount(size_t nGroup, size_t nIndex) const
{
    if (m_pTrigger == nullptr)
    {
        m_pTrigger = GetRawTrigger(ID(), m_sTrigger);
        if (m_pTrigger == nullptr)
            return 0U;
    }

    const rc_condition_t* pCondition = GetTriggerCondition(m_pTrigger, nGroup, nIndex);
    return pCondition ? pCondition->current_hits : 0U;
}

void Achievement::SetConditionHitCount(size_t nGroup, size_t nIndex, unsigned int nCurrentHits) const
{
    if (m_pTrigger == nullptr)
        m_pTrigger = GetRawTrigger(ID(), m_sTrigger);

    if (m_pTrigger != nullptr)
    {
        rc_condition_t* pCondition = GetTriggerCondition(m_pTrigger, nGroup, nIndex);
        if (pCondition)
            pCondition->current_hits = nCurrentHits;
    }
}

void Achievement::AddAltGroup() noexcept { m_vConditions.AddGroup(); }
void Achievement::RemoveAltGroup(gsl::index nIndex) noexcept { m_vConditions.RemoveAltGroup(nIndex); }

void Achievement::SetID(ra::AchievementID nID) noexcept
{
    m_pAchievementModel->SetID(nID);
    m_nAchievementID = nID;
    SetDirtyFlag(DirtyFlags::ID);
}

void Achievement::SetActive(BOOL bActive) noexcept
{
    if (m_bActive != bActive)
    {
        m_bActive = bActive;
        SetDirtyFlag(DirtyFlags::All);

        if (ra::services::ServiceLocator::Exists<ra::services::AchievementRuntime>())
        {
            auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
            if (!m_bActive)
            {
                pRuntime.DeactivateAchievement(ID());
            }
            else
            {
                const int nResult = pRuntime.ActivateAchievement(ID(), m_sTrigger);
                if (nResult != RC_OK)
                {
                    // parse error occurred
                    RA_LOG_WARN("rc_parse_trigger returned %d for achievement %u", nResult, ID());

                    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
                        ra::StringPrintf(L"Unable to activate achievement: %s", Title()),
                        ra::StringPrintf(L"Parse error %d\n%s", nResult, rc_error_str(nResult)));
                }
            }
        }
    }
}

void Achievement::SetModified(BOOL bModified) noexcept
{
    if (m_bModified != bModified)
    {
        m_bModified = bModified;
        SetDirtyFlag(DirtyFlags::All); // TBD? questionable...
    }
}

void Achievement::SetBadgeImage(const std::string& sBadgeURI)
{
    SetDirtyFlag(DirtyFlags::Badge);

    if (ra::StringEndsWith(sBadgeURI, "_lock"))
        m_sBadgeImageURI.assign(sBadgeURI.c_str(), sBadgeURI.length() - 5);
    else
        m_sBadgeImageURI = sBadgeURI;

    if (m_sBadgeImageURI.length() < 5)
        m_sBadgeImageURI.insert(0, 5 - m_sBadgeImageURI.length(), '0');

    m_pAchievementModel->SetBadge(ra::Widen(m_sBadgeImageURI));
}

size_t Achievement::AddCondition(size_t nConditionGroup, const Condition& rNewCond)
{
    while (NumConditionGroups() <= nConditionGroup) //	ENSURE this is a legal op!
        m_vConditions.AddGroup();

    ConditionGroup& group = m_vConditions.GetGroup(nConditionGroup);
    group.Add(rNewCond); //	NB. Copy by value
    SetDirtyFlag(DirtyFlags::All);

    return group.Count();
}

size_t Achievement::InsertCondition(size_t nConditionGroup, size_t nIndex, const Condition& rNewCond)
{
    while (NumConditionGroups() <= nConditionGroup) //	ENSURE this is a legal op!
        m_vConditions.AddGroup();

    ConditionGroup& group = m_vConditions.GetGroup(nConditionGroup);
    group.Insert(nIndex, rNewCond); //	NB. Copy by value
    SetDirtyFlag(DirtyFlags::All);

    return group.Count();
}

BOOL Achievement::RemoveCondition(size_t nConditionGroup, size_t nIndex)
{
    if (nConditionGroup < m_vConditions.GroupCount())
    {
        m_vConditions.GetGroup(nConditionGroup).RemoveAt(nIndex);
        SetDirtyFlag(DirtyFlags::All); //	Not Conditions:
        return TRUE;
    }

    return FALSE;
}

void Achievement::RemoveAllConditions(size_t nConditionGroup)
{
    if (nConditionGroup < m_vConditions.GroupCount())
    {
        m_vConditions.GetGroup(nConditionGroup).Clear();
        SetDirtyFlag(DirtyFlags::All); //	All - not just conditions
    }
}

std::string Achievement::CreateMemString() const
{
    std::string buffer;
    m_vConditions.Serialize(buffer);
    return buffer;
}

void Achievement::CopyFrom(const Achievement& rRHS)
{
    SetAuthor(rRHS.m_sAuthor);
    SetDescription(rRHS.m_sDescription);
    SetPoints(rRHS.m_nPointValue);
    SetTitle(rRHS.m_sTitle);
    SetModified(rRHS.m_bModified);
    SetBadgeImage(rRHS.m_sBadgeImageURI);

    // TBD: set to 'now'?
    SetModifiedDate(rRHS.m_nTimestampModified);
    SetCreatedDate(rRHS.m_nTimestampCreated);

    m_sTrigger = rRHS.m_sTrigger;
    m_vConditions.Clear();

    SetDirtyFlag(DirtyFlags::All);
}
