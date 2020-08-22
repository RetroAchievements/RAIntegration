#include "MemoryBookmarksViewModel.hh"

#include "RA_Defs.h"
#include "RA_Json.h"
#include "RA_StringUtils.h"

#include "data\EmulatorContext.hh"

#include "services\IConfiguration.hh"
#include "services\IFileSystem.hh"
#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\FileDialogViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::DescriptionProperty("MemoryBookmarkViewModel", "Description", L"");
const BoolModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::IsCustomDescriptionProperty("MemoryBookmarkViewModel", "IsCustomDescription", false);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::AddressProperty("MemoryBookmarkViewModel", "Address", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty("MemoryBookmarkViewModel", "Size", ra::etoi(MemSize::EightBit));
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::FormatProperty("MemoryBookmarkViewModel", "Format", ra::etoi(MemFormat::Hex));
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::CurrentValueProperty("MemoryBookmarkViewModel", "CurrentValue", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::PreviousValueProperty("MemoryBookmarkViewModel", "PreviousValue", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::ChangesProperty("MemoryBookmarkViewModel", "Changes", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::BehaviorProperty("MemoryBookmarkViewModel", "Behavior", ra::etoi(BookmarkBehavior::None));
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::RowColorProperty("MemoryBookmarkViewModel", "RowColor", 0);

MemoryBookmarksViewModel::MemoryBookmarksViewModel() noexcept
{
    SetWindowTitle(L"Memory Bookmarks");

    m_vSizes.Add(ra::etoi(MemSize::EightBit), L" 8-bit"); // leading space for sort order
    m_vSizes.Add(ra::etoi(MemSize::SixteenBit), L"16-bit");
    m_vSizes.Add(ra::etoi(MemSize::TwentyFourBit), L"24-bit");
    m_vSizes.Add(ra::etoi(MemSize::ThirtyTwoBit), L"32-bit");

    m_vFormats.Add(ra::etoi(MemFormat::Hex), L"Hex");
    m_vFormats.Add(ra::etoi(MemFormat::Dec), L"Dec");

    m_vBehaviors.Add(ra::etoi(BookmarkBehavior::None), L"");
    m_vBehaviors.Add(ra::etoi(BookmarkBehavior::Frozen), L"Frozen");
    m_vBehaviors.Add(ra::etoi(BookmarkBehavior::PauseOnChange), L"Pause");

    m_vBookmarks.AddNotifyTarget(*this);
}

void MemoryBookmarksViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == IsVisibleProperty)
    {
        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
        if (args.tNewValue)
        {
            pGameContext.AddNotifyTarget(*this);
            OnActiveGameChanged();
        }
        else
        {
            pGameContext.RemoveNotifyTarget(*this);
        }
    }

    WindowViewModelBase::OnValueChanged(args);
}

GSL_SUPPRESS_F6
void MemoryBookmarksViewModel::OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryBookmarkViewModel::FormatProperty ||
        args.Property == MemoryBookmarkViewModel::SizeProperty)
    {
        m_bModified = true;
    }
    else if (args.Property == MemoryBookmarkViewModel::BehaviorProperty)
    {
        switch ((BookmarkBehavior)args.tNewValue)
        {
            case BookmarkBehavior::Frozen:
                m_vBookmarks.SetItemValue(nIndex, MemoryBookmarkViewModel::RowColorProperty, 0xFFFFFFC0);
                break;

            default:
                m_vBookmarks.SetItemValue(nIndex, MemoryBookmarkViewModel::RowColorProperty,
                    MemoryBookmarkViewModel::RowColorProperty.GetDefaultValue());
                break;
        }
    }
    else if (args.Property == MemoryBookmarkViewModel::CurrentValueProperty)
    {
        if (!m_bIgnoreValueChanged)
        {
            const auto* pBookmark = m_vBookmarks.GetItemAt(nIndex);
            if (pBookmark)
            {
                const auto nAddress = pBookmark->GetAddress();
                switch (pBookmark->GetSize())
                {
                    case MemSize::ThirtyTwoBit:
                        OnEditMemory(nAddress + 3);
                        _FALLTHROUGH;
                    case MemSize::TwentyFourBit:
                        OnEditMemory(nAddress + 2);
                        _FALLTHROUGH;
                    case MemSize::SixteenBit:
                        OnEditMemory(nAddress + 1);
                        _FALLTHROUGH;
                    default:
                        OnEditMemory(nAddress);
                        break;
                }
            }
        }
    }
}

void MemoryBookmarksViewModel::OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryBookmarkViewModel::DescriptionProperty)
    {
        auto* vmBookmark = m_vBookmarks.GetItemAt(nIndex);
        Expects(vmBookmark != nullptr);

        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
        const auto* pNote = pGameContext.FindCodeNote(vmBookmark->GetAddress());

        bool bIsCustomDescription = false;
        if (pNote)
        {
            if (*pNote != args.tNewValue)
                bIsCustomDescription = true;
        }
        else if (!args.tNewValue.empty())
        {
            bIsCustomDescription = true;
        }
        vmBookmark->SetIsCustomDescription(bIsCustomDescription);
    }
}

void MemoryBookmarksViewModel::OnActiveGameChanged()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    if (m_nLoadedGameId != pGameContext.GameId())
    {
        if (m_bModified && m_nLoadedGameId != 0)
        {
            auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();

            auto pWriter = pLocalStorage.WriteText(ra::services::StorageItemType::Bookmarks, std::to_wstring(m_nLoadedGameId));
            if (pWriter != nullptr)
                SaveBookmarks(*pWriter);
        }

        if (IsVisible())
        {
            auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();

            auto pReader = pLocalStorage.ReadText(ra::services::StorageItemType::Bookmarks, std::to_wstring(pGameContext.GameId()));
            if (pReader != nullptr)
            {
                LoadBookmarks(*pReader);
            }
            else
            {
                m_vBookmarks.BeginUpdate();

                while (m_vBookmarks.Count() > 0)
                    m_vBookmarks.RemoveAt(m_vBookmarks.Count() - 1);

                m_vBookmarks.EndUpdate();
            }

            m_nLoadedGameId = pGameContext.GameId();
        }

        m_bModified = false;
    }
}

void MemoryBookmarksViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote)
{
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
    {
        auto* pBookmark = m_vBookmarks.GetItemAt(nIndex);
        if (pBookmark && pBookmark->GetAddress() == nAddress && !pBookmark->IsCustomDescription())
            pBookmark->SetDescription(sNewNote);
    }
}

void MemoryBookmarksViewModel::LoadBookmarks(ra::services::TextReader& sBookmarksFile)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    gsl::index nIndex = 0;

    m_vBookmarks.BeginUpdate();
    m_vBookmarks.RemoveNotifyTarget(*this);

    rapidjson::Document document;
    if (LoadDocument(document, sBookmarksFile))
    {
        if (document.HasMember("Bookmarks"))
        {
            const auto& bookmarks = document["Bookmarks"];

            for (const auto& bookmark : bookmarks.GetArray())
            {
                auto* vmBookmark = m_vBookmarks.GetItemAt(nIndex);
                if (vmBookmark == nullptr)
                    vmBookmark = &m_vBookmarks.Add();
                ++nIndex;

                vmBookmark->SetAddress(bookmark["Address"].GetUint());

                vmBookmark->SetIsCustomDescription(false);
                const auto* pNote = pGameContext.FindCodeNote(vmBookmark->GetAddress());
                std::wstring sDescription;
                if (bookmark.HasMember("Description"))
                {
                    sDescription = ra::Widen(bookmark["Description"].GetString());
                    if (!pNote || *pNote != sDescription)
                        vmBookmark->SetIsCustomDescription(true);
                }
                else
                {
                    if (pNote)
                        sDescription = *pNote;
                }
                vmBookmark->SetDescription(sDescription);

                if (bookmark.HasMember("Type"))
                {
                    switch (bookmark["Type"].GetInt())
                    {
                        case 1: vmBookmark->SetSize(MemSize::EightBit); break;
                        case 2: vmBookmark->SetSize(MemSize::SixteenBit); break;
                        case 3: vmBookmark->SetSize(MemSize::ThirtyTwoBit); break;
                    }
                }
                else
                {
                    vmBookmark->SetSize(ra::itoe<MemSize>(bookmark["Size"].GetInt()));
                }

                if (bookmark.HasMember("Decimal") && bookmark["Decimal"].GetBool())
                    vmBookmark->SetFormat(ra::MemFormat::Dec);
                else
                    vmBookmark->SetFormat(ra::MemFormat::Hex);

                const auto nValue = pEmulatorContext.ReadMemory(vmBookmark->GetAddress(), vmBookmark->GetSize());
                vmBookmark->SetCurrentValue(nValue);
                vmBookmark->SetPreviousValue(0U);
                vmBookmark->SetChanges(0U);
                vmBookmark->SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::None);
            }
        }
    }

    while (m_vBookmarks.Count() > ra::to_unsigned(nIndex))
        m_vBookmarks.RemoveAt(m_vBookmarks.Count() - 1);

    m_vBookmarks.AddNotifyTarget(*this);
    m_vBookmarks.EndUpdate();
}

void MemoryBookmarksViewModel::SaveBookmarks(ra::services::TextWriter& sBookmarksFile) const
{
    rapidjson::Document document;
    auto& allocator = document.GetAllocator();
    document.SetObject();

    rapidjson::Value bookmarks(rapidjson::kArrayType);
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
    {
        const auto& vmBookmark = *m_vBookmarks.GetItemAt(nIndex);

        rapidjson::Value item(rapidjson::kObjectType);
        item.AddMember("Address", vmBookmark.GetAddress(), allocator);
        item.AddMember("Size", gsl::narrow_cast<int>(ra::etoi(vmBookmark.GetSize())), allocator);

        if (vmBookmark.GetFormat() != MemFormat::Hex)
            item.AddMember("Decimal", true, allocator);

        if (vmBookmark.IsCustomDescription())
            item.AddMember("Description", ra::Narrow(vmBookmark.GetDescription()), allocator);

        bookmarks.PushBack(item, allocator);
    }

    document.AddMember("Bookmarks", bookmarks, allocator);
    SaveDocument(document, sBookmarksFile);
}

void MemoryBookmarksViewModel::DoFrame()
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    std::wstring sPauseOnChangeAddresses;

    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
    {
        auto& pBookmark = *m_vBookmarks.GetItemAt(nIndex);
        const auto nValue = pEmulatorContext.ReadMemory(pBookmark.GetAddress(), pBookmark.GetSize());

        const auto nCurrentValue = pBookmark.GetCurrentValue();
        if (nCurrentValue != nValue)
        {
            const auto nBehavior = pBookmark.GetBehavior();
            if (nBehavior == BookmarkBehavior::Frozen)
            {
                pEmulatorContext.WriteMemory(pBookmark.GetAddress(), pBookmark.GetSize(), nCurrentValue);
            }
            else
            {
                if (nBehavior == BookmarkBehavior::PauseOnChange)
                {
                    pBookmark.SetRowColor(ra::ui::Color(0xFFFFC0C0));

                    sPauseOnChangeAddresses.append(ra::StringPrintf(L"\n* %s %s",
                        m_vSizes.GetLabelForId(ra::etoi(pBookmark.GetSize())),
                        ra::ByteAddressToString(pBookmark.GetAddress())));
                }

                m_bIgnoreValueChanged = true;
                pBookmark.SetPreviousValue(nCurrentValue);
                pBookmark.SetCurrentValue(nValue);
                pBookmark.SetChanges(pBookmark.GetChanges() + 1);
                m_bIgnoreValueChanged = false;
            }
        }
        else if (pBookmark.GetBehavior() == BookmarkBehavior::PauseOnChange)
        {
            pBookmark.SetRowColor(ra::ui::Color(ra::to_unsigned(MemoryBookmarkViewModel::RowColorProperty.GetDefaultValue())));
        }
    }

    if (!sPauseOnChangeAddresses.empty())
    {
        pEmulatorContext.Pause();

        sPauseOnChangeAddresses = L"The following bookmarks have changed:" + sPauseOnChangeAddresses;
        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"The emulator has been paused.", sPauseOnChangeAddresses);
    }
}

bool MemoryBookmarksViewModel::HasBookmark(ra::ByteAddress nAddress) const
{
    return (m_vBookmarks.FindItemIndex(MemoryBookmarkViewModel::AddressProperty, nAddress) >= 0);
}

bool MemoryBookmarksViewModel::HasFrozenBookmark(ra::ByteAddress nAddress) const
{
    for (size_t nIndex = 0; nIndex < m_vBookmarks.Count(); ++nIndex)
    {
        const auto* pBookmark = m_vBookmarks.GetItemAt(nIndex);
        if (pBookmark != nullptr && pBookmark->GetBehavior() == BookmarkBehavior::Frozen && pBookmark->GetAddress() == nAddress)
            return true;
    }

    return false;
}

void MemoryBookmarksViewModel::OnEditMemory(ra::ByteAddress nAddress)
{
    // this function is very similar to DoFrame, but does not trigger behaviors.
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();

    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
    {
        auto& pBookmark = *m_vBookmarks.GetItemAt(nIndex);
        const auto nBookmarkAddress = pBookmark.GetAddress();
        const int distance = ra::to_signed(nBookmarkAddress) - ra::to_signed(nAddress);
        if (distance > 3)
            continue;

        const auto nBookmarkSize = pBookmark.GetSize();
        if (distance != 0)
        {
            bool bInBookmark = false;

            switch (nBookmarkSize)
            {
                case MemSize::SixteenBit:
                    bInBookmark = (distance <= 1);
                    break;

                case MemSize::TwentyFourBit:
                    bInBookmark = (distance <= 2);
                    break;

                case MemSize::ThirtyTwoBit:
                    bInBookmark = (distance <= 3);
                    break;
            }

            if (!bInBookmark)
                continue;
        }

        const auto nValue = pEmulatorContext.ReadMemory(nBookmarkAddress, nBookmarkSize);

        const auto nCurrentValue = pBookmark.GetCurrentValue();
        if (nCurrentValue != nValue)
        {
            m_bIgnoreValueChanged = true;
            pBookmark.SetPreviousValue(nCurrentValue);
            pBookmark.SetCurrentValue(nValue);
            pBookmark.SetChanges(pBookmark.GetChanges() + 1);
            m_bIgnoreValueChanged = false;
        }
    }
}

void MemoryBookmarksViewModel::AddBookmark(ra::ByteAddress nAddress, MemSize nSize)
{
    auto& vmBookmark = m_vBookmarks.Add();
    vmBookmark.SetAddress(nAddress);
    vmBookmark.SetSize(nSize);

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    vmBookmark.SetFormat(pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal) ? MemFormat::Dec : MemFormat::Hex);

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto* pNote = pGameContext.FindCodeNote(nAddress);
    if (pNote)
        vmBookmark.SetDescription(*pNote);

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    const auto nValue = pEmulatorContext.ReadMemory(nAddress, nSize);

    m_bIgnoreValueChanged = true;
    vmBookmark.SetCurrentValue(nValue);
    vmBookmark.SetPreviousValue(0U);
    vmBookmark.SetChanges(0U);
    m_bIgnoreValueChanged = false;

    m_bModified = true;
}

int MemoryBookmarksViewModel::RemoveSelectedBookmarks()
{
    m_vBookmarks.BeginUpdate();

    int nRemoved = 0;
    for (gsl::index nIndex = m_vBookmarks.Count() - 1; nIndex >= 0; --nIndex)
    {
        if (m_vBookmarks.GetItemAt(nIndex)->IsSelected())
        {
            m_vBookmarks.RemoveAt(nIndex);
            ++nRemoved;
            m_bModified = true;
        }
    }

    m_vBookmarks.EndUpdate();

    return nRemoved;
}

void MemoryBookmarksViewModel::ToggleFreezeSelected()
{
    m_vBookmarks.BeginUpdate();

    bool bFreeze = false;
    for (gsl::index nIndex = m_vBookmarks.Count() - 1; nIndex >= 0; --nIndex)
    {
        const auto* pItem = m_vBookmarks.GetItemAt(nIndex);
        if (pItem && pItem->IsSelected() && pItem->GetBehavior() != BookmarkBehavior::Frozen)
        {
            bFreeze = true;
            break;
        }
    }

    if (bFreeze)
    {
        for (gsl::index nIndex = m_vBookmarks.Count() - 1; nIndex >= 0; --nIndex)
        {
            auto* pItem = m_vBookmarks.GetItemAt(nIndex);
            if (pItem && pItem->IsSelected())
                pItem->SetBehavior(BookmarkBehavior::Frozen);
        }
    }
    else
    {
        for (gsl::index nIndex = m_vBookmarks.Count() - 1; nIndex >= 0; --nIndex)
        {
            auto* pItem = m_vBookmarks.GetItemAt(nIndex);
            if (pItem && pItem->IsSelected() && pItem->GetBehavior() == BookmarkBehavior::Frozen)
                pItem->SetBehavior(BookmarkBehavior::None);
        }
    }

    m_vBookmarks.EndUpdate();
}

void MemoryBookmarksViewModel::MoveSelectedBookmarksUp()
{
    m_vBookmarks.ShiftItemsUp(MemoryBookmarkViewModel::IsSelectedProperty);
}

void MemoryBookmarksViewModel::MoveSelectedBookmarksDown()
{
    m_vBookmarks.ShiftItemsDown(MemoryBookmarkViewModel::IsSelectedProperty);
}

void MemoryBookmarksViewModel::ClearAllChanges()
{
    for (gsl::index nIndex = m_vBookmarks.Count() - 1; nIndex >= 0; --nIndex)
        m_vBookmarks.SetItemValue(nIndex, MemoryBookmarksViewModel::MemoryBookmarkViewModel::ChangesProperty, 0);
}

void MemoryBookmarksViewModel::LoadBookmarkFile()
{
    ra::ui::viewmodels::FileDialogViewModel vmFileDialog;
    vmFileDialog.SetWindowTitle(L"Import Bookmark File");
    vmFileDialog.AddFileType(L"JSON File", L"*.json");
    vmFileDialog.AddFileType(L"Text File", L"*.txt");
    vmFileDialog.SetDefaultExtension(L"json");

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    vmFileDialog.SetFileName(ra::StringPrintf(L"%u-Bookmarks.json", pGameContext.GameId()));

    if (vmFileDialog.ShowOpenFileDialog() == ra::ui::DialogResult::OK)
    {
        const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
        auto pTextReader = pFileSystem.OpenTextFile(vmFileDialog.GetFileName());
        if (pTextReader == nullptr)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                ra::StringPrintf(L"Could not open %s", vmFileDialog.GetFileName()));
        }
        else
        {
            LoadBookmarks(*pTextReader);
        }
    }}

void MemoryBookmarksViewModel::SaveBookmarkFile() const
{
    ra::ui::viewmodels::FileDialogViewModel vmFileDialog;
    vmFileDialog.SetWindowTitle(L"Export Bookmark File");
    vmFileDialog.AddFileType(L"JSON File", L"*.json");
    vmFileDialog.SetDefaultExtension(L"json");

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    vmFileDialog.SetFileName(ra::StringPrintf(L"%u-Bookmarks.json", pGameContext.GameId()));

    if (vmFileDialog.ShowSaveFileDialog() == ra::ui::DialogResult::OK)
    {
        const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
        auto pTextWriter = pFileSystem.CreateTextFile(vmFileDialog.GetFileName());
        if (pTextWriter == nullptr)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                ra::StringPrintf(L"Could not create %s", vmFileDialog.GetFileName()));
        }
        else
        {
            SaveBookmarks(*pTextWriter);
        }
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
