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

enum TriggerSummaryViewModel::TriggerClauseViewModel::TriggerClauseType : int
{
    None = 0,
    Is,
    IsNot,
    Comparison,
    AlwaysTrue,
    AlwaysFalse,
    Changed,
    HasntChanged,
    ChangedTo,
    ChangedFrom,
};

using TriggerClauseType = TriggerSummaryViewModel::TriggerClauseViewModel::TriggerClauseType;

static bool IsSameMemoryReference(const rc_operand_t& pOperand1, const rc_operand_t& pOperand2)
{
    switch (pOperand1.type) {
        case RC_OPERAND_CONST:
            return (pOperand1.value.num == pOperand2.value.num);
        case RC_OPERAND_FP:
            return (pOperand1.value.dbl == pOperand2.value.dbl);
        case RC_OPERAND_RECALL:
            return (pOperand1.value.memref == pOperand2.value.memref);
        default:
            break;
    }

    if (pOperand1.size != pOperand2.size)
        return false;

    return (pOperand1.value.memref == pOperand2.value.memref);
}

static constexpr uint32_t ParseUInt32(std::wstring_view sValue)
{
    uint32_t nValue = 0;
    for (auto c : sValue)
    {
        nValue *= 10;
        nValue += (c - '0');
    }

    return nValue;
}

static void ParseRanges(std::vector<std::pair<uint32_t, uint32_t>>& vRanges, const std::wstring& sIndices)
{
    std::wstring_view sRemaining = sIndices;
    size_t nIndex;
    do
    {
        nIndex = sRemaining.find(',');
        const auto sRange = sRemaining.substr(0, nIndex);

        const auto nIndex2 = sRange.find('-');
        if (nIndex2 == std::wstring::npos)
        {
            const uint32_t nValue = ParseUInt32(sRange);
            vRanges.push_back({ nValue, nValue });
        }
        else
        {
            const uint32_t nStart = ParseUInt32(sRange.substr(0, nIndex2));
            const uint32_t nEnd = ParseUInt32(sRange.substr(nIndex2 + 1));
            vRanges.push_back({ nStart, nEnd });
        }
    } while (nIndex < sRemaining.length());
}

static void MergeIndices(TriggerSummaryViewModel::TriggerClauseViewModel& pClause1, TriggerSummaryViewModel::TriggerClauseViewModel& pClause2)
{
    std::vector<std::pair<uint32_t, uint32_t>> vRanges;
    ParseRanges(vRanges, pClause1.GetIndices());
    ParseRanges(vRanges, pClause2.GetIndices());

    std::sort(vRanges.begin(), vRanges.end(), [](const std::pair<uint32_t, uint32_t>& a, const std::pair<uint32_t, uint32_t>& b) {
        return a.first < b.first;
    });

    uint32_t nStart = 0xFFFFFFFF;
    uint32_t nEnd = nStart;

    std::wstring sNewIndices;
    for (const auto& pRange : vRanges)
    {
        if (pRange.first == nEnd + 1)
        {
            nEnd = pRange.second;
        }
        else
        {
            if (nStart != 0xFFFFFFFF)
            {
                sNewIndices.append(std::to_wstring(nStart));
                if (nStart != nEnd)
                {
                    sNewIndices.push_back(L'-');
                    sNewIndices.append(std::to_wstring(nEnd));
                }

                sNewIndices.push_back(L',');
            }

            nStart = pRange.first;
            nEnd = pRange.second;
        }
    }

    sNewIndices.append(std::to_wstring(nStart));
    if (nStart != nEnd)
    {
        sNewIndices.push_back(L'-');
        sNewIndices.append(std::to_wstring(nEnd));
    }

    pClause1.SetIndices(sNewIndices);
}

bool TriggerSummaryViewModel::MergeClauses(TriggerSummaryViewModel::TriggerClauseViewModel& pClause,
    TriggerSummaryViewModel::TriggerClauseViewModel& pDiscardClause,
    gsl::index nDiscardIndex, TriggerClauseType nNewType, const std::wstring& sNewOperation)
{
    pClause.SetOperation(sNewOperation);
    pClause.nType = nNewType;
    MergeIndices(pClause, pDiscardClause);
    m_vClauses.RemoveAt(nDiscardIndex);
    return true;
}

bool TriggerSummaryViewModel::MergeClauses(
    TriggerSummaryViewModel::TriggerClauseViewModel& pClause1,
    TriggerSummaryViewModel::TriggerClauseViewModel& pClause2,
    gsl::index nIndex1, gsl::index nIndex2)
{
    if (pClause1.nType == TriggerClauseType::Is)
    {
        if (pClause2.nType == TriggerClauseType::IsNot)
        {
            if (IsSameMemoryReference(pClause1.pCondition->operand2, pClause2.pCondition->operand2))
            {
                if (pClause1.pCondition->operand1.memref_access_type == RC_OPERAND_ADDRESS)
                {
                    // a == x && da != x
                    return MergeClauses(pClause1, pClause2, nIndex2, TriggerClauseType::ChangedTo, L"changed to");
                }
                else if (pClause2.pCondition->operand1.memref_access_type == RC_OPERAND_ADDRESS)
                {
                    // da == x && a != x
                    return MergeClauses(pClause2, pClause1, nIndex1, TriggerClauseType::ChangedFrom, L"changed from");
                }

                return false;
            }

            // a == x && a != y  ~>  a == x
            m_vClauses.RemoveAt(nIndex2);
            return true;
        }
        else if (pClause2.nType == TriggerClauseType::Comparison &&
                 pClause1.pCondition->operand1.memref_access_type == RC_OPERAND_ADDRESS)
        {
            if (IsSameMemoryReference(pClause1.pCondition->operand2, pClause2.pCondition->operand2))
            {
                if (pClause2.pCondition->oper == RC_OPERATOR_LT)
                {
                    // a == x && da < x
                    return MergeClauses(pClause1, pClause2, nIndex2, TriggerClauseType::ChangedTo, L"increased to");
                }
                else if (pClause2.pCondition->oper == RC_OPERATOR_GT)
                {
                    // a == x && da > x
                    return MergeClauses(pClause1, pClause2, nIndex2, TriggerClauseType::ChangedTo, L"decreased to");
                }
            }
        }
        else if (pClause2.nType == TriggerClauseType::Is &&
                 pClause1.pCondition->operand1.size >= RC_MEMSIZE_BIT_0 &&
                 pClause1.pCondition->operand1.size <= RC_MEMSIZE_BIT_7)
        {
            if (pClause1.pCondition->operand2.type == RC_OPERAND_CONST &&
                pClause2.pCondition->operand2.type == RC_OPERAND_CONST &&
                pClause1.pCondition->operand2.value.num != pClause2.pCondition->operand2.value.num)
            {
                if (pClause1.pCondition->operand1.memref_access_type == RC_OPERAND_ADDRESS)
                {
                    // a == n && da == ~n
                    return MergeClauses(pClause1, pClause2, nIndex2, TriggerClauseType::ChangedTo, L"changed to");
                }
                else if(pClause2.pCondition->operand1.memref_access_type == RC_OPERAND_ADDRESS)
                {
                    // da == ~n && a == n
                    return MergeClauses(pClause2, pClause1, nIndex1, TriggerClauseType::ChangedTo, L"changed to");
                }

                return false;
            }
        }
    }
    else if (pClause2.nType == TriggerClauseType::Is)
    {
        return MergeClauses(pClause2, pClause1, nIndex2, nIndex1);
    }

    return false;
}

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

static void HandleOperation(TriggerSummaryViewModel::TriggerClauseViewModel& pClause, uint8_t nOperation)
{
    std::wstring sOperation;
    if (rc_operand_is_memref(&pClause.pCondition->operand1))
    {
        switch (pClause.pCondition->operand1.type)
        {
            case RC_OPERAND_ADDRESS:
                sOperation = L"is";
                break;

            case RC_OPERAND_PRIOR:
                sOperation = L"was";
                break;

            case RC_OPERAND_DELTA:
                sOperation = L"last frame was";
                break;
        }
    }
    else
    {
        sOperation = L"is";
    }

    switch (nOperation)
    {
        case RC_OPERATOR_EQ:
            pClause.nType = TriggerClauseType::Is;
            break;

        case RC_OPERATOR_NE:
            sOperation += L" not";
            pClause.nType = TriggerClauseType::IsNot;
            break;

        case RC_OPERATOR_GE:
            sOperation += L" at least";
            pClause.nType = TriggerClauseType::Comparison;
            break;

        case RC_OPERATOR_GT:
            sOperation += L" greater than";
            pClause.nType = TriggerClauseType::Comparison;
            break;

        case RC_OPERATOR_LE:
            sOperation += L" at most";
            pClause.nType = TriggerClauseType::Comparison;
            break;

        case RC_OPERATOR_LT:
            sOperation += L" less than";
            pClause.nType = TriggerClauseType::Comparison;
            break;
    }

    pClause.SetOperation(sOperation);
}

static std::wstring OperandToString(const rc_operand_t& pOperand)
{
    switch (pOperand.type)
    {
        case RC_OPERAND_CONST:
            return std::to_wstring(pOperand.value.num);

        case RC_OPERAND_ADDRESS:
        case RC_OPERAND_DELTA:
        case RC_OPERAND_PRIOR:
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
                // delta = delta  ~>  always true
                pClause.SetOperation(L"unimportant");
                pClause.nType = TriggerClauseType::AlwaysTrue;
                break;

            case RC_OPERATOR_NE:
            case RC_OPERATOR_GT:
            case RC_OPERATOR_LT:
                // delta != delta  ~>  always false
                pClause.SetOperation(L"invalid");
                pClause.nType = TriggerClauseType::AlwaysFalse;
                break;
        }
    }
    else
    {
        switch (pCondition.oper)
        {
            case RC_OPERATOR_EQ:
                pClause.SetOperation(L"hasn't changed");
                pClause.nType = TriggerClauseType::HasntChanged;
                break;

            case RC_OPERATOR_NE:
                pClause.SetOperation(L"changed");
                break;

            case RC_OPERATOR_LT:
                pClause.SetOperation(pCondition.operand1.memref_access_type == RC_OPERAND_ADDRESS
                    ? L"decreased"   // val < delta
                    : L"increased"); // delta < val
                pClause.nType = TriggerClauseType::Changed;
                break;

            case RC_OPERATOR_GT:
                pClause.SetOperation(pCondition.operand1.memref_access_type == RC_OPERAND_ADDRESS
                    ? L"increased"   // val > delta
                    : L"decreased"); // delta > val
                pClause.nType = TriggerClauseType::Changed;
                break;

            case RC_OPERATOR_LE:
                pClause.SetOperation(pCondition.operand1.memref_access_type == RC_OPERAND_ADDRESS
                    ? L"did not increase"   // val <= delta
                    : L"did not decrease"); // delta <= val
                pClause.nType = TriggerClauseType::Changed;
                break;

            case RC_OPERATOR_GE:
                pClause.SetOperation(pCondition.operand1.memref_access_type == RC_OPERAND_ADDRESS
                    ? L"did not decrease"   // val >= delta
                    : L"did not increase"); // delta >= val
                pClause.nType = TriggerClauseType::Changed;
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

        pClause.pCondition = pCondition;

        const ra::data::models::CodeNoteModel* pNote = nullptr;
        if (pCodeNotes && rc_operand_is_memref(&pCondition->operand1))
            pNote = pCodeNotes->FindCodeNoteModel(pCondition->operand1.value.memref->address);

        if (pNote)
            pClause.SetReference(pNote->GetSummary());
        else
            pClause.SetReference(OperandToString(pCondition->operand1));

        HandleOperation(pClause, pCondition->oper);

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
                HandleOperation(pClause, RC_OPERATOR_EQ);
            }

            const auto pEnumText = pNote->GetEnumText(nTarget);
            if (!pEnumText.empty())
            {
                pClause.SetTarget(EnumValueFromText(pEnumText));

                // a > 0  ~>  a != 0
                if (nTarget == 0 && pCondition->oper == RC_OPERATOR_GT)
                    HandleOperation(pClause, RC_OPERATOR_NE);
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

        if (rc_operand_is_memref(&pCondition->operand1))
        {
            for (gsl::index nIndex = 0; nIndex < m_vClauses.Count() - 1; ++nIndex)
            {
                auto* pOtherClause = m_vClauses.GetItemAt(nIndex);
                if (pOtherClause && IsSameMemoryReference(pCondition->operand1, pOtherClause->pCondition->operand1))
                {
                    if (MergeClauses(*pOtherClause, pClause, nIndex, m_vClauses.Count() - 1))
                        break;
                }
            }
        }
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
