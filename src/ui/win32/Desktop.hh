#ifndef RA_UI_WIN32_DESKTOP
#define RA_UI_WIN32_DESKTOP

#include "ui/IDesktop.hh"
#include "ui/win32/IDialogPresenter.hh"

#include <memory>

namespace ra {
namespace ui {
namespace win32 {

class Desktop : public IDesktop
{
public:
    Desktop() noexcept;

    void ShowWindow(WindowViewModelBase& oViewModel) const override;
    ra::ui::DialogResult ShowModal(WindowViewModelBase& oViewModel) const override;

    void Shutdown() override;

private:
    ra::ui::win32::IDialogPresenter* GetDialogController(WindowViewModelBase& oViewModel) const;

    std::vector<std::unique_ptr<ra::ui::win32::IDialogPresenter>> m_vDialogPresenters;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_DESKTOP
