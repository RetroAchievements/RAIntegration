#include "AssetViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty AssetViewModelBase::TypeProperty("AssetViewModelBase", "Type", ra::etoi(AssetType::Achievement));
const IntModelProperty AssetViewModelBase::IDProperty("AssetViewModelBase", "ID", 0);
const StringModelProperty AssetViewModelBase::NameProperty("AssetViewModelBase", "Name", L"");
const StringModelProperty AssetViewModelBase::DescriptionProperty("AssetViewModelBase", "Description", L"");
const IntModelProperty AssetViewModelBase::CategoryProperty("AssetViewModelBase", "Category", ra::etoi(AssetCategory::Core));
const IntModelProperty AssetViewModelBase::StateProperty("AssetViewModelBase", "State", ra::etoi(AssetState::Inactive));
const IntModelProperty AssetViewModelBase::ChangesProperty("AssetViewModelBase", "Changes", ra::etoi(AssetChanges::None));

AssetViewModelBase::AssetViewModelBase() noexcept
{
    SetTransactional(NameProperty);
    SetTransactional(DescriptionProperty);
    SetTransactional(CategoryProperty);
}

template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
static size_t CharactersNeedingEscaped(const std::basic_string<CharT>& sText) noexcept
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

bool AssetViewModelBase::ReadNumber(ra::Tokenizer& pTokenizer, uint32_t& nValue)
{
    if (pTokenizer.EndOfString())
        return false;

    nValue = pTokenizer.ReadNumber();
    return pTokenizer.Consume(':') || pTokenizer.EndOfString();
}

bool AssetViewModelBase::ReadQuoted(ra::Tokenizer& pTokenizer, std::string& sText)
{
    if (pTokenizer.EndOfString())
        return false;

    sText = pTokenizer.ReadQuotedString();
    return pTokenizer.Consume(':') || pTokenizer.EndOfString();
}

bool AssetViewModelBase::ReadQuoted(ra::Tokenizer& pTokenizer, std::wstring& sText)
{
    if (pTokenizer.EndOfString())
        return false;

    sText = ra::Widen(pTokenizer.ReadQuotedString());
    return pTokenizer.Consume(':') || pTokenizer.EndOfString();
}

bool AssetViewModelBase::ReadPossiblyQuoted(ra::Tokenizer& pTokenizer, std::string& sText)
{
    if (pTokenizer.EndOfString())
        return false;

    if (pTokenizer.PeekChar() == '"')
        return ReadQuoted(pTokenizer, sText);

    sText = pTokenizer.ReadTo(':');
    pTokenizer.Consume(':');
    return true;
}

bool AssetViewModelBase::ReadPossiblyQuoted(ra::Tokenizer& pTokenizer, std::wstring& sText)
{
    if (pTokenizer.EndOfString())
        return false;

    if (pTokenizer.PeekChar() == '"')
        return ReadQuoted(pTokenizer, sText);

    sText = ra::Widen(pTokenizer.ReadTo(':'));
    pTokenizer.Consume(':');
    return true;
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

bool AssetViewModelBase::HasUnpublishedChanges() const noexcept
{
    return (m_pTransaction && m_pTransaction->m_pNext && m_pTransaction->m_pNext->IsModified());
}

void AssetViewModelBase::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    TransactionalViewModelBase::OnValueChanged(args);
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

    TransactionalViewModelBase::OnValueChanged(args);
}

bool AssetViewModelBase::GetLocalValue(const BoolModelProperty& pProperty) const
{
    // make sure we have a local checkpoint
    if (m_pTransaction != nullptr && m_pTransaction->m_pNext != nullptr)
    {
        // then return the previous value if it has been modified
        const auto* pValue = m_pTransaction->GetPreviousValue(pProperty);
        if (pValue)
            return *pValue;
    }

    // return the current value
    return GetValue(pProperty);
}

const std::wstring& AssetViewModelBase::GetLocalValue(const StringModelProperty& pProperty) const
{
    // make sure we have a local checkpoint
    if (m_pTransaction != nullptr && m_pTransaction->m_pNext != nullptr)
    {
        // then return the previous value if it has been modified
        const auto* pValue = m_pTransaction->GetPreviousValue(pProperty);
        if (pValue)
            return *pValue;
    }

    // return the current value
    return GetValue(pProperty);
}

int AssetViewModelBase::GetLocalValue(const IntModelProperty& pProperty) const
{
    // make sure we have a local checkpoint
    if (m_pTransaction != nullptr && m_pTransaction->m_pNext != nullptr)
    {
        // then return the previous value if it has been modified
        const auto* pValue = m_pTransaction->GetPreviousValue(pProperty);
        if (pValue)
            return *pValue;
    }

    // return the current value
    return GetValue(pProperty);
}

const std::string& AssetViewModelBase::GetAssetDefinition(const AssetDefinition& pAsset) const
{
    const auto nState = ra::itoe<AssetChanges>(GetValue(*pAsset.m_pProperty));
    switch (nState)
    {
        case AssetChanges::None:
            return pAsset.m_sCoreDefinition;

        case AssetChanges::Unpublished:
            return pAsset.m_sLocalDefinition;

        default:
            return pAsset.m_sCurrentDefinition;
    }
}

const std::string& AssetViewModelBase::GetLocalAssetDefinition(const AssetDefinition& pAsset) const noexcept
{
    if (pAsset.m_bLocalModified)
        return pAsset.m_sLocalDefinition;

    return pAsset.m_sCoreDefinition;
}

void AssetViewModelBase::SetAssetDefinition(AssetDefinition& pAsset, const std::string& sValue)
{
    if (m_pTransaction == nullptr)
    {
        // before core checkpoint
        pAsset.m_sCoreDefinition = sValue;
    }
    else if (m_pTransaction->m_pNext == nullptr)
    {
        // before local checkpoint
        if (sValue == pAsset.m_sCoreDefinition)
        {
            pAsset.m_sLocalDefinition.clear();
            SetValue(*pAsset.m_pProperty, ra::etoi(AssetChanges::None));
            pAsset.m_bLocalModified = false;
        }
        else
        {
            pAsset.m_sLocalDefinition = sValue;
            SetValue(*pAsset.m_pProperty, ra::etoi(AssetChanges::Unpublished));
            pAsset.m_bLocalModified = true;
        }
    }
    else
    {
        // after local checkpoint
        if (pAsset.m_bLocalModified && sValue == pAsset.m_sLocalDefinition)
        {
            pAsset.m_sCurrentDefinition.clear();
            SetValue(*pAsset.m_pProperty, ra::etoi(AssetChanges::Unpublished));
        }
        else if (!pAsset.m_bLocalModified && sValue == pAsset.m_sCoreDefinition)
        {
            pAsset.m_sCurrentDefinition.clear();
            SetValue(*pAsset.m_pProperty, ra::etoi(AssetChanges::None));
        }
        else
        {
            pAsset.m_sCurrentDefinition = sValue;
            SetValue(*pAsset.m_pProperty, ra::etoi(AssetChanges::Modified));
        }
    }
}

void AssetViewModelBase::CommitTransaction()
{
    Expects(m_pTransaction != nullptr);

    if (m_pTransaction->m_pNext == nullptr)
    {
        // commit local to core
        for (auto pAsset : m_vAssetDefinitions)
        {
            Expects(pAsset != nullptr);

            const auto nState = ra::itoe<AssetChanges>(GetValue(*pAsset->m_pProperty));
            if (nState == AssetChanges::Unpublished)
            {
                if (pAsset->m_bLocalModified)
                {
                    pAsset->m_sCoreDefinition.swap(pAsset->m_sLocalDefinition);
                    pAsset->m_sLocalDefinition.clear();
                    pAsset->m_bLocalModified = false;
                }

                SetValue(*pAsset->m_pProperty, ra::etoi(AssetChanges::None));
            }
        }
    }
    else
    {
        // commit modifications to local
        for (auto pAsset : m_vAssetDefinitions)
        {
            Expects(pAsset != nullptr);

            const auto nState = ra::itoe<AssetChanges>(GetValue(*pAsset->m_pProperty));
            if (nState == AssetChanges::Modified)
            {
                if (pAsset->m_sCurrentDefinition == pAsset->m_sCoreDefinition)
                {
                    pAsset->m_sCurrentDefinition.clear();
                    pAsset->m_sLocalDefinition.clear();
                    pAsset->m_bLocalModified = false;
                    SetValue(*pAsset->m_pProperty, ra::etoi(AssetChanges::None));
                }
                else
                {
                    pAsset->m_sLocalDefinition.swap(pAsset->m_sCurrentDefinition);
                    pAsset->m_sCurrentDefinition.clear();
                    pAsset->m_bLocalModified = true;
                    SetValue(*pAsset->m_pProperty, ra::etoi(AssetChanges::Unpublished));
                }
            }
        }
    }

    // call after updating so TrackingProperties are also committed
    TransactionalViewModelBase::CommitTransaction();
}

void AssetViewModelBase::RevertTransaction()
{
    // call before updating so TrackingProperties are reverted first
    TransactionalViewModelBase::RevertTransaction();

    for (auto pAsset : m_vAssetDefinitions)
    {
        Expects(pAsset != nullptr);

        const auto nState = ra::itoe<AssetChanges>(GetValue(*pAsset->m_pProperty));
        switch (nState)
        {
            case AssetChanges::None:
                pAsset->m_sCurrentDefinition.clear();
                pAsset->m_sLocalDefinition.clear();
                pAsset->m_bLocalModified = false;
                break;

            case AssetChanges::Unpublished:
                pAsset->m_sCurrentDefinition.clear();
                pAsset->m_bLocalModified = (pAsset->m_sLocalDefinition != pAsset->m_sCoreDefinition);
                break;

            default:
                Expects(!"Unexpected state after revert");
                break;
        }
    }
}

bool AssetViewModelBase::IsActive(AssetState nState) noexcept
{
    switch (nState)
    {
        case AssetState::Inactive:
        case AssetState::Triggered:
            return false;
        default:
            return true;
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
