#include "UnknownGameViewModel.hh"

#include "RA_StringUtils.h"

#include "api\FetchGamesList.hh"
#include "api\SubmitNewTitle.hh"

#include "data\ConsoleContext.hh"

#include "services\IClipboard.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty UnknownGameViewModel::SelectedGameIdProperty("UnknownGameViewModel", "SelectedGameId", 0);
const StringModelProperty UnknownGameViewModel::NewGameNameProperty("UnknownGameViewModel", "NewGameName", L"");
const StringModelProperty UnknownGameViewModel::ChecksumProperty("UnknownGameViewModel", "Checksum", L"");
const StringModelProperty UnknownGameViewModel::EstimatedGameNameProperty("UnknownGameViewModel", "EstimatedGameName", L"");
const StringModelProperty UnknownGameViewModel::SystemNameProperty("UnknownGameViewModel", "SystemName", L"");
const BoolModelProperty UnknownGameViewModel::TestModeProperty("UnknownGameViewModel", "TestMode", false);

UnknownGameViewModel::UnknownGameViewModel() noexcept
{
    SetWindowTitle(L"Unknown Title");
}

void UnknownGameViewModel::InitializeGameTitles()
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::ConsoleContext>();

    m_vGameTitles.Add(0U, L"<New Title>");

    ra::api::FetchGamesList::Request request;
    request.ConsoleId = pConsoleContext.Id();

    request.CallAsync([this, pAsyncHandle = CreateAsyncHandle()](const ra::api::FetchGamesList::Response& response)
    {
        ra::data::AsyncKeepAlive pKeepAlive(*pAsyncHandle);
        if (pAsyncHandle->IsDestroyed())
            return;

        if (response.Failed())
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(*this, L"Could not retrieve list of existing games",
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
    ra::api::SubmitNewTitle::Request request;

    const auto nGameId = GetSelectedGameId();
    if (nGameId == 0)
    {
        request.GameName = GetNewGameName();
        ra::Trim(request.GameName);

        if (request.GameName.length() < 3)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(*this, L"New game name must be at least three characters long.");
            return false;
        }

        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(ra::StringPrintf(L"Are you sure you want to create a new entry for '%s'?", request.GameName));
        vmMessageBox.SetMessage(L"If you were unable to find an existing title, please check to make sure that it's not listed under \"~unlicensed~\", \"~hack~\", \"~prototype~\", or had a leading article removed.");
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
        if (vmMessageBox.ShowModal(*this) == DialogResult::No)
            return false;
    }
    else
    {
        request.GameId = nGameId;
        request.GameName = m_vGameTitles.GetLabelForId(nGameId);

        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(ra::StringPrintf(L"Are you sure you want to add a new checksum to '%s'?", request.GameName));
        vmMessageBox.SetMessage(L"You should not do this unless you are certain that the new title is compatible. You can use 'Test' mode to check compatibility.");
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
        if (vmMessageBox.ShowModal(*this) == DialogResult::No)
            return false;
    }

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::ConsoleContext>();
    request.ConsoleId = pConsoleContext.Id();
    request.Hash = ra::Narrow(GetChecksum());
    request.Description = GetEstimatedGameName();

    auto response = request.Call();
    if (response.Succeeded())
    {
        SetSelectedGameId(ra::to_signed(response.GameId));
        return true;
    }

    ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(*this, L"Could not add new title",
                                                              ra::Widen(response.ErrorMessage));
    return false;
}

bool UnknownGameViewModel::BeginTest()
{
    const auto nGameId = GetSelectedGameId();
    if (nGameId == 0U)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(*this, L"You must select an existing game to test compatibility.");
        return false;
    }

    ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
    vmMessageBox.SetHeader(ra::StringPrintf(L"Play '%s' in compatability test mode?", m_vGameTitles.GetLabelForId(nGameId)));
    vmMessageBox.SetMessage(L"Achievements and leaderboards for the game will be loaded, but you will not be able to earn or modify them or modify code notes for the game.");
    vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
    if (vmMessageBox.ShowModal(*this) == DialogResult::No)
        return false;

    SetTestMode(true);
    return true;
}

void UnknownGameViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == NewGameNameProperty && !m_bSelectingGame)
    {
        // user is entering a custom name, make sure <New Game> is selected
        SetSelectedGameId(0);
    }

    WindowViewModelBase::OnValueChanged(args);
}

void UnknownGameViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == SelectedGameIdProperty && args.tNewValue != 0)
    {
        // copy the selected game name into the new game name field
        // disable the notifications so we don't reset the selection to <New Game>
        m_bSelectingGame = true;
        SetNewGameName(m_vGameTitles.GetLabelForId(args.tNewValue));
        m_bSelectingGame = false;
    }

    WindowViewModelBase::OnValueChanged(args);
}

void UnknownGameViewModel::CopyChecksumToClipboard() const
{
    const auto& pClipboard = ra::services::ServiceLocator::Get<ra::services::IClipboard>();
    pClipboard.SetText(GetChecksum());
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
