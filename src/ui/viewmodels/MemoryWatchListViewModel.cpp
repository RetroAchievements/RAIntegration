#include "MemoryWatchListViewModel.hh"

#include "RA_Defs.h"
#include "RA_Json.h"
#include "util\Strings.hh"

#include "data\Types.hh"
#include "data\context\EmulatorContext.hh"
#include "data\models\TriggerValidation.hh"

#include "services\AchievementLogicSerializer.hh"
#include "services\FrameEventQueue.hh"
#include "services\IConfiguration.hh"
#include "services\IFileSystem.hh"
#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\FileDialogViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

#include <rcheevos/src/rcheevos/rc_internal.h>
#include <rcheevos/src/rc_client_internal.h>

namespace ra {
namespace ui {
namespace viewmodels {

const BoolModelProperty MemoryWatchListViewModel::HasSelectionProperty("MemoryWatchListViewModel", "HasSelection", false);
const BoolModelProperty MemoryWatchListViewModel::HasSingleSelectionProperty("MemoryWatchListViewModel", "HasSingleSelection", false);
const IntModelProperty MemoryWatchListViewModel::SingleSelectionIndexProperty("MemoryWatchListViewModel", "SingleSelectionIndex", -1);

MemoryWatchListViewModel::MemoryWatchListViewModel() noexcept
{
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::EightBit), L" 8-bit"); // leading space for sort order
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::SixteenBit), ra::data::Memory::SizeString(ra::data::Memory::Size::SixteenBit));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::TwentyFourBit), ra::data::Memory::SizeString(ra::data::Memory::Size::TwentyFourBit));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::ThirtyTwoBit), ra::data::Memory::SizeString(ra::data::Memory::Size::ThirtyTwoBit));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::SixteenBitBigEndian), ra::data::Memory::SizeString(ra::data::Memory::Size::SixteenBitBigEndian));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::TwentyFourBitBigEndian), ra::data::Memory::SizeString(ra::data::Memory::Size::TwentyFourBitBigEndian));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::ThirtyTwoBitBigEndian), ra::data::Memory::SizeString(ra::data::Memory::Size::ThirtyTwoBitBigEndian));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::BitCount), ra::data::Memory::SizeString(ra::data::Memory::Size::BitCount));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::NibbleLower), ra::data::Memory::SizeString(ra::data::Memory::Size::NibbleLower));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::NibbleUpper), ra::data::Memory::SizeString(ra::data::Memory::Size::NibbleUpper));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::Float), ra::data::Memory::SizeString(ra::data::Memory::Size::Float));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::FloatBigEndian), ra::data::Memory::SizeString(ra::data::Memory::Size::FloatBigEndian));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::Double32), ra::data::Memory::SizeString(ra::data::Memory::Size::Double32));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::Double32BigEndian), ra::data::Memory::SizeString(ra::data::Memory::Size::Double32BigEndian));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::MBF32), ra::data::Memory::SizeString(ra::data::Memory::Size::MBF32));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::MBF32LE), ra::data::Memory::SizeString(ra::data::Memory::Size::MBF32LE));
    m_vSizes.Add(ra::etoi(ra::data::Memory::Size::Text), ra::data::Memory::SizeString(ra::data::Memory::Size::Text));

    m_vFormats.Add(ra::etoi(ra::data::Memory::Format::Hex), L"Hex");
    m_vFormats.Add(ra::etoi(ra::data::Memory::Format::Dec), L"Dec");

    m_vItems.AddNotifyTarget(*this);
}

void MemoryWatchListViewModel::InitializeNotifyTargets(bool syncNotes)
{
    if (syncNotes)
    {
        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        pGameContext.AddNotifyTarget(*this);
    }

    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.AddNotifyTarget(*this);
}

void MemoryWatchListViewModel::OnCodeNoteMoved(ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const std::wstring& sNote)
{
    for (auto& pItem : m_vItems)
    {
        if (!pItem.IsIndirectAddress()) // ignore move events for indirect notes
        {
            const auto nCurrentAddress = pItem.GetAddress();
            if (nCurrentAddress == nNewAddress)
                pItem.SetRealNote(sNote);
            else if (nCurrentAddress == nOldAddress)
                pItem.SetRealNote(L"");
        }
    }
}

void MemoryWatchListViewModel::OnCodeNoteChanged(ra::data::ByteAddress nAddress, const std::wstring& sNewNote)
{
    for (auto& pItem : m_vItems)
    {
        if (!pItem.IsIndirectAddress() && pItem.GetAddress() == nAddress)
            pItem.SetRealNote(sNewNote);
    }
}

void MemoryWatchListViewModel::OnByteWritten(ra::data::ByteAddress nAddress, uint8_t)
{
    ra::data::context::EmulatorContext::DispatchesReadMemory::DispatchMemoryRead([this, nAddress]() {
        for (auto& pItem : m_vItems)
        {
            const auto nItemAddress = pItem.GetAddress();
            if (nAddress < nItemAddress)
                continue;

            const auto nBytes = pItem.GetSizeBytes();
            if (nAddress >= nItemAddress + nBytes)
                continue;

            pItem.UpdateCurrentValue();
        }
    });
}

void MemoryWatchListViewModel::DoFrame()
{
    // don't watch for memory changes while we're processing the list. frozen bookmarks may write memory
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.RemoveNotifyTarget(*this);

    m_vItems.BeginUpdate();
    for (auto& pItem : m_vItems)
        pItem.DoFrame();
    m_vItems.EndUpdate();

    pEmulatorContext.AddNotifyTarget(*this);
}

void MemoryWatchListViewModel::OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == LookupItemViewModel::IsSelectedProperty)
    {
        if (!m_vItems.IsUpdating())
            UpdateHasSelection();
    }
    else if (args.Property == MemoryWatchViewModel::IsWritingMemoryProperty)
    {
        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        if (args.tNewValue)
        {
            pEmulatorContext.RemoveNotifyTarget(*this);
        }
        else
        {
            pEmulatorContext.AddNotifyTarget(*this);

            const auto* pItem = m_vItems.GetItemAt(nIndex);
            Expects(pItem != nullptr);
            const auto nAddress = pItem->GetAddress();
            const auto nBytes = pItem->GetSizeBytes();
            ra::data::context::EmulatorContext::DispatchesReadMemory::DispatchMemoryRead([this, nAddress, nBytes]() {
                for (auto& pItem : m_vItems)
                {
                    const auto nItemAddress = pItem.GetAddress();
                    if (nItemAddress >= nAddress && nItemAddress < nAddress + pItem.GetSizeBytes())
                        pItem.UpdateCurrentValue();
                }
            });
        }
    }
}

void MemoryWatchListViewModel::OnEndViewModelCollectionUpdate()
{
    UpdateHasSelection();
}

void MemoryWatchListViewModel::OnViewModelRemoved(gsl::index)
{
    UpdateHasSelection();
}

void MemoryWatchListViewModel::UpdateHasSelection()
{
    gsl::index nSelectedItemIndex = -1;
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vItems.Count(); ++nIndex)
    {
        const auto& pBookmark = *m_vItems.GetItemAt(nIndex);
        if (pBookmark.IsSelected())
        {
            if (nSelectedItemIndex != -1)
            {
                SetValue(HasSelectionProperty, true);
                SetValue(HasSingleSelectionProperty, false);
                SetValue(SingleSelectionIndexProperty, -1);
                return;
            }

            nSelectedItemIndex = nIndex;
        }
    }

    if (nSelectedItemIndex == -1)
    {
        SetValue(HasSelectionProperty, false);
        SetValue(HasSingleSelectionProperty, false);
        SetValue(SingleSelectionIndexProperty, -1);
    }
    else
    {
        SetValue(HasSelectionProperty, true);
        SetValue(HasSingleSelectionProperty, true);
        SetValue(SingleSelectionIndexProperty, gsl::narrow_cast<int>(nSelectedItemIndex));
    }
}

void MemoryWatchListViewModel::ClearAllChanges()
{
    for (gsl::index nIndex = m_vItems.Count() - 1; nIndex >= 0; --nIndex)
        m_vItems.SetItemValue(nIndex, MemoryWatchViewModel::ChangesProperty, 0);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
