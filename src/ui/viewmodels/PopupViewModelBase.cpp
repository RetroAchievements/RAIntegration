#include "PopupViewModelBase.hh"

#include "ra_math.h"

#include "services\IConfiguration.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty PopupViewModelBase::PopupTypeProperty("PopupViewModelBase", "PopupType", ra::etoi(Popup::None));
const IntModelProperty PopupViewModelBase::HorizontalOffsetProperty("PopupViewModelBase", "HorizontalOffset", 0);
const IntModelProperty PopupViewModelBase::VerticalOffsetProperty("PopupViewModelBase", "VerticalOffset", 0);
const IntModelProperty PopupViewModelBase::RenderLocationXProperty("PopupViewModelBase", "RenderLocationX", 0);
const IntModelProperty PopupViewModelBase::RenderLocationYProperty("PopupViewModelBase", "RenderLocationY", 0);

int PopupViewModelBase::GetFadeOffset(double fAnimationProgress, double fTotalAnimationTime, double fInOutTime, int nInitialOffset, int nTargetOffset)
{
    int nNewOffset = 0;

    if (fAnimationProgress < fInOutTime)
    {
        // fading in
        const auto fPercentage = (fInOutTime - fAnimationProgress) / fInOutTime;
        Expects(nTargetOffset >= nInitialOffset);
        const auto nOffset = to_unsigned(nTargetOffset - nInitialOffset) * (fPercentage * fPercentage);
        nNewOffset = ftol(nTargetOffset - nOffset);
    }
    else if (fAnimationProgress < fTotalAnimationTime - fInOutTime)
    {
        // faded in - hold position
        nNewOffset = nTargetOffset;
    }
    else if (fAnimationProgress < fTotalAnimationTime)
    {
        // fading out
        const auto fPercentage = (fTotalAnimationTime - fInOutTime - fAnimationProgress) / fInOutTime;
        Expects(nTargetOffset >= nInitialOffset);
        const auto nOffset = to_unsigned(nTargetOffset - nInitialOffset) * (fPercentage * fPercentage);
        nNewOffset = ftoi(nTargetOffset - nOffset);
    }
    else
    {
        // faded out
        nNewOffset = nInitialOffset;
    }

    return nNewOffset;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
