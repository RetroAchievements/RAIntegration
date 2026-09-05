#ifndef RA_SERVICES_MOCK_WINDOWCONFIGURATION_HH
#define RA_SERVICES_MOCK_WINDOWCONFIGURATION_HH
#pragma once

#include "services\IWindowConfiguration.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace mocks {

class MockWindowConfiguration : public IWindowConfiguration
{
public:
    MockWindowConfiguration() noexcept
        : m_Override(this)
    {
    }

    ra::ui::viewmodels::PopupLocation GetPopupLocation(ra::ui::viewmodels::Popup nPopup) const override
    {
        return m_vPopupLocations.at(ra::etoi(nPopup));
    }

    void SetPopupLocation(ra::ui::viewmodels::Popup nPopup, ra::ui::viewmodels::PopupLocation nPopupLocation) override
    {
        m_vPopupLocations.at(ra::etoi(nPopup)) = nPopupLocation;
    }

    ra::ui::Position GetWindowPosition([[maybe_unused]] const std::string& /*sPositionKey*/) const noexcept override
    {
        assert(!"Not implemented");
        return ra::ui::Position();
    }

    void SetWindowPosition([[maybe_unused]] const std::string& /*sPositionKey*/,
                           [[maybe_unused]] const ra::ui::Position& /*oPosition*/) noexcept override
    {
        assert(!"Not implemented");
    }

    ra::ui::Size GetWindowSize([[maybe_unused]] const std::string& /*sPositionKey*/) const noexcept override
    {
        assert(!"Not implemented");
        return ra::ui::Size();
    }

    void SetWindowSize([[maybe_unused]] const std::string& /*sPositionKey*/,
                       [[maybe_unused]] const ra::ui::Size& /*oSize*/) noexcept override
    {
        assert(!"Not implemented");
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::IWindowConfiguration> m_Override;

    std::array<ra::ui::viewmodels::PopupLocation, ra::etoi(ra::ui::viewmodels::Popup::NumPopups)> m_vPopupLocations = {};
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_WINDOWCONFIGURATION_HH
