#include "OverlayTheme.hh"

#include "RA_Json.h"

#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {

static void ReadColor(Color& nColor, const rapidjson::Value& pColors, const char* pJsonField)
{
    if (pColors.HasMember(pJsonField))
    {
        const auto& pField = pColors[pJsonField];
        if (pField.IsString())
        {
            std::string sValue = pField.GetString();
            if (sValue.length() == 7 && sValue.at(0) == '#')
                sValue.erase(sValue.begin());

            if (sValue.length() == 6)
            {
                char* pEnd;
                auto nValue = strtoul(sValue.c_str(), &pEnd, 16);
                if (*pEnd == '\0')
                    nColor = Color(nValue | 0xFF000000);
            }
        }
    }
}

void OverlayTheme::LoadFromFile()
{
    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    std::wstring sFullPath = pFileSystem.BaseDirectory() + L"Overlay\\theme.json";
    auto pFile = pFileSystem.OpenTextFile(sFullPath);
    if (pFile)
    {
        rapidjson::Document document;
        if (LoadDocument(document, *pFile))
        {
            if (document.HasMember("Colors"))
            {
                const rapidjson::Value& colors = document["Colors"];

                ReadColor(m_colorBackground, colors, "Background");
                ReadColor(m_colorBorder, colors, "Border");
                ReadColor(m_colorTextShadow, colors, "TextShadow");
                ReadColor(m_colorTitle, colors, "Title");
                ReadColor(m_colorDescription, colors, "Description");
                ReadColor(m_colorDetail, colors, "Detail");
                ReadColor(m_colorError, colors, "Error");
                ReadColor(m_colorLeaderboardEntry, colors, "LeaderboardEntry");
                ReadColor(m_colorLeaderboardPlayer, colors, "LeaderboardPlayer");
            }
        }
    }

}

} // namespace ui
} // namespace ra
