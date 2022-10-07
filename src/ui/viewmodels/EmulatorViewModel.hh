#ifndef RA_UI_EMULATORVIEWMODEL_H
#define RA_UI_EMULATORVIEWMODEL_H
#pragma once

#include "ui/WindowViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class EmulatorViewModel : public WindowViewModelBase
{
public:
    /// <summary>
    /// Sets the additional text to display in the application title bar.
    /// </summary>
    void SetAppTitleMessage(const std::string& sValue)
    {
        m_bAppTitleManaged = true;
        m_sAppTitleMessage = sValue;

        UpdateWindowTitle();
    }

    void UpdateWindowTitle();

private:
    bool m_bAppTitleManaged = false;
    std::string m_sAppTitleMessage;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_EMULATORVIEWMODEL_H
