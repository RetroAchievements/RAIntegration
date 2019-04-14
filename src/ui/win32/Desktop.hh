#ifndef RA_UI_WIN32_DESKTOP
#define RA_UI_WIN32_DESKTOP

#include "ui/IDesktop.hh"
#include "ui/win32/IDialogPresenter.hh"
#include "ui/win32/bindings/WindowBinding.hh"

namespace ra {
namespace ui {
namespace win32 {

class Desktop : public IDesktop
{
public:
    GSL_SUPPRESS_F6 Desktop() noexcept;

    void ShowWindow(WindowViewModelBase& vmViewModel) const override;
    ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel) const override;
    ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel, const WindowViewModelBase& vmParentViewModel) const override;

    void GetWorkArea(ra::ui::Position& oUpperLeftCorner, ra::ui::Size& oSize) const override;
    ra::ui::Size GetClientSize(const WindowViewModelBase& vmViewModel) const noexcept override;

    void OpenUrl(const std::string& sUrl) const noexcept override;

    void Shutdown() noexcept override;

    void SetMainHWnd(HWND hWnd);

private:
    _Success_(return != nullptr)
    _NODISCARD IDialogPresenter* GetDialogPresenter(_In_ const WindowViewModelBase& oViewModel) const;

    std::vector<std::unique_ptr<ra::ui::win32::IDialogPresenter>> m_vDialogPresenters;

    std::unique_ptr<ra::ui::win32::bindings::WindowBinding> m_pWindowBinding;

    mutable ra::ui::Position m_oWorkAreaPosition{};
    mutable ra::ui::Size m_oWorkAreaSize{};
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_DESKTOP
