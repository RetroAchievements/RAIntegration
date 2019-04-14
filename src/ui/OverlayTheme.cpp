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
                const auto nValue = strtoul(sValue.c_str(), &pEnd, 16);
                if (pEnd && *pEnd == '\0')
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
            if (document.HasMember("PopupColors"))
            {
                const rapidjson::Value& colors = document["PopupColors"];

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

            if (document.HasMember("OverlayColors"))
            {
                const rapidjson::Value& colors = document["OverlayColors"];

                ReadColor(m_colorOverlayPanel, colors, "Panel");
                ReadColor(m_colorOverlayText, colors, "Text");
                ReadColor(m_colorOverlayDisabledText, colors, "DisabledText");
                ReadColor(m_colorOverlaySubText, colors, "SubText");
                ReadColor(m_colorOverlayDisabledSubText, colors, "DisabledSubText");
                ReadColor(m_colorOverlaySelectionBackground, colors, "SelectionBackground");
                ReadColor(m_colorOverlaySelectionText, colors, "SelectionText");
                ReadColor(m_colorOverlaySelectionDisabledText, colors, "SelectionDisabledText");
                ReadColor(m_colorOverlayScrollBar, colors, "ScrollBar");
                ReadColor(m_colorOverlayScrollBarGripper, colors, "ScrollBarGripper");
            }
        }
    }

}

} // namespace ui
} // namespace ra
