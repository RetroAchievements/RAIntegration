#include "LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty LookupItemViewModel::LabelProperty("LookupItemViewModel", "Label", L"");
const IntModelProperty LookupItemViewModel::IdProperty("LookupItemViewModel", "Id", 0);
const BoolModelProperty LookupItemViewModel::IsSelectedProperty("LookupItemViewModel", "IsSelected", false);

} // namespace viewmodels
} // namespace ui
} // namespace ra
