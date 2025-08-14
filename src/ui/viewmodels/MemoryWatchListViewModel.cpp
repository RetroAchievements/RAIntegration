#include "MemoryWatchListViewModel.hh"

#include "RA_Defs.h"
#include "RA_Json.h"
#include "RA_StringUtils.h"

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
    m_vSizes.Add(ra::etoi(MemSize::EightBit), L" 8-bit"); // leading space for sort order
    m_vSizes.Add(ra::etoi(MemSize::SixteenBit), ra::data::MemSizeString(MemSize::SixteenBit));
    m_vSizes.Add(ra::etoi(MemSize::TwentyFourBit), ra::data::MemSizeString(MemSize::TwentyFourBit));
    m_vSizes.Add(ra::etoi(MemSize::ThirtyTwoBit), ra::data::MemSizeString(MemSize::ThirtyTwoBit));
    m_vSizes.Add(ra::etoi(MemSize::SixteenBitBigEndian), ra::data::MemSizeString(MemSize::SixteenBitBigEndian));
    m_vSizes.Add(ra::etoi(MemSize::TwentyFourBitBigEndian), ra::data::MemSizeString(MemSize::TwentyFourBitBigEndian));
    m_vSizes.Add(ra::etoi(MemSize::ThirtyTwoBitBigEndian), ra::data::MemSizeString(MemSize::ThirtyTwoBitBigEndian));
    m_vSizes.Add(ra::etoi(MemSize::BitCount), ra::data::MemSizeString(MemSize::BitCount));
    m_vSizes.Add(ra::etoi(MemSize::Nibble_Lower), ra::data::MemSizeString(MemSize::Nibble_Lower));
    m_vSizes.Add(ra::etoi(MemSize::Nibble_Upper), ra::data::MemSizeString(MemSize::Nibble_Upper));
    m_vSizes.Add(ra::etoi(MemSize::Float), ra::data::MemSizeString(MemSize::Float));
    m_vSizes.Add(ra::etoi(MemSize::FloatBigEndian), ra::data::MemSizeString(MemSize::FloatBigEndian));
    m_vSizes.Add(ra::etoi(MemSize::Double32), ra::data::MemSizeString(MemSize::Double32));
    m_vSizes.Add(ra::etoi(MemSize::Double32BigEndian), ra::data::MemSizeString(MemSize::Double32BigEndian));
    m_vSizes.Add(ra::etoi(MemSize::MBF32), ra::data::MemSizeString(MemSize::MBF32));
    m_vSizes.Add(ra::etoi(MemSize::MBF32LE), ra::data::MemSizeString(MemSize::MBF32LE));
    m_vSizes.Add(ra::etoi(MemSize::Text), ra::data::MemSizeString(MemSize::Text));

    m_vFormats.Add(ra::etoi(MemFormat::Hex), L"Hex");
    m_vFormats.Add(ra::etoi(MemFormat::Dec), L"Dec");

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

void MemoryWatchListViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote)
{
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vItems.Count(); ++nIndex)
    {
        auto* pItem = m_vItems.GetItemAt(nIndex);
        if (pItem && pItem->GetAddress() == nAddress)
            pItem->SetRealNote(sNewNote);
    }
}

void MemoryWatchListViewModel::OnByteWritten(ra::ByteAddress nAddress, uint8_t)
{
    ra::data::context::EmulatorContext::DispatchesReadMemory::DispatchMemoryRead([this, nAddress]() {
        for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vItems.Count(); ++nIndex)
        {
            auto& pItem = *m_vItems.GetItemAt(nIndex);
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
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vItems.Count(); ++nIndex)
        m_vItems.GetItemAt(nIndex)->DoFrame();
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
                for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vItems.Count(); ++nIndex)
                {
                    auto* pItem = m_vItems.GetItemAt(nIndex);
                    Expects(pItem != nullptr);
                    const auto nItemAddress = pItem->GetAddress();
                    if (nItemAddress >= nAddress && nItemAddress < nAddress + pItem->GetSizeBytes())
                        pItem->UpdateCurrentValue();
                }
            });
        }
    }
}

void MemoryWatchListViewModel::OnEndViewModelCollectionUpdate()
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
