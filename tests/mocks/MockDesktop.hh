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
    explicit MockDesktop() noexcept
        : m_Override(this)
    {
    }

    void ShowWindow(WindowViewModelBase& vmViewModel) const override
    {
        Handle(vmViewModel);
        m_bDialogShown = true;
    }

    ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel) const override
    {
        m_bDialogShown = true;
        return Handle(vmViewModel);
    }

    ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel, const WindowViewModelBase&) const override
    {
        return ShowModal(vmViewModel);
    }

    void CloseWindow(WindowViewModelBase& vmViewModel) const override
    {
        vmViewModel.SetIsVisible(false);
    }

    bool WasDialogShown() noexcept { return m_bDialogShown; }

    void GetWorkArea(ra::ui::Position& oUpperLeftCorner, ra::ui::Size& oSize) const noexcept override
    {
        oUpperLeftCorner = { 0, 0 };
        oSize = { 1920, 1080 };
    }

    ra::ui::Size GetClientSize(const WindowViewModelBase&) const noexcept override
    {
        return { 800, 600 };
    }

    void OpenUrl(const std::string& sUrl) const override
    {
        m_sLastOpenedUrl = sUrl;
    }

    std::unique_ptr<ra::ui::drawing::ISurface> CaptureClientArea(const WindowViewModelBase&) const noexcept override
    {
        return {};
    }

    const std::string& LastOpenedUrl() const noexcept { return m_sLastOpenedUrl; }

    void Shutdown() noexcept override {}

    template<typename T>
    void ExpectWindow(std::function<ra::ui::DialogResult(T&)>&& fnHandler)
    {
        auto pHandler = [fnHandler = std::move(fnHandler)](WindowViewModelBase& vmWindow)
        {
            auto* pWindow = dynamic_cast<T*>(&vmWindow);
            if (pWindow != nullptr)
                return fnHandler(*pWindow);

            return ra::ui::DialogResult::None;
        };

        m_vHandlers.emplace_back(pHandler);
    }

    void ResetExpectedWindows() noexcept { m_bDialogShown = false; m_vHandlers.clear(); }

    std::wstring GetRunningExecutable() const override { return m_sExecutable; }
    void SetRunningExecutable(const std::wstring& sExecutable) { m_sExecutable = sExecutable; }

    std::string GetOSVersionString() const override { return "UnitTests"; }

    bool IsDebuggerPresent() const noexcept override { return m_bDebuggerPresent; }
    void SetDebuggerPresent(bool bValue) noexcept { m_bDebuggerPresent = bValue; }

private:
    ra::ui::DialogResult Handle(WindowViewModelBase& vmViewModel) const
    {
        for (auto& pHandler : m_vHandlers)
        {
            auto nResult = pHandler(vmViewModel);
            if (nResult != ra::ui::DialogResult::None)
                return nResult;
        }

        vmViewModel.SetIsVisible(true);
        return ra::ui::DialogResult::None;
    }

    ra::services::ServiceLocator::ServiceOverride<IDesktop> m_Override;
    std::vector<DialogHandler> m_vHandlers;
    mutable bool m_bDialogShown = false;
    mutable std::string m_sLastOpenedUrl;
    std::wstring m_sExecutable;
    bool m_bDebuggerPresent = false;
};

} // namespace mocks
} // namespace ui
} // namespace ra

#endif // !RA_SERVICES_MOCK_DESKTOP_HH
