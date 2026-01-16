#include "UnknownGameViewModel.hh"

#include "util\Strings.hh"

#include "api\FetchGamesList.hh"
#include "api\SubmitNewTitle.hh"

#include "context\IConsoleContext.hh"
#include "context\IRcClient.hh"

#include "data\context\GameContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\AchievementRuntimeExports.hh"
#include "services\IClipboard.hh"
#include "services\ILocalStorage.hh"
#include "services\ILoginService.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "rcheevos\src\rc_client_external.h"
#include "rcheevos\src\rc_client_internal.h"

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

    ra::api::FetchGamesList::Request request;
    request.ConsoleId = consoleId;

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
            m_vGameTitles.BeginUpdate();
            for (const auto& pGame : response.Games)
                m_vGameTitles.Add(pGame.Id, pGame.Name);
            m_vGameTitles.EndUpdate();
        }

        m_vGameTitles.Freeze();

        SetValue(IsAssociateEnabledProperty, true);
        SetValue(IsSelectedGameEnabledProperty, true);

        const auto nSelectedGameId = GetSelectedGameId();
        if (nSelectedGameId == 0)
            CheckForPreviousAssociation();
    });
}

void UnknownGameViewModel::InitializeTestCompatibilityMode()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    m_vGameTitles.Add(pGameContext.GameId(), pGameContext.GameTitle());
    m_vGameTitles.Freeze();
    SetSelectedGameId(pGameContext.GameId());
    SetChecksum(ra::Widen(pGameContext.GameHash()));

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

    if (ra::ParseHex(ra::Widen(sLine), 0xFFFFFFFF, nEncodedId, sError))
        return nEncodedId ^ GenerateMask(sHash, nConsoleId);

    return 0;
}

std::string UnknownGameViewModel::EncodeID(unsigned nId, const std::wstring& sHash, ConsoleID nConsoleId)
{
    const auto nEncodedId = nId ^ GenerateMask(sHash, nConsoleId);
    return ra::StringPrintf("%08x", nEncodedId);
}

static void AddClientHash(const std::string& sHash, uint32_t nGameId, bool isUnknown)
{
    auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
    rc_client_add_game_hash(pClient, sHash.c_str(), nGameId);
    auto* game_hash = rc_client_find_game_hash(pClient, sHash.c_str());
    game_hash->is_unknown = isUnknown ? 1 : 0;
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
        vmMessageBox.SetHeader(ra::StringPrintf(L"Are you sure you want to add a new hash to '%s'?", request.GameName));
        vmMessageBox.SetMessage(L"You should not do this unless you are certain that the new title is compatible. You can use 'Test' mode to check compatibility.");
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
        if (vmMessageBox.ShowModal(*this) == DialogResult::No)
            return false;
    }

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    request.ConsoleId = pConsoleContext.Id();
    request.Hash = ra::Narrow(GetChecksum());
    request.Description = GetEstimatedGameName();

    auto response = request.Call();
    if (response.Succeeded())
    {
        SetSelectedGameId(ra::to_signed(response.GameId));
        SetTestMode(false);
        AddClientHash(request.Hash, response.GameId, 0);
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
    vmMessageBox.SetMessage(L"Achievements and leaderboards for the game will be loaded, but you will not be able to earn them.");
    vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
    if (vmMessageBox.ShowModal(*this) == DialogResult::No)
        return false;

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    auto sValue = EncodeID(nGameId, GetChecksum(), pConsoleContext.Id());
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto sMapping = pLocalStorage.WriteText(ra::services::StorageItemType::HashMapping, GetChecksum());
    sMapping->WriteLine(sValue);

    SetTestMode(true);
    AddClientHash(ra::Narrow(GetChecksum()), nGameId, 1);

    return true;
}

void UnknownGameViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == NewGameNameProperty && !m_bSelectingGame)
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

} // namespace viewmodels
} // namespace ui
} // namespace ra
