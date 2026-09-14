#include "JsonFileConfiguration.hh"

#include "util\Json.hh"
#include "util\Log.hh"
#include "util\Strings.hh"

#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

#include <rcheevos\src\rapi\rc_api_common.h>

#ifndef RA_UTEST
#include "services\impl\StringTextWriter.hh"
#include "services\impl\WindowsHttpRequester.hh"
#include "ui\win32\Desktop.hh"
#endif

namespace ra {
namespace services {
namespace impl {

static void ReadPopupLocation(JsonFileConfiguration& pConfiguration, ra::ui::viewmodels::Popup nPopup,
    const ra::util::Json::Reader& doc, const char* sFieldName, ra::ui::viewmodels::PopupLocation nDefaultLocation, bool bAllowCenter)
{
    ra::ui::viewmodels::PopupLocation nLocation = nDefaultLocation;

    std::string sValue;
    bool bValue;

    if (doc.TryGetString(sFieldName, sValue) && !sValue.empty())
    {
        if (sValue.at(0) == 'T')
        {
            if (sValue == "TopLeft")
                nLocation = ra::ui::viewmodels::PopupLocation::TopLeft;
            else if (sValue == "TopRight")
                nLocation = ra::ui::viewmodels::PopupLocation::TopRight;
            else if (bAllowCenter && sValue == "TopMiddle")
                nLocation = ra::ui::viewmodels::PopupLocation::TopMiddle;
        }
        else if (sValue.at(0) == 'B')
        {
            if (sValue == "BottomLeft")
                nLocation = ra::ui::viewmodels::PopupLocation::BottomLeft;
            else if (sValue == "BottomRight")
                nLocation = ra::ui::viewmodels::PopupLocation::BottomRight;
            else if (bAllowCenter && sValue == "BottomMiddle")
                nLocation = ra::ui::viewmodels::PopupLocation::BottomMiddle;
        }
        else if (sValue == "None")
        {
            nLocation = ra::ui::viewmodels::PopupLocation::None;
        }
    }
    else if (doc.TryGetBoolean(sFieldName, bValue) && !bValue)
    {
        nLocation = ra::ui::viewmodels::PopupLocation::None;
    }

    pConfiguration.SetPopupLocation(nPopup, nLocation);
}

bool JsonFileConfiguration::Load(const std::wstring& sFilename)
{
    m_sFilename = sFilename;

    // default values
    m_sUsername.clear();
    m_sApiToken.clear();
    m_mWindowPositions.clear();
    m_nBackgroundThreads = 8;
    m_vEnabledFeatures =
        (1 << static_cast<int>(Feature::Hardcore)) |
        (1 << static_cast<int>(Feature::Leaderboards));
    SetPopupLocation(ra::ui::viewmodels::Popup::Message, ra::ui::viewmodels::PopupLocation::BottomLeft);
    SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
    SetPopupLocation(ra::ui::viewmodels::Popup::Mastery, ra::ui::viewmodels::PopupLocation::TopMiddle);
    SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardStarted, ra::ui::viewmodels::PopupLocation::BottomLeft);
    SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardCanceled, ra::ui::viewmodels::PopupLocation::BottomLeft);
    SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker, ra::ui::viewmodels::PopupLocation::BottomRight);
    SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, ra::ui::viewmodels::PopupLocation::BottomRight);
    SetPopupLocation(ra::ui::viewmodels::Popup::Challenge, ra::ui::viewmodels::PopupLocation::BottomRight);
    SetPopupLocation(ra::ui::viewmodels::Popup::Progress, ra::ui::viewmodels::PopupLocation::BottomRight);

    RA_LOG_INFO("Loading preferences...");

    auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    auto pReader = pFileSystem.OpenTextFile(m_sFilename);
    if (pReader == nullptr)
        return false;

    ra::util::Json::Reader pJson;
    if (!pJson.Parse(*pReader))
        return false;

    m_sUsername = pJson.GetString("Username");
    m_sApiToken = pJson.GetString("Token");
    SetFeatureEnabled(Feature::Hardcore, pJson.GetBoolean("Hardcore Active", true));
    SetFeatureEnabled(Feature::NonHardcoreWarning, pJson.GetBoolean("Non Hardcore Warning", false));
    SetFeatureEnabled(Feature::OnlyHardcoreUnlocks, pJson.GetBoolean("Only Hardcore Unlocks", false));

    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::AchievementTriggered, pJson, "Achievement Triggered Notification Display", ra::ui::viewmodels::PopupLocation::BottomLeft, true);
    SetFeatureEnabled(Feature::AchievementTriggeredScreenshot, pJson.GetBoolean("Achievement Triggered Screenshot", false));
    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::Mastery, pJson, "Mastery Notification Display", ra::ui::viewmodels::PopupLocation::TopMiddle, true);
    SetFeatureEnabled(Feature::MasteryNotificationScreenshot, pJson.GetBoolean("Mastery Screenshot", false));

    std::string sScreenShotDirectory;
    if (pJson.TryGetString("Screenshot Directory", sScreenShotDirectory))
        SetScreenshotDirectory(ra::util::String::Widen(sScreenShotDirectory));

    SetFeatureEnabled(Feature::Leaderboards, pJson.GetBoolean("Leaderboards Active", true));
    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::LeaderboardStarted, pJson, "Leaderboard Notification Display", ra::ui::viewmodels::PopupLocation::BottomLeft, true);
    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::LeaderboardCanceled, pJson, "Leaderboard Cancel Display", ra::ui::viewmodels::PopupLocation::BottomLeft, true);
    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::LeaderboardTracker, pJson, "Leaderboard Counter Display", ra::ui::viewmodels::PopupLocation::BottomRight, true);
    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::LeaderboardScoreboard, pJson, "Leaderboard Scoreboard Display", ra::ui::viewmodels::PopupLocation::BottomRight, false);

    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::Challenge, pJson, "Challenge Notification Display", ra::ui::viewmodels::PopupLocation::BottomRight, false);
    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::Progress, pJson, "Progress Display", ra::ui::viewmodels::PopupLocation::BottomRight, false);

    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::Message, pJson, "Informational Notification Display", ra::ui::viewmodels::PopupLocation::BottomLeft, true);

    SetFeatureEnabled(Feature::PreferDecimal, pJson.GetBoolean("Prefer Decimal", false));

    m_nBackgroundThreads = pJson.GetInteger("Num Background Threads", 4);

    ra::util::Json::Reader::Node pWindowPositions;
    if (pJson.TryGetObject("Window Positions", pWindowPositions))
    {
        std::vector<std::string> vKeys;
        pWindowPositions.GetKeys(vKeys);
        for (const auto& sKey : vKeys)
        {
            ra::util::Json::Reader::Node pWindowPosition;
            if (pWindowPositions.TryGetObject(sKey, pWindowPosition))
            {
                WindowPosition& pos = m_mWindowPositions[sKey];
                pos.oPosition.X = pWindowPosition.GetInteger("X", INT32_MIN);
                pos.oPosition.Y = pWindowPosition.GetInteger("Y", INT32_MIN);
                pos.oSize.Width = pWindowPosition.GetInteger("Width", INT32_MIN);
                pos.oSize.Height = pWindowPosition.GetInteger("Height", INT32_MIN);
            }
        }
    }

    return true;
}

static void WritePopupLocation(ra::util::Json::Writer& pWriter,
    const std::string& sFieldName, ra::ui::viewmodels::PopupLocation nPopupLocation)
{
    switch (nPopupLocation)
    {
        case ra::ui::viewmodels::PopupLocation::None:
            pWriter.SetString(sFieldName, "None");
            break;
        case ra::ui::viewmodels::PopupLocation::TopLeft:
            pWriter.SetString(sFieldName, "TopLeft");
            break;
        case ra::ui::viewmodels::PopupLocation::TopMiddle:
            pWriter.SetString(sFieldName, "TopMiddle");
            break;
        case ra::ui::viewmodels::PopupLocation::TopRight:
            pWriter.SetString(sFieldName, "TopRight");
            break;
        case ra::ui::viewmodels::PopupLocation::BottomLeft:
            pWriter.SetString(sFieldName, "BottomLeft");
            break;
        case ra::ui::viewmodels::PopupLocation::BottomMiddle:
            pWriter.SetString(sFieldName, "BottomMiddle");
            break;
        case ra::ui::viewmodels::PopupLocation::BottomRight:
            pWriter.SetString(sFieldName, "BottomRight");
            break;
    }
}

void JsonFileConfiguration::Save() const
{
    RA_LOG_INFO("Saving preferences...");

    if (m_sFilename.empty())
    {
        RA_LOG_WARN(" - Aborting save, we don't know where to write...");
        return;
    }

    ra::util::Json::Writer pWriter;
    pWriter.SetString("Username", m_sUsername);
    pWriter.SetString("Token", m_sApiToken);

    pWriter.SetBoolean("Hardcore Active", IsFeatureEnabled(Feature::Hardcore));
    pWriter.SetBoolean("Non Hardcore Warning", IsFeatureEnabled(Feature::NonHardcoreWarning));
    pWriter.SetBoolean("Only Hardcore Unlocks", IsFeatureEnabled(Feature::OnlyHardcoreUnlocks));

    WritePopupLocation(pWriter, "Achievement Triggered Notification Display", GetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered));
    pWriter.SetBoolean("Achievement Triggered Screenshot", IsFeatureEnabled(Feature::AchievementTriggeredScreenshot));
    WritePopupLocation(pWriter, "Mastery Notification Display", GetPopupLocation(ra::ui::viewmodels::Popup::Mastery));
    pWriter.SetBoolean("Mastery Screenshot", IsFeatureEnabled(Feature::MasteryNotificationScreenshot));

    pWriter.SetBoolean("Leaderboards Active", IsFeatureEnabled(Feature::Leaderboards));
    WritePopupLocation(pWriter, "Leaderboard Notification Display", GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardStarted));
    WritePopupLocation(pWriter, "Leaderboard Cancel Display", GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardCanceled));
    WritePopupLocation(pWriter, "Leaderboard Counter Display", GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker));
    WritePopupLocation(pWriter, "Leaderboard Scoreboard Display", GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard));
    WritePopupLocation(pWriter, "Challenge Notification Display", GetPopupLocation(ra::ui::viewmodels::Popup::Challenge));
    WritePopupLocation(pWriter, "Informational Notification Display", GetPopupLocation(ra::ui::viewmodels::Popup::Message));

    pWriter.SetBoolean("Prefer Decimal", IsFeatureEnabled(Feature::PreferDecimal));
    pWriter.SetInteger("Num Background Threads", m_nBackgroundThreads);

    if (!m_sScreenshotDirectory.empty())
        pWriter.SetString("Screenshot Directory", ra::util::String::Narrow(m_sScreenshotDirectory));

    ra::util::Json::Writer::Node pWindowPositions = pWriter.SetObject("Window Positions");
    for (WindowPositionMap::const_iterator iter = m_mWindowPositions.begin(); iter != m_mWindowPositions.end(); ++iter)
    {
        ra::util::Json::Writer::Node pWindowPosition = pWindowPositions.SetObject(iter->first);
        if (iter->second.oPosition.X != INT32_MIN)
            pWindowPosition.SetInteger("X", iter->second.oPosition.X);
        if (iter->second.oPosition.Y != INT32_MIN)
            pWindowPosition.SetInteger("Y", iter->second.oPosition.Y);
        if (iter->second.oSize.Width != INT32_MIN)
            pWindowPosition.SetInteger("Width", iter->second.oSize.Width);
        if (iter->second.oSize.Height != INT32_MIN)
            pWindowPosition.SetInteger("Height", iter->second.oSize.Height);
    }

    auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    auto pFile = pFileSystem.CreateTextFile(m_sFilename);
    if (pFile != nullptr)
        pWriter.Save(*pFile);
}

bool JsonFileConfiguration::IsFeatureEnabled(Feature nFeature) const noexcept
{
    return (m_vEnabledFeatures & (1 << ra::etoi(nFeature)));
}

void JsonFileConfiguration::SetFeatureEnabled(Feature nFeature, bool bEnabled) noexcept
{
    const auto bit = 1 << ra::etoi(nFeature);

    if (bEnabled)
        m_vEnabledFeatures |= bit;
    else
        m_vEnabledFeatures &= ~bit;
}

ra::ui::viewmodels::PopupLocation JsonFileConfiguration::GetPopupLocation(ra::ui::viewmodels::Popup nPopup) const
{
    return m_vPopupLocations.at(ra::etoi(nPopup));
}

void JsonFileConfiguration::SetPopupLocation(ra::ui::viewmodels::Popup nPopup, ra::ui::viewmodels::PopupLocation nPopupLocation)
{
    m_vPopupLocations.at(ra::etoi(nPopup)) = nPopupLocation;
}

ra::ui::Position JsonFileConfiguration::GetWindowPosition(const std::string& sPositionKey) const
{
    const WindowPositionMap::const_iterator iter = m_mWindowPositions.find(sPositionKey);
    if (iter != m_mWindowPositions.end())
        return iter->second.oPosition;

    return ra::ui::Position{ INT32_MIN, INT32_MIN };
}

void JsonFileConfiguration::SetWindowPosition(const std::string& sPositionKey, const ra::ui::Position & oPosition)
{
    m_mWindowPositions[sPositionKey].oPosition = oPosition;
}

ra::ui::Size JsonFileConfiguration::GetWindowSize(const std::string & sPositionKey) const
{
    const WindowPositionMap::const_iterator iter = m_mWindowPositions.find(sPositionKey);
    if (iter != m_mWindowPositions.end())
        return iter->second.oSize;

    return ra::ui::Size{ INT32_MIN, INT32_MIN };
}

void JsonFileConfiguration::SetWindowSize(const std::string& sPositionKey, const ra::ui::Size& oSize)
{
    m_mWindowPositions[sPositionKey].oSize = oSize;
}

void JsonFileConfiguration::SetHost(const std::string& sHost)
{
    m_sHostName = sHost;
    UpdateHost();
}

const std::string& JsonFileConfiguration::GetHostName() const
{
    if (m_sHostName.empty())
        GSL_SUPPRESS_TYPE3 const_cast<JsonFileConfiguration*>(this)->ReadHostFile();

    return m_sHostName;
}

const std::string& JsonFileConfiguration::GetHostUrl() const
{
    if (m_sHostUrl.empty())
        GSL_SUPPRESS_TYPE3 const_cast<JsonFileConfiguration*>(this)->ReadHostFile();

    return m_sHostUrl;
}

void JsonFileConfiguration::ReadHostFile()
{
    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    if (pFileSystem.GetFileSize(L"host.txt") > 0)
    {
        auto pFile = pFileSystem.OpenTextFile(L"host.txt");
        if (pFile != nullptr)
            pFile->GetLine(m_sHostName);
    }

    UpdateHost();
}

void JsonFileConfiguration::UpdateHost()
{
    if (m_sHostName.empty())
    {
        m_bCustomHost = false;
        m_sHostUrl = rc_api_default_host();

#ifndef RA_UTEST
        const auto sOSVersion = ra::ui::win32::Desktop::GetWindowsVersionString();
        if (ra::util::String::StartsWith(sOSVersion, "WindowsNT "))
        {
            // Windows 7 (and Vista and XP) only support TLS 1.0 by default. Windows 8+ support TLS 1.2.
            // https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/b27a9ddd-d8f7-408c-8029-cf5f8f9ddbef/winhttp-winhttpcallbackstatusflagsecuritychannelerror-on-win7?forum=vcgeneral
            // The server requires at least TLS 1.2 due to security issues in TLS 1.0 and 1.1.

            // If this is a one of the vulnerable operating systems, try to make a secure request.
            // https://docs.microsoft.com/en-us/windows/win32/sysinfo/operating-system-version
            // https://docs.microsoft.com/en-us/windows/win32/secauthn/protocols-in-tls-ssl--schannel-ssp-
            const auto nVersion = std::atof(&sOSVersion.at(9));
            if (nVersion < 6.2) // Windows 8
            {
                // Try to make a secure request, and if it fails with ERROR_WINHTTP_SECURE_FAILURE,
                // switch to a non-secure host
                ra::services::Http::Request request(m_sHostUrl + "/dorequest.php?r=latestclient&e=0");
                std::string sResponse;
                ra::services::impl::StringTextWriter pWriter(sResponse);

                ra::services::impl::WindowsHttpRequester httpRequester;
                const auto nStatusCode = httpRequester.Request(request, pWriter);

                if (nStatusCode == 12175) // ERROR_WINHTTP_SECURE_FAILURE
                {
                    ::MessageBoxA(nullptr,
                        "An error occurred trying to communicate with the server via secure protocols. Switching to non-secure protocols.\n\n"
                        "retroachievements.org requires TLS 1.2 or higher. You may need to manually enable it on this operating system. Note that non-secure protocols are deprecated and will no longer be allowed at some point in the future.",
                        "Security Error", MB_OK);
                    m_sHostUrl.erase(m_sHostUrl.begin() + 4, m_sHostUrl.begin() + 5);
                }
            }
        }
#endif

    }
    else
    {
        m_bCustomHost = true;

        const auto nIndex = m_sHostName.find("://");
        if (nIndex == std::string::npos)
            m_sHostUrl = "http://" + m_sHostName;
        else
            m_sHostUrl.swap(m_sHostName);
    }

    m_sHostName = m_sHostUrl.substr(m_sHostUrl.find("://") + 3);

    RA_LOG_INFO("Using server: %s", m_sHostUrl.c_str());
}

} // namespace impl
} // namespace services
} // namespace ra
