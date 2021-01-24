#include "EmulatorContext.hh"

#include "Exports.hh"
#include "RA_BuildVer.h"
#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "api\LatestClient.hh"

#include "data\context\GameContext.hh"
#include "data\context\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IClock.hh"
#include "services\IConfiguration.hh"
#include "services\IFileSystem.hh"
#include "services\IHttpRequester.hh"

#include "ui\IDesktop.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "RAInterface\RA_Consoles.h"
#include "RAInterface\RA_Emulators.h"

namespace ra {
namespace data {
namespace context {

void EmulatorContext::Initialize(EmulatorID nEmulatorId, const char* sClientName)
{
    m_nEmulatorId = nEmulatorId;

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
                m_sClientName = pFileSystem.RemoveExtension(pFileSystem.GetFileName(sFileName));
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
    sUserAgent.append(m_sVersion);
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
    unsigned long long nMinimumVersion = 0;
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
            sMinimumVersion = response.MinimumVersion;
            nMinimumVersion = ParseVersion(response.MinimumVersion.c_str());

            const unsigned long long nServerVersion = ParseVersion(m_sLatestVersion.c_str());
            const unsigned long long nLocalVersion = ParseVersion(m_sVersion.c_str());
            RA_LOG_INFO("Client %s date: server %s, current %s",
                   (nLocalVersion >= nServerVersion) ? "up to" : "out of",
                   m_sLatestVersion,
                   m_sVersion);
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

    // remove any trailing ".0"s off the client version
    std::string sClientVersion = m_sVersion;
    while (ra::StringEndsWith(sClientVersion, ".0"))
        sClientVersion.resize(sClientVersion.length() - 2);
    if (sClientVersion.find('.') == std::string::npos)
        sClientVersion.append(".0");

    std::string sNewVersion = m_sLatestVersion;
    while (ra::StringEndsWith(sNewVersion, ".0"))
        sNewVersion.resize(sNewVersion.length() - 2);
    if (sNewVersion.find('.') == std::string::npos)
        sNewVersion.append(".0");

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

void EmulatorContext::DisableHardcoreMode()
{
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        pConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);

        RebuildMenu();

        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        pGameContext.RefreshUnlocks();
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

    RebuildMenu();

    // when enabling hardcore mode, force a system reset
    if (m_fResetEmulator)
        m_fResetEmulator();

#ifndef RA_UTEST
    // emulator reset code will normally call back into _RA_OnReset, but call it explicitly just in case.
    _RA_OnReset();
#endif

    auto& vmBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    for (gsl::index nIndex = 0; nIndex < ra::to_signed(vmBookmarks.Bookmarks().Count()); ++nIndex)
    {
        auto* pBookmark = vmBookmarks.Bookmarks().GetItemAt(nIndex);
        if (pBookmark)
            pBookmark->SetBehavior(ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior::None);
    }

    pGameContext.ActivateLeaderboards();
    pGameContext.RefreshUnlocks();

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

void EmulatorContext::AddMemoryBlock(gsl::index nIndex, size_t nBytes,
    EmulatorContext::MemoryReadFunction* pReader, EmulatorContext::MemoryWriteFunction* pWriter)
{
    while (m_vMemoryBlocks.size() <= ra::to_unsigned(nIndex))
        m_vMemoryBlocks.emplace_back();

    MemoryBlock& pBlock = m_vMemoryBlocks.at(nIndex);
    if (pBlock.size == 0)
    {
        pBlock.size = nBytes;
        pBlock.read = pReader;
        pBlock.write = pWriter;

        m_nTotalMemorySize += nBytes;

        OnTotalMemorySizeChanged();
    }
}

void EmulatorContext::OnTotalMemorySizeChanged()
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnTotalMemorySizeChanged();
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
    const ra::ByteAddress nOriginalAddress = nAddress;
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

uint8_t EmulatorContext::ReadMemoryByte(ra::ByteAddress nAddress) const noexcept
{
    const ra::ByteAddress nOriginalAddress = nAddress;
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

    ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().InvalidateAddress(nOriginalAddress);
    return 0;
}

_Use_decl_annotations_
void EmulatorContext::ReadMemory(ra::ByteAddress nAddress, uint8_t pBuffer[], size_t nCount) const
{
    const ra::ByteAddress nOriginalAddress = nAddress;
    Expects(pBuffer != nullptr);

    for (const auto& pBlock : m_vMemoryBlocks)
    {
        if (nAddress >= pBlock.size)
        {
            nAddress -= gsl::narrow_cast<ra::ByteAddress>(pBlock.size);
            continue;
        }

        if (!pBlock.read)
        {
            ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>().InvalidateAddress(nOriginalAddress);
            break;
        }

        const size_t nBlockRemaining = pBlock.size - nAddress;
        size_t nToRead = std::min(nCount, nBlockRemaining);
        nCount -= nToRead;

        while (nToRead >= 8) // unrolled loop to read 8-byte chunks
        {
            *pBuffer++ = pBlock.read(nAddress++);
            *pBuffer++ = pBlock.read(nAddress++);
            *pBuffer++ = pBlock.read(nAddress++);
            *pBuffer++ = pBlock.read(nAddress++);
            *pBuffer++ = pBlock.read(nAddress++);
            *pBuffer++ = pBlock.read(nAddress++);
            *pBuffer++ = pBlock.read(nAddress++);
            *pBuffer++ = pBlock.read(nAddress++);
            nToRead -= 8;
        }

        switch (nToRead) // partial Duff's device to read remaining bytes
        {
            case 7: *pBuffer++ = pBlock.read(nAddress++); _FALLTHROUGH;
            case 6: *pBuffer++ = pBlock.read(nAddress++); _FALLTHROUGH;
            case 5: *pBuffer++ = pBlock.read(nAddress++); _FALLTHROUGH;
            case 4: *pBuffer++ = pBlock.read(nAddress++); _FALLTHROUGH;
            case 3: *pBuffer++ = pBlock.read(nAddress++); _FALLTHROUGH;
            case 2: *pBuffer++ = pBlock.read(nAddress++); _FALLTHROUGH;
            case 1: *pBuffer++ = pBlock.read(nAddress++); _FALLTHROUGH;
            default: break;
        }

        if (nCount == 0)
            return;

        nAddress = 0;
    }

    if (nCount > 0)
        memset(pBuffer, 0, nCount);
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
        case MemSize::ThirtyTwoBit:
        {
            uint8_t buffer[4];
            ReadMemory(nAddress, buffer, 4);
            return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
        }
    }
}

void EmulatorContext::WriteMemoryByte(ra::ByteAddress nAddress, uint8_t nValue) const
{
    for (const auto& pBlock : m_vMemoryBlocks)
    {
        if (nAddress < pBlock.size)
        {
            if (!pBlock.write)
                return;

            pBlock.write(nAddress, nValue);
            m_bMemoryModified = true;

            // create a copy of the list of pointers in case it's modified by one of the callbacks
            NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
            for (NotifyTarget* target : vNotifyTargets)
            {
                Expects(target != nullptr);
                target->OnByteWritten(nAddress, nValue);
            }

            return;
        }

        nAddress -= gsl::narrow_cast<ra::ByteAddress>(pBlock.size);
    }
}

void EmulatorContext::WriteMemory(ra::ByteAddress nAddress, MemSize nSize, uint32_t nValue) const
{
    switch (nSize)
    {
        case MemSize::EightBit:
            WriteMemoryByte(nAddress, nValue & 0xFF);
            return;
        case MemSize::SixteenBit:
            WriteMemoryByte(nAddress, nValue & 0xFF);
            nValue >>= 8;
            WriteMemoryByte(++nAddress, nValue & 0xFF);
            return;
        case MemSize::TwentyFourBit:
            WriteMemoryByte(nAddress, nValue & 0xFF);
            nValue >>= 8;
            WriteMemoryByte(++nAddress, nValue & 0xFF);
            nValue >>= 8;
            WriteMemoryByte(++nAddress, nValue & 0xFF);
            return;
        case MemSize::ThirtyTwoBit:
            WriteMemoryByte(nAddress, nValue & 0xFF);
            nValue >>= 8;
            WriteMemoryByte(++nAddress, nValue & 0xFF);
            nValue >>= 8;
            WriteMemoryByte(++nAddress, nValue & 0xFF);
            nValue >>= 8;
            WriteMemoryByte(++nAddress, nValue & 0xFF);
            return;
    }

    const auto nCurrentValue = ReadMemoryByte(nAddress);
    uint8_t nNewValue = gsl::narrow_cast<uint8_t>(nValue);

    switch (nSize)
    {
        case MemSize::Bit_0:
            nNewValue = (nCurrentValue & ~0x01) | (nNewValue & 1);
            break;
        case MemSize::Bit_1:
            nNewValue = (nCurrentValue & ~0x02) | ((nNewValue & 1) << 1);
            break;
        case MemSize::Bit_2:
            nNewValue = (nCurrentValue & ~0x04) | ((nNewValue & 1) << 2);
            break;
        case MemSize::Bit_3:
            nNewValue = (nCurrentValue & ~0x08) | ((nNewValue & 1) << 3);
            break;
        case MemSize::Bit_4:
            nNewValue = (nCurrentValue & ~0x10) | ((nNewValue & 1) << 4);
            break;
        case MemSize::Bit_5:
            nNewValue = (nCurrentValue & ~0x20) | ((nNewValue & 1) << 5);
            break;
        case MemSize::Bit_6:
            nNewValue = (nCurrentValue & ~0x40) | ((nNewValue & 1) << 6);
            break;
        case MemSize::Bit_7:
            nNewValue = (nCurrentValue & ~0x80) | ((nNewValue & 1) << 7);
            break;
        case MemSize::Nibble_Lower:
            nNewValue = (nCurrentValue & ~0x0F) | (nNewValue & 0x0F);
            break;
        case MemSize::Nibble_Upper:
            nNewValue = (nCurrentValue & ~0xF0) | ((nNewValue & 0x0F) << 4);
            break;
        default:
            break;
    }

    WriteMemoryByte(nAddress, nNewValue);
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

} // namespace context
} // namespace data
} // namespace ra
