#ifndef RA_UI_WIN32_DESKTOP
#define RA_UI_WIN32_DESKTOP

#include "ui/IDesktop.hh"
#include "ui/win32/IDialogController.hh"

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
    ra::ui::win32::IDialogController* GetDialogController(WindowViewModelBase& oViewModel) const;

    std::vector<ra::ui::win32::IDialogController*> m_vDialogControllers;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_DESKTOP
