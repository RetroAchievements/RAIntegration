#include "MemoryInspectorViewModel.hh"

#include "RA_Defs.h"

#include "data\EmulatorContext.hh"
#include "data\GameContext.hh"

#include "services\IAudioSystem.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty MemoryInspectorViewModel::CurrentAddressProperty("MemoryInspectorViewModel", "CurrentAddress", 0);
const StringModelProperty MemoryInspectorViewModel::CurrentAddressTextProperty("MemoryInspectorViewModel", "CurrentAddressText", L"0x0000");
const StringModelProperty MemoryInspectorViewModel::CurrentAddressNoteProperty("MemoryInspectorViewModel", "CurrentAddressNote", L"");
const StringModelProperty MemoryInspectorViewModel::CurrentAddressBitsProperty("MemoryInspectorViewModel", "CurrentAddressBits", L"0 0 0 0 0 0 0 0");
const IntModelProperty MemoryInspectorViewModel::CurrentAddressValueProperty("MemoryInspectorViewModel", "CurrentAddressValue", 0);
const BoolModelProperty MemoryInspectorViewModel::CanModifyNotesProperty("MemoryInspectorViewModel", "CanModifyNotes", true);

MemoryInspectorViewModel::MemoryInspectorViewModel()
{
    SetWindowTitle(L"Memory Inspector [no game loaded]");

    m_pViewer.AddNotifyTarget(*this);

    SetValue(CanModifyNotesProperty, false);
}

void MemoryInspectorViewModel::InitializeNotifyTargets()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
    pGameContext.AddNotifyTarget(*this);

    m_pSearch.InitializeNotifyTargets();
    m_pViewer.InitializeNotifyTargets();
}

void MemoryInspectorViewModel::DoFrame()
{
    m_pSearch.DoFrame();
    m_pViewer.DoFrame();

    const auto nAddress = GetCurrentAddress();
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    const auto nValue = pEmulatorContext.ReadMemoryByte(nAddress);
    SetValue(CurrentAddressValueProperty, nValue);
}

void MemoryInspectorViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == CurrentAddressValueProperty)
    {
        if ((args.tNewValue ^ args.tOldValue) & 0xFF)
        {
            SetValue(CurrentAddressBitsProperty, ra::StringPrintf(L"%d %d %d %d %d %d %d %d",
                (args.tNewValue & 0x80) >> 7,
                (args.tNewValue & 0x40) >> 6,
                (args.tNewValue & 0x20) >> 5,
                (args.tNewValue & 0x10) >> 4,
                (args.tNewValue & 0x08) >> 3,
                (args.tNewValue & 0x04) >> 2,
                (args.tNewValue & 0x02) >> 1,
                (args.tNewValue & 0x01)));
        }
    }
    else if (args.Property == CurrentAddressProperty && !m_bTyping)
    {
        const auto nAddress = static_cast<ra::ByteAddress>(args.tNewValue);
        SetValue(CurrentAddressTextProperty, ra::Widen(ra::ByteAddressToString(nAddress)));

        OnCurrentAddressChanged(nAddress);
    }

    WindowViewModelBase::OnValueChanged(args);
}

void MemoryInspectorViewModel::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryViewerViewModel::AddressProperty)
        SetValue(CurrentAddressProperty, args.tNewValue);
}

void MemoryInspectorViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == CurrentAddressTextProperty)
    {
        const auto nAddress = ra::ByteAddressFromString(ra::Narrow(args.tNewValue));

        // ignore change event for current address so text field is not modified
        m_bTyping = true;
        SetCurrentAddress(nAddress);
        m_bTyping = false;

        OnCurrentAddressChanged(nAddress);
    }

    WindowViewModelBase::OnValueChanged(args);
}

void MemoryInspectorViewModel::OnCurrentAddressChanged(ra::ByteAddress nNewAddress)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto* pNote = pGameContext.FindCodeNote(nNewAddress);
    SetCurrentAddressNote(pNote ? *pNote : std::wstring());

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    const auto nValue = pEmulatorContext.ReadMemoryByte(nNewAddress);
    SetValue(CurrentAddressValueProperty, nValue);

    m_pViewer.SetAddress(nNewAddress);
}

void MemoryInspectorViewModel::BookmarkCurrentAddress() const
{
    auto& pBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    if (!pBookmarks.IsVisible())
        pBookmarks.Show();

    pBookmarks.AddBookmark(GetCurrentAddress(), Viewer().GetSize());
}

static std::wstring ShortenNote(const std::wstring& sNote)
{
    return sNote.length() > 256 ? (sNote.substr(0, 253) + L"...") : sNote;
}

void MemoryInspectorViewModel::SaveCurrentAddressNote()
{
    const auto& sNewNote = GetCurrentAddressNote();
    const auto nAddress = GetCurrentAddress();

    std::string sAuthor;
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
    const auto* pNote = pGameContext.FindCodeNote(nAddress, sAuthor);
    bool bUpdated = false;

    if (pNote == nullptr)
    {
        if (sNewNote.empty())
        {
            // unmodified - do nothing
            return;
        }
        else
        {
            // new note - just add it
            bUpdated = pGameContext.SetCodeNote(nAddress, sNewNote);
        }
    }
    else if (*pNote == sNewNote)
    {
        // unmodified - do nothing
        return;
    }
    else
    {
        // value changed - confirm overwrite
        ra::ui::viewmodels::MessageBoxViewModel vmPrompt;
        vmPrompt.SetHeader(ra::StringPrintf(L"Overwrite note for address %s?", ra::ByteAddressToString(nAddress)));

        if (sNewNote.length() > 256 || pNote->length() > 256)
        {
            const auto sNewNoteShort = ShortenNote(sNewNote);
            const auto sOldNoteShort = ShortenNote(*pNote);
            vmPrompt.SetMessage(ra::StringPrintf(L"Are you sure you want to replace %s's note:\n\n%s\n\nWith your note:\n\n%s",
                sAuthor, sOldNoteShort, sNewNoteShort));
        }
        else
        {
            vmPrompt.SetMessage(ra::StringPrintf(L"Are you sure you want to replace %s's note:\n\n%s\n\nWith your note:\n\n%s",
                sAuthor, *pNote, sNewNote));
        }

        vmPrompt.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
        vmPrompt.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
        if (vmPrompt.ShowModal() == ra::ui::DialogResult::Yes)
        {
            bUpdated = pGameContext.SetCodeNote(nAddress, sNewNote);
        }
    }

    if (bUpdated)
    {
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().Beep();
    }
    else
    {
        // update failed, revert to previous text
        SetCurrentAddressNote(pNote ? *pNote : std::wstring());
    }
}

void MemoryInspectorViewModel::DeleteCurrentAddressNote()
{
    const auto nAddress = GetCurrentAddress();

    std::string sAuthor;
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
    const auto* pNote = pGameContext.FindCodeNote(nAddress, sAuthor);

    if (pNote == nullptr)
    {
        // no note - do nothing
        return;
    }

    const auto pNoteShort = ShortenNote(*pNote);

    ra::ui::viewmodels::MessageBoxViewModel vmPrompt;
    vmPrompt.SetHeader(ra::StringPrintf(L"Delete note for address %s?", ra::ByteAddressToString(nAddress)));
    vmPrompt.SetMessage(ra::StringPrintf(L"Are you sure you want to delete %s's note:\n\n%s", sAuthor, pNoteShort));
    vmPrompt.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
    vmPrompt.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
    if (vmPrompt.ShowModal() == ra::ui::DialogResult::Yes)
    {
        if (pGameContext.DeleteCodeNote(nAddress))
        {
            ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().Beep();
            SetCurrentAddressNote(L"");
        }
    }
}

void MemoryInspectorViewModel::OpenNotesList()
{
    auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    pWindowManager.CodeNotes.Show();
}

bool MemoryInspectorViewModel::NextNote()
{
    const auto nCurrentAddress = GetCurrentAddress();
    ra::ByteAddress nNewAddress = nCurrentAddress;
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
    pGameContext.EnumerateCodeNotes([this, nCurrentAddress, &nNewAddress](ra::ByteAddress nAddress)
    {
        if (nAddress > nCurrentAddress)
        {
            nNewAddress = nAddress;
            return false;
        }

        return true;
    });

    if (nNewAddress != nCurrentAddress)
    {
        SetCurrentAddress(nNewAddress);
        return true;
    }

    return false;
}

bool MemoryInspectorViewModel::PreviousNote()
{
    const auto nCurrentAddress = GetCurrentAddress();
    ra::ByteAddress nNewAddress = nCurrentAddress;
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
    pGameContext.EnumerateCodeNotes([this, nCurrentAddress, &nNewAddress](ra::ByteAddress nAddress)
    {
        if (nAddress >= nCurrentAddress)
            return false;

        nNewAddress = nAddress;
        return true;
    });

    if (nNewAddress != nCurrentAddress)
    {
        SetCurrentAddress(nNewAddress);
        return true;
    }

    return false;
}

void MemoryInspectorViewModel::ToggleBit(int nBit)
{
    const auto nAddress = GetCurrentAddress();

    // push the updated value to the emulator
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    auto nValue = pEmulatorContext.ReadMemoryByte(nAddress);
    nValue ^= (1 << nBit);
    pEmulatorContext.WriteMemoryByte(nAddress, nValue);

    // if a bookmark exists for the modified address, update the current value
    auto& pBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    pBookmarks.OnEditMemory(nAddress);

    // update the local value, which will cause the bits string to get updated
    SetValue(CurrentAddressValueProperty, nValue);
}

void MemoryInspectorViewModel::OnActiveGameChanged()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    if (pGameContext.GameId() == 0)
    {
        SetWindowTitle(L"Memory Inspector [no game loaded]");
        SetValue(CanModifyNotesProperty, false);
    }
    else if (pGameContext.GetMode() == ra::data::GameContext::Mode::CompatibilityTest)
    {
        SetWindowTitle(L"Memory Inspector [compatibility mode]");
        SetValue(CanModifyNotesProperty, true);
    }
    else
    {
        SetWindowTitle(L"Memory Inspector");
        SetValue(CanModifyNotesProperty, true);
    }
}

void MemoryInspectorViewModel::OnEndGameLoad()
{
    ra::ByteAddress nFirstAddress = 0U;
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    pGameContext.EnumerateCodeNotes([&nFirstAddress](ra::ByteAddress nAddress)
    {
        nFirstAddress = nAddress;
        return false;
    });

    SetCurrentAddress(nFirstAddress);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
