#include "BrokenAchievementsViewModel.hh"

#include "util\Strings.hh"

#include "api\SubmitTicket.hh"

#include "data\context\GameContext.hh"
#include "data\context\GameAssets.hh"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\IDesktop.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty BrokenAchievementsViewModel::SelectedIndexProperty("BrokenAchievementsViewModel", "SelectedIndex", -1);

const StringModelProperty BrokenAchievementsViewModel::BrokenAchievementViewModel::DescriptionProperty("BrokenAchievementViewModel", "Description", L"");
const BoolModelProperty BrokenAchievementsViewModel::BrokenAchievementViewModel::IsAchievedProperty("BrokenAchievementViewModel", "IsAchieved", false);

bool BrokenAchievementsViewModel::InitializeAchievements()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (pGameContext.GameId() == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"You must load a game before you can report achievement problems.");
        return false;
    }

    if (pGameContext.GetMode() == ra::data::context::GameContext::Mode::CompatibilityTest)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"You cannot report achievement problems in compatibility test mode.");
        return false;
    }

    const auto& pWindowManager = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>();
    const auto& pFilteredAssets = pWindowManager.AssetList.FilteredAssets();
    bool bSawLocal = false;
    for (const auto& pAssetSummary : pFilteredAssets)
    {
        if (pAssetSummary.GetType() == ra::data::models::AssetType::Achievement)
        {
            if (pAssetSummary.GetCategory() == ra::data::models::AssetCategory::Local)
            {
                bSawLocal = true;
            }
            else
            {
                const auto* pAchievement = pGameContext.Assets().FindAchievement(pAssetSummary.GetId());
                if (pAchievement)
                {
                    auto& vmAchievement = m_vAchievements.Add();
                    vmAchievement.SetId(pAchievement->GetID());
                    vmAchievement.SetLabel(pAchievement->GetName());
                    vmAchievement.SetDescription(pAchievement->GetDescription());
                    vmAchievement.SetAchieved(!pAchievement->IsActive());
                }
            }
        }
    }

    if (m_vAchievements.Count() == 0)
    {
        if (bSawLocal)
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"You cannot report local achievement problems.");
        else
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"There are no active achievements to report.");
        return false;
    }

    SetWindowTitle(L"Report Achievement Problem");

    m_vAchievements.AddNotifyTarget(*this);

    return true;
}

bool BrokenAchievementsViewModel::Submit()
{
    const auto nSelectedIndex = GetSelectedIndex();
    if (nSelectedIndex == -1)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Please select an achievement.");
        return false;
    }

    std::string sExtra = "{";

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto nAchievementId = m_vAchievements.GetItemAt(nSelectedIndex)->GetId();
    const auto* pAchievement = pGameContext.Assets().FindAchievement(nAchievementId);
    if (pAchievement)
    {
        const auto& sRichPresence = ra::util::String::Narrow(pAchievement->GetUnlockRichPresence());
        if (!sRichPresence.empty())
        {
            sExtra.append("\"triggerRichPresence\":\"");

            for (auto c : sRichPresence)
            {
                if (c == '"')
                    sExtra.push_back('\\');
                sExtra.push_back(c);
            }

            sExtra.push_back('"');
        }
    }

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    std::string sUrl = ra::util::String::Printf("%s/achievement/%u/report-issue", pConfiguration.GetHostUrl(), nAchievementId);

    if (sExtra.length() > 2)
    {
        sExtra.push_back('}');

        sUrl.append("?extra=");
        sUrl.append(ra::util::String::EncodeBase64(sExtra));
    }

    ra::services::ServiceLocator::Get<ra::ui::IDesktop>().OpenUrl(sUrl);

    return true;
}

void BrokenAchievementsViewModel::OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == BrokenAchievementViewModel::IsSelectedProperty)
        OnItemSelectedChanged(nIndex, args);
}

void BrokenAchievementsViewModel::OnItemSelectedChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
    if (args.tNewValue)
    {
        const auto nPreviousSelectedIndex = GetSelectedIndex();
        if (nPreviousSelectedIndex != -1)
            m_vAchievements.GetItemAt(nPreviousSelectedIndex)->SetSelected(false);

        SetSelectedIndex(gsl::narrow_cast<int>(nIndex));
    }
    else
    {
        SetSelectedIndex(-1);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
