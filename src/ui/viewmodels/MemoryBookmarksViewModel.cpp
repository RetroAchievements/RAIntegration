#include "MemoryBookmarksViewModel.hh"

#include "RA_Json.h"
#include "RA_StringUtils.h"

#include "data\EmulatorContext.hh"

#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::DescriptionProperty("MemoryBookmarkViewModel", "Description", L"");
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::AddressProperty("MemoryBookmarkViewModel", "Address", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty("MemoryBookmarkViewModel", "Size", ra::etoi(MemSize::EightBit));
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::FormatProperty("MemoryBookmarkViewModel", "Format", ra::etoi(MemFormat::Hex));
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::CurrentValueProperty("MemoryBookmarkViewModel", "CurrentValue", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::PreviousValueProperty("MemoryBookmarkViewModel", "PreviousValue", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::ChangesProperty("MemoryBookmarkViewModel", "Changes", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::BehaviorProperty("MemoryBookmarkViewModel", "Behavior", ra::etoi(BookmarkBehavior::None));

MemoryBookmarksViewModel::MemoryBookmarksViewModel() noexcept
{
    m_vSizes.Add(ra::etoi(MemSize::EightBit), L"8-bit");
    m_vSizes.Add(ra::etoi(MemSize::SixteenBit), L"16-bit");
    m_vSizes.Add(ra::etoi(MemSize::ThirtyTwoBit), L"32-bit");

    m_vFormats.Add(ra::etoi(MemFormat::Hex), L"Hex");
    m_vFormats.Add(ra::etoi(MemFormat::Dec), L"Dec");

    m_vBehaviors.Add(ra::etoi(BookmarkBehavior::None), L"");
    m_vBehaviors.Add(ra::etoi(BookmarkBehavior::Frozen), L"Frozen");

    AddNotifyTarget(*this);
}

void MemoryBookmarksViewModel::OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == IsVisibleProperty)
    {
        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
        if (args.tNewValue)
        {
            pGameContext.RemoveNotifyTarget(*this);
            pGameContext.AddNotifyTarget(*this);
            OnActiveGameChanged();
        }
    }
}

void MemoryBookmarksViewModel::OnActiveGameChanged()
{
    if (IsVisible())
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
        if (m_nLoadedGameId != pGameContext.GameId())
        {
            auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();

            auto pReader = pLocalStorage.ReadText(ra::services::StorageItemType::Bookmarks, std::to_wstring(pGameContext.GameId()));
            if (pReader != nullptr)
            {
                LoadBookmarks(*pReader);
            }
            else
            {
                while (m_vBookmarks.Count() > 0)
                    m_vBookmarks.RemoveAt(m_vBookmarks.Count() - 1);
            }

            m_nLoadedGameId = pGameContext.GameId();
        }
    }
}

void MemoryBookmarksViewModel::LoadBookmarks(ra::services::TextReader& sBookmarksFile)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    gsl::index nIndex = 0;

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

                std::wstring sDescription;
                if (bookmark.HasMember("Description"))
                {
                    sDescription = ra::Widen(bookmark["Description"].GetString());
                }
                else
                {
                    const auto* pNote = pGameContext.FindCodeNote(vmBookmark->GetAddress());
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
}

void MemoryBookmarksViewModel::DoFrame()
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();

    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
    {
        auto& pBookmark = *m_vBookmarks.GetItemAt(nIndex);
        const auto nValue = pEmulatorContext.ReadMemory(pBookmark.GetAddress(), pBookmark.GetSize());

        const auto nCurrentValue = pBookmark.GetCurrentValue();
        if (nCurrentValue != nValue)
        {
            if (pBookmark.GetBehavior() == BookmarkBehavior::Frozen)
            {
                pEmulatorContext.WriteMemory(pBookmark.GetAddress(), pBookmark.GetSize(), nCurrentValue);
            }
            else
            {
                pBookmark.SetPreviousValue(nCurrentValue);
                pBookmark.SetCurrentValue(nValue);
                pBookmark.SetChanges(pBookmark.GetChanges() + 1);
            }
        }
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
