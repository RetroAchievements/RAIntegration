#ifndef RA_UI_DESKTOP
#define RA_UI_DESKTOP

#include "ui/WindowViewModelBase.hh"
#include "ui/drawing/ISurface.hh"

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
    /// Shows a modal window for the provided view model. Method will not return until the window is closed.
    /// </summary>
    virtual ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel) const = 0;

    /// <summary>
    /// Shows a modal window for the provided view model. Method will not return until the window is closed.
    /// </summary>
    virtual ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel, const WindowViewModelBase& vmParentWindow) const = 0;

    /// <summary>
    /// Closes the window for the provided view model.
    /// </summary>
    virtual void CloseWindow(WindowViewModelBase& vmViewModel) const = 0;

    /// <summary>
    /// Gets the bounds of the available work area of the desktop.
    /// </summary>
    /// <param name="oUpperLeftCorner">The upper left corner of the work area.</param>
    /// <param name="oSize">The size of the work area.</param>
    virtual void GetWorkArea(_Out_ ra::ui::Position& oUpperLeftCorner, _Out_ ra::ui::Size& oSize) const = 0;

    /// <summary>
    /// Gets the size of the client area of a window.
    /// </summary>
    virtual ra::ui::Size GetClientSize(const WindowViewModelBase& vmViewModel) const = 0;

    /// <summary>
    /// Gets the path/filename of the currently running executable.
    /// </summary>
    virtual std::wstring GetRunningExecutable() const = 0;

    /// <summary>
    /// Gets a string representing the current operating system and its version.
    /// <summary>
    virtual std::string GetOSVersionString() const = 0;

    /// <summary>
    /// Opens the specified URL using the default browser.
    /// </summary>
    /// <param name="sUrl">The URL to open.</param>
    virtual void OpenUrl(const std::string& sUrl) const = 0;

    /// <summary>
    /// Captures the currently displayed contents of the specified window in an <see cref="ISurface" />
    /// </summary>
    virtual std::unique_ptr<ra::ui::drawing::ISurface> CaptureClientArea(const WindowViewModelBase& vmViewModel) const = 0;

    /// <summary>
    /// Returns <c>true</c> if an external tool is detected that can modify memory.
    /// </summary>
    virtual bool IsDebuggerPresent() const = 0;

    /// <summary>
    /// Returns <c>true</c> if the current thread is the UI thread.
    /// </summary>
    virtual bool IsOnUIThread(const WindowViewModelBase& vmViewModel) const = 0;

    /// <summary>
    /// Executes a function on the UI thread.
    /// </summary>
    virtual void InvokeOnUIThread(const WindowViewModelBase& vmViewModel, std::function<void()> fAction) const = 0;

    virtual void Shutdown() = 0;

protected:
    IDesktop() noexcept = default;
};

} // namespace ui
} // namespace ra

#endif // !RA_UI_DESKTOP
