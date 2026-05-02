#include "UnknownGameViewModel.hh"

#include "RA_Defs.h"

#include "util\Strings.hh"

#include "context\IConsoleContext.hh"
#include "context\IRcClient.hh"

#include "data\context\GameContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\AchievementRuntimeExports.hh"
#include "services\IClipboard.hh"
#include "services\ILocalStorage.hh"
#include "services\ILoginService.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include <rcheevos\include\rc_api_editor.h>
#include <rcheevos\include\rc_api_info.h>
#include <rcheevos\src\rc_client_external.h>
#include <rcheevos\src\rc_client_internal.h>

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty UnknownGameViewModel::SelectedGameIdProperty("UnknownGameViewModel", "SelectedGameId", 0);
const BoolModelProperty UnknownGameViewModel::IsSelectedGameEnabledProperty("UnknownGameViewModel", "IsSelectedGameEnabled", true);
const BoolModelProperty UnknownGameViewModel::IsAssociateEnabledProperty("UnknownGameViewModel", "IsAssociateEnabled", true);
const StringModelProperty UnknownGameViewModel::NewGameNameProperty("UnknownGameViewModel", "NewGameName", L"");
const StringModelProperty UnknownGameViewModel::ProblemHeaderProperty("UnknownGameViewModel", "ProblemHeader", L"The provided game could not be identified.");
const StringModelProperty UnknownGameViewModel::ChecksumProperty("UnknownGameViewModel", "Checksum", L"");
const StringModelProperty UnknownGameViewModel::EstimatedGameNameProperty("UnknownGameViewModel", "EstimatedGameName", L"");
const StringModelProperty UnknownGameViewModel::SystemNameProperty("UnknownGameViewModel", "SystemName", L"");
const StringModelProperty UnknownGameViewModel::FilterTextProperty("UnknownGameViewModel", "FilterText", L"");
const StringModelProperty UnknownGameViewModel::FilterResultsProperty("UnknownGameViewModel", "FilterResults", L"1/1");
const BoolModelProperty UnknownGameViewModel::TestModeProperty("UnknownGameViewModel", "TestMode", false);

UnknownGameViewModel::UnknownGameViewModel() noexcept
{
    SetWindowTitle(L"Unknown Title");
}

void UnknownGameViewModel::InitializeGameTitles()
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    InitializeGameTitles(pConsoleContext.Id());
}

void UnknownGameViewModel::InitializeGameTitles(ConsoleID consoleId)
{
    m_vGameTitles.Add(0U, L"<New Title>");

    if (!ra::services::ServiceLocator::Get<ra::services::ILoginService>().IsLoggedIn())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(*this, L"Could not retrieve list of existing games",
                                                                  L"User is not logged in");
        return;
    }

    SetValue(IsSelectedGameEnabledProperty, false);
    SetValue(IsAssociateEnabledProperty, false);

    const auto& pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>();

    rc_api_fetch_games_list_request_t request;
    memset(&request, 0, sizeof(request));
    request.console_id = ra::etoi(consoleId);

    rc_api_request_t api_request;
    const auto nResult = rc_api_init_fetch_games_list_request_hosted(&api_request, &request, pClient.GetHost());
    Expects(nResult == RC_OK);

    pClient.DispatchRequest(api_request, [this, pAsyncHandle = CreateAsyncHandle()](const rc_api_server_response_t& api_response, void*) {
        ra::data::AsyncKeepAlive pKeepAlive(*pAsyncHandle);
        if (pAsyncHandle->IsDestroyed())
            return;

        rc_api_fetch_games_list_response_t response;
        const auto nResult = rc_api_process_fetch_games_list_server_response(&response, &api_response);
        if (nResult != RC_OK)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(*this, L"Could not retrieve list of existing games",
                ra::context::IRcClient::GetErrorMessage(nResult, response.response));
        }
        else
        {
            m_vGameTitles.BeginUpdate();
            for (uint32_t i = 0; i < response.num_entries; ++i)
            {
                const auto nId = response.entries[i].id;
                const auto sTitle = ra::util::String::Widen(response.entries[i].name);
                m_vAllGameTitles.push_back(std::make_pair(nId, sTitle));
                m_vGameTitles.Add(nId, sTitle);
            }
            m_vGameTitles.EndUpdate();
        }

        SetValue(FilterResultsProperty, ra::util::String::Printf(L"%u/%u",
            gsl::narrow_cast<uint32_t>(m_vAllGameTitles.size()),
            gsl::narrow_cast<uint32_t>(m_vAllGameTitles.size())));

        SetValue(IsAssociateEnabledProperty, true);
        SetValue(IsSelectedGameEnabledProperty, true);

        const auto nSelectedGameId = GetSelectedGameId();
        if (nSelectedGameId == 0)
            CheckForPreviousAssociation();

    }, nullptr);
}

void UnknownGameViewModel::InitializeTestCompatibilityMode()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    m_vGameTitles.Add(pGameContext.GameId(), pGameContext.GameTitle());
    m_vGameTitles.Freeze();
    SetSelectedGameId(pGameContext.GameId());
    SetChecksum(ra::util::String::Widen(pGameContext.GameHash()));

    SetValue(IsSelectedGameEnabledProperty, false);
}

void UnknownGameViewModel::CheckForPreviousAssociation()
{
    if (m_vGameTitles.Count() == 0)
        return;

    const auto& sHash = GetChecksum();
    if (sHash.empty())
        return;

    const auto nId = GetPreviousAssociation(sHash);
    if (nId != 0)
    {
        const auto& sGameName = m_vGameTitles.GetLabelForId(nId);
        if (!sGameName.empty())
            SetSelectedGameId(nId);
    }
}

unsigned int UnknownGameViewModel::GetPreviousAssociation(const std::wstring& sHash)
{
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto sMapping = pLocalStorage.ReadText(ra::services::StorageItemType::HashMapping, sHash);

    std::string sLine;
    if (sMapping && sMapping->GetLine(sLine))
    {
        const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
        const unsigned nId = DecodeID(sLine, sHash, pConsoleContext.Id());
        if (nId > 0)
            return nId;
    }

    return 0;
}

static constexpr unsigned GenerateMask(const std::wstring& sHash, ConsoleID nConsoleId)
{
    // use longs to multiply to something more than 32-bits, then truncated and invert several bits
    return gsl::narrow_cast<unsigned>(
        ra::to_unsigned(gsl::narrow_cast<long>(sHash.back())) *  // 7 bits
        ra::to_unsigned(gsl::narrow_cast<long>(sHash.front())) * // 7 bits
        (ra::etoi(nConsoleId) + 1) *                             // 7 bits
        49999                                                    // 16 bits
        ) ^ ~0x52a761cf;
}

unsigned UnknownGameViewModel::DecodeID(const std::string& sLine, const std::wstring& sHash, ConsoleID nConsoleId)
{
    std::wstring sError;
    unsigned nEncodedId;

    if (ra::ParseHex(ra::util::String::Widen(sLine), 0xFFFFFFFF, nEncodedId, sError))
        return nEncodedId ^ GenerateMask(sHash, nConsoleId);

    return 0;
}

std::string UnknownGameViewModel::EncodeID(unsigned nId, const std::wstring& sHash, ConsoleID nConsoleId)
{
    const auto nEncodedId = nId ^ GenerateMask(sHash, nConsoleId);
    return ra::util::String::Printf("%08x", nEncodedId);
}

static void AddClientHash(const std::string& sHash, uint32_t nGameId, bool isUnknown)
{
    auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
    rc_client_add_game_hash(pClient, sHash.c_str(), nGameId);
    auto* game_hash = rc_client_find_game_hash(pClient, sHash.c_str());
    game_hash->is_unknown = isUnknown ? 1 : 0;
}

void UnknownGameViewModel::Associate()
{
    rc_api_add_game_hash_request_t request;
    memset(&request, 0, sizeof(request));

    std::wstring sGameName;

    const auto nGameId = GetSelectedGameId();
    if (nGameId == 0)
    {
        sGameName = GetNewGameName();
        ra::util::String::Trim(sGameName);

        if (sGameName.length() < 3)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(*this, L"New game name must be at least three characters long.");
            return;
        }

        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(ra::util::String::Printf(L"Are you sure you want to create a new entry for '%s'?", sGameName));
        vmMessageBox.SetMessage(L"If you were unable to find an existing title, please check to make sure that it's not listed under \"~unlicensed~\", \"~hack~\", \"~prototype~\", or had a leading article removed.");
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
        if (vmMessageBox.ShowModal(*this) == DialogResult::No)
            return;
    }
    else
    {
        request.game_id = nGameId;
        sGameName = m_vGameTitles.GetLabelForId(nGameId);

        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(ra::util::String::Printf(L"Are you sure you want to add a new hash to '%s'?", sGameName));
        vmMessageBox.SetMessage(L"You should not do this unless you are certain that the new title is compatible. You can use 'Test' mode to check compatibility.");
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
        if (vmMessageBox.ShowModal(*this) == DialogResult::No)
            return;
    }

    const auto& pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>();
    pClient.AddAuthentication(&request.username, &request.api_token);

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    request.console_id = ra::etoi(pConsoleContext.Id());

    const std::string sNarrowGameName = ra::util::String::Narrow(sGameName);
    request.title = sNarrowGameName.c_str();

    const std::string sNarrowDescription = ra::util::String::Narrow(GetEstimatedGameName());
    if (!sNarrowDescription.empty())
        request.hash_description = sNarrowDescription.c_str();

    const std::string sNarrowHash = ra::util::String::Narrow(GetChecksum());
    request.hash = sNarrowHash.c_str();

    rc_api_request_t api_request;
    const auto nResult = rc_api_init_add_game_hash_request_hosted(&api_request, &request, pClient.GetHost());
    Expects(nResult == RC_OK);

    SetValue(IsAssociateEnabledProperty, false);

    pClient.DispatchRequest(api_request, [this, sNarrowHash, pAsyncHandle = CreateAsyncHandle()](const rc_api_server_response_t& api_response, void*) {
        ra::data::AsyncKeepAlive pKeepAlive(*pAsyncHandle);
        if (pAsyncHandle->IsDestroyed())
            return;

        rc_api_add_game_hash_response_t response;
        const auto nResult = rc_api_process_add_game_hash_server_response(&response, &api_response);
        if (nResult != RC_OK)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(*this, L"Could not add new title",
                ra::context::IRcClient::GetErrorMessage(nResult, response.response));

            SetValue(IsAssociateEnabledProperty, true);
        }
        else
        {
            SetSelectedGameId(ra::to_signed(response.game_id));
            SetTestMode(false);
            AddClientHash(sNarrowHash, response.game_id, 0);
            SetDialogResult(ra::ui::DialogResult::OK);
        }
    }, nullptr);
}

void UnknownGameViewModel::BeginTest()
{
    const auto nGameId = GetSelectedGameId();
    if (nGameId == 0U)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(*this, L"You must select an existing game to test compatibility.");
        return;
    }

    ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
    vmMessageBox.SetHeader(ra::util::String::Printf(L"Play '%s' in compatability test mode?", m_vGameTitles.GetLabelForId(nGameId)));
    vmMessageBox.SetMessage(L"Achievements and leaderboards for the game will be loaded, but you will not be able to earn them.");
    vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
    if (vmMessageBox.ShowModal(*this) == DialogResult::No)
        return;

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    auto sValue = EncodeID(nGameId, GetChecksum(), pConsoleContext.Id());
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto sMapping = pLocalStorage.WriteText(ra::services::StorageItemType::HashMapping, GetChecksum());
    sMapping->WriteLine(sValue);

    SetTestMode(true);
    AddClientHash(ra::util::String::Narrow(GetChecksum()), nGameId, 1);

    SetDialogResult(ra::ui::DialogResult::OK);
}

void UnknownGameViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == FilterTextProperty)
    {
        ApplyFilter();
    }
    else if (args.Property == NewGameNameProperty && !m_bSelectingGame)
    {
        // user is entering a custom name, make sure <New Game> is selected
        SetSelectedGameId(0);
    }
    else if (args.Property == ChecksumProperty)
    {
        CheckForPreviousAssociation();
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

void UnknownGameViewModel::ApplyFilter()
{
    const auto& pFilterText = GetFilterText();

    m_vGameTitles.BeginUpdate();
    gsl::index nInsertIndex = 1; // <New Game> should always be visible
    uint32_t nIdAtInsertIndex = m_vGameTitles.GetItemValue(nInsertIndex, LookupItemViewModel::IdProperty);

    for (auto& pPair : m_vAllGameTitles)
    {
        const bool bVisible = pFilterText.empty() || ra::util::String::ContainsCaseInsensitive(pPair.second, pFilterText);
        if (bVisible)
        {
            if (nInsertIndex == gsl::narrow_cast<gsl::index>(m_vGameTitles.Count()))
            {
                m_vGameTitles.Add(pPair.first, pPair.second);
            }
            else if (nIdAtInsertIndex != pPair.first)
            {
                m_vGameTitles.Add(pPair.first, pPair.second);
                m_vGameTitles.MoveItem(m_vGameTitles.Count() - 1, nInsertIndex);
            }
            else
            {
                nIdAtInsertIndex = m_vGameTitles.GetItemValue(nInsertIndex + 1, LookupItemViewModel::IdProperty);
            }

            ++nInsertIndex;
        }
        else
        {
            if (pPair.first == nIdAtInsertIndex)
            {
                m_vGameTitles.RemoveAt(nInsertIndex);
                nIdAtInsertIndex = m_vGameTitles.GetItemValue(nInsertIndex, LookupItemViewModel::IdProperty);
            }
        }
    }
    m_vGameTitles.EndUpdate();

    const auto nSelectedGameId = GetSelectedGameId();
    if (nSelectedGameId == 0 || m_vGameTitles.FindItemIndex(LookupItemViewModel::IdProperty, nSelectedGameId) == -1)
    {
        // previously selected item is no longer visible. select the first matching item, or <New Value> if no matches exist
        if (m_vGameTitles.Count() > 1)
            SetSelectedGameId(m_vGameTitles.GetItemValue(1, LookupItemViewModel::IdProperty));
        else
            SetSelectedGameId(0);
    }

    SetValue(FilterResultsProperty, ra::util::String::Printf(L"%u/%u",
        gsl::narrow_cast<uint32_t>(m_vGameTitles.Count()) - 1, // ignore <New Title>
        gsl::narrow_cast<uint32_t>(m_vAllGameTitles.size())));

}

} // namespace viewmodels
} // namespace ui
} // namespace ra
