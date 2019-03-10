#include "UnknownGameViewModel.hh"

#include "RA_StringUtils.h"

#include "api\FetchGamesList.hh"
#include "api\SubmitNewTitle.hh"

#include "data\ConsoleContext.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty UnknownGameViewModel::SelectedGameIdProperty("UnknownGameViewModel", "SelectedGameId", 0);
const StringModelProperty UnknownGameViewModel::NewGameNameProperty("UnknownGameViewModel", "NewGameName", L"");
const StringModelProperty UnknownGameViewModel::ChecksumProperty("UnknownGameViewModel", "Checksum", L"");

UnknownGameViewModel::UnknownGameViewModel() noexcept
{
    SetWindowTitle(L"Unknown Title");

    AddNotifyTarget(*this); // so we can synchronize NewGameName and SelectedGameId in one direction only
}

void UnknownGameViewModel::InitializeGameTitles()
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::ConsoleContext>();

    m_vGameTitles.Add(0U, L"<New Title>");

    ra::api::FetchGamesList::Request request;
    request.ConsoleId = pConsoleContext.Id();

    request.CallAsync([this](const ra::api::FetchGamesList::Response& response)
    {
        if (response.Failed())
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Could not retrieve list of existing games",
                                                                      ra::Widen(response.ErrorMessage));
        }
        else
        {
            for (const auto& pGame : response.Games)
                m_vGameTitles.Add(pGame.Id, pGame.Name);
        }

        m_vGameTitles.Freeze();
    });
}

bool UnknownGameViewModel::Associate()
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::ConsoleContext>();

    ra::api::SubmitNewTitle::Request request;
    request.ConsoleId = pConsoleContext.Id();
    request.Hash = ra::Narrow(GetChecksum());

    const auto nGameId = GetSelectedGameId();
    if (nGameId == 0)
        request.GameName = GetNewGameName();
    else
        request.GameName = m_vGameTitles.GetLabelForId(nGameId);

    auto response = request.Call();
    if (response.Succeeded())
    {
        SetSelectedGameId(ra::to_signed(response.GameId));
        return true;
    }

    ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Could not add new title",
                                                              ra::Widen(response.ErrorMessage));
    return false;
}

void UnknownGameViewModel::OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == NewGameNameProperty)
    {
        // user is entering a custom name, make sure <New Game> is selected
        SetSelectedGameId(0);
    }
}

void UnknownGameViewModel::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == SelectedGameIdProperty && args.tNewValue != 0)
    {
        // copy the selected game name into the new game name field
        // disable the notifications so we don't reset the selection to <New Game>
        RemoveNotifyTarget(*this);
        SetNewGameName(m_vGameTitles.GetLabelForId(args.tNewValue));
        AddNotifyTarget(*this);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
