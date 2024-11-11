#include "MemoryBookmarksViewModel.hh"

#include "RA_Defs.h"
#include "RA_Json.h"
#include "RA_StringUtils.h"

#include "data\context\EmulatorContext.hh"
#include "data\models\TriggerValidation.hh"

#include "services\FrameEventQueue.hh"
#include "services\IConfiguration.hh"
#include "services\IFileSystem.hh"
#include "services\ILocalStorage.hh"
#include "services\SearchResults.h"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\FileDialogViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\TriggerConditionViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include <rcheevos/src/rcheevos/rc_internal.h>

#ifdef RA_UTEST
// awkward workaround to allow individual bookmarks access to the bookmarks view model being tested
extern ra::ui::viewmodels::MemoryBookmarksViewModel* g_pMemoryBookmarksViewModel;
#endif

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::DescriptionProperty("MemoryBookmarkViewModel", "Description", L"");
const BoolModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::IsCustomDescriptionProperty("MemoryBookmarkViewModel", "IsCustomDescription", false);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::AddressProperty("MemoryBookmarkViewModel", "Address", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty("MemoryBookmarkViewModel", "Size", ra::etoi(MemSize::EightBit));
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::FormatProperty("MemoryBookmarkViewModel", "Format", ra::etoi(MemFormat::Hex));
const StringModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::CurrentValueProperty("MemoryBookmarkViewModel", "CurrentValue", L"0");
const StringModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::PreviousValueProperty("MemoryBookmarkViewModel", "PreviousValue", L"0");
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::ChangesProperty("MemoryBookmarkViewModel", "Changes", 0);
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::BehaviorProperty("MemoryBookmarkViewModel", "Behavior", ra::etoi(BookmarkBehavior::None));
const IntModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::RowColorProperty("MemoryBookmarkViewModel", "RowColor", 0);
const BoolModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::ReadOnlyProperty("MemoryBookmarkViewModel", "IsReadOnly", false);
const BoolModelProperty MemoryBookmarksViewModel::MemoryBookmarkViewModel::IsDirtyProperty("MemoryBookmarkViewModel", "IsDirty", false);
const BoolModelProperty MemoryBookmarksViewModel::HasSelectionProperty("MemoryBookmarkViewModel", "HasSelection", false);
const StringModelProperty MemoryBookmarksViewModel::FreezeButtonTextProperty("MemoryBookmarksViewModel", "FreezeButtonText", L"Freeze");
const StringModelProperty MemoryBookmarksViewModel::PauseButtonTextProperty("MemoryBookmarksViewModel", "PauseButtonText", L"Pause");

constexpr int MaxTextBookmarkLength = 8;

MemoryBookmarksViewModel::MemoryBookmarksViewModel() noexcept
{
    SetWindowTitle(L"Memory Bookmarks");

    m_vSizes.Add(ra::etoi(MemSize::EightBit), L" 8-bit"); // leading space for sort order
    m_vSizes.Add(ra::etoi(MemSize::SixteenBit), L"16-bit");
    m_vSizes.Add(ra::etoi(MemSize::TwentyFourBit), L"24-bit");
    m_vSizes.Add(ra::etoi(MemSize::ThirtyTwoBit), L"32-bit");
    m_vSizes.Add(ra::etoi(MemSize::SixteenBitBigEndian), L"16-bit BE");
    m_vSizes.Add(ra::etoi(MemSize::TwentyFourBitBigEndian), L"24-bit BE");
    m_vSizes.Add(ra::etoi(MemSize::ThirtyTwoBitBigEndian), L"32-bit BE");
    m_vSizes.Add(ra::etoi(MemSize::BitCount), L"BitCount");
    m_vSizes.Add(ra::etoi(MemSize::Nibble_Lower), L"Lower4");
    m_vSizes.Add(ra::etoi(MemSize::Nibble_Upper), L"Upper4");
    m_vSizes.Add(ra::etoi(MemSize::Float), L"Float");
    m_vSizes.Add(ra::etoi(MemSize::FloatBigEndian), L"Float BE");
    m_vSizes.Add(ra::etoi(MemSize::Double32), L"Double32");
    m_vSizes.Add(ra::etoi(MemSize::Double32BigEndian), L"Double32 BE");
    m_vSizes.Add(ra::etoi(MemSize::MBF32), L"MBF32");
    m_vSizes.Add(ra::etoi(MemSize::MBF32LE), L"MBF32 LE");
    m_vSizes.Add(ra::etoi(MemSize::Text), L"ASCII");

    m_vFormats.Add(ra::etoi(MemFormat::Hex), L"Hex");
    m_vFormats.Add(ra::etoi(MemFormat::Dec), L"Dec");

    m_vBehaviors.Add(ra::etoi(BookmarkBehavior::None), L"");
    m_vBehaviors.Add(ra::etoi(BookmarkBehavior::Frozen), L"Frozen");
    m_vBehaviors.Add(ra::etoi(BookmarkBehavior::PauseOnChange), L"Pause");

    m_vBookmarks.AddNotifyTarget(*this);
}

void MemoryBookmarksViewModel::InitializeNotifyTargets()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.AddNotifyTarget(*this);

    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.AddNotifyTarget(*this);
}

GSL_SUPPRESS_F6
void MemoryBookmarksViewModel::MemoryBookmarkViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryBookmarkViewModel::FormatProperty)
    {
        if (m_bInitialized)
        {
            m_bModified = true;
            SetValue(CurrentValueProperty, BuildCurrentValue());
        }
    }
    else if (args.Property == MemoryBookmarkViewModel::SizeProperty)
    {
        m_nSize = ra::itoe<MemSize>(args.tNewValue);
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
            m_nValue = ReadValue();
            SetValue(CurrentValueProperty, BuildCurrentValue());
        }
    }
    else if (args.Property == MemoryBookmarkViewModel::AddressProperty)
    {
        m_nAddress = args.tNewValue;

        if (m_bInitialized)
        {
            BeginInitialization();
            EndInitialization();
            m_bModified = true;
        }
    }
    else if (args.Property == MemoryBookmarkViewModel::BehaviorProperty)
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

    LookupItemViewModel::OnValueChanged(args);
}

void MemoryBookmarksViewModel::MemoryBookmarkViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryBookmarkViewModel::DescriptionProperty)
    {
        if (m_bInitialized)
        {
            m_bModified = true;

            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
            const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
            const auto* pNote = (pCodeNotes != nullptr) ? pCodeNotes->FindCodeNote(m_nAddress) : nullptr;

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
            SetIsCustomDescription(bIsCustomDescription);
        }
    }

    LookupItemViewModel::OnValueChanged(args);
}

unsigned MemoryBookmarksViewModel::MemoryBookmarkViewModel::ReadValue() const
{
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

void MemoryBookmarksViewModel::MemoryBookmarkViewModel::EndInitialization()
{
    m_nValue = 0;
    SetPreviousValue(BuildCurrentValue());

    m_nValue = ReadValue();
    SetValue(CurrentValueProperty, BuildCurrentValue());

    SetChanges(0);

    m_bModified = false;
    m_bInitialized = true;
}

bool MemoryBookmarksViewModel::MemoryBookmarkViewModel::MemoryChanged()
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    const auto nValue = ReadValue();
    if (nValue == m_nValue)
        return false;

    if (GetBehavior() == BookmarkBehavior::Frozen)
    {
        pEmulatorContext.WriteMemory(m_nAddress, m_nSize, m_nValue);
        return false;
    }

    m_nValue = nValue;
    OnValueChanged();

    return true;
}

void MemoryBookmarksViewModel::MemoryBookmarkViewModel::OnValueChanged()
{
    const std::wstring sValue = BuildCurrentValue();
    SetPreviousValue(GetCurrentValue());
    SetValue(CurrentValueProperty, sValue);
    SetChanges(GetChanges() + 1);
}

bool MemoryBookmarksViewModel::MemoryBookmarkViewModel::SetCurrentValue(const std::wstring& sValue, _Out_ std::wstring& sError)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    const auto nAddress = m_nAddress;
    unsigned nValue = 0;

    switch (m_nSize)
    {
        case MemSize::Float:
        case MemSize::FloatBigEndian:
        case MemSize::Double32:
        case MemSize::Double32BigEndian:
        case MemSize::MBF32:
        case MemSize::MBF32LE:
            float fValue;
            if (!ra::ParseFloat(sValue, fValue, sError))
                return false;

            nValue = ra::data::FloatToU32(fValue, m_nSize);
            break;

        default:
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
            break;
    }

    // do not set m_nValue here, it will get set by UpdateCurrentValue which will be called by the
    // OnByteWritten notification handler when WriteMemory modifies the EmulatorContext.
#ifndef RA_UTEST
    auto& vmBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
#else
    auto& vmBookmarks = *g_pMemoryBookmarksViewModel;
#endif
    vmBookmarks.BeginWritingMemory();
    pEmulatorContext.WriteMemory(nAddress, m_nSize, nValue);
    vmBookmarks.EndWritingMemory();

    return true;
}

void MemoryBookmarksViewModel::MemoryBookmarkViewModel::UpdateCurrentValue()
{
    const auto nValue = ReadValue();
    if (nValue != m_nValue)
    {
        m_nValue = nValue;
        OnValueChanged();
    }
}

std::wstring MemoryBookmarksViewModel::MemoryBookmarkViewModel::BuildCurrentValue() const
{
    if (m_nSize == MemSize::Text)
    {
        ra::services::SearchResults pResults;
        pResults.Initialize(m_nAddress, MaxTextBookmarkLength, ra::services::SearchType::AsciiText);
        return pResults.GetFormattedValue(m_nAddress, MemSize::Text);
    }

    return ra::data::MemSizeFormat(m_nValue, m_nSize, GetFormat());
}

bool MemoryBookmarksViewModel::IsModified() const
{
    if (m_vBookmarks.Count() != m_nUnmodifiedBookmarkCount)
        return true;

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vBookmarks.Count()); ++nIndex)
    {
        const auto& vmBookmark = *m_vBookmarks.GetItemAt(nIndex);
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
        if (m_nLoadedGameId != 0 && IsModified())
        {
            auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();

            auto pWriter = pLocalStorage.WriteText(ra::services::StorageItemType::Bookmarks, std::to_wstring(m_nLoadedGameId));
            if (pWriter != nullptr)
                SaveBookmarks(*pWriter);
        }

        auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();

        auto pReader = pLocalStorage.ReadText(ra::services::StorageItemType::Bookmarks, std::to_wstring(pGameContext.GameId()));
        if (pReader != nullptr)
        {
            LoadBookmarks(*pReader);
        }
        else
        {
            m_vBookmarks.Clear();
            m_nUnmodifiedBookmarkCount = 0;
        }

        m_nLoadedGameId = pGameContext.GameId();
    }
}

void MemoryBookmarksViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote)
{
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
    {
        auto* pBookmark = m_vBookmarks.GetItemAt(nIndex);
        if (pBookmark && pBookmark->GetAddress() == nAddress)
        {
            if (!pBookmark->IsCustomDescription())
            {
                pBookmark->SetDescription(sNewNote);
            }
            else
            {
                if (pBookmark->GetDescription() == sNewNote)
                    pBookmark->SetIsCustomDescription(false);
            }
        }
    }
}

void MemoryBookmarksViewModel::LoadBookmarks(ra::services::TextReader& sBookmarksFile)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    gsl::index nIndex = 0;

    m_vBookmarks.BeginUpdate();

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
                {
                    vmBookmark = &m_vBookmarks.Add();
                    Ensures(vmBookmark != nullptr);
                }
                ++nIndex;

                vmBookmark->BeginInitialization();

                if (bookmark.HasMember("MemAddr"))
                {
                    // third bookmark format uses the memref serializer
                    uint8_t size = 0;
                    unsigned address = 0;
                    const char* memaddr = bookmark["MemAddr"].GetString();
                    if (rc_parse_memref(&memaddr, &size, &address) == RC_OK)
                    {
                        vmBookmark->SetAddress(address);
                        vmBookmark->SetSize(ra::data::models::TriggerValidation::MapRcheevosMemSize(size));
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

                const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
                const auto* pNote = (pCodeNotes != nullptr) ? pCodeNotes->FindCodeNote(vmBookmark->GetAddress()) : nullptr;

                std::wstring sDescription;
                if (bookmark.HasMember("Description"))
                {
                    sDescription = ra::Widen(bookmark["Description"].GetString());
                    vmBookmark->SetIsCustomDescription(!pNote || sDescription != *pNote);
                }
                else
                {
                    vmBookmark->SetIsCustomDescription(false);
                    if (pNote)
                        sDescription = *pNote;
                }
                vmBookmark->SetDescription(sDescription);

                if (bookmark.HasMember("Decimal") && bookmark["Decimal"].GetBool())
                    vmBookmark->SetFormat(ra::MemFormat::Dec);
                else
                    vmBookmark->SetFormat(ra::MemFormat::Hex);

                vmBookmark->SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::None);

                vmBookmark->EndInitialization();
            }
        }
    }

    while (m_vBookmarks.Count() > ra::to_unsigned(nIndex))
        m_vBookmarks.RemoveAt(m_vBookmarks.Count() - 1);

    m_vBookmarks.EndUpdate();

    m_nUnmodifiedBookmarkCount = m_vBookmarks.Count();
}

void MemoryBookmarksViewModel::SaveBookmarks(ra::services::TextWriter& sBookmarksFile)
{
    TriggerConditionViewModel vmCondition;
    vmCondition.SetOperator(ra::ui::viewmodels::TriggerOperatorType::None);

    rapidjson::Document document;
    auto& allocator = document.GetAllocator();
    document.SetObject();

    rapidjson::Value bookmarks(rapidjson::kArrayType);
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
    {
        auto& vmBookmark = *m_vBookmarks.GetItemAt(nIndex);

        rapidjson::Value item(rapidjson::kObjectType);

        const auto nSize = vmBookmark.GetSize();
        switch (nSize)
        {
            case MemSize::Text:
                item.AddMember("Size", 15, allocator);
                item.AddMember("Address", vmBookmark.GetAddress(), allocator);
                break;

            default:
                vmCondition.SetSourceSize(nSize);
                vmCondition.SetSourceValue(vmBookmark.GetAddress());
                item.AddMember("MemAddr", vmCondition.Serialize(), allocator);
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

void MemoryBookmarksViewModel::OnByteWritten(ra::ByteAddress nAddress, uint8_t)
{
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
    {
        auto& pBookmark = *m_vBookmarks.GetItemAt(nIndex);
        const auto nBookmarkAddress = pBookmark.GetAddress();
        if (nAddress < nBookmarkAddress)
            continue;

        const auto nSize = pBookmark.GetSize();
        const auto nBytes = (nSize == MemSize::Text) ? MaxTextBookmarkLength : ra::data::MemSizeBytes(nSize);
        if (nAddress >= nBookmarkAddress + nBytes)
            continue;

        if (m_nWritingMemoryCount)
            pBookmark.SetDirty(true);
        else
            pBookmark.UpdateCurrentValue();
    }
}

void MemoryBookmarksViewModel::EndWritingMemory()
{
    if (--m_nWritingMemoryCount == 0)
    {
        for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
        {
            auto& pBookmark = *m_vBookmarks.GetItemAt(nIndex);
            if (pBookmark.IsDirty())
            {
                pBookmark.SetDirty(false);
                pBookmark.UpdateCurrentValue();
            }
        }
    }
}

void MemoryBookmarksViewModel::DoFrame()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.RemoveNotifyTarget(*this);

    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
    {
        auto& pBookmark = *m_vBookmarks.GetItemAt(nIndex);
        UpdateBookmark(pBookmark);
    }

    pEmulatorContext.AddNotifyTarget(*this);
}

void MemoryBookmarksViewModel::UpdateBookmark(MemoryBookmarksViewModel::MemoryBookmarkViewModel& pBookmark)
{
    if (pBookmark.MemoryChanged())
    {
        if (pBookmark.GetBehavior() == BookmarkBehavior::PauseOnChange)
        {
            pBookmark.SetRowColor(ra::ui::Color(0xFFFFC0C0));

            const auto nSizeIndex = m_vSizes.FindItemIndex(LookupItemViewModel::IdProperty, ra::etoi(pBookmark.GetSize()));
            Expects(nSizeIndex >= 0);

            auto sMessage = ra::StringPrintf(L"%s %s", m_vSizes.GetItemAt(nSizeIndex)->GetLabel(), ra::ByteAddressToString(pBookmark.GetAddress()));

            // remove leading space of " 8-bit"
            if (isspace(sMessage.at(0)))
                sMessage.erase(0, 1);

            const auto& pDescription = pBookmark.GetDescription();
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
    }
    else if (pBookmark.GetBehavior() == BookmarkBehavior::PauseOnChange)
    {
        pBookmark.SetRowColor(ra::ui::Color(ra::to_unsigned(MemoryBookmarkViewModel::RowColorProperty.GetDefaultValue())));
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

void MemoryBookmarksViewModel::AddBookmark(ra::ByteAddress nAddress, MemSize nSize)
{
    auto vmBookmark = std::make_unique<MemoryBookmarkViewModel>();
    vmBookmark->BeginInitialization();
    vmBookmark->SetAddress(nAddress);
    vmBookmark->SetSize(nSize);

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    vmBookmark->SetFormat(pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal) ? MemFormat::Dec : MemFormat::Hex);

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    const auto* pNote = (pCodeNotes != nullptr) ? pCodeNotes->FindCodeNote(nAddress) : nullptr;
    if (pNote)
    {
        vmBookmark->SetDescription(*pNote);

        // if bookmarking an 8-byte double, automatically adjust the bookmark for the significant bytes
        if (nSize == MemSize::Double32 && pCodeNotes->GetCodeNoteBytes(nAddress) == 8)
            vmBookmark->SetAddress(nAddress + 4);
    }

    vmBookmark->EndInitialization();

    m_vBookmarks.Append(std::move(vmBookmark));
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
        }
    }

    m_vBookmarks.EndUpdate();

    return nRemoved;
}

void MemoryBookmarksViewModel::OnViewModelBoolValueChanged(gsl::index, const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == LookupItemViewModel::IsSelectedProperty)
    {
        if (!m_vBookmarks.IsUpdating())
        {
            UpdateHasSelection();
            UpdateFreezeButtonText();
            UpdatePauseButtonText();
        }
    }
}

void MemoryBookmarksViewModel::OnViewModelIntValueChanged(gsl::index, const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == MemoryBookmarkViewModel::BehaviorProperty)
    {
        if (!m_vBookmarks.IsUpdating())
        {
            UpdateFreezeButtonText();
            UpdatePauseButtonText();
        }
    }
}

void MemoryBookmarksViewModel::OnEndViewModelCollectionUpdate()
{
    UpdateHasSelection();
    UpdateFreezeButtonText();
    UpdatePauseButtonText();
}

void MemoryBookmarksViewModel::UpdateHasSelection()
{
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
    {
        const auto& pBookmark = *m_vBookmarks.GetItemAt(nIndex);
        if (pBookmark.IsSelected())
        {
            SetValue(HasSelectionProperty, true);
            return;
        }
    }

    SetValue(HasSelectionProperty, false);
}

bool MemoryBookmarksViewModel::ShouldFreeze() const
{
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
    {
        const auto& pBookmark = *m_vBookmarks.GetItemAt(nIndex);
        if (pBookmark.IsSelected() && pBookmark.GetBehavior() != BookmarkBehavior::Frozen)
            return true;
    }

    return false;
}

void MemoryBookmarksViewModel::UpdateFreezeButtonText()
{
    if (!HasSelection() || ShouldFreeze())
        SetValue(FreezeButtonTextProperty, FreezeButtonTextProperty.GetDefaultValue());
    else
        SetValue(FreezeButtonTextProperty, L"Unfreeze");
}

void MemoryBookmarksViewModel::ToggleFreezeSelected()
{
    m_vBookmarks.BeginUpdate();

    if (ShouldFreeze())
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

bool MemoryBookmarksViewModel::ShouldPause() const
{
    for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < m_vBookmarks.Count(); ++nIndex)
    {
        const auto& pBookmark = *m_vBookmarks.GetItemAt(nIndex);
        if (pBookmark.IsSelected() && pBookmark.GetBehavior() != BookmarkBehavior::PauseOnChange)
            return true;
    }

    return false;
}

void MemoryBookmarksViewModel::UpdatePauseButtonText()
{
    if (!HasSelection() || ShouldPause())
        SetValue(PauseButtonTextProperty, PauseButtonTextProperty.GetDefaultValue());
    else
        SetValue(PauseButtonTextProperty, L"Stop Pausing");
}

void MemoryBookmarksViewModel::TogglePauseSelected()
{
    m_vBookmarks.BeginUpdate();

    if (ShouldPause())
    {
        for (gsl::index nIndex = m_vBookmarks.Count() - 1; nIndex >= 0; --nIndex)
        {
            auto* pItem = m_vBookmarks.GetItemAt(nIndex);
            if (pItem && pItem->IsSelected())
                pItem->SetBehavior(BookmarkBehavior::PauseOnChange);
        }
    }
    else
    {
        for (gsl::index nIndex = m_vBookmarks.Count() - 1; nIndex >= 0; --nIndex)
        {
            auto* pItem = m_vBookmarks.GetItemAt(nIndex);
            if (pItem && pItem->IsSelected() && pItem->GetBehavior() == BookmarkBehavior::PauseOnChange)
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
