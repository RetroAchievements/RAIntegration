#ifndef RA_UI_NEWASSETVIEWMODEL_H
#define RA_UI_NEWASSETVIEWMODEL_H
#pragma once

#include "data/models/AssetModelBase.hh"

#include "ui/WindowViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class NewAssetViewModel : public WindowViewModelBase
{
public:
    GSL_SUPPRESS_F6 NewAssetViewModel() noexcept;
    ~NewAssetViewModel() = default;

    NewAssetViewModel(const NewAssetViewModel&) noexcept = delete;
    NewAssetViewModel& operator=(const NewAssetViewModel&) noexcept = delete;
    NewAssetViewModel(NewAssetViewModel&&) noexcept = delete;
    NewAssetViewModel& operator=(NewAssetViewModel&&) noexcept = delete;

    /// <summary>
    /// Selects New Achievement and closes the dialog.
    /// </summary>
    void NewAchievement();

    /// <summary>
    /// Selects New Leaderboard and closes the dialog.
    /// </summary>
    void NewLeaderboard();

    /// <summary>
    /// The <see cref="ModelProperty" /> for the selected asset type.
    /// </summary>
    static const IntModelProperty AssetTypeProperty;

    /// <summary>
    /// Gets the asset category.
    /// </summary>
    ra::data::models::AssetType GetSelectedType() const { return ra::itoe<ra::data::models::AssetType>(GetValue(AssetTypeProperty)); }
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_NEWASSETVIEWMODEL_H
