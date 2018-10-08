#ifndef RA_UI_WIN32_WINDOWBINDING_H
#define RA_UI_WIN32_WINDOWBINDING_H
#pragma once

#include "RA_Defs.h" // windows types

#include "ui/BindingBase.hh"
#include "ui/WindowViewModelBase.hh"

#include <unordered_map>

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

enum class RelativePosition
{
    None = 0,
    Before,    // To the left of, or above the target
    Near,      // Aligned with the target's left or top side
    Center,    // Centered relative to the target
    Far,       // Aligned with the target's right or bottom side
    After,     // To the right of, or below the target
};

class WindowBinding : protected BindingBase
{
public:
    WindowBinding(WindowViewModelBase& vmWindowViewModel) noexcept
        : BindingBase(vmWindowViewModel)
    {
    }
    
    /// <summary>
    /// Sets the unique identifier for remembering this window's size and position.
    /// </summary>
    /// <param name="nDefaultHorizontalLocation">The default horizontal location if a custom placement is not known.</param>
    /// <param name="nDefaultVerticalLocation">The default vertical location if a custom placement is not known.</param>
    /// <param name="sSizeAndPositionKey">The unique identifier for the window.</param>
    void SetInitialPosition(RelativePosition nDefaultHorizontalLocation, RelativePosition nDefaultVerticalLocation, const char* sSizeAndPositionKey = nullptr);
    
    /// <summary>
    /// Associates the <see cref="HWND" /> of a window for binding.
    /// </summary>
    /// <param name="hWnd">The window handle.</param>
    void SetHWND(HWND hWnd);
    
    /// <summary>
    /// Binds the a label to a property of the viewmodel.
    /// </summary>
    /// <param name="nDlgItemId">The unique identifier of the label in the dialog.</param>
    /// <param name="pSourceProperty">The property to bind to.</param>
    void BindLabel(int nDlgItemId, const StringModelProperty& pSourceProperty);

    
    /// <summary>
    /// Called when the window's size changes.
    /// </summary>
    /// <param name="oSize">The new size.</param>
    void OnSizeChanged(ra::ui::Size oSize);
    
    /// <summary>
    /// Called when the window's position changes.
    /// </summary>
    /// <param name="oPosition">The new position.</param>
    void OnPositionChanged(ra::ui::Position oPosition);

protected:
    void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override;

    void RestoreSizeAndPosition();

private:
    std::unordered_map<int, int> m_mLabelBindings;

    std::string m_sSizeAndPositionKey;
    RelativePosition m_nDefaultHorizontalLocation{ RelativePosition::None };
    RelativePosition m_nDefaultVerticalLocation{ RelativePosition::None };
    
    HWND m_hWnd{};
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_WINDOWBINDING_H
