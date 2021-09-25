#include "AssetModelBase.hh"

namespace ra {
namespace data {
namespace models {

const IntModelProperty AssetModelBase::TypeProperty("AssetModelBase", "Type", ra::etoi(AssetType::Achievement));
const IntModelProperty AssetModelBase::IDProperty("AssetModelBase", "ID", 0);
const StringModelProperty AssetModelBase::NameProperty("AssetModelBase", "Name", L"");
const StringModelProperty AssetModelBase::DescriptionProperty("AssetModelBase", "Description", L"");
const IntModelProperty AssetModelBase::CategoryProperty("AssetModelBase", "Category", ra::etoi(AssetCategory::Core));
const StringModelProperty AssetModelBase::AuthorProperty("AssetModelBase", "Author", L"");
const IntModelProperty AssetModelBase::StateProperty("AssetModelBase", "State", ra::etoi(AssetState::Inactive));
const IntModelProperty AssetModelBase::ChangesProperty("AssetModelBase", "Changes", ra::etoi(AssetChanges::None));
const IntModelProperty AssetModelBase::CreationTimeProperty("AssetModelBase", "CreationTime", 0);
const IntModelProperty AssetModelBase::UpdatedTimeProperty("AssetModelBase", "UpdatedTime", 0);
const StringModelProperty AssetModelBase::ValidationErrorProperty("AssetModelBase", "ValidationError", L"");

AssetModelBase::AssetModelBase() noexcept
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

void AssetModelBase::WritePossiblyQuoted(ra::services::TextWriter& pWriter, const std::string& sText)
{
    WritePossiblyQuotedString(pWriter, sText);
}

void AssetModelBase::WritePossiblyQuoted(ra::services::TextWriter& pWriter, const std::wstring& sText)
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

void AssetModelBase::WriteQuoted(ra::services::TextWriter& pWriter, const std::string& sText)
{
    WriteQuotedString(pWriter, sText);
}

void AssetModelBase::WriteQuoted(ra::services::TextWriter& pWriter, const std::wstring& sText)
{
    WriteQuotedString(pWriter, sText);
}

void AssetModelBase::WriteNumber(ra::services::TextWriter& pWriter, const uint32_t nValue)
{
    pWriter.Write(":");
    pWriter.Write(std::to_string(nValue));
}

bool AssetModelBase::ReadNumber(ra::Tokenizer& pTokenizer, uint32_t& nValue)
{
    if (pTokenizer.EndOfString())
        return false;

    nValue = pTokenizer.ReadNumber();
    return pTokenizer.Consume(':') || pTokenizer.EndOfString();
}

bool AssetModelBase::ReadQuoted(ra::Tokenizer& pTokenizer, std::string& sText)
{
    if (pTokenizer.EndOfString())
        return false;

    sText = pTokenizer.ReadQuotedString();
    return pTokenizer.Consume(':') || pTokenizer.EndOfString();
}

bool AssetModelBase::ReadQuoted(ra::Tokenizer& pTokenizer, std::wstring& sText)
{
    if (pTokenizer.EndOfString())
        return false;

    sText = ra::Widen(pTokenizer.ReadQuotedString());
    return pTokenizer.Consume(':') || pTokenizer.EndOfString();
}

bool AssetModelBase::ReadPossiblyQuoted(ra::Tokenizer& pTokenizer, std::string& sText)
{
    if (pTokenizer.EndOfString())
        return false;

    if (pTokenizer.PeekChar() == '"')
        return ReadQuoted(pTokenizer, sText);

    sText = pTokenizer.ReadTo(':');
    pTokenizer.Consume(':');
    return true;
}

bool AssetModelBase::ReadPossiblyQuoted(ra::Tokenizer& pTokenizer, std::wstring& sText)
{
    if (pTokenizer.EndOfString())
        return false;

    if (pTokenizer.PeekChar() == '"')
        return ReadQuoted(pTokenizer, sText);

    sText = ra::Widen(pTokenizer.ReadTo(':'));
    pTokenizer.Consume(':');
    return true;
}

void AssetModelBase::CreateServerCheckpoint()
{
    Expects(m_pTransaction == nullptr);
    BeginTransaction();

    SetValue(ChangesProperty, ra::etoi(AssetChanges::None));
}

void AssetModelBase::CreateLocalCheckpoint()
{
    Expects(m_pTransaction != nullptr);
    Expects(m_pTransaction->m_pNext == nullptr);
    const bool bModified = m_pTransaction->IsModified();
    BeginTransaction();       // start transaction for in-memory changes

    SetValue(ChangesProperty, bModified ? ra::etoi(AssetChanges::Unpublished) : ra::etoi(AssetChanges::None));
    Validate();
}

void AssetModelBase::UpdateLocalCheckpoint()
{
    Expects(m_pTransaction != nullptr);
    Expects(m_pTransaction->m_pNext != nullptr);
    CommitTransaction();      // commit in-memory changes
    CreateLocalCheckpoint();  // start transaction for in-memory changes
}

void AssetModelBase::UpdateServerCheckpoint()
{
    Expects(m_pTransaction != nullptr);
    Expects(m_pTransaction->m_pNext != nullptr);
    CommitTransaction();     // commit in-memory changes
    CommitTransaction();     // commit unpublished changes
    BeginTransaction();      // start transaction for unpublished changes
    BeginTransaction();      // start transaction for in-memory changes

    SetValue(ChangesProperty, ra::etoi(AssetChanges::None));
    Validate();
}

void AssetModelBase::RestoreLocalCheckpoint()
{
    Expects(m_pTransaction != nullptr);
    Expects(m_pTransaction->m_pNext != nullptr);
    RevertTransaction();      // discard in-memory changes
    CreateLocalCheckpoint();  // start transaction for in-memory changes
}

void AssetModelBase::RestoreServerCheckpoint()
{
    Expects(m_pTransaction != nullptr);
    Expects(m_pTransaction->m_pNext != nullptr);
    BeginUpdate();
    RevertTransaction();     // discard in-memory changes
    RevertTransaction();     // discard unpublished changes
    BeginTransaction();      // start transaction for unpublished changes
    BeginTransaction();      // start transaction for in-memory changes

    SetValue(ChangesProperty, ra::etoi(AssetChanges::None));
    Validate();
    EndUpdate();
}

void AssetModelBase::ResetLocalCheckpoint(ra::Tokenizer& pTokenizer)
{
    // this function is a conglomeration of RestoreServerCheckpoint and UpdateLocalCheckpoint that calls
    // Deserialize to populate the local checkpoint instead of making local changes and commiting them.
    Expects(m_pTransaction != nullptr);
    Expects(m_pTransaction->m_pNext != nullptr);

    BeginUpdate();           // disable notifications while rebuilding
    RevertTransaction();     // discard in-memory changes
    RevertTransaction();     // discard unpublished changes
    BeginTransaction();      // start transaction for unpublished changes
    Deserialize(pTokenizer); // load unpublished changes
    CreateLocalCheckpoint(); // start transaction for in-memory changes
    EndUpdate();             // re-enable notifications
}

void AssetModelBase::SetNew()
{
    SetValue(ChangesProperty, ra::etoi(AssetChanges::New));
}

void AssetModelBase::SetDeleted()
{
    SetValue(ChangesProperty, ra::etoi(AssetChanges::Deleted));
    SetValue(ValidationErrorProperty, ValidationErrorProperty.GetDefaultValue());
}

bool AssetModelBase::HasUnpublishedChanges() const noexcept
{
    return (m_pTransaction && m_pTransaction->m_pNext && m_pTransaction->m_pNext->IsModified());
}

bool AssetModelBase::IsUpdating() const
{
    if (DataModelBase::IsUpdating())
        return true;

    // IsUpdating is used to prevent processing change notifications while things are in flux.
    // leverage that to also prevent processing change notifications while initializing the asset.
    return (!m_pTransaction || !m_pTransaction->m_pNext);
}

void AssetModelBase::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    DataModelBase::OnValueChanged(args);
}

void AssetModelBase::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    DataModelBase::OnValueChanged(args);
}

void AssetModelBase::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    // if IsUpdating is true, we're either setting up the object or updating the checkpoints.
    // the Changes state will be updated appropriately later.
    if (!IsUpdating())
    {
        if (args.Property == IsModifiedProperty)
        {
            if (args.tNewValue)
            {
                const auto nChanges = GetChanges();
                if (nChanges != AssetChanges::Modified && nChanges != AssetChanges::New)
                    SetValue(ChangesProperty, ra::etoi(AssetChanges::Modified));
            }
            else if (m_pTransaction->m_pNext->IsModified())
            {
                SetValue(ChangesProperty, ra::etoi(AssetChanges::Unpublished));
            }
            else
            {
                SetValue(ChangesProperty, ra::etoi(AssetChanges::None));
            }
        }
    }

    DataModelBase::OnValueChanged(args);
}

bool AssetModelBase::GetLocalValue(const BoolModelProperty& pProperty) const
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

const std::wstring& AssetModelBase::GetLocalValue(const StringModelProperty& pProperty) const
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

int AssetModelBase::GetLocalValue(const IntModelProperty& pProperty) const
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

AssetChanges AssetModelBase::GetAssetDefinitionState(const AssetDefinition& pAsset) const
{
    return ra::itoe<AssetChanges>(GetValue(*pAsset.m_pProperty) & 0x03);
}

void AssetModelBase::UpdateAssetDefinitionVersion(const AssetDefinition& pAsset, AssetChanges nState)
{
    const auto& sDefinition = GetAssetDefinition(pAsset, nState);

    // DJB2 hash the definition
    int nHash = 5381;
    for (const auto c : sDefinition)
        nHash = ((nHash << 5) + nHash) + c; /* hash * 33 + c */

    // inject the state into the two lowermost bits - we want the state in the checkpoints so it gets restored
    // when reverted. we can reconstruct the other properties from the state (see RevertTransaction).
    nHash <<= 2; // make sure to shift, as DJB2 is sensitive to the last character of the string.
    nHash |= ra::etoi(nState);

    SetValue(*pAsset.m_pProperty, nHash);
}

const std::string& AssetModelBase::GetAssetDefinition(const AssetDefinition& pAsset) const
{
    return GetAssetDefinition(pAsset, GetAssetDefinitionState(pAsset));
}

const std::string& AssetModelBase::GetAssetDefinition(const AssetDefinition& pAsset, AssetChanges nState) noexcept
{
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

const std::string& AssetModelBase::GetLocalAssetDefinition(const AssetDefinition& pAsset) const noexcept
{
    if (pAsset.m_bLocalModified)
        return pAsset.m_sLocalDefinition;

    return pAsset.m_sCoreDefinition;
}

void AssetModelBase::SetAssetDefinition(AssetDefinition& pAsset, const std::string& sValue)
{
    if (m_pTransaction == nullptr)
    {
        // before core checkpoint
        if (pAsset.m_sCoreDefinition != sValue)
        {
            pAsset.m_sCoreDefinition = sValue;
            UpdateAssetDefinitionVersion(pAsset, AssetChanges::None);
        }
    }
    else if (m_pTransaction->m_pNext == nullptr)
    {
        // before local checkpoint
        if (sValue == pAsset.m_sCoreDefinition)
        {
            if (!pAsset.m_sLocalDefinition.empty())
            {
                pAsset.m_sLocalDefinition.clear();
                pAsset.m_bLocalModified = false;
                UpdateAssetDefinitionVersion(pAsset, AssetChanges::None);
            }
        }
        else if (sValue != pAsset.m_sLocalDefinition)
        {
            pAsset.m_sLocalDefinition = sValue;
            pAsset.m_bLocalModified = true;
            UpdateAssetDefinitionVersion(pAsset, AssetChanges::Unpublished);
        }
    }
    else
    {
        // after local checkpoint
        const auto nState = GetAssetDefinitionState(pAsset);

        if (pAsset.m_bLocalModified && sValue == pAsset.m_sLocalDefinition)
        {
            // value being set to unpublished value
            if (nState != AssetChanges::Unpublished)
            {
                pAsset.m_sCurrentDefinition.clear();
                UpdateAssetDefinitionVersion(pAsset, AssetChanges::Unpublished);
            }
        }
        else if (!pAsset.m_bLocalModified && sValue == pAsset.m_sCoreDefinition)
        {
            // value being set to core value (and no unpublished value exists)
            if (nState != AssetChanges::None)
            {
                pAsset.m_sCurrentDefinition.clear();
                UpdateAssetDefinitionVersion(pAsset, AssetChanges::None);
            }
        }
        else if (pAsset.m_sCurrentDefinition != sValue)
        {
            // value being changed
            pAsset.m_sCurrentDefinition = sValue;
            UpdateAssetDefinitionVersion(pAsset, AssetChanges::Modified);
        }
        else if (sValue.empty() && !GetAssetDefinition(pAsset, nState).empty())
        {
            // value being changed to empty string
            UpdateAssetDefinitionVersion(pAsset, AssetChanges::Modified);
        }
    }
}

void AssetModelBase::CommitTransaction()
{
    Expects(m_pTransaction != nullptr);

    if (m_pTransaction->m_pNext == nullptr)
    {
        // commit local to core
        for (auto pAsset : m_vAssetDefinitions)
        {
            Expects(pAsset != nullptr);

            if (GetAssetDefinitionState(*pAsset) == AssetChanges::Unpublished)
            {
                if (pAsset->m_bLocalModified)
                {
                    pAsset->m_sCoreDefinition.swap(pAsset->m_sLocalDefinition);
                    pAsset->m_sLocalDefinition.clear();
                    pAsset->m_bLocalModified = false;
                    UpdateAssetDefinitionVersion(*pAsset, AssetChanges::None);
                }
            }
        }
    }
    else
    {
        // commit modifications to local
        if (GetChanges() == AssetChanges::New)
            SetValue(ChangesProperty, ra::etoi(AssetChanges::Modified));

        for (auto pAsset : m_vAssetDefinitions)
        {
            Expects(pAsset != nullptr);

            if (GetAssetDefinitionState(*pAsset) == AssetChanges::Modified)
            {
                if (pAsset->m_sCurrentDefinition == pAsset->m_sCoreDefinition)
                {
                    pAsset->m_sCurrentDefinition.clear();
                    pAsset->m_sLocalDefinition.clear();
                    pAsset->m_bLocalModified = false;
                    UpdateAssetDefinitionVersion(*pAsset, AssetChanges::None);
                }
                else if (pAsset->m_sLocalDefinition != pAsset->m_sCurrentDefinition)
                {
                    pAsset->m_sLocalDefinition.swap(pAsset->m_sCurrentDefinition);
                    pAsset->m_sCurrentDefinition.clear();
                    pAsset->m_bLocalModified = true;
                    UpdateAssetDefinitionVersion(*pAsset, AssetChanges::Unpublished);
                }
            }
        }
    }

    // call after updating so TrackingProperties are also committed
    DataModelBase::CommitTransaction();
}

void AssetModelBase::RevertTransaction()
{
    // call before updating so TrackingProperties are reverted first
    DataModelBase::RevertTransaction();

    for (auto pAsset : m_vAssetDefinitions)
    {
        Expects(pAsset != nullptr);

        const auto nState = GetAssetDefinitionState(*pAsset);
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

bool AssetModelBase::IsActive(AssetState nState) noexcept
{
    switch (nState)
    {
        case AssetState::Inactive:
        case AssetState::Triggered:
        case AssetState::Disabled:
            return false;
        default:
            return true;
    }
}

const char* AssetModelBase::GetAssetTypeString(AssetType nType) noexcept
{
    switch (nType)
    {
        case AssetType::None: return "None";
        case AssetType::Achievement: return "Achievement";
        case AssetType::Leaderboard: return "Leaderboard";
        case AssetType::LocalBadges: return "LocalBadges";
        default: return "Unknown";
    }
}

bool AssetModelBase::Validate()
{
    std::wstring sError;
    const bool bResult = ValidateAsset(sError);

    SetValue(ValidationErrorProperty, sError);
    return bResult;
}

} // namespace models
} // namespace data
} // namespace ra
