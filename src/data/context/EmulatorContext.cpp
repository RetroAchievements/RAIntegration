#include "EmulatorContext.hh"

#include "Exports.hh"
#include "RA_BuildVer.h"
#include "RA_Log.h"
#include "RA_Resource.h"
#include "RA_StringUtils.h"

#include "api\LatestClient.hh"

#include "data\context\GameContext.hh"
#include "data\context\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\AchievementRuntimeExports.hh"
#include "services\IClock.hh"
#include "services\IConfiguration.hh"
#include "services\IFileSystem.hh"
#include "services\IHttpRequester.hh"

#include "ui\IDesktop.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "RAInterface\RA_Consoles.h"
#include "RAInterface\RA_Emulators.h"

#include <rcheevos/src/rc_client_internal.h>

namespace ra {
namespace data {
namespace context {

static std::string FormatAddressLarge(ra::ByteAddress nAddress)
{
    std::string sAddress;
    sAddress.resize(10);
    sprintf_s(sAddress.data(), 11, "0x%08x", nAddress);
    return sAddress;
}

static std::string FormatAddressMedium(ra::ByteAddress nAddress)
{
    if (nAddress & 0xFF000000)
        return FormatAddressLarge(nAddress);

    std::string sAddress;
    sAddress.resize(8);
    sprintf_s(sAddress.data(), 9, "0x%06x", nAddress);
    return sAddress;
}

static std::string FormatAddressSmall(ra::ByteAddress nAddress)
{
    if (nAddress & 0xFFFF0000)
        return FormatAddressMedium(nAddress);

    std::string sAddress;
    sAddress.resize(6);
    sprintf_s(sAddress.data(), 7, "0x%04x", nAddress);
    return sAddress;
}

void EmulatorContext::Initialize(EmulatorID nEmulatorId, const char* sClientName)
{
    m_nEmulatorId = nEmulatorId;
    m_fFormatAddress = FormatAddressSmall;

    switch (nEmulatorId)
    {
        case RA_Gens:
            m_sClientName = "RAGens_REWiND";
            m_sAcceptButtonText = L"\u24B8"; // encircled C
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
            m_sAcceptButtonText = L"Enter";
            m_sCancelButtonText = L"Backspace";
            _RA_SetConsoleID(ConsoleID::PC8800);
            break;

        case RA_AppleWin:
            m_sClientName = "RAppleWin";
            m_sAcceptButtonText = L"Enter";
            m_sCancelButtonText = L"Backspace";
            _RA_SetConsoleID(ConsoleID::AppleII);
            break;

        case RA_Oricutron:
            m_sClientName = "RAOricutron";
            m_sAcceptButtonText = L"Enter";
            m_sCancelButtonText = L"Backspace";
            _RA_SetConsoleID(ConsoleID::Oric);
            break;

        default:
            if (sClientName != nullptr && *sClientName)
            {
                m_sClientName = sClientName;
            }
            else
            {
                const auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();
                const auto sFileName = pDesktop.GetRunningExecutable();
                const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
                m_sClientName = ra::Narrow(pFileSystem.RemoveExtension(pFileSystem.GetFileName(sFileName)));
            }
            m_nEmulatorId = EmulatorID::UnknownEmulator;
            break;
    }
}

static void AppendIntegrationVersion(_Inout_ std::string& sUserAgent)
{
    sUserAgent.append(ra::StringPrintf("%d.%d.%d.%d", RA_INTEGRATION_VERSION_MAJOR,
        RA_INTEGRATION_VERSION_MINOR, RA_INTEGRATION_VERSION_PATCH,
        RA_INTEGRATION_VERSION_REVISION));

    // include branch if specified
    if constexpr (_CONSTANT_LOC pos{ std::string_view{ RA_INTEGRATION_VERSION_PRODUCT }.find('-') }; pos != std::string_view::npos)
    {
        constexpr std::string_view sAppend{ RA_INTEGRATION_VERSION_PRODUCT };
        sUserAgent.append(sAppend, pos);
    }
}

void EmulatorContext::UpdateUserAgent()
{
    std::string sUserAgent;
    sUserAgent.reserve(128U);

    if (!m_sClientName.empty())
    {
        sUserAgent.append(m_sClientName);
        sUserAgent.push_back('/');
    }
    else
    {
        sUserAgent.append("UnknownClient/");
    }

    size_t nOffset = 0;
    if (!m_sVersion.empty() && (m_sVersion.at(0) == 'v' || m_sVersion.at(0) == 'V'))
    {
        /* version string starts with a "v". assume it's part of a prefix (i.e. "v1.2", or "version 1.2") and ignore it */
        while (nOffset < m_sVersion.length() && (isalpha(m_sVersion.at(nOffset)) || isspace(m_sVersion.at(nOffset))))
            ++nOffset;
    }

    sUserAgent.append(m_sVersion, nOffset);
    sUserAgent.append(" (");

    const auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();
    sUserAgent.append(pDesktop.GetOSVersionString());
    sUserAgent.append(") Integration/");
    AppendIntegrationVersion(sUserAgent);

    if (!m_sClientUserAgentDetail.empty())
    {
        sUserAgent.push_back(' ');
        sUserAgent.append(m_sClientUserAgentDetail);
    }

    RA_LOG_INFO("User-Agent: %s", sUserAgent.c_str());

    ra::services::ServiceLocator::GetMutable<ra::services::IHttpRequester>().SetUserAgent(sUserAgent);
}

std::string EmulatorContext::GetUserAgentClause() const
{
    std::string sClause = "Integration/";
    AppendIntegrationVersion(sClause);
    return sClause;
}

static unsigned long long ParseVersion(const char* sVersion)
{
    Expects(sVersion != nullptr);

    while (isalpha(*sVersion) || isspace(*sVersion))
        ++sVersion;

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
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    const bool bWasHardcore = pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
    bool bHardcore = bWasHardcore;
    if (!ValidateClientVersion(bHardcore))
        return false;

    // ValidateClientVersion can set bHardcore to false if it was true and the user chose not to upgrade
    if (bWasHardcore && !bHardcore)
        DisableHardcoreMode();

    return true;
}

/// <summary>
/// Returns <c>true</c> if the player is allowed to play using the current client.
/// </summary>
bool EmulatorContext::ValidateClientVersion(bool& bHardcore)
{
    std::string sMinimumVersion;

    // client not specified - assume pre-release of new client
    if (m_nEmulatorId == EmulatorID::UnknownEmulator)
        return true;

    // fetch the latest version
    if (m_sLatestVersion.empty())
    {
        ra::api::LatestClient::Request request;
        request.EmulatorId = ra::etoi(m_nEmulatorId);
        auto response = request.Call();
        if (!response.Succeeded())
        {
            if (ra::StringStartsWith(response.ErrorMessage, "Unknown client"))
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
            m_sMinimumVersion = response.MinimumVersion;

#ifndef RA_UTEST
            const unsigned long long nServerVersion = ParseVersion(m_sLatestVersion.c_str());
            const unsigned long long nLocalVersion = ParseVersion(m_sVersion.c_str());
            RA_LOG_INFO("Client %s date: server %s, current %s",
                   (nLocalVersion >= nServerVersion) ? "up to" : "out of",
                   m_sLatestVersion,
                   m_sVersion);
#endif
        }

        if (m_nEmulatorId == EmulatorID::RA_Gens)
            ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"RAGens is being retired", L"With the next major release of the toolkit, you will no longer be able to play games using RAGens. Please switch over to RALibretro or RetroArch.");
    }

    // if we failed to fetch the latest version, abort
    if (m_sLatestVersion == "Unknown")
    {
        const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();

        std::wstring sError;
        if (bHardcore)
        {
            sError = L"The latest client is required for hardcore mode.";
            auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>();
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

    // determine if current version is valid
    const unsigned long long nServerVersion = ParseVersion(m_sLatestVersion.c_str());
    const unsigned long long nLocalVersion = ParseVersion(m_sVersion.c_str());

    if (nLocalVersion >= nServerVersion)
        return true;

    // remove leading prefix and any trailing ".0"s off the client version
    std::string sClientVersion = m_sVersion;
    while (!sClientVersion.empty() && (isalpha(sClientVersion.at(0)) || isspace(sClientVersion.at(0))))
        sClientVersion.erase(0, 1);
    while (ra::StringEndsWith(sClientVersion, ".0"))
        sClientVersion.resize(sClientVersion.length() - 2);
    if (sClientVersion.find('.') == std::string::npos)
        sClientVersion.append(".0");

    std::string sNewVersion = m_sLatestVersion;
    while (ra::StringEndsWith(sNewVersion, ".0"))
        sNewVersion.resize(sNewVersion.length() - 2);
    if (sNewVersion.find('.') == std::string::npos)
        sNewVersion.append(".0");

    const unsigned long long nMinimumVersion = ParseVersion(m_sMinimumVersion.c_str());
    bool bUpdate = false;
    bool bResult = true;

    std::wstring sMessage;
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (nLocalVersion < nMinimumVersion && bHardcore)
    {
        // enforce minimum version required for hardcore
        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(L"A newer client is required for hardcore mode.");
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
            RA_LOG_INFO("Hardcore disabled by version check %s < %s", m_sVersion, sMinimumVersion);
            bHardcore = false;
        }
        else
        {
            bUpdate = true;
            bResult = false;
        }
    }
    else
    {
        // allow any version in non-hardcore, but inform the user a new version is available
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
            ra::StringPrintf("%s/download.php", pConfiguration.GetHostUrl()));
    }

    return bResult;
}

bool EmulatorContext::WarnDisableHardcoreMode(const std::string& sActivity)
{
    // already disabled, just return success
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        return true;

    if (!sActivity.empty())
    {
        // prompt. if user doesn't consent, return failure - caller should not continue
        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(L"Disable Hardcore mode?");
        vmMessageBox.SetMessage(ra::StringPrintf(L"You cannot %s while Hardcore mode is active.", sActivity));
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
        vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
        if (vmMessageBox.ShowModal() != ra::ui::DialogResult::Yes)
            return false;

        RA_LOG_INFO("User chose to disable hardcore to %s", sActivity);
    }
    else
    {
        // no activity specified. just proceed to disabling without prompting.
        RA_LOG_INFO("Disabling hardcore. No activity provided to WarnDisableHardcore");
    }

    // user consented, switch to non-hardcore mode
    DisableHardcoreMode();

    // return success
    return true;
}

void EmulatorContext::DisableHardcoreMode()
{
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        pConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);

        // updating the enabled-ness of the buttons of the asset list
        auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
        pWindowManager.AssetList.UpdateButtons();

        SyncClientExternalHardcoreState();

        UpdateMenuState(IDM_RA_HARDCORE_MODE);

        // GameContext::DoFrame synchronizes the models to the runtime
        ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().QueueMemoryRead([]() {
            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
            pGameContext.DoFrame();
        });
    }
}

bool EmulatorContext::EnableHardcoreMode(bool bShowWarning)
{
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
        return true;

    // pass true to ValidateClientVersion so it behaves as if hardcore is already enabled
    bool bHardcore = true;
    if (!ValidateClientVersion(bHardcore))
    {
        // The version could not be validated, or the user has chosen to update. Log them out.
        auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>();
        pUserContext.Logout();
        return false;
    }

    if (!bHardcore)
    {
        // The user has chosen to continue playing without upgrading their client. Abort the switch to hardcore
        return false;
    }

    // The user is on the latest version. If a game is loaded, inform them that the emulator must be reset
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    if (pGameContext.GameId() != 0 && bShowWarning)
    {
        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(L"Enable hardcore mode?");
        vmMessageBox.SetMessage(
            L"Enabling hardcore mode will reset the emulator. You will lose any progress that has not been saved "
            L"through the game.");
        vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

        if (vmMessageBox.ShowModal() == ra::ui::DialogResult::No)
            return false;
    }

    RA_LOG_INFO("Hardcore enabled");

    // User has agreed to reset the emulator, or no game is loaded. Enabled hardcore!
    pConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

    // close any windows not permitted in hardcore
    auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();

    if (pWindowManager.MemoryInspector.IsVisible())
        pDesktop.CloseWindow(pWindowManager.MemoryInspector);

    if (pWindowManager.MemoryBookmarks.IsVisible())
        pDesktop.CloseWindow(pWindowManager.MemoryBookmarks);

    if (pWindowManager.AssetEditor.IsVisible())
        pDesktop.CloseWindow(pWindowManager.AssetEditor);

    if (pWindowManager.CodeNotes.IsVisible())
        pDesktop.CloseWindow(pWindowManager.CodeNotes);

    if (pWindowManager.PointerFinder.IsVisible())
        pDesktop.CloseWindow(pWindowManager.PointerFinder);

    // updating the enabled-ness of the buttons of the asset list
    pWindowManager.AssetList.UpdateButtons();

    // update the runtime
    SyncClientExternalHardcoreState();

    // update the integration menu
    UpdateMenuState(IDM_RA_HARDCORE_MODE);

    // GameContext::DoFrame synchronizes the models to the runtime
    pGameContext.DoFrame();

    // Disable any modified assets
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()); ++nIndex)
    {
        auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
        if (pAsset != nullptr && pAsset->GetChanges() != ra::data::models::AssetChanges::None)
        {
            if (pAsset->IsActive())
                pAsset->SetState(ra::data::models::AssetState::Inactive);
        }
    }

    // unfreeze bookmarks so the user doesn't get hit with a modified memory error
    auto& vmBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    vmBookmarks.ClearBehaviors();

    return true;
}

std::wstring EmulatorContext::GetAppTitle(const std::string& sMessage) const
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

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    const auto& sDisplayName = pUserContext.GetDisplayName();
    if (!sDisplayName.empty())
    {
        builder.Append(" - ");
        builder.Append(sDisplayName);
    }

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    const auto& sHostName =
        pConfiguration.IsFeatureEnabled(ra::services::Feature::Offline) ? "OFFLINE": pConfiguration.GetHostName();
    if (sHostName != "retroachievements.org")
    {
        builder.Append(" [");
        builder.Append(sHostName);
        builder.Append("]");
    }

    return builder.ToWString();
}

std::string EmulatorContext::GetGameTitle() const
{
    if (!m_fGetGameTitle)
        return std::string();

    std::array<char, 256> buffer{};
    m_fGetGameTitle(buffer.data());
    return std::string(buffer.data());
}

void EmulatorContext::ClearMemoryBlocks()
{
    m_vMemoryBlocks.clear();

    if (m_nTotalMemorySize != 0U)
    {
        m_nTotalMemorySize = 0U;
        OnTotalMemorySizeChanged();
    }
}

void EmulatorContext::AddMemoryBlock(gsl::index nIndex, size_t nBytes,
    EmulatorContext::MemoryReadFunction pReader, EmulatorContext::MemoryWriteFunction pWriter)
{
    while (m_vMemoryBlocks.size() <= ra::to_unsigned(nIndex))
        m_vMemoryBlocks.emplace_back();

    MemoryBlock& pBlock = m_vMemoryBlocks.at(nIndex);
    if (pBlock.size == 0)
    {
        pBlock.size = nBytes;
        pBlock.read = pReader;
        pBlock.write = pWriter;
        pBlock.readBlock = nullptr;

        m_nTotalMemorySize += nBytes;

        OnTotalMemorySizeChanged();
    }
}

void EmulatorContext::AddMemoryBlockReader(gsl::index nIndex,
    EmulatorContext::MemoryReadBlockFunction pReader)
{
    if (nIndex < gsl::narrow_cast<gsl::index>(m_vMemoryBlocks.size()))
        m_vMemoryBlocks.at(nIndex).readBlock = pReader;
}

void EmulatorContext::OnTotalMemorySizeChanged()
{
    if (m_nTotalMemorySize <= 0x10000)
        m_fFormatAddress = FormatAddressSmall;
    else if (m_nTotalMemorySize <= 0x1000000)
        m_fFormatAddress = FormatAddressMedium;
    else
        m_fFormatAddress = FormatAddressLarge;

    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnTotalMemorySizeChanged();

        m_vNotifyTargets.Unlock();
    }
}

bool EmulatorContext::HasInvalidRegions() const noexcept
{
    for (const auto& pBlock : m_vMemoryBlocks)
    {
        if (!pBlock.read)
            return true;
    }

    return false;
}

bool EmulatorContext::IsValidAddress(ra::ByteAddress nAddress) const noexcept
{
    for (const auto& pBlock : m_vMemoryBlocks)
    {
        if (nAddress < pBlock.size)
        {
            if (pBlock.read)
                return true;

            break;
        }

        nAddress -= gsl::narrow_cast<ra::ByteAddress>(pBlock.size);
    }

    return false;
}

void EmulatorContext::AssertIsOnDoFrameThread() const
{
#if RA_INTEGRATION_VERSION_REVISION != 0
 #if !defined(RA_UTEST)
    const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
    if (!pRuntime.GetClient()->state.allow_background_memory_reads &&
        !pRuntime.IsOnDoFrameThread())
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        if (pGameContext.GameId() != 0 && !pGameContext.IsGameLoading())
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
                L"Memory Read on Background Thread",
                L"Memory is being read on a thread other than the thread that last called rc_client_do_frame and the "
                L"client has specified that should not be allowed. Please notify a RetroAchievements engineer what you "
                L"were doing when this occurred.");
        }
    }
 #endif
#endif
}

uint8_t EmulatorContext::ReadMemoryByte(ra::ByteAddress nAddress) const
{
    AssertIsOnDoFrameThread();

    for (const auto& pBlock : m_vMemoryBlocks)
    {
        if (nAddress < pBlock.size)
        {
            if (pBlock.read)
                return pBlock.read(nAddress);

            break;
        }

        nAddress -= gsl::narrow_cast<ra::ByteAddress>(pBlock.size);
    }

    return 0;
}

uint32_t EmulatorContext::ReadMemory(ra::ByteAddress nAddress, uint8_t pBuffer[], size_t nCount,
                                   const EmulatorContext::MemoryBlock& pBlock)
{
    Expects(pBuffer != nullptr);

    if (pBlock.readBlock)
    {
        const size_t nRead = pBlock.readBlock(nAddress, pBuffer, gsl::narrow_cast<uint32_t>(nCount));
        if (nRead < nCount)
            memset(pBuffer + nRead, 0, nCount - nRead);

        return gsl::narrow_cast<uint32_t>(nRead);
    }

    if (!pBlock.read)
    {
        memset(pBuffer, 0, nCount);
        return 0;
    }

    auto nRemaining = nCount;
    while (nRemaining >= 8) // unrolled loop to read 8-byte chunks
    {
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        nRemaining -= 8;
    }

    switch (nRemaining) // partial Duff's device to read remaining bytes
    {
        case 7:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        case 6:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        case 5:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        case 4:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        case 3:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        case 2:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        case 1:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        default:
            break;
    }

    return gsl::narrow_cast<uint32_t>(nCount);
}

_Use_decl_annotations_
uint32_t EmulatorContext::ReadMemory(ra::ByteAddress nAddress, uint8_t pBuffer[], size_t nCount) const
{
    AssertIsOnDoFrameThread();

    uint32_t nBytesRead = 0;
    Expects(pBuffer != nullptr);

    for (const auto& pBlock : m_vMemoryBlocks)
    {
        if (nAddress >= pBlock.size)
        {
            nAddress -= gsl::narrow_cast<ra::ByteAddress>(pBlock.size);
            continue;
        }

        const size_t nBlockRemaining = pBlock.size - nAddress;
        size_t nToRead = std::min(nCount, nBlockRemaining);

        nBytesRead += ReadMemory(nAddress, pBuffer, nToRead, pBlock);

        pBuffer += nToRead;
        nCount -= nToRead;

        if (nCount == 0)
            return nBytesRead;

        nAddress = 0;
    }

    if (nCount > 0)
        memset(pBuffer, 0, nCount);

    return nBytesRead;
}

uint32_t EmulatorContext::ReadMemory(ra::ByteAddress nAddress, MemSize nSize) const
{
    switch (nSize)
    {
        case MemSize::Bit_0:
            return (ReadMemoryByte(nAddress) & 0x01);
        case MemSize::Bit_1:
            return (ReadMemoryByte(nAddress) & 0x02) ? 1 : 0;
        case MemSize::Bit_2:
            return (ReadMemoryByte(nAddress) & 0x04) ? 1 : 0;
        case MemSize::Bit_3:
            return (ReadMemoryByte(nAddress) & 0x08) ? 1 : 0;
        case MemSize::Bit_4:
            return (ReadMemoryByte(nAddress) & 0x10) ? 1 : 0;
        case MemSize::Bit_5:
            return (ReadMemoryByte(nAddress) & 0x20) ? 1 : 0;
        case MemSize::Bit_6:
            return (ReadMemoryByte(nAddress) & 0x40) ? 1 : 0;
        case MemSize::Bit_7:
            return (ReadMemoryByte(nAddress) & 0x80) ? 1 : 0;
        case MemSize::Nibble_Lower:
            return (ReadMemoryByte(nAddress) & 0x0F);
        case MemSize::Nibble_Upper:
            return ((ReadMemoryByte(nAddress) >> 4) & 0x0F);
        case MemSize::EightBit:
            return ReadMemoryByte(nAddress);
        default:
        case MemSize::SixteenBit:
        {
            uint8_t buffer[2];
            ReadMemory(nAddress, buffer, 2);
            return buffer[0] | (buffer[1] << 8);
        }
        case MemSize::TwentyFourBit:
        {
            uint8_t buffer[3];
            ReadMemory(nAddress, buffer, 3);
            return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16);
        }
        case MemSize::Float:
        case MemSize::FloatBigEndian:
        case MemSize::Double32:
        case MemSize::Double32BigEndian:
        case MemSize::MBF32:
        case MemSize::MBF32LE:
        case MemSize::ThirtyTwoBit:
        {
            uint8_t buffer[4];
            ReadMemory(nAddress, buffer, 4);
            return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
        }
        case MemSize::SixteenBitBigEndian:
        {
            uint8_t buffer[2];
            ReadMemory(nAddress, buffer, 2);
            return buffer[1] | (buffer[0] << 8);
        }
        case MemSize::TwentyFourBitBigEndian:
        {
            uint8_t buffer[3];
            ReadMemory(nAddress, buffer, 3);
            return buffer[2] | (buffer[1] << 8) | (buffer[0] << 16);
        }
        case MemSize::ThirtyTwoBitBigEndian:
        {
            uint8_t buffer[4];
            ReadMemory(nAddress, buffer, 4);
            return buffer[3] | (buffer[2] << 8) | (buffer[1] << 16) | (buffer[0] << 24);
        }
        case MemSize::BitCount:
        {
            const uint8_t nValue = ReadMemoryByte(nAddress);
            static const std::array<uint8_t, 16> nBitsSet = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };
            return nBitsSet.at(nValue & 0x0F) + nBitsSet.at((nValue >> 4) & 0x0F);
        }
    }
}

void EmulatorContext::WriteMemory(ra::ByteAddress nAddress, const uint8_t* pBytes, size_t nBytes) const
{
    Expects(pBytes != nullptr);
    size_t nBytesWritten = 0;
    auto nBlockAddress = nAddress;

    for (const auto& pBlock : m_vMemoryBlocks)
    {
        if (nBlockAddress < pBlock.size)
        {
            if (!pBlock.write)
                break;

            m_bMemoryModified = true;

            if (nBytes == 1)
            {
                pBlock.write(nBlockAddress, pBytes[0]);
                nBytesWritten = 1;
                break;
            }
            else
            {
                do
                {
                    pBlock.write(nBlockAddress++, pBytes[nBytesWritten++]);
                } while (nBytesWritten < nBytes && nBlockAddress < pBlock.size);

                if (nBytesWritten == nBytes)
                    break;
            }
        }

        nBlockAddress -= gsl::narrow_cast<ra::ByteAddress>(pBlock.size);
    }

    if (nBytesWritten > 0)
    {
        if (m_vNotifyTargets.LockIfNotEmpty())
        {
            for (auto& target : m_vNotifyTargets.Targets())
            {
                for (unsigned nIndex = 0; nIndex < nBytesWritten; ++nIndex)
                    target.OnByteWritten(nAddress + nIndex, pBytes[nIndex]);
            }

            m_vNotifyTargets.Unlock();
        }
    }
}

void EmulatorContext::WriteMemoryByte(ra::ByteAddress nAddress, uint8_t nValue) const
{
    WriteMemory(nAddress, &nValue, 1);
}

void EmulatorContext::WriteMemory(ra::ByteAddress nAddress, MemSize nSize, uint32_t nValue) const
{
    union
    {
        uint32_t u32;
        uint8_t u8[4];
    } m_pCurrent;
    m_pCurrent.u32 = nValue;

    switch (nSize)
    {
        case MemSize::EightBit:
            WriteMemory(nAddress, m_pCurrent.u8, 1);
            break;

        case MemSize::SixteenBit:
            WriteMemory(nAddress, m_pCurrent.u8, 2);
            break;

        case MemSize::TwentyFourBit:
            WriteMemory(nAddress, m_pCurrent.u8, 3);
            break;

        case MemSize::ThirtyTwoBit:
            // already little endian
            WriteMemory(nAddress, m_pCurrent.u8, 4);
            break;

        case MemSize::Float:
        case MemSize::FloatBigEndian:
        case MemSize::Double32:
        case MemSize::Double32BigEndian:
        case MemSize::MBF32:
        case MemSize::MBF32LE:
            // assume the value has already been encoded into a 32-bit little endian value
            WriteMemory(nAddress, m_pCurrent.u8, 4);
            break;

        case MemSize::SixteenBitBigEndian:
            m_pCurrent.u8[3] = m_pCurrent.u8[0];
            m_pCurrent.u8[2] = m_pCurrent.u8[1];
            WriteMemory(nAddress, &m_pCurrent.u8[2], 2);
            break;

        case MemSize::TwentyFourBitBigEndian:
            m_pCurrent.u8[3] = m_pCurrent.u8[0];
            m_pCurrent.u8[0] = m_pCurrent.u8[2];
            m_pCurrent.u8[2] = m_pCurrent.u8[3];
            WriteMemory(nAddress, m_pCurrent.u8, 3);
            break;

        case MemSize::ThirtyTwoBitBigEndian:
            m_pCurrent.u32 = ReverseBytes(nValue);
            WriteMemory(nAddress, m_pCurrent.u8, 4);
            break;

        case MemSize::Bit_0:
            m_pCurrent.u32 = (ReadMemoryByte(nAddress) & ~0x01) | (nValue & 1);
            WriteMemory(nAddress, m_pCurrent.u8, 1);
            break;
        case MemSize::Bit_1:
            m_pCurrent.u32 = (ReadMemoryByte(nAddress) & ~0x02) | ((nValue & 1) << 1);
            WriteMemory(nAddress, m_pCurrent.u8, 1);
            break;
        case MemSize::Bit_2:
            m_pCurrent.u32 = (ReadMemoryByte(nAddress) & ~0x04) | ((nValue & 1) << 2);
            WriteMemory(nAddress, m_pCurrent.u8, 1);
            break;
        case MemSize::Bit_3:
            m_pCurrent.u32 = (ReadMemoryByte(nAddress) & ~0x08) | ((nValue & 1) << 3);
            WriteMemory(nAddress, m_pCurrent.u8, 1);
            break;
        case MemSize::Bit_4:
            m_pCurrent.u32 = (ReadMemoryByte(nAddress) & ~0x10) | ((nValue & 1) << 4);
            WriteMemory(nAddress, m_pCurrent.u8, 1);
            break;
        case MemSize::Bit_5:
            m_pCurrent.u32 = (ReadMemoryByte(nAddress) & ~0x20) | ((nValue & 1) << 5);
            WriteMemory(nAddress, m_pCurrent.u8, 1);
            break;
        case MemSize::Bit_6:
            m_pCurrent.u32 = (ReadMemoryByte(nAddress) & ~0x40) | ((nValue & 1) << 6);
            WriteMemory(nAddress, m_pCurrent.u8, 1);
            break;
        case MemSize::Bit_7:
            m_pCurrent.u32 = (ReadMemoryByte(nAddress) & ~0x80) | ((nValue & 1) << 7);
            WriteMemory(nAddress, m_pCurrent.u8, 1);
            break;

        case MemSize::Nibble_Lower:
            m_pCurrent.u32 = (ReadMemoryByte(nAddress) & ~0x0F) | (nValue & 0x0F);
            WriteMemory(nAddress, m_pCurrent.u8, 1);
            break;
        case MemSize::Nibble_Upper:
            m_pCurrent.u32 = (ReadMemoryByte(nAddress) & ~0xF0) | ((nValue & 0x0F) << 4);
            WriteMemory(nAddress, m_pCurrent.u8, 1);
            break;

        default:
            break;
    }
}

void EmulatorContext::ResetMemoryModified()
{
    m_bMemoryModified = false;

    if (m_bMemoryInsecure)
    {
        // memory was insecure, immediately do a check
        const auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();
        m_bMemoryInsecure = pDesktop.IsDebuggerPresent();

        const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
        m_tLastInsecureCheck = pClock.UpTime();
    }
}

bool EmulatorContext::IsMemoryInsecure() const
{
    if (m_bMemoryInsecure)
        return true;

    // limit checks to once every 10 seconds
    const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
    const auto tNow = pClock.UpTime();
    const auto tElapsed = tNow - m_tLastInsecureCheck;
    if (tElapsed > std::chrono::seconds(10))
    {
        const auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();
        m_bMemoryInsecure = pDesktop.IsDebuggerPresent();
        m_tLastInsecureCheck = tNow;
    }

    return m_bMemoryInsecure;
}

_CONSTANT_VAR MAX_BLOCK_SIZE = 256U * 1024; // 256K

void EmulatorContext::CaptureMemory(std::vector<ra::data::search::MemBlock>& vBlocks, ra::ByteAddress nAddress, uint32_t nCount, uint32_t nPadding) const
{
    ra::ByteAddress nAdjustedAddress = nAddress;
    for (const auto& pMemoryBlock : m_vMemoryBlocks)
    {
        if (nAdjustedAddress >= pMemoryBlock.size)
        {
            nAdjustedAddress -= gsl::narrow_cast<ra::ByteAddress>(pMemoryBlock.size);
            continue;
        }

        if (!pMemoryBlock.read && !pMemoryBlock.readBlock)
        {
            nCount -= gsl::narrow_cast<uint32_t>(pMemoryBlock.size);
            nAddress += gsl::narrow_cast<ra::ByteAddress>(pMemoryBlock.size);
            nAdjustedAddress = 0;
            continue;
        }

        const uint32_t nBlockRemaining = gsl::narrow_cast<uint32_t>(pMemoryBlock.size) - nAdjustedAddress;
        uint32_t nToRead = std::min(nCount, nBlockRemaining);
        nCount -= nToRead;

        while (nToRead > 0)
        {
            ra::data::search::MemBlock* pBlock = nullptr;

            const uint32_t nBlockSize = std::min(nToRead, MAX_BLOCK_SIZE);
            const bool bIsLastBlock = (nCount == 0 && nBlockSize == nToRead);
            if (!bIsLastBlock)
            {
                // capture nPadding overlapping bytes so we don't have to stitch multiple
                // blocks together if a read occurs at the block boundary
                pBlock = &vBlocks.emplace_back(nAddress, nBlockSize + nPadding, nBlockSize);
            }
            else
            {
                pBlock = &vBlocks.emplace_back(nAddress, nBlockSize, nBlockSize);
            }

            Expects(pBlock != nullptr);
            ReadMemory(nAdjustedAddress, pBlock->GetBytes(), nBlockSize, pMemoryBlock);
            pBlock->OptimizeMemory(vBlocks);

            nAddress += nBlockSize;
            nAdjustedAddress += nBlockSize;
            nToRead -= nBlockSize;
        }

        if (nCount == 0)
            break;

        nAdjustedAddress = 0;
    }

    if (nPadding)
    {
        // copy the first four bytes of each block to the last four bytes of the previous
        // block to cover the overlap
        for (size_t i = 1; i < vBlocks.size(); ++i)
        {
            auto& pDstBlock = vBlocks.at(i - 1);
            // ASSERT: pDstBlock->GetBytes() is a size that's a multiple of 4 even if
            //         pDstBlock->GetBytesSize() is not.
            auto* pDstBytes = pDstBlock.GetBytes() + (pDstBlock.GetBytesSize() & ~3);
            const auto* pSrcBytes = vBlocks.at(i).GetBytes();
            memcpy(pDstBytes, pSrcBytes, 4);
        }
    }
}

void EmulatorContext::UpdateMenuState(int nMenuItemId) const
{
    SyncClientExternalRAIntegrationMenuItem(nMenuItemId);

    RebuildMenu();
}

void EmulatorContext::RebuildMenu() const
{
    if (m_fRebuildMenu)
        ra::services::ServiceLocator::Get<ra::ui::IDesktop>().InvokeOnUIThread(m_fRebuildMenu);
}

void EmulatorContext::Reset() const
{
    if (m_fResetEmulator)
    {
        if (IsExternalRcheevosClient()) // AchievementRuntimeExports will determine which thread to notify
            m_fResetEmulator();
        else
            ra::services::ServiceLocator::Get<ra::ui::IDesktop>().InvokeOnUIThread(m_fResetEmulator);
    }
}

void EmulatorContext::Pause() const
{
    if (m_fPauseEmulator)
    {
        if (IsExternalRcheevosClient()) // AchievementRuntimeExports will determine which thread to notify
            m_fPauseEmulator();
        else
            ra::services::ServiceLocator::Get<ra::ui::IDesktop>().InvokeOnUIThread(m_fPauseEmulator);
    }
}

void EmulatorContext::Unpause() const
{
    if (m_fUnpauseEmulator)
        ra::services::ServiceLocator::Get<ra::ui::IDesktop>().InvokeOnUIThread(m_fUnpauseEmulator);
}

void EmulatorContext::DispatchesReadMemory::DispatchMemoryRead(std::function<void()>&& fFunction)
{
    if (ra::services::ServiceLocator::Exists<ra::services::AchievementRuntime>())
    {
        const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
        pRuntime.QueueMemoryRead(std::move(fFunction));
    }
    else
    {
        fFunction();
    }
}


} // namespace context
} // namespace data
} // namespace ra
