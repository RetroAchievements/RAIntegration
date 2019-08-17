#include "WindowViewModelBase.hh"

#include "services/ServiceLocator.hh"
#include "ui/IDesktop.hh"

namespace ra {
namespace ui {

StringModelProperty WindowViewModelBase::WindowTitleProperty("WindowViewModelBase", "WindowTitle", L"Window");
const IntModelProperty WindowViewModelBase::DialogResultProperty("WindowViewModelBase", "DialogResult", static_cast<int>(DialogResult::None));
BoolModelProperty WindowViewModelBase::IsVisibleProperty("WindowViewModelBase", "IsVisible", false);

const Color Color::Transparent(0x00FF01FE);

DialogResult WindowViewModelBase::ShowModal()
{
    return ra::services::ServiceLocator::Get<ra::ui::IDesktop>().ShowModal(*this);
}

DialogResult WindowViewModelBase::ShowModal(const ra::ui::WindowViewModelBase& vmParentWindow)
{
    return ra::services::ServiceLocator::Get<ra::ui::IDesktop>().ShowModal(*this, vmParentWindow);
}

void WindowViewModelBase::Show()
{
    return ra::services::ServiceLocator::Get<ra::ui::IDesktop>().ShowWindow(*this);
}

} // namespace ui
} // namespace ra
