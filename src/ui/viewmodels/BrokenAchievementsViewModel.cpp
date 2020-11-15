#include "BrokenAchievementsViewModel.hh"

#include "RA_StringUtils.h"

#ifndef RA_UTEST
#include "RA_Dlg_Achievement.h"
#endif

#include "api\SubmitTicket.hh"

#include "data\context\GameContext.hh"

#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty BrokenAchievementsViewModel::SelectedProblemIdProperty("BrokenAchievementsViewModel", "SelectedProblemId", 0);
const StringModelProperty BrokenAchievementsViewModel::CommentProperty("BrokenAchievementsViewModel", "Comment", L"");

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

    bool bSawLocal = false;
    pGameContext.EnumerateFilteredAchievements([&bSawLocal, this](const Achievement& pAchievement)
    {
        if (pAchievement.GetCategory() == Achievement::Category::Local)
        {
            bSawLocal = true;
        }
        else
        {
            auto& vmAchievement = m_vAchievements.Add();
            vmAchievement.SetId(pAchievement.ID());
            vmAchievement.SetLabel(ra::Widen(pAchievement.Title()));
            vmAchievement.SetDescription(ra::Widen(pAchievement.Description()));
            vmAchievement.SetAchieved(!pAchievement.Active());
        }

        return true;
    });

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

static std::wstring GetRichPresence(const ra::data::context::GameContext& pGameContext, const std::set<unsigned int> vAchievementIds)
{
    bool bMultipleRichPresence = false;
    std::wstring sRichPresence;
    for (const auto nId : vAchievementIds)
    {
        const auto* pAchievement = pGameContext.FindAchievement(nId);
        if (pAchievement == nullptr)
            continue;

        if (!pAchievement->GetUnlockRichPresence().empty())
        {
            if (sRichPresence.empty())
                sRichPresence = pAchievement->GetUnlockRichPresence();
            else if (sRichPresence != pAchievement->GetUnlockRichPresence())
                bMultipleRichPresence = true;
        }
    }

    if (bMultipleRichPresence)
    {
        sRichPresence.clear();

        for (const auto nId : vAchievementIds)
        {
            const auto* pAchievement = pGameContext.FindAchievement(nId);
            if (pAchievement != nullptr && !pAchievement->GetUnlockRichPresence().empty())
                sRichPresence.append(ra::StringPrintf(L"%u: %s\n", nId, pAchievement->GetUnlockRichPresence()));
        }

        sRichPresence.pop_back(); // remove last newline
    }

    return sRichPresence;
}

bool BrokenAchievementsViewModel::Submit()
{
    if (GetSelectedProblemId() == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Please select a problem type.");
        return false;
    }

    ra::api::SubmitTicket::Request request;
    std::string sBuggedIDs;
    size_t nAchievedSelected = 0U;
    size_t nUnachievedSelected = 0U;

    for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vAchievements.Count()); ++nIndex)
    {
        const auto* pAchievement = m_vAchievements.GetItemAt(nIndex);
        if (pAchievement && pAchievement->IsSelected())
        {
            request.AchievementIds.insert(pAchievement->GetId());
            sBuggedIDs.append(std::to_string(pAchievement->GetId()));
            sBuggedIDs.push_back(',');

            if (pAchievement->IsAchieved())
                ++nAchievedSelected;
            else
                ++nUnachievedSelected;
        }
    }

    if (request.AchievementIds.empty())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"You must select at least one achievement to submit a ticket.");
        return false;
    }

    sBuggedIDs.pop_back(); // remove last comma

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

    const char* sProblemType = "";
    request.Problem = ra::itoe<ra::api::SubmitTicket::ProblemType>(GetSelectedProblemId());
    switch (request.Problem)
    {
        case ra::api::SubmitTicket::ProblemType::DidNotTrigger:
            if (nUnachievedSelected == 0)
            {
                if (ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Submit anyway?",
                    ra::StringPrintf(L"The achievement%s you have selected %s triggered, but you have selected 'Did not trigger'.",
                        request.AchievementIds.size() == 1 ? "" : "s", request.AchievementIds.size() == 1 ? "has" : "have"),
                    ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo) == ra::ui::DialogResult::No)
                {
                    return false;
                }
            }

            sProblemType = " did not trigger";
            break;

        case ra::api::SubmitTicket::ProblemType::WrongTime:
            if (nAchievedSelected == 0)
            {
                if (ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Submit anyway?",
                    ra::StringPrintf(L"The achievement%s you have selected %s not triggered, but you have selected 'Triggered at the wrong time'.",
                        request.AchievementIds.size() == 1 ? "" : "s", request.AchievementIds.size() == 1 ? "has" : "have"),
                    ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo) == ra::ui::DialogResult::No)
                {
                    return false;
                }
            }

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
        "RetroAchievements Hash: %s\n"
        "Comment: %s",
        request.AchievementIds.size() == 1 ? "" : "s",
        sBuggedIDs, pGameContext.GameHash(), GetComment()
    ));
    vmConfirm.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

    if (vmConfirm.ShowModal() != ra::ui::DialogResult::Yes)
        return false;

    request.GameHash = pGameContext.GameHash();
    request.Comment = ra::Narrow(GetComment());

    const std::wstring sRichPresence = GetRichPresence(pGameContext, request.AchievementIds);
    if (!sRichPresence.empty())
    {
        request.Comment.append("\n\nRich Presence at time of trigger:\n");
        request.Comment.append(ra::Narrow(sRichPresence));
    }

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

void BrokenAchievementsViewModel::OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == BrokenAchievementViewModel::IsSelectedProperty)
        OnItemSelectedChanged(nIndex, args);
}

void BrokenAchievementsViewModel::OnItemSelectedChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
    if (args.tNewValue)
    {
        // if something is checked and no problem type has been selected, automatically select it
        if (GetSelectedProblemId() == 0)
        {
            const auto* pItem = m_vAchievements.GetItemAt(nIndex);
            if (pItem != nullptr)
            {
                SetSelectedProblemId(pItem->IsAchieved() ?
                    ra::etoi(ra::api::SubmitTicket::ProblemType::WrongTime) :
                    ra::etoi(ra::api::SubmitTicket::ProblemType::DidNotTrigger));
            }
        }
    }
    else
    {
        // if all items are unchecked, reset the problem type to unknown
        if (GetSelectedProblemId() != 0)
        {
            bool bHasCheck = false;
            for (gsl::index i = 0; i < ra::to_signed(m_vAchievements.Count()); ++i)
            {
                const auto* pItem = m_vAchievements.GetItemAt(i);
                if (pItem != nullptr && pItem->IsSelected())
                {
                    bHasCheck = true;
                    break;
                }
            }

            if (!bHasCheck)
                SetSelectedProblemId(0);
        }
    }
}


} // namespace viewmodels
} // namespace ui
} // namespace ra
