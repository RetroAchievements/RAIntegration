#ifndef RA_UI_OVERLAY_THEME_H
#define RA_UI_OVERLAY_THEME_H
#pragma once

#include "ui/Types.hh"

namespace ra {
namespace ui {

class OverlayTheme
{
public:
    OverlayTheme() noexcept = default;
    virtual ~OverlayTheme() noexcept = default;
    OverlayTheme(const OverlayTheme&) noexcept = delete;
    OverlayTheme& operator=(const OverlayTheme&) noexcept = delete;
    OverlayTheme(OverlayTheme&&) noexcept = delete;
    OverlayTheme& operator=(OverlayTheme&&) noexcept = delete;

    // ===== popup style =====

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

    // ===== methods =====

    void LoadFromFile();

private:
    Color m_colorBackground{ 255, 251, 102, 0 };
    Color m_colorBorder{ 255, 96, 48, 0 };
    Color m_colorTextShadow{ 255, 180, 80, 0 };
    Color m_colorTitle{ 255, 0, 0, 0 };
    Color m_colorDescription{ 255, 0, 0, 0 };
    Color m_colorDetail{ 255, 0, 0, 0 };
    Color m_colorError{ 255, 128, 0, 0 };
    Color m_colorLeaderboardEntry{ 255, 0, 0, 0 };
    Color m_colorLeaderboardPlayer{ 255, 96, 40, 0 };
};

} // namespace ui
} // namespace ra

#endif RA_UI_OVERLAY_THEME_H
