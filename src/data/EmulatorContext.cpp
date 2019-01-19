#include "EmulatorContext.hh"

#include "Exports.hh"
#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "api\LatestClient.hh"

#include "data\GameContext.hh"
#include "data\UserContext.hh"

#include "services\IConfiguration.hh"
#include "services\ILeaderboardManager.hh"

#include "ui\IDesktop.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#ifdef RA_UTEST
API void CCONV _RA_SetConsoleID(unsigned int)
{
}
#endif

namespace ra {
namespace data {

void EmulatorContext::Initialize(EmulatorID nEmulatorId)
{
    m_nEmulatorId = nEmulatorId;

    switch (nEmulatorId)
    {
        case RA_Gens:
            m_sClientName = "RAGens_REWiND";
            _RA_SetConsoleID(ConsoleID::MegaDrive);
            break;

        case RA_Project64:
            m_sClientName = "RAP64";
            _RA_SetConsoleID(ConsoleID::N64);
            break;

        case RA_Snes9x:
            m_sClientName = "RASnes9X";
            _RA_SetConsoleID(ConsoleID::SNES);
            break;

        case RA_VisualboyAdvance:
            m_sClientName = "RAVisualBoyAdvance";
            _RA_SetConsoleID(ConsoleID::GB);
            break;

        case RA_Nester:
        case RA_FCEUX:
            m_sClientName = "RANes";
            _RA_SetConsoleID(ConsoleID::NES);
            break;

        case RA_PCE:
            m_sClientName = "RAPCE";
            _RA_SetConsoleID(ConsoleID::PCEngine);
            break;

        case RA_Libretro:
            m_sClientName = "RALibRetro";
            // don't provide a default console - wait for user to load core
            break;

        case RA_Meka:
            m_sClientName = "RAMeka";
            _RA_SetConsoleID(ConsoleID::MasterSystem);
            break;

        case RA_QUASI88:
            m_sClientName = "RAQUASI88";
            _RA_SetConsoleID(ConsoleID::PC8800);
            break;
    }
}

static unsigned long long ParseVersion(const char* sVersion)
{
    char* pPart{};
    const auto major = strtoull(sVersion, &pPart, 10);
    Expects(pPart != nullptr);
    if (*pPart == '.')
        ++pPart;

    const auto minor = strtoul(pPart, &pPart, 10);
    if (*pPart == '.')
        ++pPart;

    const auto patch = strtoul(pPart, &pPart, 10);
    if (*pPart == '.')
        ++pPart;

    const auto revision = strtoul(pPart, &pPart, 10);
    // 64-bit max signed value is 9223 37203 68547 75807
    auto version = (major * 100000) + minor;
    version = (version * 100000) + patch;
    version = (version * 100000) + revision;
    return version;
}

bool EmulatorContext::ValidateClientVersion()
{
    if (m_sLatestVersion.empty())
    {
        ra::api::LatestClient::Request request;
        request.EmulatorId = ra::etoi(m_nEmulatorId);
        auto response = request.Call();
        if (!response.Succeeded())
        {
            if (ra::StringStartsWith(response.ErrorMessage, "Unknown client!"))
            {
                // if m_nEmulatorID is not recognized by the server, let it through regardless of version.
                // assume it's a new emulator that hasn't been released yet.
                m_sLatestVersion = "0.0.0.0";
                ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Could not retrieve latest client version.", ra::Widen(response.ErrorMessage));
            }
            else
            {
                m_sLatestVersion = "Unknown";
                m_sLatestVersionError = response.ErrorMessage;
            }
        }
        else
        {
            m_sLatestVersion = response.LatestVersion;

            const unsigned long long nServerVersion = ParseVersion(m_sLatestVersion.c_str());
            const unsigned long long nLocalVersion = ParseVersion(m_sVersion.c_str());
            RA_LOG("Client %s date: server %s, current %s",
                   (nLocalVersion >= nServerVersion) ? "up to" : "out of",
                   m_sLatestVersion,
                   m_sVersion);
        }
    }

    if (m_sLatestVersion == "Unknown")
    {
        const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();

        std::wstring sError;
        if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        {
            sError = L"The latest client is required for hardcore mode.";
            auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::UserContext>();
            if (pUserContext.IsLoggedIn())
            {
                sError += L" You will be logged out.";
                pUserContext.Logout();
            }
            else
            {
                if (!pUserContext.IsLoginDisabled() && !pConfiguration.GetApiToken().empty())
                    sError += L" Login canceled.";
            }

            pUserContext.DisableLogin();
        }

        if (!m_sLatestVersionError.empty())
        {
            if (!sError.empty())
                sError += L"\n";
            sError += ra::Widen(m_sLatestVersionError);
        }

        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Could not retrieve latest client version.", sError);
        return false;
    }

    const unsigned long long nServerVersion = ParseVersion(m_sLatestVersion.c_str());
    const unsigned long long nLocalVersion = ParseVersion(m_sVersion.c_str());

    if (nLocalVersion >= nServerVersion)
        return true;

    // remove any trailing ".0"s off the client version
    std::string sClientVersion = m_sVersion;
    while (ra::StringEndsWith(sClientVersion, ".0"))
        sClientVersion.resize(sClientVersion.length() - 2);

    std::string sNewVersion = m_sLatestVersion;
    while (ra::StringEndsWith(sNewVersion, ".0"))
        sNewVersion.resize(sNewVersion.length() - 2);

    bool bUpdate = false;
    bool bResult = true;

    std::wstring sMessage;
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(L"The latest client is required for hardcore mode.");
        vmMessageBox.SetMessage(ra::StringPrintf(
            L"A new version of %s is available to download at %s.\n\n- Current version: %s\n- New version: %s\n\n"
            L"Press OK to logout and download the new version, or Cancel to disable hardcore mode and proceed.",
            m_sClientName,
            pConfiguration.GetHostName(),
            sClientVersion,
            sNewVersion));
        vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::OKCancel);

        if (vmMessageBox.ShowModal() == ra::ui::DialogResult::Cancel)
        {
            DisableHardcoreMode();
        }
        else
        {
            bUpdate = true;
            bResult = false;
        }
    }
    else
    {
        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(L"Would you like to update?");
        vmMessageBox.SetMessage(ra::StringPrintf(
            L"A new version of %s is available to download at %s.\n\n- Current version: %s\n- New version: %s",
            m_sClientName,
            pConfiguration.GetHostName(),
            sClientVersion,
            sNewVersion));
        vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Info);
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

        bUpdate = (vmMessageBox.ShowModal() == ra::ui::DialogResult::Yes);
    }

    if (bUpdate)
    {
        ra::services::ServiceLocator::Get<ra::ui::IDesktop>().OpenUrl(
            ra::StringPrintf("http://%s/download.php", pConfiguration.GetHostName()));
    }

    return bResult;
}

void EmulatorContext::DisableHardcoreMode()
{
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        pConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);

#ifndef RA_UTEST
        _RA_RebuildMenu();

        auto& pLeaderboardManager = ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>();
        pLeaderboardManager.Reset();
#endif

        ra::services::ServiceLocator::GetMutable<ra::data::GameContext>().RefreshUnlocks();
    }
}

bool EmulatorContext::EnableHardcoreMode()
{
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        return true;

    // Enable the hardcore flag before calling ValidateClientVersion so it does the hardcore validation
    // correctly.
    pConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
    if (!ValidateClientVersion())
    {
        // The version could not be validated, or the user has chosen to update. Log them out.
        auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::UserContext>();
        pUserContext.Logout();

        // hardcore is still enabled, which is okay. if the user isn't logged in, they'll have to log in again,
        // at which point they'll go through the version validation logic again.
        return true; 
    }

    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        // The user has chosen to abort the switch to hardcore mode.
        return false;
    }

    // The user is on the latest verion
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    if (pGameContext.GameId() != 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(L"Enable hardcore mode?");
        vmMessageBox.SetMessage(
            L"Enabling hardcore mode will reset the emulator. You will lose any progress that has not been saved "
            L"through the game.");
        vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

        if (vmMessageBox.ShowModal() == ra::ui::DialogResult::No)
        {
            pConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
            return false;
        }
    }

#ifndef RA_UTEST
    // TODO: move these into EmulatorContext
    _RA_RebuildMenu();

    // when enabling hardcore mode, force a system reset
    _RA_ResetEmulation();
    _RA_OnReset();
#endif

    ra::services::ServiceLocator::GetMutable<ra::data::GameContext>().RefreshUnlocks();

    return true;
}

std::string EmulatorContext::GetAppTitle(const std::string& sMessage) const
{
    ra::StringBuilder builder;
    builder.Append(m_sClientName);
    builder.Append(" - ");

    // only copy the first two parts of the version string to the title bar: 0.12.7.1 => 0.12
    std::string sClientVersion = m_sVersion;
    auto nIndex = sClientVersion.find('.');
    if (nIndex != std::string::npos)
    {
        nIndex = sClientVersion.find('.', nIndex + 1);
        if (nIndex != std::string::npos)
            sClientVersion.erase(nIndex);
    }
    builder.Append(sClientVersion);

    if (!sMessage.empty())
    {
        builder.Append(" - ");
        builder.Append(sMessage);
    }

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::UserContext>();
    if (pUserContext.IsLoggedIn())
    {
        builder.Append(" - ");
        builder.Append(pUserContext.GetUsername());
    }

    const auto& sHostName = ra::services::ServiceLocator::Get<ra::services::IConfiguration>().GetHostName();
    if (sHostName != "retroachievements.org")
    {
        builder.Append(" [");
        builder.Append(sHostName);
        builder.Append("]");
    }

    return builder.ToString();
}

} // namespace data
} // namespace ra
