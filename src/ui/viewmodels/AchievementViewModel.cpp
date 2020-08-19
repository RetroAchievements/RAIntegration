#include "AchievementViewModel.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty AchievementViewModel::PointsProperty("AchievementViewModel", "Points", 5);
const StringModelProperty AchievementViewModel::BadgeProperty("AchievementViewModel", "Badge", L"00000");
const BoolModelProperty AchievementViewModel::PauseOnResetProperty("AchievementViewModel", "PauseOnReset", false);
const BoolModelProperty AchievementViewModel::PauseOnTriggerProperty("AchievementViewModel", "PauseOnTrigger", false);
const IntModelProperty AchievementViewModel::TriggerProperty("AchievementViewModel", "Trigger", ra::etoi(AssetChanges::None));

AchievementViewModel::AchievementViewModel() noexcept
{
    SetTransactional(PointsProperty);
    SetTransactional(BadgeProperty);

    AddAssetDefinition(m_pTrigger, TriggerProperty);
}

void AchievementViewModel::SelectBadgeFile()
{
    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Not implemented");
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
