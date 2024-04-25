#include "MemoryInspectorViewModel.hh"

#include "RA_Defs.h"

#include "data\context\EmulatorContext.hh"
#include "data\context\GameContext.hh"
#include "data\context\UserContext.hh"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\AssetUploadViewModel.hh"
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
const BoolModelProperty MemoryInspectorViewModel::CurrentBitsVisibleProperty("MemoryInspectorViewModel", "CurrentBitsVisibleProperty", true);
const BoolModelProperty MemoryInspectorViewModel::CanEditCurrentAddressNoteProperty("MemoryInspectorViewModel", "CanEditCurrentAddressNote", true);
const BoolModelProperty MemoryInspectorViewModel::CanRevertCurrentAddressNoteProperty("MemoryInspectorViewModel", "CanRevertCurrentAddressNote", false);
const BoolModelProperty MemoryInspectorViewModel::CanPublishCurrentAddressNoteProperty("MemoryInspectorViewModel", "CanPublishCurrentAddressNote", false);
const BoolModelProperty MemoryInspectorViewModel::IsCurrentAddressNoteReadOnlyProperty("MemoryInspectorViewModel", "IsCurrentAddressNoteReadOnly", false);

MemoryInspectorViewModel::MemoryInspectorViewModel()
{
    SetWindowTitle(L"Memory Inspector [no game loaded]");

    m_pViewer.AddNotifyTarget(*this);

    SetValue(CanModifyNotesProperty, false);
    SetValue(CanEditCurrentAddressNoteProperty, false);
}

void MemoryInspectorViewModel::InitializeNotifyTargets()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.AddNotifyTarget(*this);

    m_pSearch.InitializeNotifyTargets();
    m_pViewer.InitializeNotifyTargets();
}

void MemoryInspectorViewModel::DoFrame()
{
    m_pSearch.DoFrame();
    m_pViewer.DoFrame();

    const auto nAddress = GetCurrentAddress();
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
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
    else if (args.Property == CurrentAddressProperty && !m_bSyncingAddress)
    {
        m_bSyncingAddress = true;
        const auto nAddress = static_cast<ra::ByteAddress>(args.tNewValue);
        SetValue(CurrentAddressTextProperty, ra::Widen(ra::ByteAddressToString(nAddress)));
        m_bSyncingAddress = false;

        OnCurrentAddressChanged(nAddress);
    }

    WindowViewModelBase::OnValueChanged(args);
}

void MemoryInspectorViewModel::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryViewerViewModel::AddressProperty)
        SetValue(CurrentAddressProperty, args.tNewValue);
    else if (args.Property == MemoryViewerViewModel::SizeProperty)
        SetValue(CurrentBitsVisibleProperty, ra::itoe<MemSize>(args.tNewValue) == MemSize::EightBit);
}

void MemoryInspectorViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == CurrentAddressTextProperty && !m_bSyncingAddress)
    {
        const auto nAddress = ra::ByteAddressFromString(ra::Narrow(args.tNewValue));

        // ignore change event for current address so text field is not modified
        m_bSyncingAddress = true;
        SetCurrentAddress(nAddress);
        m_bSyncingAddress = false;

        OnCurrentAddressChanged(nAddress);
    }
    else if (args.Property == CurrentAddressNoteProperty && !m_bSyncingCodeNote)
    {
        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
        if (pCodeNotes != nullptr)
        {
            const auto nAddress = GetCurrentAddress();
            pCodeNotes->SetCodeNote(nAddress, args.tNewValue);

            // don't immediately save, the user may be about to publish. instead, set a flag to
            // do the save when the address changes.
            UpdateNoteButtons();
        }
    }

    WindowViewModelBase::OnValueChanged(args);
}

void MemoryInspectorViewModel::SaveNotes()
{
    if (m_nSavedNoteAddress == 0xFFFFFFFF)
        return;

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes != nullptr)
    {
        std::wstring sEmpty;
        const auto* pNote = pCodeNotes->FindCodeNote(m_nSavedNoteAddress);
        if (pNote == nullptr)
            pNote = &sEmpty;

        if (*pNote != m_sSavedNote)
        {
            m_sSavedNote = *pNote;

            std::vector<ra::data::models::AssetModelBase*> vAssets;
            vAssets.push_back(pCodeNotes);
            pGameContext.Assets().SaveAssets(vAssets);

            UpdateNoteButtons();
        }
    }
}

void MemoryInspectorViewModel::UpdateNoteButtons()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes == nullptr || !CanModifyNotes())
    {
        SetValue(IsCurrentAddressNoteReadOnlyProperty, true);
        SetValue(CanEditCurrentAddressNoteProperty, false);
        SetValue(CanPublishCurrentAddressNoteProperty, false);
        SetValue(CanRevertCurrentAddressNoteProperty, false);
    }
    else
    {
        SetValue(CanEditCurrentAddressNoteProperty, true);

        if (m_bNoteIsIndirect)
        {
            SetValue(IsCurrentAddressNoteReadOnlyProperty, true);
            SetValue(CanPublishCurrentAddressNoteProperty, false);
            SetValue(CanRevertCurrentAddressNoteProperty, false);
        }
        else
        {
            const bool bOffline = ra::services::ServiceLocator::Get<ra::services::IConfiguration>()
                .IsFeatureEnabled(ra::services::Feature::Offline);
            const auto nAddress = GetCurrentAddress();
            const auto bModified = pCodeNotes->IsNoteModified(nAddress);

            SetValue(IsCurrentAddressNoteReadOnlyProperty, false);
            SetValue(CanPublishCurrentAddressNoteProperty, !bOffline && bModified);
            SetValue(CanRevertCurrentAddressNoteProperty, bModified);
        }
    }
}

void MemoryInspectorViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote)
{
    if (nAddress == GetCurrentAddress())
    {
        // call the override that asks for an author to see if there's a non-indirect note at
        // the address. if so, use it.
        std::string sAuthor;
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
        Expects(pCodeNotes != nullptr);
        const auto* pDirectNote = pCodeNotes->FindCodeNote(nAddress, sAuthor);
        if (pDirectNote)
        {
            // non indirect note found. normally, this will match sNewNote, but sometimes sNewNote
            // will be for an indirect note sharing the addres, so always use the direct note
            m_bNoteIsIndirect = false;
            SetCurrentAddressNote(*pDirectNote);
        }
        else
        {
            // no direct note at address
            if (sNewNote.length() == 0)
            {
                // empty note notification is probably a deleted note, but check to be sure
                const auto* pIndirectNote = pCodeNotes->FindCodeNote(nAddress);
                m_bNoteIsIndirect = (pIndirectNote != nullptr);
            }
            else
            {
                // non-empty note. must be indirect
                m_bNoteIsIndirect = true;
            }

            SetCurrentAddressNote(sNewNote);
        }

        UpdateNoteButtons();
    }
}

void MemoryInspectorViewModel::SetCurrentAddressNote(const std::wstring& sValue)
{
    m_bSyncingCodeNote = true;
    SetValue(CurrentAddressNoteProperty, sValue);
    m_bSyncingCodeNote = false;
}

void MemoryInspectorViewModel::OnCurrentAddressChanged(ra::ByteAddress nNewAddress)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();

    if (m_nSavedNoteAddress != 0xFFFFFFFF)
        SaveNotes();

    m_nSavedNoteAddress = nNewAddress;
    m_bNoteIsIndirect = false;
    const std::wstring* pNote = nullptr;
    if (pCodeNotes != nullptr)
    {
        pNote = pCodeNotes->FindCodeNote(nNewAddress);
        if (pNote)
        {
            // found a note. call the override that asks for an author to
            // see if there's a non-indirect note at the address. if so, use it.
            std::string sAuthor;
            const auto* pDirectNote = pCodeNotes->FindCodeNote(nNewAddress, sAuthor);
            if (pDirectNote != nullptr)
                pNote = pDirectNote;
            else
                m_bNoteIsIndirect = true;
        }
    }

    if (pNote)
    {
        m_sSavedNote = *pNote;

        const auto nIndirectSource = m_bNoteIsIndirect ? pCodeNotes->GetIndirectSource(nNewAddress) : 0xFFFFFFFF;
        if (nIndirectSource != 0xFFFFFFFF)
            SetCurrentAddressNote(ra::StringPrintf(L"[Indirect from %s]\r\n%s", ra::ByteAddressToString(nIndirectSource), *pNote));
        else
            SetCurrentAddressNote(*pNote);
    }
    else
    {
        m_sSavedNote.clear();
        SetCurrentAddressNote(L"");
    }

    UpdateNoteButtons();

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    const auto nValue = pEmulatorContext.ReadMemoryByte(nNewAddress);
    SetValue(CurrentAddressValueProperty, nValue);

    m_pViewer.SetAddress(nNewAddress);
    m_pViewer.SaveToMemViewHistory();
}

void MemoryInspectorViewModel::BookmarkCurrentAddress() const
{
    auto& pBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    if (!pBookmarks.IsVisible())
        pBookmarks.Show();

    const auto nAddress = GetCurrentAddress();

    // if the code note specifies an explicit size, use it. otherwise, use the selected viewer mode size.
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    auto nSize = (pCodeNotes != nullptr) ? pCodeNotes->GetCodeNoteMemSize(nAddress) : MemSize::Unknown;
    if (nSize >= MemSize::Unknown)
        nSize = Viewer().GetSize();

    pBookmarks.AddBookmark(nAddress, nSize);
}

void MemoryInspectorViewModel::PublishCurrentAddressNote()
{
    if (!CanPublishCurrentAddressNote())
        return;

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes == nullptr)
        return;

    // disable buttons while publishing
    SetValue(CanPublishCurrentAddressNoteProperty, false);

    const auto nAddress = GetCurrentAddress();

    ra::ui::viewmodels::AssetUploadViewModel vmAssetUpload;
    vmAssetUpload.QueueCodeNote(*pCodeNotes, nAddress);
    vmAssetUpload.ShowModal(*this);

    if (vmAssetUpload.HasFailures())
        vmAssetUpload.ShowResults();

    if (pCodeNotes->IsNoteModified(nAddress))
    {
        // if canceled, re-enable buttons
        SetValue(CanPublishCurrentAddressNoteProperty, true);
    }
    else
    {
        // if we flagged the text as modified to be written on address change, do so now
        SaveNotes();

        UpdateNoteButtons();
    }
}

void MemoryInspectorViewModel::RevertCurrentAddressNote()
{
    if (!CanRevertCurrentAddressNote())
        return;

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes == nullptr)
        return;

    const auto nAddress = GetCurrentAddress();
    const auto* pOriginalNote = pCodeNotes->GetServerCodeNote(nAddress);
    if (pOriginalNote != nullptr)
    {
        ra::ui::viewmodels::MessageBoxViewModel vmPrompt;
        vmPrompt.SetHeader(ra::StringPrintf(L"Revert note for address %s?", ra::ByteAddressToString(nAddress)));
        vmPrompt.SetMessage(L"This will discard all local work and revert the note to the last state retrieved from the server.");
        vmPrompt.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
        if (vmPrompt.ShowModal() == DialogResult::No)
            return;

        // make a copy as the original note will be destroyed when we eliminate the modification
        const std::wstring pOriginalNoteCopy = *pOriginalNote;
        pCodeNotes->SetCodeNote(nAddress, pOriginalNoteCopy);

        SaveNotes();
        UpdateNoteButtons();
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
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes != nullptr)
    {
        nNewAddress = pCodeNotes->GetNextNoteAddress(nCurrentAddress, true);
        if (nNewAddress == 0xFFFFFFFF)
            nNewAddress = nCurrentAddress;
    }

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
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes != nullptr)
    {
        nNewAddress = pCodeNotes->GetPreviousNoteAddress(nCurrentAddress, true);
        if (nNewAddress == 0xFFFFFFFF)
            nNewAddress = nCurrentAddress;
    }

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
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    auto nValue = pEmulatorContext.ReadMemoryByte(nAddress);
    nValue ^= (1 << nBit);
    pEmulatorContext.WriteMemoryByte(nAddress, nValue);

    // update the local value, which will cause the bits string to get updated
    SetValue(CurrentAddressValueProperty, nValue);
}

void MemoryInspectorViewModel::OnBeforeActiveGameChanged()
{
    if (m_nSavedNoteAddress != 0xFFFFFFFF)
    {
        SaveNotes();

        m_nSavedNoteAddress = 0xFFFFFFFF;
        m_sSavedNote.clear();
    }
}

void MemoryInspectorViewModel::OnActiveGameChanged()
{
    m_nSavedNoteAddress = 0xFFFFFFFF;
    m_sSavedNote.clear();

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (pGameContext.GameId() == 0)
    {
        SetWindowTitle(L"Memory Inspector [no game loaded]");
        SetValue(CanModifyNotesProperty, false);
    }
    else
    {
        if (pGameContext.GetMode() == ra::data::context::GameContext::Mode::CompatibilityTest)
            SetWindowTitle(L"Memory Inspector [compatibility mode]");
        else
            SetWindowTitle(L"Memory Inspector");

        SetValue(CanModifyNotesProperty, true);
    }

    UpdateNoteButtons();

    if (pGameContext.GameId() != m_nGameId)
    {
        m_nGameId = pGameContext.GameId();
        Search().ClearResults();
    }
}

void MemoryInspectorViewModel::OnEndGameLoad()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    const auto nFirstAddress = (pCodeNotes != nullptr) ? pCodeNotes->FirstCodeNoteAddress() : 0U;
    SetCurrentAddress(nFirstAddress);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
