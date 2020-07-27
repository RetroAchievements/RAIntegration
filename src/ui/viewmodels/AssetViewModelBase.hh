#ifndef RA_UI_ASSET_VIEW_MODEL_BASE_H
#define RA_UI_ASSET_VIEW_MODEL_BASE_H
#pragma once

#include "services\TextWriter.hh"

#include "ui\TransactionalViewModelBase.hh"

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

class AssetViewModelBase : protected TransactionalViewModelBase
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
    /// The <see cref="ModelProperty" /> for the asset state.
    /// </summary>
    static const IntModelProperty ChangesProperty;

    /// <summary>
    /// Gets the asset state.
    /// </summary>
    AssetChanges GetChanges() const { return ra::itoe<AssetChanges>(GetValue(ChangesProperty)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the whether the bookmark is selected.
    /// </summary>
    static const BoolModelProperty IsSelectedProperty;

    /// <summary>
    /// Gets whether the bookmark is selected.
    /// </summary>
    bool IsSelected() const { return GetValue(IsSelectedProperty); }

    /// <summary>
    /// Sets whether the bookmark is selected.
    /// </summary>
    void SetSelected(bool bValue) { SetValue(IsSelectedProperty, bValue); }

    virtual void Serialize(ra::services::TextWriter& pWriter) const = 0;

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
    static void WriteNumber(ra::services::TextWriter& pWriter, const uint32_t nValue);
    static void WriteQuoted(ra::services::TextWriter& pWriter, const std::string& sText);
    static void WriteQuoted(ra::services::TextWriter& pWriter, const std::wstring& sText);
    static void WritePossiblyQuoted(ra::services::TextWriter& pWriter, const std::string& sText);
    static void WritePossiblyQuoted(ra::services::TextWriter& pWriter, const std::wstring& sText);

    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif RA_UI_ASSET_VIEW_MODEL_BASE_H
