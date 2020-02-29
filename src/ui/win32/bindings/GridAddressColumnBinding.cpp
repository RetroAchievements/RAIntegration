#include "GridAddressColumnBinding.hh"

#include "GridBinding.hh"

#include "ra_math.h"
#include "RA_StringUtils.h"

#include "ui/viewmodels/WindowManager.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

bool GridAddressColumnBinding::HandleDoubleClick(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex)
{
    if (IsReadOnly())
    {
        const auto nValue = vmItems.GetItemValue(nIndex, *m_pBoundProperty);
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryInspector.SetCurrentAddress(nValue);

        return true;
    }

    return false;
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
