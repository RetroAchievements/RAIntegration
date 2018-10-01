#include "WindowViewModelBase.hh"

#include "services/ServiceLocator.hh"
#include "ui/IDesktop.hh"

namespace ra {
namespace ui {

StringModelProperty WindowViewModelBase::WindowTitleProperty("WindowViewModelBase", "WindowTitle", L"Window");
const IntModelProperty WindowViewModelBase::DialogResultProperty("WindowViewModelBase", "DialogResult", static_cast<int>(DialogResult::None));

DialogResult WindowViewModelBase::ShowModal()
{
    return ra::services::ServiceLocator::Get<ra::ui::IDesktop>().ShowModal(*this);
}

void WindowViewModelBase::Show()
{
    return ra::services::ServiceLocator::Get<ra::ui::IDesktop>().ShowWindow(*this);
}

} // namespace ui
} // namespace ra
