#ifndef RA_DATA_MODELS_ASSETMODELBASE_H
#define RA_DATA_MODELS_ASSETMODELBASE_H
#pragma once

#include "data/DataModelBase.hh"

#include "services/TextWriter.hh"

#include "util/Strings.hh"
#include "util/Tokenizer.hh"

namespace ra {
namespace data {
namespace models {

enum class AssetType
{
    None = 0,
    Achievement,
    Leaderboard,
    RichPresence,
    LocalBadges,
    CodeNotes,
    MemoryRegions,
};

enum class AssetCategory
{
    None = -1,
    Local = 0,
    Core = 3,
    Unofficial = 5,
};

enum class AssetState
{
    Inactive = 0,
    Active,
    Triggered,
    Waiting,
    Paused,
    Disabled,
    Primed,
};

enum class AssetChanges
{
    None = 0,
    Modified,    // differs from local data
    Unpublished, // local data differs from server data
    New,         // asset not in local or server data
    Deleted,     // asset to be removed from local data
};

class AssetModelBase : public DataModelBase
{
public:
    AssetModelBase() noexcept;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the asset type.
    /// </summary>
    static const IntModelProperty TypeProperty;

    /// <summary>
    /// Gets the asset type.
    /// </summary>
    AssetType GetType() const { return ra::itoe<AssetType>(GetValue(TypeProperty)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the asset id.
    /// </summary>
    static const IntModelProperty IDProperty;

    /// <summary>
    /// Gets the asset id.
    /// </summary>
    uint32_t GetID() const { return ra::to_unsigned(GetValue(IDProperty)); }

    /// <summary>
    /// Sets the asset id.
    /// </summary>
    void SetID(uint32_t nValue) { SetValue(IDProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the name.
    /// </summary>
    static const StringModelProperty NameProperty;

    /// <summary>
    /// Gets the name to display.
    /// </summary>
    const std::wstring& GetName() const { return GetValue(NameProperty); }

    /// <summary>
    /// Sets the name to display.
    /// </summary>
    void SetName(const std::wstring& sValue) { SetValue(NameProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the description of the asset.
    /// </summary>
    static const StringModelProperty DescriptionProperty;

    /// <summary>
    /// Gets the description of the asset.
    /// </summary>
    const std::wstring& GetDescription() const { return GetValue(DescriptionProperty); }

    /// <summary>
    /// Sets the description of the asset.
    /// </summary>
    void SetDescription(const std::wstring& sValue) { SetValue(DescriptionProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the asset category.
    /// </summary>
    static const IntModelProperty CategoryProperty;

    /// <summary>
    /// Gets the asset category.
    /// </summary>
    AssetCategory GetCategory() const { return ra::itoe<AssetCategory>(GetValue(CategoryProperty)); }

    /// <summary>
    /// Sets the asset category.
    /// </summary>
    void SetCategory(AssetCategory nValue) { SetValue(CategoryProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the owning set id.
    /// </summary>
    static const IntModelProperty SubsetIDProperty;

    /// <summary>
    /// Gets the asset id.
    /// </summary>
    uint32_t GetSubsetID() const { return ra::to_unsigned(GetValue(SubsetIDProperty)); }

    /// <summary>
    /// Sets the asset id.
    /// </summary>
    void SetSubsetID(uint32_t nValue) { SetValue(SubsetIDProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the badge.
    /// </summary>
    static const StringModelProperty AuthorProperty;

    /// <summary>
    /// Gets the name of the user who created the achievement.
    /// </summary>
    const std::wstring& GetAuthor() const { return GetValue(AuthorProperty); }

    /// <summary>
    /// Sets the name of the user who created the achievement.
    /// </summary>
    void SetAuthor(const std::wstring& sValue) { SetValue(AuthorProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for when the asset was created.
    /// </summary>
    static const IntModelProperty CreationTimeProperty;

    /// <summary>
    /// Gets when the asset was created.
    /// </summary>
    time_t GetCreationTime() const { return gsl::narrow_cast<time_t>(GetValue(CreationTimeProperty)); }

    /// <summary>
    /// Sets when the asset was created.
    /// </summary>
    void SetCreationTime(time_t nValue) { SetValue(CreationTimeProperty, gsl::narrow_cast<int>(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for when the asset was last updated.
    /// </summary>
    static const IntModelProperty UpdatedTimeProperty;

    /// <summary>
    /// Gets when the asset was last updated.
    /// </summary>
    time_t GetUpdatedTime() const { return gsl::narrow_cast<time_t>(GetValue(UpdatedTimeProperty)); }

    /// <summary>
    /// Sets when the asset was last updated.
    /// </summary>
    void SetUpdatedTime(time_t nValue) { SetValue(UpdatedTimeProperty, gsl::narrow_cast<int>(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the asset state.
    /// </summary>
    static const IntModelProperty StateProperty;

    /// <summary>
    /// Gets the asset state.
    /// </summary>
    AssetState GetState() const { return ra::itoe<AssetState>(GetValue(StateProperty)); }

    /// <summary>
    /// Sets the asset state.
    /// </summary>
    void SetState(AssetState nValue) { SetValue(StateProperty, ra::etoi(nValue)); }

    /// <summary>
    /// Gets whether or not the asset is in an active state.
    /// </summary>
    bool IsActive() const { return IsActive(GetState()); }
    static bool IsActive(AssetState nState) noexcept;

    /// <summary>
    /// Activates the asset.
    /// </summary>
    virtual void Activate() noexcept(false) {}

    /// <summary>
    /// Activates the asset.
    /// </summary>
    virtual void Deactivate() noexcept(false) {}

    /// <summary>
    /// The <see cref="ModelProperty" /> for the reason why the asset is not valid.
    /// </summary>
    static const StringModelProperty ValidationErrorProperty;

    /// <summary>
    /// Gets the reason why the asset is not valid.
    /// </summary>
    const std::wstring& GetValidationError() const { return GetValue(ValidationErrorProperty); }

    /// <summary>
    /// Updates the <see cref="ValidationErrorProperty" />.
    /// </summary>
    /// <returns>
    /// <c>true</c> if the asset is valid (GetValidationError will return empty string).
    /// <c>false</c> if the asset is not valid (GetValidationError will return more information about why).
    /// </returns>
    bool Validate();

    /// <summary>
    /// Updates the asset for the current frame.
    /// </summary>
    virtual void DoFrame() noexcept(false) {}

    /// <summary>
    /// The <see cref="ModelProperty" /> for the asset state.
    /// </summary>
    static const IntModelProperty ChangesProperty;

    /// <summary>
    /// Gets the asset state.
    /// </summary>
    AssetChanges GetChanges() const { return ra::itoe<AssetChanges>(GetValue(ChangesProperty)); }

    /// <summary>
    /// Marks the asset as not existing in local or on the server.
    /// </summary>
    void SetNew();

    /// <summary>
    /// Marks the asset as to be removed from local.
    /// </summary>
    void SetDeleted();

    /// <summary>
    /// Gets whether the asset should not be displayed in the asset list.
    /// </summary>
    /// <returns><c>true</c> to display the asset in the asset list, <c>false</c> otherwise.</returns>
    virtual bool IsShownInList() const noexcept(false) { return true; }

    /// <summary>
    /// Determines if the local checkpoint differs from the server checkpoint. Ignores any changes
    /// made after the local checkpoint - use GetChanges() for overall modification state.
    /// </summary>
    bool HasUnpublishedChanges() const noexcept;

    virtual void Serialize(ra::services::TextWriter& pWriter) const = 0;
    virtual bool Deserialize(ra::util::Tokenizer& pTokenizer) = 0;

    void DeserializeLocalCheckpoint(ra::util::Tokenizer& pTokenizer);

    /// <summary>
    /// Captures the current state of the asset as a reflection of the state on the server.
    /// </summary>
    void CreateServerCheckpoint();

    /// <summary>
    /// Captures the current state of the asset as a reflection of the state on disk.
    /// </summary>
    void CreateLocalCheckpoint();

    /// <summary>
    /// Updates the local checkpoint to match the current state of the asset.
    /// </summary>
    void UpdateLocalCheckpoint();

    /// <summary>
    /// Updates the server and local checkpoints to match the current state of the asset.
    /// </summary>
    void UpdateServerCheckpoint();

    /// <summary>
    /// Discards any changes made since the local checkpoint.
    /// </summary>
    void RestoreLocalCheckpoint();

    /// <summary>
    /// Discards any changes made since the server checkpoint.
    /// </summary>
    void RestoreServerCheckpoint();

    /// <summary>
    /// Discards any changes made since the server checkpoint and repopulates the local checkpoint using Deserialize.
    /// </summary>
    void ResetLocalCheckpoint(ra::util::Tokenizer& pTokenizer);

    /// <summary>
    /// Gets a human-readable string for the AssetType enum values.
    /// </summary>
    static const char* GetAssetTypeString(AssetType nType) noexcept;

protected:
    /// <summary>
    /// Helper class for versioned ASCII strings to avoid overhead of UNICODE characters
    /// </summary>
    struct AssetDefinition
    {
        const IntModelProperty* m_pProperty = nullptr;
        std::string m_sCoreDefinition;
        std::string m_sLocalDefinition;
        std::string m_sCurrentDefinition;
        bool m_bLocalModified = false;
    };

    std::vector<AssetDefinition*> m_vAssetDefinitions;

    void AddAssetDefinition(AssetDefinition& pAsset, const IntModelProperty& pProperty)
    {
        pAsset.m_pProperty = &pProperty;
        m_vAssetDefinitions.push_back(&pAsset);
        SetTransactional(pProperty);
    }

    const std::string& GetAssetDefinition(const AssetDefinition& pAsset) const;
    void SetAssetDefinition(AssetDefinition& pAsset, const std::string& sValue);
    const std::string& GetLocalAssetDefinition(const AssetDefinition& pAsset) const noexcept;

    bool GetLocalValue(const BoolModelProperty& pProperty) const;
    const std::wstring& GetLocalValue(const StringModelProperty& pProperty) const;
    int GetLocalValue(const IntModelProperty& pProperty) const;

    virtual bool ValidateAsset(std::wstring& sErrorMessage) noexcept(false) { (void)sErrorMessage; return true; }

    static void WriteNumber(ra::services::TextWriter& pWriter, const uint32_t nValue);
    static void WriteQuoted(ra::services::TextWriter& pWriter, const std::string& sText);
    static void WriteQuoted(ra::services::TextWriter& pWriter, const std::wstring& sText);
    static void WritePossiblyQuoted(ra::services::TextWriter& pWriter, const std::string& sText);
    static void WritePossiblyQuoted(ra::services::TextWriter& pWriter, const std::wstring& sText);

    static bool ReadNumber(ra::util::Tokenizer& pTokenizer, uint32_t& nValue);
    static bool ReadQuoted(ra::util::Tokenizer& pTokenizer, std::string& sText);
    static bool ReadQuoted(ra::util::Tokenizer& pTokenizer, std::wstring& sText);
    static bool ReadPossiblyQuoted(ra::util::Tokenizer& pTokenizer, std::string& sText);
    static bool ReadPossiblyQuoted(ra::util::Tokenizer& pTokenizer, std::wstring& sText);

    bool IsUpdating() const override;

    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;

    // Hide the public members of DataModelBase that we're redefining
    using DataModelBase::BeginTransaction;
    using DataModelBase::CommitTransaction;
    using DataModelBase::RevertTransaction;

    void CommitTransaction() override;
    void RevertTransaction() override;

private:
    static const std::string& GetAssetDefinition(const AssetDefinition& pAsset, AssetChanges nState) noexcept;
    AssetChanges GetAssetDefinitionState(const AssetDefinition& pAsset) const;
    void UpdateAssetDefinitionVersion(const AssetDefinition&, AssetChanges nState);
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_MODELS_ASSETMODELBASE_H
