#include "TriggerValidation.hh"

#include "RA_StringUtils.h"

#include "data/context/EmulatorContext.hh"

#include "services/ServiceLocator.hh"

#include <rcheevos/include/rc_runtime_types.h>

namespace ra {
namespace data {
namespace models {

MemSize TriggerValidation::MapRcheevosMemSize(char nSize) noexcept
{
    switch (nSize)
    {
        case RC_MEMSIZE_BIT_0: return MemSize::Bit_0;
        case RC_MEMSIZE_BIT_1: return MemSize::Bit_1;
        case RC_MEMSIZE_BIT_2: return MemSize::Bit_2;
        case RC_MEMSIZE_BIT_3: return MemSize::Bit_3;
        case RC_MEMSIZE_BIT_4: return MemSize::Bit_4;
        case RC_MEMSIZE_BIT_5: return MemSize::Bit_5;
        case RC_MEMSIZE_BIT_6: return MemSize::Bit_6;
        case RC_MEMSIZE_BIT_7: return MemSize::Bit_7;
        case RC_MEMSIZE_LOW: return MemSize::Nibble_Lower;
        case RC_MEMSIZE_HIGH: return MemSize::Nibble_Upper;
        case RC_MEMSIZE_8_BITS: return MemSize::EightBit;
        case RC_MEMSIZE_16_BITS: return MemSize::SixteenBit;
        case RC_MEMSIZE_24_BITS: return MemSize::TwentyFourBit;
        case RC_MEMSIZE_32_BITS: return MemSize::ThirtyTwoBit;
        case RC_MEMSIZE_BITCOUNT: return MemSize::BitCount;
        case RC_MEMSIZE_16_BITS_BE: return MemSize::SixteenBitBigEndian;
        case RC_MEMSIZE_24_BITS_BE: return MemSize::TwentyFourBitBigEndian;
        case RC_MEMSIZE_32_BITS_BE: return MemSize::ThirtyTwoBitBigEndian;
        default:
            assert(!"Unsupported operand size");
            return MemSize::EightBit;
    }
}

static unsigned MaxValue(const rc_operand_t* pOperand)
{
    if (pOperand->type == RC_OPERAND_CONST)
        return pOperand->value.num;

    if (!rc_operand_is_memref(pOperand))
        return 0xFFFFFFFF;

    return MemSizeMax(TriggerValidation::MapRcheevosMemSize(pOperand->size));
}

static int ValidateRange(unsigned nMinValue, unsigned nMaxValue, char nOperator, unsigned nMax, std::wstring& sError) {
    switch (nOperator) {
        case RC_OPERATOR_AND:
            if (nMinValue > nMax) {
                sError = L"Mask has more bits than source";
                return false;
            }
            else if (nMinValue == 0 && nMaxValue == 0) {
                sError = L"Result of mask is always 0";
                return false;
            }
            break;

        case RC_OPERATOR_EQ:
            if (nMinValue > nMax) {
                sError = L"Comparison is never true";
                return false;
            }
            break;

        case RC_OPERATOR_NE:
            if (nMinValue > nMax) {
                sError = L"Comparison is always true";
                return false;
            }
            break;

        case RC_OPERATOR_GE:
            if (nMinValue > nMax) {
                sError = L"Comparison is never true";
                return false;
            }
            if (nMaxValue == 0) {
                sError = L"Comparison is always true";
                return false;
            }
            break;

        case RC_OPERATOR_GT:
            if (nMinValue >= nMax) {
                sError = L"Comparison is never true";
                return false;
            }
            break;

        case RC_OPERATOR_LE:
            if (nMinValue >= nMax) {
                sError = L"Comparison is always true";
                return false;
            }
            break;

        case RC_OPERATOR_LT:
            if (nMinValue > nMax) {
                sError = L"Comparison is always true";
                return false;
            }
            if (nMaxValue == 0) {
                sError = L"Comparison is never true";
                return false;
            }
            break;
    }

    return true;
}

static bool ValidateCondSet(const rc_condset_t* pCondSet, std::wstring& sError)
{
    bool bInAddHits = false;
    bool bIsCombining = false;
    bool bInAddAddress = false;
    unsigned long long nAddSourceMax = 0;
    ra::ByteAddress nMaxAddress = 0;

    int nIndex = 1;
    const rc_condition_t* pCondition = pCondSet->conditions;
    for (; pCondition != nullptr; ++nIndex, pCondition = pCondition->next)
    {
        unsigned nMax = MaxValue(&pCondition->operand1); // maximum possible calculated value for comparison

        if (!bInAddAddress)
        {
            if (nMaxAddress == 0)
            {
                if (ra::services::ServiceLocator::Exists<ra::data::context::EmulatorContext>())
                {
                    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
                    nMaxAddress = gsl::narrow_cast<ra::ByteAddress>(pEmulatorContext.TotalMemorySize());
                }

                if (nMaxAddress == 0)
                    nMaxAddress = 0xFFFFFFFF;
                else
                    nMaxAddress--;
            }

            if ((rc_operand_is_memref(&pCondition->operand1) && pCondition->operand1.value.memref->address > nMaxAddress) ||
                (rc_operand_is_memref(&pCondition->operand2) && pCondition->operand2.value.memref->address > nMaxAddress))
            {
                sError = ra::StringPrintf(L"Condition %u: Address out of range (max %04X).", nIndex, nMaxAddress);
                return false;
            }
        }

        switch (pCondition->type)
        {
            case RC_CONDITION_ADD_SOURCE:
                nAddSourceMax += nMax;
                bIsCombining = true;
                continue;

            case RC_CONDITION_SUB_SOURCE:
                if (nAddSourceMax < nMax) // ignore potential underflow - may be expected
                    nAddSourceMax = 0xFFFFFFFF;
                bIsCombining = true;
                continue;

            case RC_CONDITION_ADD_HITS:
            case RC_CONDITION_SUB_HITS:
                bIsCombining = true;
                bInAddHits = true;
                break;

            case RC_CONDITION_ADD_ADDRESS:
            case RC_CONDITION_AND_NEXT:
            case RC_CONDITION_OR_NEXT:
            case RC_CONDITION_RESET_NEXT_IF:
                bIsCombining = true;
                break;

            default:
                if (bInAddHits)
                {
                    if (pCondition->required_hits == 0)
                    {
                        sError = ra::StringPrintf(L"Condition %u: Final condition in AddHits chain must have a hit target.", nIndex);
                        return false;
                    }

                    bInAddHits = false;
                }

                bIsCombining = false;
                break;
        }

        if (nAddSourceMax)
        {
            const unsigned long long overflow = nAddSourceMax + nMax;
            if (overflow > 0xFFFFFFFFUL)
                nMax = 0xFFFFFFFF;
            else
                nMax = gsl::narrow_cast<unsigned>(overflow);
        }

        // maximum comparison value
        const unsigned nMaxValue = MaxValue(&pCondition->operand2);

        // minimum comparison value
        const unsigned nMinValue = (pCondition->operand2.type == RC_OPERAND_CONST) ? pCondition->operand2.value.num : 0;

        // if comparing two memrefs outside of an AddSource chain, make sure their sizes match
        const bool bIsMemRef = rc_operand_is_memref(&pCondition->operand1);
        if (bIsMemRef && nMaxValue != nMax && nAddSourceMax == 0 && rc_operand_is_memref(&pCondition->operand2))
        {
            sError = ra::StringPrintf(L"Condition %u: Comparing different memory sizes", nIndex);
            return false;
        }

        // if nMax is from a memory reference or AddSource chain, or the right side is a memory reference,
        // validate that nMax can fall between bMinValue and nMaxValue
        if (bIsMemRef || nMinValue == 0 || nAddSourceMax)
        {
            if (!ValidateRange(nMinValue, nMaxValue, pCondition->oper, nMax, sError))
            {
                sError.insert(0, L"Condition : ");
                sError.insert(10, std::to_wstring(nIndex));
                return false;
            }
        }

        // reset AddSource chain
        nAddSourceMax = 0;

        // AddAddress only ever effects the next condition
        bInAddAddress = (pCondition->type == RC_CONDITION_ADD_ADDRESS);
    }

    if (bIsCombining)
    {
        sError = L"Final condition type expects another condition to follow.";
        return false;
    }

    return true;
}

static bool ValidateTrigger(const rc_trigger_t* pTrigger, std::wstring& sError)
{
    if (pTrigger->requirement && !ValidateCondSet(pTrigger->requirement, sError))
    {
        if (pTrigger->alternative != nullptr)
            sError.insert(0, L"Core: ");

        return false;
    }

    int nAlt = 1;
    const rc_condset_t* pCondSet = pTrigger->alternative;
    while (pCondSet != nullptr)
    {
        if (!ValidateCondSet(pCondSet, sError))
        {
            sError.insert(0, L"Alt : ");
            sError.insert(4, std::to_wstring(nAlt));
            return false;
        }

        ++nAlt;
        pCondSet = pCondSet->next;
    }

    return true;
}

bool TriggerValidation::Validate(const std::string& sTrigger, std::wstring& sError)
{
    const auto nSize = rc_trigger_size(sTrigger.c_str());
    if (nSize < 0)
    {
        sError = ra::Widen(rc_error_str(nSize));
        return false;
    }

    std::string sTriggerBuffer;
    sTriggerBuffer.resize(nSize);
    const auto* pTrigger = rc_parse_trigger(sTriggerBuffer.data(), sTrigger.c_str(), nullptr, 0);

    return ValidateTrigger(pTrigger, sError);
}

} // namespace models
} // namespace data
} // namespace ra
