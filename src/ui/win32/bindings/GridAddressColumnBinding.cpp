#include "GridAddressColumnBinding.hh"

#include "GridBinding.hh"

#include "ra_math.h"
#include "RA_StringUtils.h"

#include "RA_Dlg_Memory.h"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

bool GridAddressColumnBinding::HandleDoubleClick(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex)
{
    if (IsReadOnly())
    {
        const auto nValue = vmItems.GetItemValue(nIndex, *m_pBoundProperty);
        g_MemoryDialog.GoToAddress(nValue);

        return true;
    }

    return false;
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
