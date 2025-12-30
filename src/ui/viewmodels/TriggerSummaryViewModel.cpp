#include "TriggerSummaryViewModel.hh"

#include "RA_Defs.h"

#include "data\Memory.hh"
#include "data\context\GameContext.hh"

#include "services\ServiceLocator.hh"

#include "util\Strings.hh"

#include <rcheevos/src/rcheevos/rc_internal.h>

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty TriggerSummaryViewModel::TriggerClauseViewModel::IndicesProperty("TriggerClauseViewModel", "Indices", L"1");
const StringModelProperty TriggerSummaryViewModel::TriggerClauseViewModel::ReferenceProperty("TriggerClauseViewModel", "Reference", L"0x0000");
const StringModelProperty TriggerSummaryViewModel::TriggerClauseViewModel::OperationProperty("TriggerClauseViewModel", "Operation", L"is");
const StringModelProperty TriggerSummaryViewModel::TriggerClauseViewModel::TargetProperty("TriggerClauseViewModel", "Target", L"0");

static constexpr bool IsValueCharacter(wchar_t c)
{
    if (c >= '0' && c <= '9')
        return true;
    if (c >= 'a' && c <= 'f')
        return true;
    if (c >= 'A' && c <= 'F')
        return true;
    if (isspace(c))
        return true;
    if (c == 'h' || c == 'H' || c == 'x' || c == '-')
        return true;
    return false;
}

static std::wstring EnumValueFromText(std::wstring_view sEnumText)
{
    const auto nSplit = sEnumText.find_first_of(L"=:");
    auto sEnumDescription = sEnumText.substr(nSplit + 1);
    for (size_t i = 0; i < nSplit; ++i)
    {
        if (!IsValueCharacter(sEnumText.at(i)))
        {
            sEnumDescription = sEnumText.substr(0, nSplit);
            break;
        }
    }

    auto nIndex = sEnumDescription.find_first_not_of(L" \t\n\r");
    if (nIndex != 0 && nIndex != std::string::npos)
        sEnumDescription = sEnumDescription.substr(nIndex);

    nIndex = sEnumDescription.find_last_not_of(L" \t\n\r");
    if (nIndex < sEnumDescription.length() - 1)
        sEnumDescription = sEnumDescription.substr(0, nIndex);

    return std::wstring(sEnumDescription);
}

static std::wstring OperatorToString(uint8_t oper)
{
    switch (oper)
    {
        case RC_OPERATOR_EQ: return L"is";
        case RC_OPERATOR_NE: return L"is not";
        case RC_OPERATOR_GE: return L"is at least";
        case RC_OPERATOR_GT: return L"is greater than";
        case RC_OPERATOR_LE: return L"is at most";
        case RC_OPERATOR_LT: return L"is less than";
    }
}

static std::wstring OperandToString(const rc_operand_t& pOperand)
{
    switch (pOperand.type)
    {
        case RC_OPERAND_CONST:
            return std::to_wstring(pOperand.value.num);

        case RC_OPERAND_ADDRESS:
            return ra::Widen(ra::ByteAddressToString(pOperand.value.memref->address));

        default:
            return L"???";
    }
}

static void HandleCompareMemoryReferenceToSelf(TriggerSummaryViewModel::TriggerClauseViewModel& pClause, const rc_condition_t& pCondition)
{
    pClause.SetTarget(L"");

    if (pCondition.operand1.memref_access_type == pCondition.operand2.memref_access_type)
    {
        switch (pCondition.oper)
        {
            case RC_OPERATOR_EQ:
            case RC_OPERATOR_GE:
            case RC_OPERATOR_LE:
                pClause.SetOperation(L"unimportant"); // delta = delta  ~>  always true
                break;

            case RC_OPERATOR_NE:
            case RC_OPERATOR_GT:
            case RC_OPERATOR_LT:
                pClause.SetOperation(L"invalid"); // delta != delta  ~>  always false
                break;
        }
    }
    else
    {
        switch (pCondition.oper)
        {
            case RC_OPERATOR_EQ:
                pClause.SetOperation(L"hasn't changed");
                break;

            case RC_OPERATOR_NE:
                pClause.SetOperation(L"changed");
                break;

            case RC_OPERATOR_LT:
                pClause.SetOperation(pCondition.operand1.memref_access_type == RC_OPERAND_ADDRESS
                    ? L"decreased"   // val < delta
                    : L"increased"); // delta < val
                break;

            case RC_OPERATOR_GT:
                pClause.SetOperation(pCondition.operand1.memref_access_type == RC_OPERAND_ADDRESS
                    ? L"increased"   // val > delta
                    : L"decreased"); // delta > val
                break;

            case RC_OPERATOR_LE:
                pClause.SetOperation(pCondition.operand1.memref_access_type == RC_OPERAND_ADDRESS
                    ? L"did not increase"   // val <= delta
                    : L"did not decrease"); // delta <= val
                break;

            case RC_OPERATOR_GE:
                pClause.SetOperation(pCondition.operand1.memref_access_type == RC_OPERAND_ADDRESS
                    ? L"did not decrease"   // val >= delta
                    : L"did not increase"); // delta >= val
                break;
        }
    }
}

void TriggerSummaryViewModel::InitializeFrom(const rc_condset_t& pCondSet)
{
    uint32_t nFirstIndex = 0;
    uint32_t nLastIndex = 0;

    const auto* pCodeNotes = ra::services::ServiceLocator::Get<ra::data::context::GameContext>().Assets().FindCodeNotes();

    const auto* pCondition = pCondSet.conditions;
    for (; pCondition; pCondition = pCondition->next)
    {
        auto& pClause = m_vClauses.Add();

        nFirstIndex = nLastIndex + 1;
        nLastIndex = nFirstIndex;
        if (pCondition->type == RC_CONDITION_ADD_ADDRESS)
        {
            do
            {
                nLastIndex++;
                pCondition = pCondition->next;
            } while (pCondition && pCondition->type == RC_CONDITION_ADD_ADDRESS);

            if (!pCondition)
            {
                m_vClauses.RemoveAt(m_vClauses.Count() - 1);
                break;
            }

            pClause.SetIndices(ra::StringPrintf(L"%u-%u", nFirstIndex, nLastIndex));
        }
        else
        {
            pClause.SetIndices(std::to_wstring(nFirstIndex));
        }

        const ra::data::models::CodeNoteModel* pNote = nullptr;
        if (pCodeNotes && rc_operand_is_memref(&pCondition->operand1))
            pNote = pCodeNotes->FindCodeNoteModel(pCondition->operand1.value.memref->address);

        if (pNote)
            pClause.SetReference(pNote->GetSummary());
        else
            pClause.SetReference(OperandToString(pCondition->operand1));

        pClause.SetOperation(OperatorToString(pCondition->oper));

        if (rc_operand_is_memref(&pCondition->operand2))
        {
            if (pCondition->operand1.value.memref == pCondition->operand2.value.memref)
            {
                // comparing value to itself
                HandleCompareMemoryReferenceToSelf(pClause, *pCondition);
                continue;
            }
        }
        else if (pNote)
        {
            auto nTarget = pCondition->operand2.value.num;

            // a < 1  ~>  a == 0
            if (nTarget == 1 && pCondition->oper == RC_OPERATOR_LT)
            {
                nTarget = 0;
                pClause.SetOperation(OperatorToString(RC_OPERATOR_EQ));
            }

            const auto pEnumText = pNote->GetEnumText(nTarget);
            if (!pEnumText.empty())
            {
                pClause.SetTarget(EnumValueFromText(pEnumText));

                // a > 0  ~>  a != 0
                if (nTarget == 0 && pCondition->oper == RC_OPERATOR_GT)
                    pClause.SetOperation(OperatorToString(RC_OPERATOR_NE));
            }
            else
            {
                pClause.SetTarget(std::to_wstring(nTarget));
            }
        }
        else
        {
            pClause.SetTarget(OperandToString(pCondition->operand2));
        }
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
