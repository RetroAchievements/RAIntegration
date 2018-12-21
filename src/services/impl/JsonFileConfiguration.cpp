#include "JsonFileConfiguration.hh"

#include "RA_Json.h"
#include "RA_Log.h"
#include "ra_utility.h"

#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace impl {

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
        (1 << static_cast<int>(Feature::Leaderboards)) |
        (1 << static_cast<int>(Feature::LeaderboardNotifications)) |
        (1 << static_cast<int>(Feature::LeaderboardCounters)) |
        (1 << static_cast<int>(Feature::LeaderboardScoreboards));

    RA_LOG("Loading preferences...");

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

    if (doc.HasMember("Leaderboards Active"))
        SetFeatureEnabled(Feature::Leaderboards, doc["Leaderboards Active"].GetBool());
    if (doc.HasMember("Leaderboard Notification Display"))
        SetFeatureEnabled(Feature::LeaderboardNotifications, doc["Leaderboard Notification Display"].GetBool());
    if (doc.HasMember("Leaderboard Counter Display"))
        SetFeatureEnabled(Feature::LeaderboardCounters, doc["Leaderboard Counter Display"].GetBool());
    if (doc.HasMember("Leaderboard Scoreboard Display"))
        SetFeatureEnabled(Feature::LeaderboardScoreboards, doc["Leaderboard Scoreboard Display"].GetBool());

    if (doc.HasMember("Prefer Decimal"))
        SetFeatureEnabled(Feature::PreferDecimal, doc["Prefer Decimal"].GetBool());

    if (doc.HasMember("Num Background Threads"))
        m_nBackgroundThreads = doc["Num Background Threads"].GetUint();
    if (doc.HasMember("ROM Directory"))
        m_sRomDirectory = doc["ROM Directory"].GetString();

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

void JsonFileConfiguration::Save() const
{
    RA_LOG("Saving preferences...");

    if (m_sFilename.empty())
    {
        RA_LOG(" - Aborting save, we don't know where to write...");
        return;
    }

    rapidjson::Document doc;
    doc.SetObject();

    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    doc.AddMember("Username", rapidjson::StringRef(m_sUsername), a);
    doc.AddMember("Token", rapidjson::StringRef(m_sApiToken), a);
    doc.AddMember("Hardcore Active", IsFeatureEnabled(Feature::Hardcore), a);
    doc.AddMember("Leaderboards Active", IsFeatureEnabled(Feature::Leaderboards), a);
    doc.AddMember("Leaderboard Notification Display", IsFeatureEnabled(Feature::LeaderboardNotifications), a);
    doc.AddMember("Leaderboard Counter Display", IsFeatureEnabled(Feature::LeaderboardCounters), a);
    doc.AddMember("Leaderboard Scoreboard Display", IsFeatureEnabled(Feature::LeaderboardScoreboards), a);
    doc.AddMember("Prefer Decimal", IsFeatureEnabled(Feature::PreferDecimal), a);
    doc.AddMember("Num Background Threads", m_nBackgroundThreads, a);

    if (!m_sRomDirectory.empty())
        doc.AddMember("ROM Directory", rapidjson::StringRef(m_sRomDirectory), a);

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
    {
        const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
        if (pFileSystem.GetFileSize(L"host.txt") > 0)
        {
            auto pFile = pFileSystem.OpenTextFile(L"host.txt");
            if (pFile != nullptr)
                pFile->GetLine(m_sHostName);
        }

        if (m_sHostName.empty())
            m_sHostName = "retroachievements.org";
    }

    return m_sHostName;
}

} // namespace impl
} // namespace services
} // namespace ra
