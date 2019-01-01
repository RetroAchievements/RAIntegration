#ifndef RA_UI_DESKTOP
#define RA_UI_DESKTOP

#include "ui/WindowViewModelBase.hh"

namespace ra {
namespace ui {

class IDesktop
{
public:
    virtual ~IDesktop() noexcept = default;
    IDesktop(const IDesktop&) noexcept = delete;
    IDesktop& operator=(const IDesktop&) noexcept = delete;
    IDesktop(IDesktop&&) noexcept = delete;
    IDesktop& operator=(IDesktop&&) noexcept = delete;

    /// <summary>
    /// Shows a window for the provided view model.
    /// </summary>
    virtual void ShowWindow(WindowViewModelBase& vmViewModel) const = 0;

    /// <summary>
    /// Shows a model window for the provided view model. Method will not return until the window is closed.
    /// </summary>
    virtual ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel) const = 0;
    
    /// <summary>
    /// Gets the bounds of the available work area of the desktop.
    /// </summary>
    /// <param name="oUpperLeftCorner">The upper left corner of the work area.</param>
    /// <param name="oSize">The size of the work area.</param>
    virtual void GetWorkArea(_Out_ ra::ui::Position& oUpperLeftCorner, _Out_ ra::ui::Size& oSize) const = 0;

    /// <summary>
    /// Opens the specified URL using the default browser.
    /// </summary>
    /// <param name="sUrl">The URL to open.</param>
    virtual void OpenUrl(const std::string& sUrl) const = 0;

    virtual void Shutdown() = 0;

protected:
    IDesktop() noexcept = default;
};

} // namespace ui
} // namespace ra

#endif // !RA_UI_DESKTOP
