#ifndef RA_SERVICES_MOCK_DESKTOP_HH
#define RA_SERVICES_MOCK_DESKTOP_HH
#pragma once

#include "services\ServiceLocator.hh"

#include "ui\IDesktop.hh"

namespace ra {
namespace ui {
namespace mocks {

using DialogHandler = std::function<ra::ui::DialogResult(WindowViewModelBase&)>;

class MockDesktop : public IDesktop
{
public:
    explicit MockDesktop(DialogHandler&& pHandler) noexcept
        : m_Override(this), m_pHandler(pHandler)
    {
    }

    void ShowWindow(WindowViewModelBase& vmViewModel) const override
    {
        m_pHandler(vmViewModel);
        m_bDialogShown = true;
    }

    ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel) const override
    {
        m_bDialogShown = true;
        return m_pHandler(vmViewModel);
    }

    bool WasDialogShown() { return m_bDialogShown; }

    void GetWorkArea(ra::ui::Position& oUpperLeftCorner, ra::ui::Size& oSize) const override
    {
        oUpperLeftCorner = { 0, 0 };
        oSize = { 1920, 1080 };
    }

    void Shutdown() override {}

private:
    ra::services::ServiceLocator::ServiceOverride<IDesktop> m_Override;
    DialogHandler m_pHandler = nullptr;
    mutable bool m_bDialogShown = false;
};

} // namespace mocks
} // namespace ui
} // namespace ra

#endif // !RA_SERVICES_MOCK_DESKTOP_HH
