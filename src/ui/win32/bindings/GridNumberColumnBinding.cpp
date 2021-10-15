#include "GridNumberColumnBinding.hh"

#include "GridBinding.hh"

#include "ra_math.h"
#include "RA_StringUtils.h"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

bool GridNumberColumnBinding::SetText(ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex, const std::wstring& sValue)
{
    std::wstring sError;
    unsigned int nValue = 0U;

    if (!ra::ParseUnsignedInt(sValue, m_nMaximum, nValue, sError))
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
        return false;
    }

    vmItems.SetItemValue(nIndex, *m_pBoundProperty, ra::to_signed(nValue));
    return true;
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
