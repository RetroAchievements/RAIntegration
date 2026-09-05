#ifndef RA_SERVICES_IWINDOWCONFIGURATION
#define RA_SERVICES_IWINDOWCONFIGURATION
#pragma once

#include "ui\Types.hh"
#include "ui\viewmodels\PopupViewModelBase.hh"

namespace ra {
namespace services {

class IWindowConfiguration
{
public:
    virtual ~IWindowConfiguration() noexcept = default;
    IWindowConfiguration(const IWindowConfiguration&) noexcept = delete;
    IWindowConfiguration& operator=(const IWindowConfiguration&) noexcept = delete;
    IWindowConfiguration(IWindowConfiguration&&) noexcept = delete;
    IWindowConfiguration& operator=(IWindowConfiguration&&) noexcept = delete;

    /// <summary>
    /// Gets where the specified popup should be displayed.
    /// </summary>
    virtual ra::ui::viewmodels::PopupLocation GetPopupLocation(ra::ui::viewmodels::Popup nPopup) const = 0;

    /// <summary>
    /// Sets where the specified popup should be displayed.
    /// </summary>
    virtual void SetPopupLocation(ra::ui::viewmodels::Popup nPopup, ra::ui::viewmodels::PopupLocation nPopupLocation) = 0;

    /// <summary>
    /// Gets the remembered position of the window identified by <paramref name="sPositionKey"/>.
    /// </summary>
    virtual ra::ui::Position GetWindowPosition(const std::string& sPositionKey) const = 0;

    /// <summary>
    /// Sets the position to remember for the window identified by <paramref name="sPositionKey"/>.
    /// </summary>
    virtual void SetWindowPosition(const std::string& sPositionKey, const ra::ui::Position& oPosition) = 0;

    /// <summary>
    /// Gets the remembered size of the window identified by <paramref name="sPositionKey"/>.
    /// </summary>
    virtual ra::ui::Size GetWindowSize(const std::string& sPositionKey) const = 0;

    /// <summary>
    /// Sets the size to remember for the window identified by <paramref name="sPositionKey"/>.
    /// </summary>
    virtual void SetWindowSize(const std::string& sPositionKey, const ra::ui::Size& oSize) = 0;

protected:
    IWindowConfiguration() noexcept = default;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_IWINDOWCONFIGURATION
