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

void GridAddressColumnBinding::UpdateWidth()
{
    SetWidth(GridColumnBinding::WidthType::Pixels, CalculateWidth());
}

unsigned GridAddressColumnBinding::CalculateWidth()
{
    unsigned nMaxChars = 6; // 0x1234

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    const auto nTotalMemorySize = pEmulatorContext.TotalMemorySize();
    if (nTotalMemorySize > 0x1000000)
        nMaxChars = 10; // 0x12345678
    else if (nTotalMemorySize > 0x10000)
        nMaxChars = 8; // 0x123456

    constexpr int nCharWidth = 6;
    constexpr int nPadding = 6;
    return (nCharWidth * nMaxChars) + nPadding * 2;
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
