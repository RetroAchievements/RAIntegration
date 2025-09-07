#include "MemoryBookmarksViewModel.hh"

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

const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::BehaviorProperty("MemoryBookmarkViewModel", "Behavior", ra::etoi(BookmarkBehavior::None));
const StringModelProperty MemoryBookmarksViewModel::FreezeButtonTextProperty("MemoryBookmarksViewModel", "FreezeButtonText", L"Freeze");
const StringModelProperty MemoryBookmarksViewModel::PauseButtonTextProperty("MemoryBookmarksViewModel", "PauseButtonText", L"Pause");

MemoryBookmarksViewModel::MemoryBookmarksViewModel() noexcept
{
    SetWindowTitle(L"Memory Bookmarks");

    m_vBehaviors.Add(ra::etoi(BookmarkBehavior::None), L"");
    m_vBehaviors.Add(ra::etoi(BookmarkBehavior::Frozen), L"Frozen");
    m_vBehaviors.Add(ra::etoi(BookmarkBehavior::PauseOnChange), L"Pause");

    m_vmMemoryWatchList.Items().AddNotifyTarget(*this);
}

void MemoryBookmarksViewModel::InitializeNotifyTargets()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.AddNotifyTarget(*this);

    m_vmMemoryWatchList.InitializeNotifyTargets();
}

GSL_SUPPRESS_F6
void MemoryBookmarksViewModel::MemoryBookmarkViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryBookmarkViewModel::BehaviorProperty)
    {
        switch (ra::itoe<BookmarkBehavior>(args.tNewValue))
        {
            case BookmarkBehavior::Frozen:
                SetValue(RowColorProperty, 0xFFFFFFC0);
                break;

            default:
                SetValue(RowColorProperty, MemoryBookmarkViewModel::RowColorProperty.GetDefaultValue());
                break;
        }
    }

    MemoryWatchViewModel::OnValueChanged(args);
}

bool MemoryBookmarksViewModel::MemoryBookmarkViewModel::ChangeValue(uint32_t nNewValue)
{
    const auto nBehavior = GetBehavior();

    if (nNewValue == GetCurrentValueRaw())
    {
        // if the value didn't change, remove any highlighting from previous pause events
        if (nBehavior == BookmarkBehavior::PauseOnChange)
            SetRowColor(ra::ui::Color(ra::to_unsigned(RowColorProperty.GetDefaultValue())));

        return false;
    }

    // value changed. if it's frozen, write the frozen value back into memory
    if (nBehavior == BookmarkBehavior::Frozen)
    {
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
        pEmulatorContext.WriteMemory(GetAddress(), GetSize(), GetCurrentValueRaw());
        return false;
    }

    if (!MemoryWatchViewModel::ChangeValue(nNewValue))
        return false;

    if (nBehavior == BookmarkBehavior::PauseOnChange)
        HandlePauseOnChange();

    return true;
}

bool MemoryBookmarksViewModel::MemoryBookmarkViewModel::IgnoreValueChange(uint32_t nNewValue)
{
    // if there's an invalid pointer in the indirect chain, don't try to write the frozen value back
    if (!IsIndirectAddressChainValid() && GetBehavior() == BookmarkBehavior::Frozen)
        return true;

    return MemoryWatchViewModel::IgnoreValueChange(nNewValue);
}

void MemoryBookmarksViewModel::MemoryBookmarkViewModel::HandlePauseOnChange()
{
    SetRowColor(ra::ui::Color(0xFFFFC0C0));

    auto sMessage =
        ra::StringPrintf(L"%s %s", ra::data::MemSizeString(GetSize()), ra::ByteAddressToString(GetAddress()));

    const auto& pDescription = GetRealNote();
    if (!pDescription.empty())
    {
        auto nDescriptionLength = pDescription.find(L'\n');
        if (nDescriptionLength == std::string::npos)
        {
            if (pDescription.length() < 40)
            {
                nDescriptionLength = pDescription.length();
            }
            else
            {
                nDescriptionLength = pDescription.find_last_of(L' ', 40);
                if (nDescriptionLength == std::string::npos)
                    nDescriptionLength = 40;
            }
        }
        sMessage.append(L": ");
        sMessage.append(pDescription, 0, nDescriptionLength);
    }

    auto& pFrameEventQueue = ra::services::ServiceLocator::GetMutable<ra::services::FrameEventQueue>();
    pFrameEventQueue.QueuePauseOnChange(sMessage);
}

bool MemoryBookmarksViewModel::IsModified() const
{
    if (m_vmMemoryWatchList.Items().Count() != m_nUnmodifiedBookmarkCount)
        return true;

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vmMemoryWatchList.Items().Count()); ++nIndex)
    {
        const auto& vmBookmark = *m_vmMemoryWatchList.Items().GetItemAt(nIndex);
        if (vmBookmark.IsModified())
            return true;
    }

    return false;
}

void MemoryBookmarksViewModel::OnActiveGameChanged()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (m_nLoadedGameId != pGameContext.GameId())
    {
        auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();

        if (m_nLoadedGameId != 0 && IsModified())
        {
            auto pWriter = pLocalStorage.WriteText(ra::services::StorageItemType::Bookmarks, std::to_wstring(m_nLoadedGameId));
            if (pWriter != nullptr)
                SaveBookmarks(*pWriter);
        }

        m_vmMemoryWatchList.Items().Clear();
        m_nUnmodifiedBookmarkCount = 0;

        m_nLoadedGameId = pGameContext.GameId();

        if (m_nLoadedGameId != 0)
        {
            auto pReader = pLocalStorage.ReadText(ra::services::StorageItemType::Bookmarks, std::to_wstring(m_nLoadedGameId));
            if (pReader != nullptr)
                LoadBookmarks(*pReader);
        }
    }
}

void MemoryBookmarksViewModel::OnEndGameLoad()
{
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vmMemoryWatchList.Items().Count(); ++nIndex)
    {
        auto& vmBookmark = *m_vmMemoryWatchList.Items().GetItemAt(nIndex);
        vmBookmark.UpdateRealNote();
    }
}

void MemoryBookmarksViewModel::LoadBookmarks(ra::services::TextReader& sBookmarksFile)
{
    gsl::index nIndex = 0;

    m_vmMemoryWatchList.Items().BeginUpdate();

    rapidjson::Document document;
    if (LoadDocument(document, sBookmarksFile))
    {
        if (document.HasMember("Bookmarks"))
        {
            const auto& bookmarks = document["Bookmarks"];
            for (const auto& bookmark : bookmarks.GetArray())
            {
                auto* vmBookmark = m_vmMemoryWatchList.Items().GetItemAt<MemoryBookmarkViewModel>(nIndex);
                if (vmBookmark == nullptr)
                {
                    vmBookmark = &m_vmMemoryWatchList.Items().Add<MemoryBookmarkViewModel>();
                    Ensures(vmBookmark != nullptr);
                }
                ++nIndex;

                vmBookmark->BeginInitialization();

                if (bookmark.HasMember("MemAddr"))
                {
                    // third bookmark format uses the memref serializer
                    const char* memaddr = bookmark["MemAddr"].GetString();
                    InitializeBookmark(*vmBookmark, memaddr);

                    if (bookmark.HasMember("Size"))
                    {
                        switch (bookmark["Size"].GetInt())
                        {
                            case 15: vmBookmark->SetSize(MemSize::Text); break;
                        }
                    }
                }
                else
                {
                    MemSize nSize = MemSize::EightBit;

                    if (bookmark.HasMember("Type"))
                    {
                        // original bookmark format used Type for the three supported sizes.
                        switch (bookmark["Type"].GetInt())
                        {
                            case 1: nSize = MemSize::EightBit; break;
                            case 2: nSize = MemSize::SixteenBit; break;
                            case 3: nSize = MemSize::ThirtyTwoBit; break;
                        }
                    }
                    else
                    {
                        // second bookmark format used the raw enum values, which was fragile.
                        // this enumerates the mapping for backwards compatibility.
                        switch (bookmark["Size"].GetInt())
                        {
                            case 0: nSize = MemSize::Bit_0; break;
                            case 1: nSize = MemSize::Bit_1; break;
                            case 2: nSize = MemSize::Bit_2; break;
                            case 3: nSize = MemSize::Bit_3; break;
                            case 4: nSize = MemSize::Bit_4; break;
                            case 5: nSize = MemSize::Bit_5; break;
                            case 6: nSize = MemSize::Bit_6; break;
                            case 7: nSize = MemSize::Bit_7; break;
                            case 8: nSize = MemSize::Nibble_Lower; break;
                            case 9: nSize = MemSize::Nibble_Upper; break;
                            case 10: nSize = MemSize::EightBit; break;
                            case 11: nSize = MemSize::SixteenBit; break;
                            case 12: nSize = MemSize::TwentyFourBit; break;
                            case 13: nSize = MemSize::ThirtyTwoBit; break;
                            case 14: nSize = MemSize::BitCount; break;
                            case 15: nSize = MemSize::Text; break;
                        }
                    }

                    vmBookmark->SetSize(nSize);
                    vmBookmark->SetAddress(bookmark["Address"].GetUint());
                }

                if (bookmark.HasMember("Decimal") && bookmark["Decimal"].GetBool())
                    vmBookmark->SetFormat(ra::MemFormat::Dec);
                else
                    vmBookmark->SetFormat(ra::MemFormat::Hex);

                if (!vmBookmark->IsIndirectAddress()) // Indirect note already called UpdateRealNote
                    vmBookmark->UpdateRealNote();

                if (bookmark.HasMember("Description"))
                {
                    const auto sDescription = ra::Widen(bookmark["Description"].GetString());
                    vmBookmark->SetDescription(sDescription);
                }

                vmBookmark->SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::None);

                vmBookmark->EndInitialization();
            }
        }
    }

    while (m_vmMemoryWatchList.Items().Count() > ra::to_unsigned(nIndex))
        m_vmMemoryWatchList.Items().RemoveAt(m_vmMemoryWatchList.Items().Count() - 1);

    m_vmMemoryWatchList.Items().EndUpdate();

    m_nUnmodifiedBookmarkCount = m_vmMemoryWatchList.Items().Count();
}

void MemoryBookmarksViewModel::SaveBookmarks(ra::services::TextWriter& sBookmarksFile)
{
    std::string sSerialized;

    rapidjson::Document document;
    auto& allocator = document.GetAllocator();
    document.SetObject();

    rapidjson::Value bookmarks(rapidjson::kArrayType);
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vmMemoryWatchList.Items().Count(); ++nIndex)
    {
        auto& vmBookmark = *m_vmMemoryWatchList.Items().GetItemAt(nIndex);

        rapidjson::Value item(rapidjson::kObjectType);

        const auto nSize = vmBookmark.GetSize();
        switch (nSize)
        {
            case MemSize::Text:
                item.AddMember("Size", 15, allocator);
                if (vmBookmark.IsIndirectAddress())
                    item.AddMember("MemAddr", vmBookmark.GetIndirectAddress(), allocator);
                else
                    item.AddMember("Address", vmBookmark.GetAddress(), allocator);
                break;

            default:
                if (vmBookmark.IsIndirectAddress())
                {
                    item.AddMember("MemAddr", vmBookmark.GetIndirectAddress(), allocator);
                }
                else
                {
                    sSerialized.clear();
                    ra::services::AchievementLogicSerializer::AppendOperand(
                        sSerialized, ra::services::TriggerOperandType::Address, nSize, vmBookmark.GetAddress());
                    item.AddMember("MemAddr", sSerialized, allocator);
                }
                break;
        }

        if (vmBookmark.GetFormat() != MemFormat::Hex)
            item.AddMember("Decimal", true, allocator);

        if (vmBookmark.IsCustomDescription())
            item.AddMember("Description", ra::Narrow(vmBookmark.GetDescription()), allocator);

        bookmarks.PushBack(item, allocator);
        vmBookmark.ResetModified();
    }

    document.AddMember("Bookmarks", bookmarks, allocator);
    SaveDocument(document, sBookmarksFile);
}

void MemoryBookmarksViewModel::DoFrame()
{
    m_vmMemoryWatchList.DoFrame();
}

bool MemoryBookmarksViewModel::HasBookmark(ra::ByteAddress nAddress) const
{
    for (size_t nIndex = 0; nIndex < m_vmMemoryWatchList.Items().Count(); ++nIndex)
    {
        const auto* pBookmark = m_vmMemoryWatchList.Items().GetItemAt(nIndex);
        if (pBookmark != nullptr && pBookmark->GetAddress() == nAddress)
            return true;
    }

    return false;
}

bool MemoryBookmarksViewModel::HasFrozenBookmark(ra::ByteAddress nAddress) const
{
    for (size_t nIndex = 0; nIndex < m_vmMemoryWatchList.Items().Count(); ++nIndex)
    {
        const auto* pBookmark = m_vmMemoryWatchList.Items().GetItemAt<MemoryBookmarkViewModel>(nIndex);
        if (pBookmark != nullptr && pBookmark->GetBehavior() == BookmarkBehavior::Frozen && pBookmark->GetAddress() == nAddress)
            return true;
    }

    return false;
}

void MemoryBookmarksViewModel::AddBookmark(ra::ByteAddress nAddress, MemSize nSize)
{
    auto vmBookmark = std::make_unique<MemoryBookmarkViewModel>();
    vmBookmark->BeginInitialization();

    vmBookmark->SetAddress(nAddress);
    vmBookmark->SetSize(nSize);
    InitializeBookmark(*vmBookmark);

    vmBookmark->EndInitialization();

    m_vmMemoryWatchList.Items().Append(std::move(vmBookmark));
}

void MemoryBookmarksViewModel::AddBookmark(const std::string& sSerialized)
{
    auto vmBookmark = std::make_unique<MemoryBookmarkViewModel>();
    vmBookmark->BeginInitialization();

    InitializeBookmark(*vmBookmark, sSerialized);
    InitializeBookmark(*vmBookmark);

    vmBookmark->EndInitialization();

    m_vmMemoryWatchList.Items().Append(std::move(vmBookmark));
}

void MemoryBookmarksViewModel::InitializeBookmark(MemoryWatchViewModel& vmBookmark)
{
    vmBookmark.SetFormat(MemFormat::Unknown);

    vmBookmark.UpdateRealNote();
}

void MemoryBookmarksViewModel::InitializeBookmark(MemoryWatchViewModel& vmBookmark, const std::string& sSerialized)
{
    // if there's no condition separator, it's a simple memref
    if (sSerialized.find('_') == std::string::npos)
    {
        uint8_t size = 0;
        uint32_t address = 0;
        const char* memaddr = sSerialized.c_str();
        if (ra::StringStartsWith(sSerialized, "M:"))
            memaddr += 2;

        if (rc_parse_memref(&memaddr, &size, &address) == RC_OK)
        {
            vmBookmark.SetAddress(address);
            vmBookmark.SetSize(ra::data::models::TriggerValidation::MapRcheevosMemSize(size));
        }

        return;
    }

    // complex memref.
    vmBookmark.SetIndirectAddress(sSerialized);
}

int MemoryBookmarksViewModel::RemoveSelectedBookmarks()
{
    m_vmMemoryWatchList.Items().BeginUpdate();

    int nRemoved = 0;
    for (gsl::index nIndex = m_vmMemoryWatchList.Items().Count() - 1; nIndex >= 0; --nIndex)
    {
        if (m_vmMemoryWatchList.Items().GetItemAt(nIndex)->IsSelected())
        {
            m_vmMemoryWatchList.Items().RemoveAt(nIndex);
            ++nRemoved;
        }
    }

    m_vmMemoryWatchList.Items().EndUpdate();

    return nRemoved;
}

void MemoryBookmarksViewModel::OnViewModelBoolValueChanged(gsl::index, const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == LookupItemViewModel::IsSelectedProperty)
    {
        if (!m_vmMemoryWatchList.Items().IsUpdating())
        {
            UpdateFreezeButtonText();
            UpdatePauseButtonText();
        }
    }
}

void MemoryBookmarksViewModel::OnViewModelIntValueChanged(gsl::index, const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryBookmarkViewModel::BehaviorProperty)
    {
        if (!m_vmMemoryWatchList.Items().IsUpdating())
        {
            UpdateFreezeButtonText();
            UpdatePauseButtonText();
        }
    }
}

void MemoryBookmarksViewModel::OnEndViewModelCollectionUpdate()
{
    UpdateFreezeButtonText();
    UpdatePauseButtonText();
}

bool MemoryBookmarksViewModel::ShouldFreeze() const
{
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vmMemoryWatchList.Items().Count(); ++nIndex)
    {
        const auto& pBookmark = *m_vmMemoryWatchList.Items().GetItemAt<MemoryBookmarkViewModel>(nIndex);
        if (pBookmark.IsSelected() && pBookmark.GetBehavior() != BookmarkBehavior::Frozen)
            return true;
    }

    return false;
}

void MemoryBookmarksViewModel::UpdateFreezeButtonText()
{
    if (!m_vmMemoryWatchList.HasSelection() || ShouldFreeze())
        SetValue(FreezeButtonTextProperty, FreezeButtonTextProperty.GetDefaultValue());
    else
        SetValue(FreezeButtonTextProperty, L"Unfreeze");
}

void MemoryBookmarksViewModel::ToggleFreezeSelected()
{
    m_vmMemoryWatchList.Items().BeginUpdate();

    if (ShouldFreeze())
    {
        for (gsl::index nIndex = m_vmMemoryWatchList.Items().Count() - 1; nIndex >= 0; --nIndex)
        {
            auto* pItem = m_vmMemoryWatchList.Items().GetItemAt<MemoryBookmarkViewModel>(nIndex);
            if (pItem && pItem->IsSelected())
                pItem->SetBehavior(BookmarkBehavior::Frozen);
        }
    }
    else
    {
        for (gsl::index nIndex = m_vmMemoryWatchList.Items().Count() - 1; nIndex >= 0; --nIndex)
        {
            auto* pItem = m_vmMemoryWatchList.Items().GetItemAt<MemoryBookmarkViewModel>(nIndex);
            if (pItem && pItem->IsSelected() && pItem->GetBehavior() == BookmarkBehavior::Frozen)
                pItem->SetBehavior(BookmarkBehavior::None);
        }
    }

    m_vmMemoryWatchList.Items().EndUpdate();
}

bool MemoryBookmarksViewModel::ShouldPause() const
{
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vmMemoryWatchList.Items().Count(); ++nIndex)
    {
        const auto& pBookmark = *m_vmMemoryWatchList.Items().GetItemAt<MemoryBookmarkViewModel>(nIndex);
        if (pBookmark.IsSelected() && pBookmark.GetBehavior() != BookmarkBehavior::PauseOnChange)
            return true;
    }

    return false;
}

void MemoryBookmarksViewModel::UpdatePauseButtonText()
{
    if (!m_vmMemoryWatchList.HasSelection() || ShouldPause())
        SetValue(PauseButtonTextProperty, PauseButtonTextProperty.GetDefaultValue());
    else
        SetValue(PauseButtonTextProperty, L"Stop Pausing");
}

void MemoryBookmarksViewModel::TogglePauseSelected()
{
    m_vmMemoryWatchList.Items().BeginUpdate();

    if (ShouldPause())
    {
        for (gsl::index nIndex = m_vmMemoryWatchList.Items().Count() - 1; nIndex >= 0; --nIndex)
        {
            auto* pItem = m_vmMemoryWatchList.Items().GetItemAt<MemoryBookmarkViewModel>(nIndex);
            if (pItem && pItem->IsSelected())
                pItem->SetBehavior(BookmarkBehavior::PauseOnChange);
        }
    }
    else
    {
        for (gsl::index nIndex = m_vmMemoryWatchList.Items().Count() - 1; nIndex >= 0; --nIndex)
        {
            auto* pItem = m_vmMemoryWatchList.Items().GetItemAt<MemoryBookmarkViewModel>(nIndex);
            if (pItem && pItem->IsSelected() && pItem->GetBehavior() == BookmarkBehavior::PauseOnChange)
                pItem->SetBehavior(BookmarkBehavior::None);
        }
    }

    m_vmMemoryWatchList.Items().EndUpdate();
}

void MemoryBookmarksViewModel::MoveSelectedBookmarksUp()
{
    m_vmMemoryWatchList.Items().ShiftItemsUp(MemoryBookmarkViewModel::IsSelectedProperty);
}

void MemoryBookmarksViewModel::MoveSelectedBookmarksDown()
{
    m_vmMemoryWatchList.Items().ShiftItemsDown(MemoryBookmarkViewModel::IsSelectedProperty);
}

void MemoryBookmarksViewModel::ClearBehaviors()
{
    for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vmMemoryWatchList.Items().Count()); ++nIndex)
    {
        auto* pBookmark = m_vmMemoryWatchList.Items().GetItemAt<MemoryBookmarkViewModel>(nIndex);
        if (pBookmark)
            pBookmark->SetBehavior(BookmarkBehavior::None);
    }
}

void MemoryBookmarksViewModel::LoadBookmarkFile()
{
    ra::ui::viewmodels::FileDialogViewModel vmFileDialog;
    vmFileDialog.SetWindowTitle(L"Import Bookmark File");
    vmFileDialog.AddFileType(L"JSON File", L"*.json");
    vmFileDialog.AddFileType(L"Text File", L"*.txt");
    vmFileDialog.SetDefaultExtension(L"json");

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
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

void MemoryBookmarksViewModel::SaveBookmarkFile()
{
    ra::ui::viewmodels::FileDialogViewModel vmFileDialog;
    vmFileDialog.SetWindowTitle(L"Export Bookmark File");
    vmFileDialog.AddFileType(L"JSON File", L"*.json");
    vmFileDialog.SetDefaultExtension(L"json");

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
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
