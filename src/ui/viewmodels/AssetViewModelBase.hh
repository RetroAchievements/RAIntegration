#ifndef RA_UI_ASSET_VIEW_MODEL_BASE_H
#define RA_UI_ASSET_VIEW_MODEL_BASE_H
#pragma once

#include "services\TextWriter.hh"

#include "ui\TransactionalViewModelBase.hh"

#include "RA_StringUtils.h"
#include "ra_utility.h"

namespace ra {
namespace ui {
namespace viewmodels {

enum class AssetType
{
	None = 0,
	Achievement,
	Leaderboard,
};

enum class AssetCategory
{
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
};

enum class AssetChanges
{
    None = 0,
    Modified,    // differs from local data
    Unpublished, // local data differs from server data
};

class AssetViewModelBase : public TransactionalViewModelBase
{
public:
    AssetViewModelBase() noexcept;

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

    /// <summary>
    /// Activates the asset.
    /// </summary>
    virtual void Activate() noexcept(false) { }

    /// <summary>
    /// Activates the asset.
    /// </summary>
    virtual void Deactivate() noexcept(false) { }

    /// <summary>
    /// Updates the asset for the current frame.
    /// </summary>
    virtual void DoFrame() noexcept(false) { }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the asset state.
    /// </summary>
    static const IntModelProperty ChangesProperty;

    /// <summary>
    /// Gets the asset state.
    /// </summary>
    AssetChanges GetChanges() const { return ra::itoe<AssetChanges>(GetValue(ChangesProperty)); }

    /// <summary>
    /// Determines if the local checkpoint differs from the server checkpoint. Ignores any changes
    /// made after the local checkpoint - use GetChanges() for overall modification state.
    /// </summary>
    bool HasUnpublishedChanges() const noexcept;

    virtual void Serialize(ra::services::TextWriter& pWriter) const = 0;
    virtual bool Deserialize(ra::Tokenizer& pTokenizer) = 0;

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

    static void WriteNumber(ra::services::TextWriter& pWriter, const uint32_t nValue);
    static void WriteQuoted(ra::services::TextWriter& pWriter, const std::string& sText);
    static void WriteQuoted(ra::services::TextWriter& pWriter, const std::wstring& sText);
    static void WritePossiblyQuoted(ra::services::TextWriter& pWriter, const std::string& sText);
    static void WritePossiblyQuoted(ra::services::TextWriter& pWriter, const std::wstring& sText);

    static bool ReadNumber(ra::Tokenizer& pTokenizer, uint32_t& nValue);
    static bool ReadQuoted(ra::Tokenizer& pTokenizer, std::string& sText);
    static bool ReadQuoted(ra::Tokenizer& pTokenizer, std::wstring& sText);
    static bool ReadPossiblyQuoted(ra::Tokenizer& pTokenizer, std::string& sText);
    static bool ReadPossiblyQuoted(ra::Tokenizer& pTokenizer, std::wstring& sText);

    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;

    // Hide the public members of TransactionalViewModelBase that we're redefining
    using TransactionalViewModelBase::BeginTransaction;
    using TransactionalViewModelBase::CommitTransaction;
    using TransactionalViewModelBase::RevertTransaction;

    void CommitTransaction() override;
    void RevertTransaction() override;

    static bool IsActive(AssetState nState) noexcept;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif RA_UI_ASSET_VIEW_MODEL_BASE_H
