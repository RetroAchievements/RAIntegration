#ifndef RA_UI_WIN32_DESKTOP
#define RA_UI_WIN32_DESKTOP

#include "ui/IDesktop.hh"
#include "ui/win32/IDialogPresenter.hh"

namespace ra {
namespace ui {
namespace win32 {

class Desktop : public IDesktop
{
public:
    /* clang-format off */
    GSL_SUPPRESS(f.6) Desktop() noexcept;
    /* clang-format on */

    void ShowWindow(WindowViewModelBase& oViewModel) const override;
    ra::ui::DialogResult ShowModal(WindowViewModelBase& oViewModel) const override;

    void GetWorkArea(ra::ui::Position& oUpperLeftCorner, ra::ui::Size& oSize) const override;

    void Shutdown() noexcept override;

private:
    _Success_(return != nullptr)
    _NODISCARD IDialogPresenter* GetDialogPresenter(_In_ const WindowViewModelBase& oViewModel) const;

    std::vector<std::unique_ptr<ra::ui::win32::IDialogPresenter>> m_vDialogPresenters;

    mutable ra::ui::Position m_oWorkAreaPosition{};
    mutable ra::ui::Size m_oWorkAreaSize{};
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_DESKTOP
