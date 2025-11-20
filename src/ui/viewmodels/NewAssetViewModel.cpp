#include "NewAssetViewModel.hh"

#include "util\Strings.hh"

#include "data\context\GameContext.hh"

#include "services\IClipboard.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty NewAssetViewModel::AssetTypeProperty("NewAssetViewModel", "AssetType", ra::etoi(ra::data::models::AssetType::Achievement));

NewAssetViewModel::NewAssetViewModel() noexcept
{
    SetWindowTitle(L"New Asset");
}

void NewAssetViewModel::NewAchievement()
{
    SetValue(AssetTypeProperty, ra::etoi(ra::data::models::AssetType::Achievement));
    SetDialogResult(DialogResult::OK);
}

void NewAssetViewModel::NewLeaderboard()
{
    SetValue(AssetTypeProperty, ra::etoi(ra::data::models::AssetType::Leaderboard));
    SetDialogResult(DialogResult::OK);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
