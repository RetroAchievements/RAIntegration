#include "WindowBinding.hh"

#include "RA_Core.h"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\IDesktop.hh"
#include "ui\viewmodels\WindowManager.hh"
#include "ui\win32\bindings\ControlBinding.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

std::vector<WindowBinding*> WindowBinding::s_vKnownBindings;

void WindowBinding::SetHWND(HWND hWnd)
{
    m_hWnd = hWnd;

    if (m_hWnd)
    {
        // immediately push values from the viewmodel to the UI
        SetWindowTextW(m_hWnd, GetValue(WindowViewModelBase::WindowTitleProperty).c_str());

        for (auto& pIter : m_mLabelBindings)
        {
            auto* pProperty = dynamic_cast<const StringModelProperty*>(ModelPropertyBase::GetPropertyForKey(pIter.first));
            if (pProperty != nullptr)
                SetDlgItemTextW(m_hWnd, pIter.second, GetValueFromAny(*pProperty).c_str());
        }

        for (auto& pIter : m_mEnabledBindings)
        {
            auto* pProperty = dynamic_cast<const BoolModelProperty*>(ModelPropertyBase::GetPropertyForKey(pIter.first));
            if (pProperty != nullptr)
            {
                for (const auto nDlgItemId : pIter.second)
                {
                    auto hControl = GetDlgItem(m_hWnd, nDlgItemId);
                    if (hControl)
                        EnableWindow(hControl, GetValueFromAny(*pProperty) ? TRUE : FALSE);
                }
            }
        }

        RestoreSizeAndPosition();
    }
}

void WindowBinding::SetInitialPosition(RelativePosition nDefaultHorizontalLocation, RelativePosition nDefaultVerticalLocation, const char* sSizeAndPositionKey)
{
    if (sSizeAndPositionKey)
        m_sSizeAndPositionKey = sSizeAndPositionKey;

    m_nDefaultHorizontalLocation = nDefaultHorizontalLocation;
    m_nDefaultVerticalLocation = nDefaultVerticalLocation;

    if (m_hWnd)
        RestoreSizeAndPosition();
}

void WindowBinding::RestoreSizeAndPosition()
{
    const auto& pEmulatorViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().Emulator;
    const auto* pMainWindowBinding = GetBindingFor(pEmulatorViewModel);
    if (!pMainWindowBinding || pMainWindowBinding == this)
        return;

    ra::ui::Position oWorkAreaPosition;
    ra::ui::Size oWorkAreaSize;
    const auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();
    pDesktop.GetWorkArea(oWorkAreaPosition, oWorkAreaSize);

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    auto oPosition = pConfiguration.GetWindowPosition(m_sSizeAndPositionKey);
    auto oSize = pConfiguration.GetWindowSize(m_sSizeAndPositionKey);

    if (oSize.Width == INT32_MIN || oSize.Height == INT32_MIN)
    {
        RECT rcDialog;
        GetWindowRect(m_hWnd, &rcDialog);

        const int nDlgWidth = rcDialog.right - rcDialog.left;
        if (oSize.Width == INT32_MIN)
            oSize.Width = nDlgWidth;
        else if (oSize.Width < nDlgWidth)
            oSize.Width = INT32_MIN;

        const int nDlgHeight = rcDialog.bottom - rcDialog.top;
        if (oSize.Height == INT32_MIN)
            oSize.Height = nDlgHeight;
        else if (oSize.Height < nDlgHeight)
            oSize.Height = INT32_MIN;
    }

    RECT rcMainWindow;
    GetWindowRect(pMainWindowBinding->GetHWnd(), &rcMainWindow);

    if (oPosition.X != INT32_MIN)
    {
        // stored position is relative to main window
        oPosition.X += rcMainWindow.left;
    }
    else
    {
        switch (m_nDefaultHorizontalLocation)
        {
            case RelativePosition::None:
                oPosition.X = (oWorkAreaSize.Width - oSize.Width) / 2;
                break;

            case RelativePosition::Before:
                oPosition.X = rcMainWindow.left - oSize.Width - 4;
                break;

            case RelativePosition::Near:
                oPosition.X = rcMainWindow.left;
                break;

            default:
            case RelativePosition::Center:
                oPosition.X = (rcMainWindow.right + rcMainWindow.left - oSize.Width) / 2;
                break;

            case RelativePosition::Far:
                oPosition.X = rcMainWindow.right - oSize.Width;
                break;

            case RelativePosition::After:
                oPosition.X = rcMainWindow.right;
                break;
        }
    }

    if (oPosition.Y != INT32_MIN)
    {
        // stored position is relative to main window
        oPosition.Y += rcMainWindow.top;
    }
    else
    {
        switch (m_nDefaultVerticalLocation)
        {
            case RelativePosition::None:
                oPosition.Y = (oWorkAreaSize.Height - oSize.Height) / 2;
                break;

            case RelativePosition::Before:
                oPosition.Y = rcMainWindow.top - oSize.Height - 4;
                break;

            case RelativePosition::Near:
                oPosition.Y = rcMainWindow.top;
                break;

            default:
            case RelativePosition::Center:
                oPosition.Y = (rcMainWindow.bottom + rcMainWindow.top - oSize.Height) / 2;
                break;

            case RelativePosition::Far:
                oPosition.Y = rcMainWindow.bottom - oSize.Height;
                break;

            case RelativePosition::After:
                oPosition.Y = rcMainWindow.bottom;
                break;
        }
    }

    // make sure the window is on screen
    auto offset = oPosition.X + oSize.Width - (oWorkAreaPosition.X + oWorkAreaSize.Width);
    if (offset > 0)
        oPosition.X -= offset;

    offset = oWorkAreaPosition.X - oPosition.X;
    if (offset > 0)
        oPosition.X += offset;

    offset = oPosition.Y + oSize.Height - (oWorkAreaPosition.Y + oWorkAreaSize.Height);
    if (offset > 0)
        oPosition.Y -= offset;

    offset = oWorkAreaPosition.Y - oPosition.Y;
    if (offset > 0)
        oPosition.Y += offset;

    // temporarily clear out the key while we reposition so OnSizeChanged and OnPositionChanged ignore the message
    std::string sSizeAndPositionKey = m_sSizeAndPositionKey;
    m_sSizeAndPositionKey.clear();

    SetWindowPos(m_hWnd, nullptr, oPosition.X, oPosition.Y, oSize.Width, oSize.Height, 0);

    m_sSizeAndPositionKey = sSizeAndPositionKey;
}

void WindowBinding::OnSizeChanged(_UNUSED ra::ui::Size oSize)
{
    if (!m_sSizeAndPositionKey.empty())
    {
        // oSize is the size of client area, we need the size of the window (including non-client area)
        RECT rcDialog;
        GetWindowRect(m_hWnd, &rcDialog);
        ra::ui::Size oDialogSize{ rcDialog.right - rcDialog.left, rcDialog.bottom - rcDialog.top };

        ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>().SetWindowSize(m_sSizeAndPositionKey, oDialogSize);
    }
}

void WindowBinding::OnPositionChanged(_UNUSED ra::ui::Position oPosition)
{
    if (!m_sSizeAndPositionKey.empty())
    {
        // oPosition is topleft corner of client area, we need topleft corner of window (including non-client area)
        RECT rcDialog;
        GetWindowRect(m_hWnd, &rcDialog);

        // capture position relative to main window
        RECT rcMainWindow;
        GetWindowRect(g_RAMainWnd, &rcMainWindow);
        ra::ui::Position oRelativePosition{ rcDialog.left - rcMainWindow.left, rcDialog.top - rcMainWindow.top };

        ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>().SetWindowPosition(m_sSizeAndPositionKey, oRelativePosition);
    }
}

void WindowBinding::OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) noexcept
{
    if (args.Property == WindowViewModelBase::WindowTitleProperty)
    {
        SetWindowTextW(m_hWnd, args.tNewValue.c_str());
        return;
    }

    const auto pIter = m_mLabelBindings.find(args.Property.GetKey());
    if (pIter != m_mLabelBindings.end())
        SetDlgItemTextW(m_hWnd, pIter->second, args.tNewValue.c_str());
}

void WindowBinding::BindLabel(int nDlgItemId, const StringModelProperty& pSourceProperty)
{
    m_mLabelBindings.insert_or_assign(pSourceProperty.GetKey(), nDlgItemId);

    if (m_hWnd)
    {
        // immediately push values from the viewmodel to the UI
        SetDlgItemTextW(m_hWnd, nDlgItemId, GetValueFromAny(pSourceProperty).c_str());
    }
}

void WindowBinding::OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) noexcept
{
    const auto pEnabledIter = m_mEnabledBindings.find(args.Property.GetKey());
    if (pEnabledIter != m_mEnabledBindings.end())
    {
        bool bRepaint = false;

        for (const auto nDlgItemId : pEnabledIter->second)
        {
            auto hControl = GetDlgItem(m_hWnd, nDlgItemId);
            if (hControl)
            {
                EnableWindow(hControl, args.tNewValue ? TRUE : FALSE);
                bRepaint = true;
            }
        }

        if (bRepaint)
            ControlBinding::ForceRepaint(m_hWnd);
    }
}

void WindowBinding::BindEnabled(int nDlgItemId, const BoolModelProperty& pSourceProperty)
{
    m_mEnabledBindings[pSourceProperty.GetKey()].insert(nDlgItemId);

    if (m_hWnd)
    {
        // immediately push values from the viewmodel to the UI
        auto hControl = GetDlgItem(m_hWnd, nDlgItemId);
        if (hControl)
            EnableWindow(hControl, GetValueFromAny(pSourceProperty) ? TRUE : FALSE);
    }
}

const std::wstring& WindowBinding::GetValueFromAny(const StringModelProperty& pProperty) const
{
    const std::wstring& sValue = GetValue(pProperty);
    if (sValue == pProperty.GetDefaultValue())
    {
        for (const auto& pChild : m_vChildViewModels)
        {
            const std::wstring& sChildValue = pChild->GetValue(pProperty);
            if (sChildValue != pProperty.GetDefaultValue())
                return sChildValue;
        }
    }

    return sValue;
}

bool WindowBinding::GetValueFromAny(const BoolModelProperty& pProperty) const
{
    bool bValue = GetValue(pProperty);
    if (bValue == pProperty.GetDefaultValue())
    {
        for (const auto& pChild : m_vChildViewModels)
        {
            bValue = pChild->GetValue(pProperty);
            if (bValue != pProperty.GetDefaultValue())
                break;
        }
    }

    return bValue;
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
