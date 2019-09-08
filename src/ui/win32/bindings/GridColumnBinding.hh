#ifndef RA_UI_WIN32_GRIDCOLUMNBINDING_H
#define RA_UI_WIN32_GRIDCOLUMNBINDING_H
#pragma once

#include "ui\Types.hh"
#include "ui\ViewModelCollection.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridColumnBinding
{
public:
    GridColumnBinding() noexcept = default;
    virtual ~GridColumnBinding() noexcept = default;

    GridColumnBinding(const GridColumnBinding&) noexcept = delete;
    GridColumnBinding& operator=(const GridColumnBinding&) noexcept = delete;
    GridColumnBinding(GridColumnBinding&&) noexcept = delete;
    GridColumnBinding& operator=(GridColumnBinding&&) noexcept = delete;

    void SetHeader(const std::wstring& sHeader) { m_sHeader = sHeader; }
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

    virtual bool DependsOn(const ra::ui::BoolModelProperty&) const noexcept(false) { return false; }
    virtual bool DependsOn(const ra::ui::IntModelProperty&) const noexcept(false) { return false; }
    virtual bool DependsOn(const ra::ui::StringModelProperty&) const noexcept(false) { return false; }

    ra::ui::RelativePosition GetAlignment() const noexcept { return m_nAlignment; }
    void SetAlignment(ra::ui::RelativePosition value) noexcept { m_nAlignment = value; }

    bool IsReadOnly() const noexcept { return m_bReadOnly; }
    void SetReadOnly(bool value) noexcept { m_bReadOnly = value; }
    
    struct InPlaceEditorInfo
    {
        gsl::index nItemIndex;
        gsl::index nColumnIndex;
        RECT rcSubItem;
        WNDPROC pOriginalWndProc;
        void* pGridBinding;
        GridColumnBinding* pColumnBinding;
    };

    virtual HWND CreateInPlaceEditor(HWND, InPlaceEditorInfo&) noexcept(false) { return nullptr; }

protected:
    std::wstring m_sHeader;
    WidthType m_nWidthType = WidthType::Fill;
    int m_nWidth = 1;
    ra::ui::RelativePosition m_nAlignment = ra::ui::RelativePosition::Near;
    bool m_bReadOnly = true;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDCOLUMNBINDING_H
