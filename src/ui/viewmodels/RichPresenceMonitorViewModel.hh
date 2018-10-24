#ifndef RA_UI_RICHPRESENCEMONITORVIEWMODEL_H
#define RA_UI_RICHPRESENCEMONITORVIEWMODEL_H
#pragma once

#include "ui/WindowViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class RichPresenceMonitorViewModel : public WindowViewModelBase
{
public:
    explicit RichPresenceMonitorViewModel() noexcept;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the message.
    /// </summary>
    static const StringModelProperty DisplayStringProperty;

    /// <summary>
    /// Gets the message to display.
    /// </summary>
    const std::wstring& GetDisplayString() const { return GetValue(DisplayStringProperty); }

protected:
    /// <summary>
    /// Sets the message to display.
    /// </summary>
    void SetDisplayString(const std::wstring& sValue) { SetValue(DisplayStringProperty, sValue); }

public:
    void UpdateDisplayString();
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_RICHPRESENCEMONITORVIEWMODEL_H
