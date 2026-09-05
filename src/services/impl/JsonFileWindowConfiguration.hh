#ifndef RA_SERVICES_JSON_FILE_WINDOWCONFIGUATION_HH
#define RA_SERVICES_JSON_FILE_WINDOWCONFIGUATION_HH
#pragma once

#include "services\IWindowConfiguration.hh"

namespace ra {
namespace services {
namespace impl {

class JsonFileWindowConfiguration : public IWindowConfiguration
{
public:
    JsonFileWindowConfiguration(IWindowConfiguration& pParent) noexcept
        : m_pParent(pParent)
    {
    }

    ra::ui::viewmodels::PopupLocation GetPopupLocation(ra::ui::viewmodels::Popup nPopup) const override
    {
        return m_pParent.GetPopupLocation(nPopup);
    }
    void SetPopupLocation(ra::ui::viewmodels::Popup nPopup, ra::ui::viewmodels::PopupLocation nPopupLocation) override
    {
        m_pParent.SetPopupLocation(nPopup, nPopupLocation);
    }

    ra::ui::Position GetWindowPosition(const std::string& sPositionKey) const override
    {
        return m_pParent.GetWindowPosition(sPositionKey);
    }
    void SetWindowPosition(const std::string& sPositionKey, const ra::ui::Position& oPosition) override
    {
        m_pParent.SetWindowPosition(sPositionKey, oPosition);
    }

    ra::ui::Size GetWindowSize(const std::string& sPositionKey) const override
    {
        return m_pParent.GetWindowSize(sPositionKey);
    }
    void SetWindowSize(const std::string& sPositionKey, const ra::ui::Size& oSize) override
    {
        m_pParent.SetWindowSize(sPositionKey, oSize);
    }

private:
    IWindowConfiguration& m_pParent;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_JSON_FILE_WINDOWCONFIGUATION_HH
