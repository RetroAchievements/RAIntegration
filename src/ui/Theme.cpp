#include "EditorTheme.hh"
#include "OverlayTheme.hh"

#include "RA_Json.h"
#include "RA_Log.h"

#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {

static void ReadSize(int& nSize, const rapidjson::Value& pSizes, const char* pJsonField)
{
    if (pSizes.HasMember(pJsonField))
    {
        const auto& pField = pSizes[pJsonField];
        if (pField.IsInt())
            nSize = pField.GetInt();
    }
}

static void ReadBool(bool& bValue, const rapidjson::Value& pContainer, const char* pJsonField)
{
    if (pContainer.HasMember(pJsonField))
    {
        const auto& pField = pContainer[pJsonField];
        if (pField.IsBool())
            bValue = pField.GetBool();
    }
}

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
    if (pFileSystem.GetFileSize(sFullPath) == -1)
        return;

    auto pFile = pFileSystem.OpenTextFile(sFullPath);
    if (!pFile)
        return;

    rapidjson::Document document;
    if (!LoadDocument(document, *pFile))
    {
        RA_LOG_ERR("Unable to read Overlay\\theme.json: %s (%zu)",
                    GetParseError_En(document.GetParseError()), document.GetErrorOffset());
        return;
    }

    if (document.HasMember("Popup"))
    {
        const rapidjson::Value& popup = document["Popup"];

        if (popup.HasMember("Font"))
            m_sFontPopup = popup["Font"].GetString();

        if (popup.HasMember("FontSizes"))
        {
            const rapidjson::Value& sizes = popup["FontSizes"];

            ReadSize(m_nFontSizePopupTitle, sizes, "Title");
            ReadSize(m_nFontSizePopupSubtitle, sizes, "Subtitle");
            ReadSize(m_nFontSizePopupDetail, sizes, "Detail");
            ReadSize(m_nFontSizePopupLeaderboardTitle, sizes, "LeaderboardTitle");
            ReadSize(m_nFontSizePopupLeaderboardEntry, sizes, "LeaderboardEntry");
            ReadSize(m_nFontSizePopupLeaderboardTracker, sizes, "LeaderboardTracker");
        }

        if (popup.HasMember("Colors"))
        {
            const rapidjson::Value& colors = popup["Colors"];

            ReadColor(m_colorBackground, colors, "Background");
            ReadColor(m_colorMasteryBackground, colors, "MasteryBackground");
            ReadColor(m_colorNonHardcoreBackground, colors, "NonHardcoreBackground");
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

    if (document.HasMember("Overlay"))
    {
        const rapidjson::Value& overlay = document["Overlay"];

        if (overlay.HasMember("Font"))
            m_sFontOverlay = overlay["Font"].GetString();

        if (overlay.HasMember("FontSizes"))
        {
            const rapidjson::Value& sizes = overlay["FontSizes"];

            ReadSize(m_nFontSizeOverlayTitle, sizes, "Title");
            ReadSize(m_nFontSizeOverlayHeader, sizes, "Header");
            ReadSize(m_nFontSizeOverlaySummary, sizes, "Summary");
        }

        if (overlay.HasMember("Colors"))
        {
            const rapidjson::Value& colors = overlay["Colors"];

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

    ReadBool(m_bTransparent, document, "Transparent");
}

void EditorTheme::LoadFromFile()
{
    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    std::wstring sFullPath = pFileSystem.BaseDirectory() + L"Overlay\\editor_theme.json";
    if (pFileSystem.GetFileSize(sFullPath) == -1)
        return;

    auto pFile = pFileSystem.OpenTextFile(sFullPath);
    if (!pFile)
        return;

    rapidjson::Document document;
    if (!LoadDocument(document, *pFile))
    {
        RA_LOG_ERR("Unable to read Overlay\\editor_theme.json: %s (%zu)",
            GetParseError_En(document.GetParseError()), document.GetErrorOffset());
        return;
    }

    if (document.HasMember("MemoryViewer"))
    {
        const rapidjson::Value& memoryViewer = document["MemoryViewer"];

        if (memoryViewer.HasMember("Font"))
            m_sFontMemoryViewer = memoryViewer["Font"].GetString();

        if (memoryViewer.HasMember("FontSize"))
            ReadSize(m_nFontSizeMemoryViewer, memoryViewer, "FontSize");

        if (memoryViewer.HasMember("Colors"))
        {
            const rapidjson::Value& colors = memoryViewer["Colors"];

            ReadColor(m_colorBackground, colors, "Background");
            ReadColor(m_colorSeparator, colors, "Separator");
            ReadColor(m_colorCursor, colors, "Cursor");
            ReadColor(m_colorNormal, colors, "Normal");
            ReadColor(m_colorSelected, colors, "Selected");
            ReadColor(m_colorHasNote, colors, "HasNote");
            ReadColor(m_colorHasSurrogateNote, colors, "HasSurrogateNote");
            ReadColor(m_colorHasBookmark, colors, "HasBookmark");
            ReadColor(m_colorFrozen, colors, "Frozen");
            ReadColor(m_colorHeader, colors, "Header");
            ReadColor(m_colorHeaderSelected, colors, "HeaderSelected");
        }
    }

    if (document.HasMember("CodeNotes"))
    {
        const rapidjson::Value& codeNotes = document["CodeNotes"];

        if (codeNotes.HasMember("Colors"))
        {
            const rapidjson::Value& colors = codeNotes["Colors"];

            ReadColor(m_colorModified, colors, "Modified");
        }
    }

    if (document.HasMember("TriggerColors"))
    {
        const rapidjson::Value& triggerColors = document["TriggerColors"];
        ReadColor(m_colorTriggerIsTrue, triggerColors, "IsTrue");
        ReadColor(m_colorTriggerWasTrue, triggerColors, "WasTrue");
        ReadColor(m_colorTriggerBecomingTrue, triggerColors, "BecomingTrue");
        ReadColor(m_colorTriggerResetTrue, triggerColors, "ResetTrue");
        ReadColor(m_colorTriggerPauseTrue, triggerColors, "PauseTrue");
    }
}

} // namespace ui
} // namespace ra
