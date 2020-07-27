#ifndef RA_UI_RICHPRESENCEMONITORVIEWMODEL_H
#define RA_UI_RICHPRESENCEMONITORVIEWMODEL_H
#pragma once

#include "ui/WindowViewModelBase.hh"

#include "data/GameContext.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class RichPresenceMonitorViewModel : public WindowViewModelBase, 
    protected ra::data::GameContext::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 RichPresenceMonitorViewModel() noexcept;

    void InitializeNotifyTargets();

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
    /// Refreshes the display string.
    /// </summary>
    void UpdateDisplayString();

    /// <summary>
    /// Sets the message to display.
    /// </summary>
    void SetDisplayString(const std::wstring& sValue) { SetValue(DisplayStringProperty, sValue); }

    /// <summary>
    /// Starts periodically updating the display string.
    /// </summary>
    void StartMonitoring();

    /// <summary>
    /// Stops periodically updating the display string.
    /// </summary>
    void StopMonitoring() noexcept;

    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;

    // GameContext::NotifyTarget
    void OnActiveGameChanged() override;

private:
    void ScheduleUpdateDisplayString();

    enum class MonitorState
    {
        None,
        Active,
        Deactivated,
        Static,
    };

    MonitorState m_nState{ MonitorState::None };

    time_t m_tRichPresenceFileTime{ 0 };
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_RICHPRESENCEMONITORVIEWMODEL_H
