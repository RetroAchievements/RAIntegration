#ifndef RA_UI_OVERLAY_THEME_H
#define RA_UI_OVERLAY_THEME_H
#pragma once

#include "ra_fwd.h"

#include "ui/Types.hh"

namespace ra {
namespace ui {

class OverlayTheme
{
public:
    GSL_SUPPRESS_F6 OverlayTheme() = default;
    virtual ~OverlayTheme() noexcept = default;
    OverlayTheme(const OverlayTheme&) noexcept = delete;
    OverlayTheme& operator=(const OverlayTheme&) noexcept = delete;
    OverlayTheme(OverlayTheme&&) noexcept = delete;
    OverlayTheme& operator=(OverlayTheme&&) noexcept = delete;

    // ===== popup style =====

    const std::string& FontPopup() const noexcept { return m_sFontPopup; }
    int FontSizePopupTitle() const noexcept { return m_nFontSizePopupTitle; }
    int FontSizePopupSubtitle() const noexcept { return m_nFontSizePopupSubtitle; }
    int FontSizePopupDetail() const noexcept { return m_nFontSizePopupDetail; }
    int FontSizePopupLeaderboardTitle() const noexcept { return m_nFontSizePopupLeaderboardTitle; }
    int FontSizePopupLeaderboardEntry() const noexcept { return m_nFontSizePopupLeaderboardEntry; }
    int FontSizePopupLeaderboardTracker() const noexcept { return m_nFontSizePopupLeaderboardTracker; }

    Color ColorBackground() const noexcept { return m_colorBackground; }
    Color ColorBorder() const noexcept { return m_colorBorder; }
    Color ColorTextShadow() const noexcept { return m_colorTextShadow; }
    Color ColorTitle() const noexcept { return m_colorTitle; }
    Color ColorDescription() const noexcept { return m_colorDescription; }
    Color ColorDetail() const noexcept { return m_colorDetail; }
    Color ColorError() const noexcept { return m_colorError; }
    Color ColorLeaderboardEntry() const noexcept { return m_colorLeaderboardEntry; }
    Color ColorLeaderboardPlayer() const noexcept { return m_colorLeaderboardPlayer; }

    Color ColorShadow() const noexcept { return Color{ 255, 0, 0, 0 }; }

    unsigned int ShadowOffset() const noexcept { return 2; }

    // ===== overlay style =====

    const std::string& FontOverlay() const noexcept { return m_sFontOverlay; }
    int FontSizeOverlayTitle() const noexcept { return m_nFontSizeOverlayTitle; }
    int FontSizeOverlayHeader() const noexcept { return m_nFontSizeOverlayHeader; }
    int FontSizeOverlaySummary() const noexcept { return m_nFontSizeOverlaySummary; }

    Color ColorOverlayPanel() const noexcept { return m_colorOverlayPanel; }
    Color ColorOverlayText() const noexcept { return m_colorOverlayText; }
    Color ColorOverlayDisabledText() const noexcept { return m_colorOverlayDisabledText; }
    Color ColorOverlaySubText() const noexcept { return m_colorOverlaySubText; }
    Color ColorOverlayDisabledSubText() const noexcept { return m_colorOverlayDisabledSubText; }
    Color ColorOverlaySelectionBackground() const noexcept { return m_colorOverlaySelectionBackground; }
    Color ColorOverlaySelectionText() const noexcept { return m_colorOverlaySelectionText; }
    Color ColorOverlaySelectionDisabledText() const noexcept { return m_colorOverlaySelectionDisabledText; }
    Color ColorOverlayScrollBar() const noexcept { return m_colorOverlayScrollBar; }
    Color ColorOverlayScrollBarGripper() const noexcept { return m_colorOverlayScrollBarGripper; }

    // ===== methods =====

    void LoadFromFile();

private:
    std::string m_sFontPopup = "Tahoma";
    int m_nFontSizePopupTitle = 26;
    int m_nFontSizePopupSubtitle = 18;
    int m_nFontSizePopupDetail = 16;
    int m_nFontSizePopupLeaderboardTitle = 22;
    int m_nFontSizePopupLeaderboardEntry = 18;
    int m_nFontSizePopupLeaderboardTracker = 18;

    Color m_colorBackground{ 255, 64, 64, 64 };
    Color m_colorBorder{ 255, 32, 32, 32 };
    Color m_colorTextShadow{ 255, 32, 32, 32 };
    Color m_colorTitle{ 255, 255, 255, 255 };
    Color m_colorDescription{ 255, 240, 224, 64 };
    Color m_colorDetail{ 255, 64, 192, 224 };
    Color m_colorError{ 255, 224, 32, 32 };
    Color m_colorLeaderboardEntry{ 255, 180, 180, 180 };
    Color m_colorLeaderboardPlayer{ 255, 240, 224, 128 };

    std::string m_sFontOverlay = "Tahoma";
    int m_nFontSizeOverlayTitle = 32;
    int m_nFontSizeOverlayHeader = 26;
    int m_nFontSizeOverlaySummary = 22;

    Color m_colorOverlayPanel{ 255, 32, 32, 32 };
    Color m_colorOverlayText{ 255, 17, 102, 221 };
    Color m_colorOverlayDisabledText{ 255, 116, 122, 144 };
    Color m_colorOverlaySubText{ 255, 140, 140, 140 };
    Color m_colorOverlayDisabledSubText{ 255, 96, 96, 96 };
    Color m_colorOverlaySelectionBackground{ 255, 22, 22, 60 };
    Color m_colorOverlaySelectionText{ 255, 255, 255, 255 };
    Color m_colorOverlaySelectionDisabledText{ 255, 200, 200, 200 };
    Color m_colorOverlayScrollBar{ 255, 64, 64, 64 };
    Color m_colorOverlayScrollBarGripper{ 255, 192, 192, 192 };
};

} // namespace ui
} // namespace ra

#endif RA_UI_OVERLAY_THEME_H
