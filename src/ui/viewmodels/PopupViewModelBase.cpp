#include "PopupViewModelBase.hh"

#include "ra_math.h"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty PopupViewModelBase::RenderLocationXProperty("PopupViewModelBase", "RenderLocationX", 0);
const IntModelProperty PopupViewModelBase::RenderLocationYProperty("PopupViewModelBase", "RenderLocationY", 0);
const IntModelProperty PopupViewModelBase::RenderLocationXRelativePositionProperty("PopupViewModelBase", "RenderLocationXRelativePosition", ra::etoi(RelativePosition::Near));
const IntModelProperty PopupViewModelBase::RenderLocationYRelativePositionProperty("PopupViewModelBase", "RenderLocationYRelativePosition", ra::etoi(RelativePosition::Near));

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
