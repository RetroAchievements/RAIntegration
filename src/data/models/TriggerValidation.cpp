#include "TriggerValidation.hh"

#include "context/IConsoleContext.hh"

#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

#include "RA_Defs.h"

#include "data/context/GameContext.hh"
#include "data/context/EmulatorContext.hh"

#include <rcheevos/src/rcheevos/rc_validate.h>
#include <rcheevos/src/rcheevos/rc_internal.h>

namespace ra {
namespace data {
namespace models {

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
    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    const auto nMemRefSize = Memory::SizeFromRcheevosSize(pOperand.size);

    const auto nAddress = pOperand.value.memref->address;
    ra::data::ByteAddress nStartAddress = nAddress;
    Memory::Size nNoteSize = Memory::Size::Unknown;

    const auto* pNote = pNotes.FindCodeNoteModel(nAddress, false);
    if (pNote)
    {
        // ignore bit/nibble reads inside a known address
        if (nMemRefSize == Memory::Size::BitCount || Memory::SizeBits(nMemRefSize) < 8)
            return true;

        nNoteSize = pNote->GetMemSize();
    }
    else
    {
        // no note at address. see if it's included in a larger container note
        nStartAddress = pNotes.FindCodeNoteStart(nAddress);
        if (nStartAddress == 0xFFFFFFFF)
        {
            sError = ra::StringPrintf(L"No code note for address %s", pMemoryContext.FormatAddress(nAddress).substr(2));
            return false;
        }

        nNoteSize = pNotes.GetCodeNoteMemSize(nStartAddress);
    }

    if (nMemRefSize == nNoteSize)
        return true;

    // "array" and "text" are not real sizes to validate against
    if (nNoteSize == Memory::Size::Array || nNoteSize == Memory::Size::Text)
        return true;

    if (nNoteSize == Memory::Size::Unknown)
    {
        // note exists, but did not specify a size. assume 8-bit
        if (Memory::SizeBits(nMemRefSize) <= 8)
            return true;

        sError = ra::StringPrintf(L"%s read of address %s differs from implied code note size %s", Memory::SizeString(nMemRefSize),
                                  pMemoryContext.FormatAddress(nAddress).substr(2), Memory::SizeString(Memory::Size::EightBit));
    }
    else
    {
        sError = ra::StringPrintf(L"%s read of address %s differs from code note size %s", Memory::SizeString(nMemRefSize),
                                  pMemoryContext.FormatAddress(nAddress).substr(2), Memory::SizeString(nNoteSize));
    }

    if (nStartAddress != nAddress)
    {
        sError.append(L" at ");
        sError.append(pMemoryContext.FormatAddress(nStartAddress).substr(2));
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

        const auto* pOperand1 = rc_condition_get_real_operand1(pCondition);
        if (pOperand1 && rc_operand_is_memref(pOperand1) &&
            !ValidateCodeNotesOperand(*pOperand1, pNotes, sError))
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
    rc_preparse_state_t preparse;
    rc_init_preparse_state(&preparse);

    rc_trigger_with_memrefs_t* trigger = RC_ALLOC(rc_trigger_with_memrefs_t, &preparse.parse);
    const char* sMemaddr = sTrigger.c_str();
    rc_parse_trigger_internal(&trigger->trigger, &sMemaddr, &preparse.parse);
    rc_preparse_alloc_memrefs(nullptr, &preparse);

    const auto nSize = preparse.parse.offset;
    if (nSize < 0)
    {
        sError = ra::Widen(rc_error_str(nSize));
        return false;
    }

    std::string sTriggerBuffer;
    sTriggerBuffer.resize(nSize);

    rc_reset_parse_state(&preparse.parse, sTriggerBuffer.data());
    trigger = RC_ALLOC(rc_trigger_with_memrefs_t, &preparse.parse);
    rc_preparse_alloc_memrefs(&trigger->memrefs, &preparse);

    sMemaddr = sTrigger.c_str();
    rc_parse_trigger_internal(&trigger->trigger, &sMemaddr, &preparse.parse);
    trigger->trigger.has_memrefs = 1;

    char sErrorBuffer[256] = "";
    int nResult = 1;

    if (ra::services::ServiceLocator::Exists<ra::context::IConsoleContext>())
    {
        const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
        unsigned nMaxAddress = pConsoleContext.MaxAddress();

        if (nMaxAddress == 0)
        {
            // if console definition doesn't specify the max address, see how much was exposed by the emulator
            const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
            nMaxAddress = gsl::narrow_cast<unsigned>(pMemoryContext.TotalMemorySize()) - 1;

            nResult = rc_validate_trigger(&trigger->trigger, sErrorBuffer, sizeof(sErrorBuffer), nMaxAddress);
        }
        else
        {
            // if console definition does specify a max address, call the console-specific validator for additional validation
            nResult = rc_validate_trigger_for_console(&trigger->trigger, sErrorBuffer, sizeof(sErrorBuffer),
                                                      ra::etoi(pConsoleContext.Id()));
        }
    }
    else
    {
        // shouldn't get here, but if we do (unit tests), validate the logic but not the addresses.
        nResult = rc_validate_trigger(&trigger->trigger, sErrorBuffer, sizeof(sErrorBuffer), 0xFFFFFFFF);
    }

    if (nResult)
    {
        if (nType == AssetType::Leaderboard)
        {
            if (!ValidateLeaderboardTrigger(&trigger->trigger, sError))
                return false;
        }

        if (!ValidateCodeNotes(&trigger->trigger, sError))
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
