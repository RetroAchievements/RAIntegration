#include "BrokenAchievementsViewModel.hh"

#include "RA_StringUtils.h"

#include "api\SubmitTicket.hh"

#include "data\GameContext.hh"

#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty BrokenAchievementsViewModel::SelectedProblemIdProperty("BrokenAchievementsViewModel", "SelectedProblemId", -1);
const StringModelProperty BrokenAchievementsViewModel::CommentProperty("BrokenAchievementsViewModel", "Comment", L"");

const BoolModelProperty BrokenAchievementsViewModel::BrokenAchievementViewModel::IsSelectedProperty("BrokenAchievementViewModel", "IsSelected", false);
const BoolModelProperty BrokenAchievementsViewModel::BrokenAchievementViewModel::IsAchievedProperty("BrokenAchievementViewModel", "IsAchieved", false);

bool BrokenAchievementsViewModel::InitializeAchievements()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    if (pGameContext.GameId() == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"You must load a game before you can report broken achievements.");
        return false;
    }

    if (pGameContext.ActiveAchievementType() == AchievementSet::Type::Local)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"You cannot report broken local achievements.");
        return false;
    }

    if (pGameContext.GetMode() == ra::data::GameContext::Mode::CompatibilityTest)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"You cannot report broken achievements in compatibility test mode.");
        return false;
    }

    const auto nActiveAchievementType = ra::etoi(pGameContext.ActiveAchievementType());
    pGameContext.EnumerateAchievements([this, nActiveAchievementType](const Achievement& pAchievement)
    {
        if (pAchievement.Category() == nActiveAchievementType)
        {
            auto& vmAchievement = m_vAchievements.Add();
            vmAchievement.SetId(pAchievement.ID());
            vmAchievement.SetLabel(ra::Widen(pAchievement.Title()));
            vmAchievement.SetAchieved(!pAchievement.Active());
        }

        return true;
    });

    if (m_vAchievements.Count() == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"There are no active achievements to report.");
        return false;
    }

    return true;
}

bool BrokenAchievementsViewModel::Submit()
{
    if (GetSelectedProblemId() == -1)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Please select a problem type.");
        return false;
    }

    ra::api::SubmitTicket::Request request;
    std::string sBuggedIDs;

    for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vAchievements.Count()); ++nIndex)
    {
        const auto* pAchievement = m_vAchievements.GetItemAt(nIndex);
        if (pAchievement && pAchievement->IsSelected())
        {
            request.AchievementIds.insert(pAchievement->GetId());
            sBuggedIDs.append(std::to_string(pAchievement->GetId()));
            sBuggedIDs.push_back(',');
        }
    }

    if (request.AchievementIds.empty())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"You must select at least one achievement to submit a ticket.");
        return false;
    }

    sBuggedIDs.pop_back(); // remove last comma

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    const char* sProblemType = "";
    request.Problem = ra::itoe<ra::api::SubmitTicket::ProblemType>(GetSelectedProblemId());
    switch (request.Problem)
    {
        case ra::api::SubmitTicket::ProblemType::DidNotTrigger:
            sProblemType = " did not trigger";
            break;

        case ra::api::SubmitTicket::ProblemType::WrongTime:
            if (GetComment().length() < 5)
            {
                ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(ra::StringPrintf(
                    L"Please describe when the achievement%s did trigger.", request.AchievementIds.size() == 1 ? "" : "s"));
                return false;
            }

            sProblemType = " triggered at the wrong time";
            break;
    }

    ra::ui::viewmodels::MessageBoxViewModel vmConfirm;
    if (request.AchievementIds.size() == 1)
    {
        const auto nIndex = m_vAchievements.FindItemIndex(LookupItemViewModel::IdProperty, *request.AchievementIds.begin());
        const auto* pAchievement = m_vAchievements.GetItemAt(nIndex);

        vmConfirm.SetHeader(ra::StringPrintf(L"Are you sure that you want to create a%s ticket for %s?",
            sProblemType, pAchievement ? pAchievement->GetLabel() : pGameContext.GameTitle()));
    }
    else
    {
        vmConfirm.SetHeader(ra::StringPrintf(L"Are you sure that you want to create %u%s tickets for %s?",
            request.AchievementIds.size(), sProblemType, pGameContext.GameTitle()));
    }

    vmConfirm.SetMessage(ra::StringPrintf(L""
        "Achievement ID%s: %s\n"
        "Game Checksum: %s\n"
        "Comment: %s",
        request.AchievementIds.size() == 1 ? "" : "s",
        sBuggedIDs, pGameContext.GameHash(), GetComment()
    ));
    vmConfirm.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

    if (vmConfirm.ShowModal() != ra::ui::DialogResult::Yes)
        return false;

    request.GameHash = pGameContext.GameHash();
    request.Comment = ra::Narrow(GetComment());

    const auto response = request.Call();
    if (!response.Succeeded())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to submit tickets",
            !response.ErrorMessage.empty() ? ra::Widen(response.ErrorMessage) : L"Unknown error");
        return false;
    }

    if (response.TicketsCreated == 1)
    {
        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(L"1 ticket created");
        vmMessageBox.SetMessage(L"Thank you for reporting the issue. The development team will investigate and you will be notified when the ticket is updated or resolved.");
        vmMessageBox.ShowModal();
    }
    else
    {
        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(ra::StringPrintf(L"%u tickets created", response.TicketsCreated));
        vmMessageBox.SetMessage(L"Thank you for reporting the issues. The development team will investigate and you will be notified when the tickets are updated or resolved.");
        vmMessageBox.ShowModal();
    }

    return true;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
