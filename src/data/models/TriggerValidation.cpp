#include "TriggerValidation.hh"

#include "RA_StringUtils.h"
#include "RA_Defs.h"

#include "data/context/ConsoleContext.hh"
#include "data/context/GameContext.hh"
#include "data/context/EmulatorContext.hh"

#include "services/ServiceLocator.hh"

#include <rcheevos/src/rcheevos/rc_validate.h>

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
        case RC_MEMSIZE_FLOAT: return MemSize::Float;
        case RC_MEMSIZE_FLOAT_BE: return MemSize::FloatBigEndian;
        case RC_MEMSIZE_MBF32: return MemSize::MBF32;
        case RC_MEMSIZE_MBF32_LE: return MemSize::MBF32LE;
        case RC_MEMSIZE_DOUBLE32: return MemSize::Double32;
        case RC_MEMSIZE_DOUBLE32_BE: return MemSize::Double32BigEndian;
        default:
            assert(!"Unsupported operand size");
            return MemSize::EightBit;
    }
}

static bool ValidateLeaderboardCondSet(const rc_condset_t* pCondSet, std::wstring& sError)
{
    if (!pCondSet)
        return true;

    const auto* pCondition = pCondSet->conditions;
    for (; pCondition; pCondition = pCondition->next)
    {
        switch (pCondition->type)
        {
            case RC_CONDITION_MEASURED:
                sError = ra::StringPrintf(L"%s has no effect in leaderboard triggers", L"Measured");
                return false;
            case RC_CONDITION_MEASURED_IF:
                sError = ra::StringPrintf(L"%s has no effect in leaderboard triggers", L"MeasuredIf");
                return false;
            case RC_CONDITION_TRIGGER:
                sError = ra::StringPrintf(L"%s has no effect in leaderboard triggers", L"Trigger");
                return false;
            default:
                break;
        }
    }

    return true;
}

static bool ValidateLeaderboardTrigger(const rc_trigger_t* pTrigger, std::wstring& sError)
{
    if (!ValidateLeaderboardCondSet(pTrigger->requirement, sError))
        return false;

    const auto* pCondSet = pTrigger->alternative;
    for (; pCondSet; pCondSet = pCondSet->next)
    {
        if (!ValidateLeaderboardCondSet(pCondSet, sError))
            return false;
    }

    return true;
}

static bool ValidateCodeNotesOperand(const rc_operand_t& pOperand, const ra::data::models::CodeNotesModel& pNotes,
                                     std::wstring& sError)
{
    const auto nMemRefSize = TriggerValidation::MapRcheevosMemSize(pOperand.size);

    const auto nAddress = pOperand.value.memref->address;
    auto nNoteSize = pNotes.GetCodeNoteMemSize(nAddress);
    if (nMemRefSize == nNoteSize)
        return true;

    const auto nStartAddress = pNotes.FindCodeNoteStart(nAddress);
    if (nStartAddress != nAddress)
    {
        if (nStartAddress == 0xFFFFFFFF)
        {
            sError = ra::StringPrintf(L"No code note for address %s", ra::ByteAddressToString(nAddress).substr(2));
            return false;
        }
           
        nNoteSize = pNotes.GetCodeNoteMemSize(nStartAddress);
        if (nNoteSize == MemSize::Array)
        {
            // ignore addresses in the middle of an array
            return true;
        }
    }

    if (nNoteSize == MemSize::Unknown)
    {
        // note exists, but did not specify a size. assume 8-bit
        if (nMemRefSize == MemSize::EightBit)
            return true;

        nNoteSize = MemSize::EightBit;
    }

    // ignore bit reads inside a known address
    if (nMemRefSize == MemSize::BitCount || MemSizeBits(nMemRefSize) < 8)
        return true;

    sError = ra::StringPrintf(L"%s read of address %s differs from code note size %s",
        MemSizeString(nMemRefSize), ra::ByteAddressToString(nAddress).substr(2), MemSizeString(nNoteSize));

    if (nStartAddress != nAddress)
    {
        sError.append(L" at ");
        sError.append(ra::Widen(ra::ByteAddressToString(nStartAddress).substr(2)));
    }

    return false;
}

static bool ValidateCodeNotesCondSet(const rc_condset_t* pCondSet, const ra::data::models::CodeNotesModel& pNotes,
                                     std::wstring& sError)
{
    if (!pCondSet)
        return true;

    bool bIsAddAddressChain = false;
    size_t nIndex = 0;
    const auto* pCondition = pCondSet->conditions;
    for (; pCondition; pCondition = pCondition->next)
    {
        ++nIndex;
        if (pCondition->type == RC_CONDITION_ADD_ADDRESS)
        {
            bIsAddAddressChain = true;
            continue;
        }

        if (bIsAddAddressChain)
        {
            bIsAddAddressChain = false;
            continue;
        }

        if (rc_operand_is_memref(&pCondition->operand1) &&
            !ValidateCodeNotesOperand(pCondition->operand1, pNotes, sError))
        {
            sError = ra::StringPrintf(L"Condition %u: %s", nIndex, sError);
            return false;
        }

        if (rc_operand_is_memref(&pCondition->operand2) &&
            !ValidateCodeNotesOperand(pCondition->operand2, pNotes, sError))
        {
            sError = ra::StringPrintf(L"Condition %u: %s", nIndex, sError);
            return false;
        }
    }

    return true;
}

static bool ValidateCodeNotes(const rc_trigger_t* pTrigger, std::wstring& sError)
{
    if (!ra::services::ServiceLocator::Exists<ra::data::context::GameContext>())
        return true;

    const auto* pNotes = ra::services::ServiceLocator::Get<ra::data::context::GameContext>().Assets().FindCodeNotes();
    if (!pNotes)
        return true;

    if (!ValidateCodeNotesCondSet(pTrigger->requirement, *pNotes, sError))
    {
        if (pTrigger->alternative)
            sError = L"Core " + sError;
        return false;
    }

    size_t nIndex = 0;
    const auto* pCondSet = pTrigger->alternative;
    for (; pCondSet; pCondSet = pCondSet->next)
    {
        nIndex++;
        if (!ValidateCodeNotesCondSet(pCondSet, *pNotes, sError))
        {
            sError = ra::StringPrintf(L"Alt%u %s", nIndex, sError);
            return false;
        }
    }

    return true;
}

bool TriggerValidation::Validate(const std::string& sTrigger, std::wstring& sError, AssetType nType)
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

    char sErrorBuffer[256] = "";
    int nResult = 1;

    if (ra::services::ServiceLocator::Exists<ra::data::context::ConsoleContext>())
    {
        const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();
        unsigned nMaxAddress = pConsoleContext.MaxAddress();

        if (nMaxAddress == 0)
        {
            // if console definition doesn't specify the max address, see how much was exposed by the emulator
            const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
            nMaxAddress = gsl::narrow_cast<unsigned>(pEmulatorContext.TotalMemorySize()) - 1;

            nResult = rc_validate_trigger(pTrigger, sErrorBuffer, sizeof(sErrorBuffer), nMaxAddress);
        }
        else
        {
            // if console definition does specify a max address, call the console-specific validator for additional validation
            nResult = rc_validate_trigger_for_console(pTrigger, sErrorBuffer, sizeof(sErrorBuffer), ra::etoi(pConsoleContext.Id()));
        }
    }
    else
    {
        // shouldn't get here, but if we do (unit tests), validate the logic but not the addresses.
        nResult = rc_validate_trigger(pTrigger, sErrorBuffer, sizeof(sErrorBuffer), 0xFFFFFFFF);
    }

    if (nResult)
    {
        if (nType == AssetType::Leaderboard)
        {
            if (!ValidateLeaderboardTrigger(pTrigger, sError))
                return false;
        }

        if (!ValidateCodeNotes(pTrigger, sError))
            return false;

        sError.clear();
        return true;
    }
    else
    {
        sError = ra::Widen(sErrorBuffer);
        return false;
    }
}

} // namespace models
} // namespace data
} // namespace ra
