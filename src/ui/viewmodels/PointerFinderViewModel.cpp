#include "PointerFinderViewModel.hh"

#include "RA_Defs.h"

#include "data/context/ConsoleContext.hh"
#include "data/context/GameContext.hh"

#include "ui/viewmodels/MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty PointerFinderViewModel::StateViewModel::AddressProperty("StateViewModel", "Address", L"");
const StringModelProperty PointerFinderViewModel::StateViewModel::CaptureButtonTextProperty("StateViewModel", "CaptureButtonText", L"Capture");
const BoolModelProperty PointerFinderViewModel::StateViewModel::CanCaptureProperty("StateViewModel", "CanCapture", true);

void PointerFinderViewModel::DoFrame()
{
    if (!IsVisible())
        return;

    for (auto& pState : m_vStates)
        pState.DoFrame();
}

void PointerFinderViewModel::StateViewModel::DoFrame()
{
    if (CanCapture())
        m_pViewer.DoFrame();
}

void PointerFinderViewModel::StateViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == AddressProperty)
    {
        const auto nAddress = ra::ByteAddressFromString(ra::Narrow(GetAddress()));
        m_pViewer.InitializeFixedViewer(nAddress);
    }

    ViewModelBase::OnValueChanged(args);
}

void PointerFinderViewModel::StateViewModel::ToggleCapture()
{
    if (CanCapture())
        Capture();
    else
        ClearCapture();
}

void PointerFinderViewModel::StateViewModel::Capture()
{
    if (ra::services::ServiceLocator::Get<ra::data::context::GameContext>().GameId() == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"No game loaded.", L"Cannot capture memory without a loaded game.");
        return;
    }

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();
    const auto nMemorySize = pConsoleContext.MemoryRegions().at(pConsoleContext.MemoryRegions().size() - 1).EndAddress + 1;
    const auto nAddress = ra::ByteAddressFromString(ra::Narrow(GetAddress()));

    m_pCapture.reset(new ra::services::SearchResults());
    m_pCapture->Initialize(0, nMemorySize,
        (nMemorySize > 0xFFFF) ? ra::services::SearchType::ThirtyTwoBit : ra::services::SearchType::SixteenBit);

    SetValue(CaptureButtonTextProperty, L"Release");
    SetValue(CanCaptureProperty, false);
}

void PointerFinderViewModel::StateViewModel::ClearCapture()
{
    m_pCapture.reset();

    SetValue(CaptureButtonTextProperty, CaptureButtonTextProperty.GetDefaultValue());
    SetValue(CanCaptureProperty, true);
}

// ------------------------------------

PointerFinderViewModel::PointerFinderViewModel()
{
    SetWindowTitle(L"Pointer Finder");
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
