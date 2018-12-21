#include "WindowBinding.hh"

#include "RA_Core.h"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\IDesktop.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

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
                SetDlgItemTextW(m_hWnd, pIter.second, GetValue(*pProperty).c_str());
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
    GetWindowRect(g_RAMainWnd, &rcMainWindow);

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

    auto pIter = m_mLabelBindings.find(args.Property.GetKey());
    if (pIter != m_mLabelBindings.end())
    {
        SetDlgItemTextW(m_hWnd, pIter->second, args.tNewValue.c_str());
        return;
    }
}

void WindowBinding::BindLabel(int nDlgItemId, const StringModelProperty& pSourceProperty)
{
    m_mLabelBindings.insert_or_assign(pSourceProperty.GetKey(), nDlgItemId);

    if (m_hWnd)
    {
        // immediately push values from the viewmodel to the UI
        SetDlgItemTextW(m_hWnd, nDlgItemId, GetValue(pSourceProperty).c_str());
    }
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
