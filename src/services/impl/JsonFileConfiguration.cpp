#include "JsonFileConfiguration.hh"

#include "RA_Json.h"
#include "RA_Log.h"
#include "RA_StringUtils.h"
#include "ra_utility.h"

#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace impl {

static void ReadPopupLocation(JsonFileConfiguration& pConfiguration, ra::ui::viewmodels::Popup nPopup,
    const rapidjson::Document& doc, const char* sFieldName, ra::ui::viewmodels::PopupLocation nDefaultLocation, bool bAllowCenter)
{
    ra::ui::viewmodels::PopupLocation nLocation = nDefaultLocation;

    if (doc.HasMember(sFieldName))
    {
        const auto& pField = doc[sFieldName];
        if (pField.IsBool())
        {
            if (!pField.GetBool())
                nLocation = ra::ui::viewmodels::PopupLocation::None;
        }
        else if (pField.IsString())
        {
            const char* pValue = pField.GetString();
            Expects(pValue != nullptr);
            if (*pValue == 'T')
            {
                if (strcmp(pValue, "TopLeft") == 0)
                    nLocation = ra::ui::viewmodels::PopupLocation::TopLeft;
                else if (strcmp(pValue, "TopRight") == 0)
                    nLocation = ra::ui::viewmodels::PopupLocation::TopRight;
                else if (bAllowCenter && strcmp(pValue, "TopMiddle") == 0)
                    nLocation = ra::ui::viewmodels::PopupLocation::TopMiddle;
            }
            else if (*pValue == 'B')
            {
                if (strcmp(pValue, "BottomLeft") == 0)
                    nLocation = ra::ui::viewmodels::PopupLocation::BottomLeft;
                else if (strcmp(pValue, "BottomRight") == 0)
                    nLocation = ra::ui::viewmodels::PopupLocation::BottomRight;
                else if (bAllowCenter && strcmp(pValue, "BottomMiddle") == 0)
                    nLocation = ra::ui::viewmodels::PopupLocation::BottomMiddle;
            }
            else if (strcmp(pValue, "None") == 0)
            {
                nLocation = ra::ui::viewmodels::PopupLocation::None;
            }
        }
    }

    pConfiguration.SetPopupLocation(nPopup, nLocation);
}

bool JsonFileConfiguration::Load(const std::wstring& sFilename)
{
    m_sFilename = sFilename;

    // default values
    m_sUsername.clear();
    m_sApiToken.clear();
    m_sRomDirectory.clear();
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

    RA_LOG_INFO("Loading preferences...");

    auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    auto pReader = pFileSystem.OpenTextFile(m_sFilename);
    if (pReader == nullptr)
        return false;

    rapidjson::Document doc;
    if (!LoadDocument(doc, *pReader))
        return false;

    if (doc.HasMember("Username"))
        m_sUsername = doc["Username"].GetString();
    if (doc.HasMember("Token"))
        m_sApiToken = doc["Token"].GetString();
    if (doc.HasMember("Hardcore Active"))
        SetFeatureEnabled(Feature::Hardcore, doc["Hardcore Active"].GetBool());
    if (doc.HasMember("Non Hardcore Warning"))
        SetFeatureEnabled(Feature::NonHardcoreWarning, doc["Non Hardcore Warning"].GetBool());

    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::AchievementTriggered, doc, "Achievement Triggered Notification Display", ra::ui::viewmodels::PopupLocation::BottomLeft, true);
    if (doc.HasMember("Achievement Triggered Screenshot"))
        SetFeatureEnabled(Feature::AchievementTriggeredScreenshot, doc["Achievement Triggered Screenshot"].GetBool());
    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::Mastery, doc, "Mastery Notification Display", ra::ui::viewmodels::PopupLocation::TopMiddle, true);
    if (doc.HasMember("Mastery Screenshot"))
        SetFeatureEnabled(Feature::MasteryNotificationScreenshot, doc["Mastery Screenshot"].GetBool());
    if (doc.HasMember("Screenshot Directory"))
        SetScreenshotDirectory(ra::Widen(doc["Screenshot Directory"].GetString()));

    if (doc.HasMember("Leaderboards Active"))
        SetFeatureEnabled(Feature::Leaderboards, doc["Leaderboards Active"].GetBool());
    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::LeaderboardStarted, doc, "Leaderboard Notification Display", ra::ui::viewmodels::PopupLocation::BottomLeft, true);
    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::LeaderboardCanceled, doc, "Leaderboard Cancel Display", ra::ui::viewmodels::PopupLocation::BottomLeft, true);
    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::LeaderboardTracker, doc, "Leaderboard Counter Display", ra::ui::viewmodels::PopupLocation::BottomRight, true);
    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::LeaderboardScoreboard, doc, "Leaderboard Scoreboard Display", ra::ui::viewmodels::PopupLocation::BottomRight, false);

    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::Challenge, doc, "Challenge Notification Display", ra::ui::viewmodels::PopupLocation::BottomRight, true);

    ReadPopupLocation(*this, ra::ui::viewmodels::Popup::Message, doc, "Informational Notification Display", ra::ui::viewmodels::PopupLocation::BottomLeft, true);

    if (doc.HasMember("Prefer Decimal"))
        SetFeatureEnabled(Feature::PreferDecimal, doc["Prefer Decimal"].GetBool());

    if (doc.HasMember("Num Background Threads"))
        m_nBackgroundThreads = doc["Num Background Threads"].GetUint();
    if (doc.HasMember("ROM Directory"))
        m_sRomDirectory = ra::Widen(doc["ROM Directory"].GetString());

    if (doc.HasMember("Window Positions"))
    {
        const rapidjson::Value& positions = doc["Window Positions"];
        if (positions.IsObject())
        {
            for (rapidjson::Value::ConstMemberIterator iter = positions.MemberBegin(); iter != positions.MemberEnd(); ++iter)
            {
                WindowPosition& pos = m_mWindowPositions[iter->name.GetString()];
                pos.oPosition.X = pos.oPosition.Y = pos.oSize.Width = pos.oSize.Height = INT32_MIN;

                if (iter->value.HasMember("X"))
                    pos.oPosition.X = iter->value["X"].GetInt();
                if (iter->value.HasMember("Y"))
                    pos.oPosition.Y = iter->value["Y"].GetInt();
                if (iter->value.HasMember("Width"))
                    pos.oSize.Width = iter->value["Width"].GetInt();
                if (iter->value.HasMember("Height"))
                    pos.oSize.Height = iter->value["Height"].GetInt();
            }
        }
    }

    return true;
}

static void WritePopupLocation(rapidjson::Document& doc, rapidjson::Document::AllocatorType& a,
    const char* sFieldName, ra::ui::viewmodels::PopupLocation nPopupLocation)
{
    switch (nPopupLocation)
    {
        case ra::ui::viewmodels::PopupLocation::None:
            doc.AddMember(rapidjson::StringRef(sFieldName), rapidjson::StringRef("None"), a);
            break;
        case ra::ui::viewmodels::PopupLocation::TopLeft:
            doc.AddMember(rapidjson::StringRef(sFieldName), rapidjson::StringRef("TopLeft"), a);
            break;
        case ra::ui::viewmodels::PopupLocation::TopMiddle:
            doc.AddMember(rapidjson::StringRef(sFieldName), rapidjson::StringRef("TopMiddle"), a);
            break;
        case ra::ui::viewmodels::PopupLocation::TopRight:
            doc.AddMember(rapidjson::StringRef(sFieldName), rapidjson::StringRef("TopRight"), a);
            break;
        case ra::ui::viewmodels::PopupLocation::BottomLeft:
            doc.AddMember(rapidjson::StringRef(sFieldName), rapidjson::StringRef("BottomLeft"), a);
            break;
        case ra::ui::viewmodels::PopupLocation::BottomMiddle:
            doc.AddMember(rapidjson::StringRef(sFieldName), rapidjson::StringRef("BottomMiddle"), a);
            break;
        case ra::ui::viewmodels::PopupLocation::BottomRight:
            doc.AddMember(rapidjson::StringRef(sFieldName), rapidjson::StringRef("BottomRight"), a);
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

    rapidjson::Document doc;
    doc.SetObject();

    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    doc.AddMember("Username", rapidjson::StringRef(m_sUsername), a);
    doc.AddMember("Token", rapidjson::StringRef(m_sApiToken), a);
    doc.AddMember("Hardcore Active", IsFeatureEnabled(Feature::Hardcore), a);
    doc.AddMember("Non Hardcore Warning", IsFeatureEnabled(Feature::NonHardcoreWarning), a);
    WritePopupLocation(doc, a, "Achievement Triggered Notification Display", GetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered));
    doc.AddMember("Achievement Triggered Screenshot", IsFeatureEnabled(Feature::AchievementTriggeredScreenshot), a);
    WritePopupLocation(doc, a, "Mastery Notification Display", GetPopupLocation(ra::ui::viewmodels::Popup::Mastery));
    doc.AddMember("Mastery Screenshot", IsFeatureEnabled(Feature::MasteryNotificationScreenshot), a);
    doc.AddMember("Leaderboards Active", IsFeatureEnabled(Feature::Leaderboards), a);
    WritePopupLocation(doc, a, "Leaderboard Notification Display", GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardStarted));
    WritePopupLocation(doc, a, "Leaderboard Cancel Display", GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardCanceled));
    WritePopupLocation(doc, a, "Leaderboard Counter Display", GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker));
    WritePopupLocation(doc, a, "Leaderboard Scoreboard Display", GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard));
    WritePopupLocation(doc, a, "Challenge Notification Display", GetPopupLocation(ra::ui::viewmodels::Popup::Challenge));
    WritePopupLocation(doc, a, "Informational Notification Display", GetPopupLocation(ra::ui::viewmodels::Popup::Message));
    doc.AddMember("Prefer Decimal", IsFeatureEnabled(Feature::PreferDecimal), a);
    doc.AddMember("Num Background Threads", m_nBackgroundThreads, a);

    if (!m_sRomDirectory.empty())
        doc.AddMember("ROM Directory", ra::Narrow(m_sRomDirectory), a);

    if (!m_sScreenshotDirectory.empty())
        doc.AddMember("Screenshot Directory", ra::Narrow(m_sScreenshotDirectory), a);

    rapidjson::Value positions(rapidjson::kObjectType);
    for (WindowPositionMap::const_iterator iter = m_mWindowPositions.begin(); iter != m_mWindowPositions.end(); ++iter)
    {
        rapidjson::Value rect(rapidjson::kObjectType);
        if (iter->second.oPosition.X != INT32_MIN)
            rect.AddMember("X", iter->second.oPosition.X, a);
        if (iter->second.oPosition.Y != INT32_MIN)
            rect.AddMember("Y", iter->second.oPosition.Y, a);
        if (iter->second.oSize.Width != INT32_MIN)
            rect.AddMember("Width", iter->second.oSize.Width, a);
        if (iter->second.oSize.Height != INT32_MIN)
            rect.AddMember("Height", iter->second.oSize.Height, a);

        if (rect.MemberCount() > 0)
            positions.AddMember(rapidjson::StringRef(iter->first), rect, a);
    }

    if (positions.MemberCount() > 0)
        doc.AddMember("Window Positions", positions.Move(), a);

    auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    auto pWriter = pFileSystem.CreateTextFile(m_sFilename);
    if (pWriter != nullptr)
        SaveDocument(doc, *pWriter);
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

void JsonFileConfiguration::SetWindowSize(const std::string & sPositionKey, const ra::ui::Size & oSize)
{
    m_mWindowPositions[sPositionKey].oSize = oSize;
}

const std::string& JsonFileConfiguration::GetHostName() const
{
    if (m_sHostName.empty())
        GSL_SUPPRESS_TYPE3 const_cast<JsonFileConfiguration*>(this)->UpdateHost();

    return m_sHostName;
}

const std::string& JsonFileConfiguration::GetHostUrl() const
{
    if (m_sHostUrl.empty())
        GSL_SUPPRESS_TYPE3 const_cast<JsonFileConfiguration*>(this)->UpdateHost();

    return m_sHostUrl;
}

const std::string& JsonFileConfiguration::GetImageHostUrl() const
{
    if (m_sImageHostUrl.empty())
        GSL_SUPPRESS_TYPE3 const_cast<JsonFileConfiguration*>(this)->UpdateHost();

    return m_sImageHostUrl;
}

void JsonFileConfiguration::UpdateHost()
{
    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    if (pFileSystem.GetFileSize(L"host.txt") > 0)
    {
        auto pFile = pFileSystem.OpenTextFile(L"host.txt");
        if (pFile != nullptr)
            pFile->GetLine(m_sHostName);
    }

    if (m_sHostName.empty())
    {
        m_sHostName = "retroachievements.org";
        m_sHostUrl = "https://retroachievements.org";
        m_sImageHostUrl = "http://i.retroachievements.org";
    }
    else
    {
        m_bCustomHost = true;

        const auto nIndex = m_sHostName.find("://");
        if (nIndex == std::string::npos)
        {
            m_sHostUrl = "http://" + m_sHostName;
        }
        else
        {
            m_sHostUrl.swap(m_sHostName);
            m_sHostName = m_sHostUrl.substr(nIndex + 3);
        }

        m_sImageHostUrl = m_sHostUrl;
    }

    RA_LOG_INFO("Using server: %s", m_sHostUrl.c_str());
}

} // namespace impl
} // namespace services
} // namespace ra
