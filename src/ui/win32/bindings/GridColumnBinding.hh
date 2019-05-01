#ifndef RA_UI_WIN32_GRIDCOLUMNBINDING_H
#define RA_UI_WIN32_GRIDCOLUMNBINDING_H
#pragma once

#include "ui\ViewModelCollection.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridColumnBinding
{
public:
    void SetHeader(const std::wstring& sHeader) noexcept { m_sHeader = sHeader; }
    const std::wstring& GetHeader() const noexcept { return m_sHeader; }

    enum class WidthType
    {
        Pixels,
        Percentage,
        Fill
    };

    void SetWidth(WidthType nType, int nAmount) noexcept
    {
        m_nWidthType = nType;
        m_nWidth = nAmount;
    }

    WidthType GetWidthType() const noexcept { return m_nWidthType; }
    int GetWidth() const noexcept { return m_nWidth; }

    virtual std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const = 0;

protected:
    std::wstring m_sHeader;
    WidthType m_nWidthType = WidthType::Fill;
    int m_nWidth = 1;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDCOLUMNBINDING_H
