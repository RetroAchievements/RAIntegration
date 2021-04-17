#ifndef RA_UI_EDITOR_THEME_H
#define RA_UI_EDITOR_THEME_H
#pragma once

#include "ra_fwd.h"

#include "ui/Types.hh"

namespace ra {
namespace ui {

class EditorTheme
{
public:
    GSL_SUPPRESS_F6 EditorTheme() = default;
    virtual ~EditorTheme() noexcept = default;
    EditorTheme(const EditorTheme&) noexcept = delete;
    EditorTheme& operator=(const EditorTheme&) noexcept = delete;
    EditorTheme(EditorTheme&&) noexcept = delete;
    EditorTheme& operator=(EditorTheme&&) noexcept = delete;

    // ===== memory viewer =====

    const std::string& FontMemoryViewer() const noexcept { return m_sFontMemoryViewer; }
    int FontSizeMemoryViewer() const noexcept { return m_nFontSizeMemoryViewer; }

    Color ColorBackground() const noexcept { return m_colorBackground; }
    Color ColorSeparator() const noexcept { return m_colorSeparator; }
    Color ColorCursor() const noexcept { return m_colorCursor; }
    Color ColorNormal() const noexcept { return m_colorNormal; }
    Color ColorSelected() const noexcept { return m_colorSelected; }
    Color ColorHasNote() const noexcept { return m_colorHasNote; }
    Color ColorHasBookmark() const noexcept { return m_colorHasBookmark; }
    Color ColorFrozen() const noexcept { return m_colorFrozen; }
    Color ColorHeader() const noexcept { return m_colorHeader; }
    Color ColorHeaderSelected() const noexcept { return m_colorHeaderSelected; }

    // ===== triggers =====

    Color ColorTriggerIsTrue() const noexcept { return m_colorTriggerIsTrue; }
    Color ColorTriggerWasTrue() const noexcept { return m_colorTriggerWasTrue; }
    Color ColorTriggerBecomingTrue() const noexcept { return m_colorTriggerBecomingTrue; }
    Color ColorTriggerResetTrue() const noexcept { return m_colorTriggerResetTrue; }
    Color ColorTriggerPauseTrue() const noexcept { return m_colorTriggerPauseTrue; }

    // ===== methods =====

    void LoadFromFile();

private:
    std::string m_sFontMemoryViewer = "Consolas";
    int m_nFontSizeMemoryViewer = 17;

    Color m_colorBackground{ 255, 255, 255, 255 };
    Color m_colorSeparator{ 255, 192, 192, 192 };
    Color m_colorCursor{ 255, 192, 192, 192 };
    Color m_colorNormal{ 255, 0, 0, 0 };
    Color m_colorSelected{ 255, 255, 0, 0 };
    Color m_colorHasNote{ 255, 0, 0, 255 };
    Color m_colorHasBookmark{ 255, 0, 160, 0 };
    Color m_colorFrozen{ 255, 255, 200, 0 };
    Color m_colorHeader{ 255, 128, 128, 128 };
    Color m_colorHeaderSelected{ 255, 255, 96, 96 };

    Color m_colorTriggerIsTrue{ 255, 192, 255, 192 };
    Color m_colorTriggerWasTrue{ 255, 192, 192, 255 };
    Color m_colorTriggerBecomingTrue{ 255, 192, 255, 255 };
    Color m_colorTriggerResetTrue{ 255, 255, 255, 192 };
    Color m_colorTriggerPauseTrue{ 255, 255, 192, 192 };
};

} // namespace ui
} // namespace ra

#endif RA_UI_EDITOR_THEME_H
