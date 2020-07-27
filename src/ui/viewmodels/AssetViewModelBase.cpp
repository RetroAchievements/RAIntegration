#include "AssetViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty AssetViewModelBase::TypeProperty("AssetViewModelBase", "Type", ra::etoi(AssetType::Achievement));
const IntModelProperty AssetViewModelBase::IDProperty("AssetViewModelBase", "ID", 0);
const StringModelProperty AssetViewModelBase::NameProperty("AssetViewModelBase", "Name", L"");
const IntModelProperty AssetViewModelBase::CategoryProperty("AssetViewModelBase", "Category", ra::etoi(AssetCategory::Core));
const IntModelProperty AssetViewModelBase::StateProperty("AssetViewModelBase", "State", ra::etoi(AssetState::Inactive));
const IntModelProperty AssetViewModelBase::ChangesProperty("AssetViewModelBase", "Changes", ra::etoi(AssetChanges::None));
const BoolModelProperty AssetViewModelBase::IsSelectedProperty("AssetViewModelBase", "IsSelected", false);

AssetViewModelBase::AssetViewModelBase() noexcept
{
    SetTransactional(NameProperty);
    SetTransactional(CategoryProperty);
}

template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
static size_t CharactersNeedingEscaped(const std::basic_string<CharT>& sText)
{
    size_t nToEscape = 0;
    for (auto c : sText)
    {
        if (c == ':' || c == '"' || c == '\\')
            ++nToEscape;
    }

    return nToEscape;
}

template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
static void WriteEscapedString(ra::services::TextWriter& pWriter, const std::basic_string<CharT>& sText, const size_t nToEscape)
{
    std::basic_string<CharT> sEscaped;
    sEscaped.reserve(sText.length() + nToEscape + 2);
    sEscaped.push_back('"');
    for (CharT c : sText)
    {
        if (c == '"' || c == '\\')
            sEscaped.push_back('\\');
        sEscaped.push_back(c);
    }
    sEscaped.push_back('"');
    pWriter.Write(sEscaped);
}

template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
static void WritePossiblyQuotedString(ra::services::TextWriter& pWriter, const std::basic_string<CharT>& sText)
{
    pWriter.Write(":");
    const size_t nToEscape = CharactersNeedingEscaped(sText);
    if (nToEscape == 0)
        pWriter.Write(sText);
    else
        WriteEscapedString(pWriter, sText, nToEscape);
}

void AssetViewModelBase::WritePossiblyQuoted(ra::services::TextWriter& pWriter, const std::string& sText)
{
    WritePossiblyQuotedString(pWriter, sText);
}

void AssetViewModelBase::WritePossiblyQuoted(ra::services::TextWriter& pWriter, const std::wstring& sText)
{
    WritePossiblyQuotedString(pWriter, sText);
}

template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
static void WriteQuotedString(ra::services::TextWriter& pWriter, const std::basic_string<CharT>& sText)
{
    pWriter.Write(":");
    const size_t nToEscape = CharactersNeedingEscaped(sText);
    if (nToEscape == 0)
    {
        pWriter.Write("\"");
        pWriter.Write(sText);
        pWriter.Write("\"");
    }
    else
    {
        WriteEscapedString(pWriter, sText, nToEscape);
    }
}

void AssetViewModelBase::WriteQuoted(ra::services::TextWriter& pWriter, const std::string& sText)
{
    WriteQuotedString(pWriter, sText);
}

void AssetViewModelBase::WriteQuoted(ra::services::TextWriter& pWriter, const std::wstring& sText)
{
    WriteQuotedString(pWriter, sText);
}

void AssetViewModelBase::WriteNumber(ra::services::TextWriter& pWriter, const uint32_t nValue)
{
    pWriter.Write(":");
    pWriter.Write(std::to_string(nValue));
}

void AssetViewModelBase::CreateServerCheckpoint()
{
    Expects(m_pTransaction == nullptr);
    BeginTransaction();

    SetValue(ChangesProperty, ra::etoi(AssetChanges::None));
}

void AssetViewModelBase::CreateLocalCheckpoint()
{
    Expects(m_pTransaction != nullptr);
    Expects(m_pTransaction->m_pNext == nullptr);
    const bool bModified = m_pTransaction->IsModified();
    BeginTransaction();

    SetValue(ChangesProperty, bModified ? ra::etoi(AssetChanges::Unpublished) : ra::etoi(AssetChanges::None));
}

void AssetViewModelBase::UpdateLocalCheckpoint()
{
    Expects(m_pTransaction != nullptr);
    Expects(m_pTransaction->m_pNext != nullptr);
    CommitTransaction();
    BeginTransaction();

    const bool bModified = m_pTransaction->m_pNext->IsModified();
    SetValue(ChangesProperty, bModified ? ra::etoi(AssetChanges::Unpublished) : ra::etoi(AssetChanges::None));
}

void AssetViewModelBase::UpdateServerCheckpoint()
{
    Expects(m_pTransaction != nullptr);
    Expects(m_pTransaction->m_pNext != nullptr);
    CommitTransaction();
    CommitTransaction();
    BeginTransaction();
    BeginTransaction();

    SetValue(ChangesProperty, ra::etoi(AssetChanges::None));
}

void AssetViewModelBase::RestoreLocalCheckpoint()
{
    Expects(m_pTransaction != nullptr);
    Expects(m_pTransaction->m_pNext != nullptr);
    RevertTransaction();
    BeginTransaction();

    const bool bModified = m_pTransaction->m_pNext->IsModified();
    SetValue(ChangesProperty, bModified ? ra::etoi(AssetChanges::Unpublished) : ra::etoi(AssetChanges::None));
}

void AssetViewModelBase::RestoreServerCheckpoint()
{
    Expects(m_pTransaction != nullptr);
    Expects(m_pTransaction->m_pNext != nullptr);
    RevertTransaction();
    RevertTransaction();
    BeginTransaction();
    BeginTransaction();

    SetValue(ChangesProperty, ra::etoi(AssetChanges::None));
}

void AssetViewModelBase::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == IsModifiedProperty)
    {
        // if either transaction doesn't exist, we're either setting up the object
        // or updating the checkpoints. the state will be updated appropriately later
        if (m_pTransaction && m_pTransaction->m_pNext)
        {
            if (args.tNewValue)
                SetValue(ChangesProperty, ra::etoi(AssetChanges::Modified));
            else if (m_pTransaction->m_pNext->IsModified())
                SetValue(ChangesProperty, ra::etoi(AssetChanges::Unpublished));
            else
                SetValue(ChangesProperty, ra::etoi(AssetChanges::None));
        }
    }
}


} // namespace viewmodels
} // namespace ui
} // namespace ra
