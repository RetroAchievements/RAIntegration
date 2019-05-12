#ifndef RA_UI_DRAWING_GDI_RESOURCEREPOSITORY_HH
#define RA_UI_DRAWING_GDI_RESOURCEREPOSITORY_HH
#pragma once

#include "ui\Types.hh"

#include "ra_utility.h"

namespace ra {
namespace ui {
namespace drawing {
namespace gdi {

class ResourceRepository
{
public:
    ResourceRepository() noexcept = default;
    ~ResourceRepository() noexcept
    {
        for (auto& pFont : m_vFonts)
            DeleteFont(pFont.hFont);

        m_vFonts.clear();
    }

    ResourceRepository(const ResourceRepository&) = delete;
    ResourceRepository& operator=(const ResourceRepository&) = delete;
    ResourceRepository(ResourceRepository&&) = delete;
    ResourceRepository& operator=(ResourceRepository&&) = delete;

    int LoadFont(const std::string& sFont, int nFontSize, FontStyles nStyle)
    {
        int i = 1;
        for (const auto& pFont : m_vFonts)
        {
            if (pFont.nFontSize == nFontSize && pFont.nStyle == nStyle && pFont.sFontName == sFont)
                return i;
            ++i;
        }

        using namespace ra::bitwise_ops;
        const auto nWeight = ((nStyle & FontStyles::Bold) == FontStyles::Bold) ? FW_BOLD : FW_NORMAL;
        const auto nItalic = ((nStyle & FontStyles::Italic) == FontStyles::Italic) ? TRUE : FALSE;
        const auto nUnderline = ((nStyle & FontStyles::Underline) == FontStyles::Underline) ? TRUE : FALSE;
        const auto nStrikeOut = ((nStyle & FontStyles::Strikethrough) == FontStyles::Strikethrough) ? TRUE : FALSE;

        HFONT hFont = CreateFontA(nFontSize, 0, 0, 0, nWeight, nItalic, nUnderline, nStrikeOut,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, sFont.c_str());

        if (hFont)
        {
            m_vFonts.emplace_back(sFont, nFontSize, nStyle, hFont);
            return m_vFonts.size();
        }

        return 0;
    }

    HFONT GetHFont(int nFont)
    {
        if (nFont > 0 && ra::to_unsigned(nFont) <= m_vFonts.size())
            return m_vFonts.at(nFont - 1).hFont;

        return nullptr;
    }

private:
    struct GDIFont
    {
        GDIFont(const std::string& sFontName, int nFontSize, FontStyles nStyle, HFONT hFont)
            : sFontName(sFontName), nFontSize(nFontSize), nStyle(nStyle), hFont(hFont)
        {
        }

        std::string sFontName;
        int nFontSize{};
        FontStyles nStyle{};
        HFONT hFont{};
    };
    std::vector<GDIFont> m_vFonts;
};

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra

#endif /* !RA_UI_DRAWING_GDI_RESOURCEREPOSITORY_HH */
