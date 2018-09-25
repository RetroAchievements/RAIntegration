#ifndef RA_UI_WIN32_IDIALOGCONTROLLER_HH
#define RA_UI_WIN32_IDIALOGCONTROLLER_HH

#include "ui/WindowViewModelBase.hh"

namespace ra {
namespace ui {
namespace win32 {

class IDialogController
{
public:
    virtual ~IDialogController() noexcept = default;

    /// <summary>
    /// Determines whether the specified view model can be shown by this controller.
    /// </summary>
    virtual bool IsSupported(const ra::ui::WindowViewModelBase& oViewModel) = 0;

    /// <summary>
    /// Shows the window associated to the provided view model.
    /// </summary>
    virtual void ShowWindow(ra::ui::WindowViewModelBase& oViewModel) = 0;

    /// <summary>
    /// Shows a modal window for the provided view model.
    /// </summary>
    virtual void ShowModal(ra::ui::WindowViewModelBase& oViewModel) = 0;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_IDIALOGCONTROLLER_HH
