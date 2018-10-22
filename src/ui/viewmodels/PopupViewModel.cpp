#include "PopupViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty PopupViewModel::TitleProperty("PopupViewModel", "Title", L"");
const StringModelProperty PopupViewModel::DescriptionProperty("PopupViewModel", "Description", L"");
const StringModelProperty PopupViewModel::DetailProperty("PopupViewModel", "Detail", L"");
const StringModelProperty PopupViewModel::ImageNameProperty("PopupViewModel", "ImageName", L"");
const IntModelProperty PopupViewModel::TypeProperty("PopupViewModel", "Type", static_cast<int>(PopupViewModel::Type::Message));

} // namespace viewmodels
} // namespace ui
} // namespace ra
