#ifndef RA_UI_DESKTOP
#define RA_UI_DESKTOP

#include "ui/WindowViewModelBase.hh"

namespace ra {
namespace ui {

class IDesktop
{
public:
    virtual ~IDesktop() noexcept = default;

    /// <summary>
    /// Shows a window for the provided view model.
    /// </summary>
    virtual void ShowWindow(WindowViewModelBase& vmViewModel) const = 0;

    /// <summary>
    /// Shows a model window for the provided view model. Method will not return until the window is closed.
    /// </summary>
    virtual ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel) const = 0;

    virtual void Shutdown() = 0;
};

} // namespace ui
} // namespace ra

#endif // !RA_UI_DESKTOP
