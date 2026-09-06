#include "TriggerValidation.hh"

#include "context/IConsoleContext.hh"
#include "context/IEmulatorMemoryContext.hh"
#include "context/IGameContext.hh"

#include "data/models/MemoryNotesModel.hh"

#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

#include <rcheevos/src/rcheevos/rc_validate.h>
#include <rcheevos/src/rcheevos/rc_internal.h>

namespace ra {
namespace data {
namespace util {

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
                sError = ra::util::String::Printf(L"%s has no effect in leaderboard triggers", L"Measured");
                return false;
            case RC_CONDITION_MEASURED_IF:
                sError = ra::util::String::Printf(L"%s has no effect in leaderboard triggers", L"MeasuredIf");
                return false;
            case RC_CONDITION_TRIGGER:
                sError = ra::util::String::Printf(L"%s has no effect in leaderboard triggers", L"Trigger");
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

static std::wstring FormatAddress(ra::data::ByteAddress nAddress, bool bIsOffset)
{
    if (bIsOffset)
    {
        if (nAddress < 10)
            return L"offset " + std::to_wstring(nAddress);

        return ra::util::String::Printf(L"offset %02x", nAddress);
    }

    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    return L"address " + pMemoryContext.FormatAddress(nAddress).substr(2);
}

static const ra::data::models::MemoryNoteModel* ValidateMemoryNotesOperand(
    const ra::data::models::MemoryNotesModel& pNotes,
    const ra::data::models::MemoryNoteModel* pParentNote,
    const rc_operand_t& pOperand, bool bIsAddAddressChain,
    std::wstring& sError)
{
    // Find the memory note for the operand.
    const ra::data::models::MemoryNoteModel* pOperandNote = nullptr;
    const auto nAddress = pOperand.value.memref->address;
    auto nStartAddress = nAddress;
    if (!bIsAddAddressChain)
        pOperandNote = pNotes.FindMemoryNoteModel(nAddress, false);
    else if (pParentNote)
        pOperandNote = pParentNote->GetPointerNoteAtOffset(nAddress);

    if (!pOperandNote)
    {
        if (!bIsAddAddressChain)
        {
            // No note at address. See if it's included in a larger container note.
            nStartAddress = pNotes.FindNoteStart(nAddress);
            if (nStartAddress != 0xFFFFFFFF)
                pOperandNote = pNotes.FindMemoryNoteModel(nStartAddress, false);
        }

        if (!pOperandNote)
        {
            sError = ra::util::String::Printf(L"No memory note for %s", FormatAddress(nAddress, bIsAddAddressChain));
            return nullptr;
        }
    }

    // Ignore bit/nibble reads inside a known address.
    const auto nMemRefSize = Memory::SizeFromRcheevosSize(pOperand.size);
    if (nMemRefSize == Memory::Size::BitCount || Memory::SizeBits(nMemRefSize) < 8)
        return pOperandNote;

    // Size match. Note is valid.
    const Memory::Size nNoteSize = pOperandNote->GetMemSize();
    if (nNoteSize == nMemRefSize)
        return pOperandNote;

    // "array" and "text" are not real sizes to validate against.
    if (nNoteSize == Memory::Size::Array || nNoteSize == Memory::Size::Text)
        return pOperandNote;

    // A pointer may be masked by reading a smaller size.
    if (pOperandNote->IsPointer())
    {
        const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
        ra::data::Memory::Size nReadSize;
        uint32_t nMask, nOffset;
        if (pConsoleContext.GetRealAddressConversion(&nReadSize, &nMask, &nOffset))
        {
            if (nReadSize == nMemRefSize)
                return pOperandNote;
        }
    }

    // If the note did not specify a size, assume 8-bit.
    if (nNoteSize == Memory::Size::Unknown)
    {
        // Ignore bit/nibble reads.
        if (Memory::SizeBits(nMemRefSize) <= 8)
            return pOperandNote;

        sError = ra::util::String::Printf(L"%s read of %s differs from implied memory note size %s",
            Memory::SizeString(nMemRefSize), FormatAddress(nAddress, bIsAddAddressChain),
            Memory::SizeString(Memory::Size::EightBit));
    }
    else
    {
        sError = ra::util::String::Printf(L"%s read of %s differs from memory note size %s",
            Memory::SizeString(nMemRefSize), FormatAddress(nAddress, bIsAddAddressChain),
            Memory::SizeString(nNoteSize));
    }

    if (nStartAddress != nAddress)
    {
        sError.append(L" at ");
        sError.append(FormatAddress(nStartAddress, bIsAddAddressChain));
    }

    return nullptr;
}

static bool ValidateMemoryNotesCondSet(const rc_condset_t* pCondSet, const ra::data::models::MemoryNotesModel& pNotes,
                                       std::wstring& sError)
{
    if (!pCondSet)
        return true;

    bool bIsAddAddressChain = false;
    size_t nIndex = 0;
    const auto* pCondition = pCondSet->conditions;
    const ra::data::models::MemoryNoteModel* pParentNote = nullptr;
    for (; pCondition; pCondition = pCondition->next)
    {
        ++nIndex;

        const auto* pOperand1 = rc_condition_get_real_operand1(pCondition);
        if (!pOperand1)
            continue;

        const ra::data::models::MemoryNoteModel* pOperand1Note = nullptr;
        if (rc_operand_is_memref(pOperand1))
        {
            pOperand1Note = ValidateMemoryNotesOperand(pNotes, pParentNote, *pOperand1, bIsAddAddressChain, sError);
            if (!pOperand1Note)
            {
                sError = ra::util::String::Printf(L"Condition %u: %s", nIndex, sError);
                return false;
            }
        }

        if (rc_operand_is_memref(&pCondition->operand2))
        {
            const auto* pOperand2Note = ValidateMemoryNotesOperand(pNotes, pParentNote, pCondition->operand2, bIsAddAddressChain, sError);
            if (!pOperand2Note)
            {
                sError = ra::util::String::Printf(L"Condition %u: %s", nIndex, sError);
                return false;
            }
        }
        else if (pCondition->oper == RC_OPERATOR_AND && pCondition->operand2.type == RC_OPERAND_CONST)
        {
            const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
            ra::data::Memory::Size nReadSize;
            uint32_t nMask, nOffset;
            if (pConsoleContext.GetRealAddressConversion(&nReadSize, &nMask, &nOffset))
            {
                if (nMask != 0xFFFFFFFF && nMask > pCondition->operand2.value.num)
                {
                    sError = ra::util::String::Printf(L"Condition %u: Pointer mask is too small for system", nIndex);
                    return false;
                }
            }
        }

        if (pCondition->type == RC_CONDITION_ADD_ADDRESS)
        {
            pParentNote = pOperand1Note;
            bIsAddAddressChain = true;
        }
        else
        {
            bIsAddAddressChain = false;
        }
    }

    return true;
}

static bool ValidateMemoryNotes(const rc_trigger_t* pTrigger, std::wstring& sError)
{
    if (!ra::services::ServiceLocator::Exists<ra::context::IGameContext>())
        return true;

    const auto& pNotes = ra::services::ServiceLocator::Get<ra::context::IGameContext>().MemoryNotes();

    if (!ValidateMemoryNotesCondSet(pTrigger->requirement, pNotes, sError))
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
        if (!ValidateMemoryNotesCondSet(pCondSet, pNotes, sError))
        {
            sError = ra::util::String::Printf(L"Alt%u %s", nIndex, sError);
            return false;
        }
    }

    return true;
}

bool TriggerValidation::Validate(const std::string& sTrigger, std::wstring& sError, ra::data::models::AssetType nType)
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
        sError = ra::util::String::Widen(rc_error_str(nSize));
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
        if (nType == ra::data::models::AssetType::Leaderboard)
        {
            if (!ValidateLeaderboardTrigger(&trigger->trigger, sError))
                return false;
        }

        if (!ValidateMemoryNotes(&trigger->trigger, sError))
            return false;

        sError.clear();
        return true;
    }
    else
    {
        sError = ra::util::String::Widen(sErrorBuffer);
        return false;
    }
}

} // namespace util
} // namespace data
} // namespace ra
