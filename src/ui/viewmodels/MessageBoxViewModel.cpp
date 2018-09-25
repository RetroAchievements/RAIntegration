#include "MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

StringModelProperty MessageBoxViewModel::MessageProperty("MessageBoxViewModel", "Message", L"");
StringModelProperty MessageBoxViewModel::HeaderProperty("MessageBoxViewModel", "Header", L"");
IntModelProperty MessageBoxViewModel::IconProperty("MessageBoxViewModel", "Icon", static_cast<int>(MessageBoxIcon::None));
IntModelProperty MessageBoxViewModel::ButtonsProperty("MessageBoxViewModel", "Buttons", static_cast<int>(MessageBoxButtons::OK));

} // namespace viewmodels
} // namespace ui
} // namespace ra
