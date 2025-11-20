#include "MemoryWatchViewModel.hh"

#include "RA_Defs.h"
#include "RA_Json.h"
#include "util\Strings.hh"

#include "data\Types.hh"
#include "data\context\ConsoleContext.hh"
#include "data\context\EmulatorContext.hh"
#include "data\models\TriggerValidation.hh"

#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"
#include "services\SearchResults.h"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\WindowManager.hh"

#include <rcheevos/src/rcheevos/rc_internal.h>
#include <rcheevos/src/rc_client_internal.h>

#ifdef RA_UTEST
// awkward workaround to allow individual bookmarks access to the bookmarks view model being tested
extern ra::ui::viewmodels::MemoryBookmarksViewModel* g_pMemoryBookmarksViewModel;
#endif

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty MemoryWatchViewModel::DescriptionProperty("MemoryWatchViewModel", "Description", L"");
const BoolModelProperty MemoryWatchViewModel::IsCustomDescriptionProperty("MemoryWatchViewModel", "IsCustomDescription", false);
const StringModelProperty MemoryWatchViewModel::RealNoteProperty("MemoryWatchViewModel", "Description", L"");
const IntModelProperty MemoryWatchViewModel::AddressProperty("MemoryWatchViewModel", "Address", 0);
const IntModelProperty MemoryWatchViewModel::SizeProperty("MemoryWatchViewModel", "Size", ra::etoi(MemSize::EightBit));
const IntModelProperty MemoryWatchViewModel::FormatProperty("MemoryWatchViewModel", "Format", ra::etoi(MemFormat::Hex));
const StringModelProperty MemoryWatchViewModel::CurrentValueProperty("MemoryWatchViewModel", "CurrentValue", L"0");
const StringModelProperty MemoryWatchViewModel::PreviousValueProperty("MemoryWatchViewModel", "PreviousValue", L"0");
const IntModelProperty MemoryWatchViewModel::ChangesProperty("MemoryWatchViewModel", "Changes", 0);
const IntModelProperty MemoryWatchViewModel::RowColorProperty("MemoryWatchViewModel", "RowColor", 0);
const BoolModelProperty MemoryWatchViewModel::ReadOnlyProperty("MemoryWatchViewModel", "IsReadOnly", false);
const BoolModelProperty MemoryWatchViewModel::IsWritingMemoryProperty("MemoryWatchViewModel", "IsWritingMemory", false);

void MemoryWatchViewModel::SetAddressWithoutUpdatingValue(ra::ByteAddress nNewAddress)
{
    // set m_bInitialized to false while updating the address to prevent synchronizing the value
    const bool bInitialized = m_bInitialized;
    m_bInitialized = false;

    SetAddress(nNewAddress);

    m_bInitialized = bInitialized;
}

GSL_SUPPRESS_F6
void MemoryWatchViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryWatchViewModel::FormatProperty)
    {
        if (m_bInitialized)
        {
            m_bModified = true;
            SetValue(CurrentValueProperty, BuildCurrentValue());
        }
    }
    else if (args.Property == MemoryWatchViewModel::SizeProperty)
    {
        m_nSize = ra::itoe<MemSize>(args.tNewValue);
        OnSizeChanged();
    }
    else if (args.Property == MemoryWatchViewModel::AddressProperty)
    {
        m_nAddress = args.tNewValue;

        if (m_bInitialized)
        {
            BeginInitialization();
            EndInitialization();
            m_bModified = true;
        }
    }

    LookupItemViewModel::OnValueChanged(args);
}

void MemoryWatchViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryWatchViewModel::RealNoteProperty)
    {
        const auto sNewHeader = ExtractDescriptionHeader(args.tNewValue);
        if (sNewHeader == GetDescription())
        {
            SetIsCustomDescription(false);
        }
        else if (!IsCustomDescription())
        {
            m_bSyncingDescriptionHeader = true;
            SetDescription(sNewHeader);
            m_bSyncingDescriptionHeader = false;
        }
    }
    else if (args.Property == MemoryWatchViewModel::DescriptionProperty && !m_bSyncingDescriptionHeader)
    {
        const auto& sFullNote = GetRealNote();
        const auto sHeader = ExtractDescriptionHeader(sFullNote);
        if (args.tNewValue.empty() && !sHeader.empty())
        {
            // custom description was cleared out - reset to default
            m_bSyncingDescriptionHeader = true;
            SetDescription(sHeader);
            m_bSyncingDescriptionHeader = false;
            SetIsCustomDescription(false);
        }
        else
        {
            SetIsCustomDescription(sHeader != args.tNewValue);
        }
    }

    LookupItemViewModel::OnValueChanged(args);
}

std::wstring MemoryWatchViewModel::ExtractDescriptionHeader(const std::wstring& sFullNote)
{
    // extract the first line into DescriptionHeader
    const auto nIndex = sFullNote.find('\n');
    if (nIndex == std::string::npos)
        return ra::data::models::CodeNoteModel::TrimSize(sFullNote, true);

    std::wstring sNote = sFullNote;
    sNote.resize(nIndex);

    if (sNote.back() == '\r')
        sNote.pop_back();

    return ra::data::models::CodeNoteModel::TrimSize(sNote, true);
}

static rc_condition_t* FindMeasuredCondition(rc_value_t* pValue) noexcept
{
    rc_condition_t* condition = pValue->conditions->conditions;
    for (; condition; condition = condition->next)
    {
        if (condition->type == RC_CONDITION_MEASURED && rc_operand_is_memref(&condition->operand1))
            return condition;
    }

    return nullptr;
}

void MemoryWatchViewModel::OnSizeChanged()
{
    switch (m_nSize)
    {
        case MemSize::BitCount:
        case MemSize::Text:
            SetReadOnly(true);
            break;

        default:
            SetReadOnly(false);
            break;
    }

    if (m_bInitialized)
    {
        m_bModified = true;

        if (m_pValue)
        {
            const auto* pCondition = FindMeasuredCondition(m_pValue);
            if (pCondition)
            {
                std::string sSerialized;
                ra::services::AchievementLogicSerializer::AppendOperand(
                    sSerialized, ra::services::TriggerOperandType::Address, m_nSize, 0U);

                const char* memaddr = sSerialized.c_str();
                uint32_t unused;
                uint8_t nNewSize;
                rc_parse_memref(&memaddr, &nNewSize, &unused);

                GSL_SUPPRESS_TYPE3
                auto* pOperand = const_cast<rc_operand_t*>(rc_condition_get_real_operand1(pCondition));
                pOperand->value.memref->value.size = nNewSize;
                pOperand->size = nNewSize;
            }
        }

        ra::data::context::EmulatorContext::DispatchesReadMemory::DispatchMemoryRead([this]() {
            m_nValue = ReadValue();
            SetValue(CurrentValueProperty, BuildCurrentValue());
        });
    }
}

uint32_t MemoryWatchViewModel::ReadValue()
{
    if (m_pValue)
    {
        rc_typed_value_t value;
        rc_evaluate_value_typed(m_pValue, &value, rc_peek_callback, nullptr);

        if (IsIndirectAddress())
            UpdateCurrentAddressFromIndirectAddress();

        if (m_nSize != MemSize::Text)
        {
            // if value is a float, convert it back to the raw bytes appropriate for the size
            if (ra::data::MemSizeIsFloat(m_nSize))
                return ra::data::FloatToU32(value.value.f32, m_nSize);

            return value.value.u32;
        }

        // MemSize::Text requires special processing. now that m_nAddress has been
        // updated, proceeed to logic below to do the special processing.
    }

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (m_nSize == MemSize::Text)
    {
        // only have 32 bits to store the value in. generate a hash for the string
        std::array<uint8_t, MaxTextBookmarkLength + 1> pBuffer;
        pEmulatorContext.ReadMemory(m_nAddress, &pBuffer.at(0), pBuffer.size() - 1);
        pBuffer.at(pBuffer.size() - 1) = '\0';

        const char* pText;
        GSL_SUPPRESS_TYPE1 pText = reinterpret_cast<char*>(&pBuffer.at(0));
        std::string sText(pText);

        return ra::StringHash(sText);
    }

    return pEmulatorContext.ReadMemory(m_nAddress, m_nSize);
}

void MemoryWatchViewModel::EndInitialization()
{
    m_nValue = 0;
    SetPreviousValue(BuildCurrentValue());

    ra::data::context::EmulatorContext::DispatchesReadMemory::DispatchMemoryRead([this]() {
        m_nValue = ReadValue();
        SetValue(CurrentValueProperty, BuildCurrentValue());
    });

    SetChanges(0);

    m_bModified = false;
    m_bInitialized = true;
}

bool MemoryWatchViewModel::SetCurrentValue(const std::wstring& sValue, _Out_ std::wstring& sError)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    const auto nAddress = m_nAddress;
    unsigned nValue = 0;

    if (ra::data::MemSizeIsFloat(m_nSize))
    {
        float fValue;
        if (!ra::ParseFloat(sValue, fValue, sError))
            return false;

        nValue = ra::data::FloatToU32(fValue, m_nSize);
    }
    else
    {
        const auto nMaximumValue = ra::data::MemSizeMax(m_nSize);

        if (GetFormat() == MemFormat::Dec)
        {
            if (!ra::ParseUnsignedInt(sValue, nMaximumValue, nValue, sError))
                return false;
        }
        else
        {
            if (!ra::ParseHex(sValue, nMaximumValue, nValue, sError))
                return false;
        }
    }

    // set m_nValue directly to avoid bookmark behaviors from firing
    m_nValue = nValue;

    // use the IsWritingMemoryProperty to disable the event handler in the list
    // while we write the memory so it doesn't try to sync the value back here
    SetValue(IsWritingMemoryProperty, true);
    pEmulatorContext.WriteMemory(nAddress, m_nSize, nValue);
    SetValue(IsWritingMemoryProperty, false);

    // update the fields dependent on m_nValue
    OnValueChanged();

#ifndef RA_UTEST
    // memory inspector does not automatically redraw if the emulator is paused. force it to redraw.
    // will be a no-op if the modified memory is not in the visible addresses.
    auto& vmMemoryInspector = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryInspector;
    vmMemoryInspector.Viewer().Redraw();
#endif

    return true;
}

bool MemoryWatchViewModel::DoFrame()
{
    const auto nValue = ReadValue();

    if (IsIndirectAddress() && IgnoreValueChange(nValue))
        return false;

    return ChangeValue(nValue);
}

bool MemoryWatchViewModel::ChangeValue(uint32_t nNewValue)
{
    if (m_nValue == nNewValue)
        return false;

    m_nValue = nNewValue;
    OnValueChanged();

    return true;
}

void MemoryWatchViewModel::UpdateCurrentValue()
{
    const auto nValue = ReadValue();
    SetCurrentValueRaw(nValue);
}

void MemoryWatchViewModel::SetCurrentValueRaw(uint32_t nValue)
{
    if (m_nValue != nValue)
    {
        m_nValue = nValue;
        OnValueChanged();
    }
}

void MemoryWatchViewModel::OnValueChanged()
{
    const std::wstring sValue = BuildCurrentValue();
    SetPreviousValue(GetCurrentValue());
    SetValue(CurrentValueProperty, sValue);
    SetChanges(GetChanges() + 1);
}

std::wstring MemoryWatchViewModel::BuildCurrentValue() const
{
    if (m_nSize == MemSize::Text)
    {
        ra::services::SearchResults pResults;
        pResults.Initialize(m_nAddress, MaxTextBookmarkLength, ra::services::SearchType::AsciiText);
        return pResults.GetFormattedValue(m_nAddress, MemSize::Text);
    }

    return ra::data::MemSizeFormat(m_nValue, m_nSize, GetFormat());
}

bool MemoryWatchViewModel::UpdateCurrentAddressFromIndirectAddress()
{
    if (!m_pValue)
        return true;

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();

    m_bIndirectAddressValid = true;
    for (const auto* pCondition = m_pValue->conditions->conditions; pCondition; pCondition = pCondition->next)
    {
        if (pCondition->type == RC_CONDITION_ADD_ADDRESS)
        {
            if (m_bIndirectAddressValid) // don't need to check validity if we've already found a problem
            {
                rc_typed_value_t address;
                rc_evaluate_operand(&address, &pCondition->operand1, nullptr);
                rc_typed_value_convert(&address, RC_VALUE_TYPE_UNSIGNED);

                if (address.value.u32 == 0) // pointer is null
                    m_bIndirectAddressValid = false;

                const auto nAdjustedAddress = pConsoleContext.ByteAddressFromRealAddress(address.value.u32);
                if (nAdjustedAddress == 0xFFFFFFFF) // pointer is invalid
                    m_bIndirectAddressValid = false;
            }
        }
        else if (pCondition->type == RC_CONDITION_MEASURED)
        {
            if (rc_operand_is_memref(&pCondition->operand1) &&
                pCondition->operand1.value.memref->value.memref_type == RC_MEMREF_TYPE_MODIFIED_MEMREF)
            {
                GSL_SUPPRESS_TYPE1 const auto* pModifiedMemref =
                    reinterpret_cast<rc_modified_memref_t*>(pCondition->operand1.value.memref);
                if (pModifiedMemref->modifier_type == RC_OPERATOR_INDIRECT_READ)
                {
                    // calculate the new address and update the local reference if it changed
                    rc_typed_value_t address, offset;
                    rc_evaluate_operand(&address, &pModifiedMemref->parent, nullptr);
                    rc_evaluate_operand(&offset, &pModifiedMemref->modifier, nullptr);
                    rc_typed_value_add(&address, &offset);
                    rc_typed_value_convert(&address, RC_VALUE_TYPE_UNSIGNED);
                    const auto nNewAddress = gsl::narrow_cast<ra::ByteAddress>(address.value.u32);

                    if (m_nAddress != nNewAddress)
                    {
                        m_nAddress = nNewAddress;
                        SetAddressWithoutUpdatingValue(nNewAddress);
                    }
                }
            }

            break;
        }
    }

    return m_bIndirectAddressValid;
}

void MemoryWatchViewModel::SetIndirectAddress(const std::string& sSerialized)
{
    const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
    auto* pGame = pRuntime.GetClient()->game;

    rc_preparse_state_t preparse;
    rc_init_preparse_state(&preparse);
    preparse.parse.existing_memrefs = pGame ? pGame->runtime.memrefs : nullptr;

    rc_value_with_memrefs_t* value = RC_ALLOC(rc_value_with_memrefs_t, &preparse.parse);
    const char* memaddr = sSerialized.c_str();
    rc_parse_value_internal(&value->value, &memaddr, &preparse.parse);
    rc_preparse_alloc_memrefs(nullptr, &preparse);

    const auto nSize = preparse.parse.offset;
    if (nSize < 0)
        return;

    m_pBuffer.reset(new uint8_t[nSize]);
    if (!m_pBuffer)
        return;

    rc_reset_parse_state(&preparse.parse, m_pBuffer.get());
    value = RC_ALLOC(rc_value_with_memrefs_t, &preparse.parse);
    rc_preparse_alloc_memrefs(&value->memrefs, &preparse);
    Expects(preparse.parse.memrefs == &value->memrefs);

    preparse.parse.existing_memrefs = pGame ? pGame->runtime.memrefs : nullptr;

    memaddr = sSerialized.c_str();
    rc_parse_value_internal(&value->value, &memaddr, &preparse.parse);
    Expects(preparse.parse.offset == nSize);
    value->value.has_memrefs = 1;

    m_pValue = &value->value;
    m_sIndirectAddress = sSerialized;

    const auto* pCondition = FindMeasuredCondition(m_pValue);
    if (pCondition != nullptr)
        SetSize(ra::data::models::TriggerValidation::MapRcheevosMemSize(rc_condition_get_real_operand1(pCondition)->size));

    SetFormat(ra::MemFormat::Unknown);

    DispatchMemoryRead([this]() {
        // value must be updated first to populate memrefs
        const auto nValue = ReadValue();
        SetCurrentValueRaw(nValue);

        UpdateRealNote();
    });
}

void MemoryWatchViewModel::UpdateRealNote()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (pGameContext.IsGameLoading()) // no notes available
        return;

    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    const ra::data::models::CodeNoteModel* pNote = nullptr;

    if (pCodeNotes)
    {
        if (!IsIndirectAddress())
        {
            pNote = pCodeNotes->FindCodeNoteModel(GetAddress());
        }
        else
        {
            // manually traverse the offset chain to find the leaf note to avoid
            // any null pointers in the chain or conflicting addresses
            for (const auto* pCondition = m_pValue->conditions->conditions; pCondition; pCondition = pCondition->next)
            {
                const auto* pOperand = rc_condition_get_real_operand1(pCondition);
                if (rc_operand_is_memref(pOperand) && pOperand->value.memref->value.memref_type == RC_MEMREF_TYPE_MODIFIED_MEMREF)
                {
                    GSL_SUPPRESS_TYPE1 const auto* pModifiedMemref =
                        reinterpret_cast<rc_modified_memref_t*>(pCondition->operand1.value.memref);
                    if (pModifiedMemref->modifier_type == RC_OPERATOR_INDIRECT_READ)
                        pOperand = &pModifiedMemref->modifier;
                }

                const auto nAddress = rc_operand_is_memref(pOperand) ? pOperand->value.memref->address : pOperand->value.num;
                if (pNote)
                    pNote = pNote->GetPointerNoteAtOffset(nAddress);
                else
                    pNote = pCodeNotes->FindCodeNoteModel(nAddress);

                if (!pNote)
                    break;
            }
        }
    }

    if (pNote)
    {
        SetRealNote(pNote->GetNote());

        // if bookmarking an 8-byte double, automatically adjust the bookmark for the significant bytes
        if (GetSize() == MemSize::Double32 && pNote->GetBytes() == 8)
            SetAddress(pNote->GetAddress() + 4);
    }

    if (GetFormat() == MemFormat::Unknown)
    {
        if (pNote)
        {
            SetFormat(pNote->GetDefaultMemFormat());
        }
        else
        {
            const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            const auto bPreferDecimal = pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal);
            SetFormat(bPreferDecimal ? MemFormat::Dec : MemFormat::Hex);
        }
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
