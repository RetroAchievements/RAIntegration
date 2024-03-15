#include "WindowBinding.hh"

#include "RA_Core.h"
#include "RA_Log.h"
#include "RA_Resource.h"

#include "data\ModelProperty.hh"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\IDesktop.hh"
#include "ui\viewmodels\WindowManager.hh"
#include "ui\win32\bindings\ControlBinding.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class DispatchingWindow : public DialogBase
{
public:
    explicit DispatchingWindow(WindowViewModelBase& vmWindow) : DialogBase(vmWindow) {}
    virtual ~DispatchingWindow() noexcept = default;
    DispatchingWindow(const DispatchingWindow&) noexcept = delete;
    DispatchingWindow& operator=(const DispatchingWindow&) noexcept = delete;
    DispatchingWindow(DispatchingWindow&&) noexcept = delete;
    DispatchingWindow& operator=(DispatchingWindow&&) noexcept = delete;

    class ViewModel : public WindowViewModelBase
    {
    };

    class Presenter : public IDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase&) noexcept override { return true; }
        void ShowWindow(ra::ui::WindowViewModelBase&) noexcept override {}
        void ShowModal(ra::ui::WindowViewModelBase&, HWND) noexcept override {}
    };

    Presenter m_pPresenter;

private:

    ViewModel m_vmViewModel;
};
static std::unique_ptr<DispatchingWindow> s_pDispatchingWindow;
static DispatchingWindow::ViewModel s_vmDispatchingWindow;

std::vector<WindowBinding*> WindowBinding::s_vKnownBindings;
DWORD WindowBinding::s_hUIThreadId;

void WindowBinding::SetHWND(DialogBase* pDialog, HWND hWnd)
{
    m_pDialog = pDialog;
    m_hWnd = hWnd;

    if (m_hWnd)
    {
        // immediately push values from the viewmodel to the UI
        UpdateAppTitle();

        for (auto& pIter : m_mLabelBindings)
        {
            auto* pProperty = ra::data::ModelPropertyBase::GetPropertyForKey(pIter.first);
            if (pProperty == nullptr)
                continue;

            auto* pStringProperty = dynamic_cast<const StringModelProperty*>(pProperty);
            if (pStringProperty != nullptr)
            {
                SetDlgItemTextW(m_hWnd, pIter.second, GetValueFromAny(*pStringProperty).c_str());
            }
            else
            {
                auto* pIntProperty = dynamic_cast<const IntModelProperty*>(pProperty);
                if (pIntProperty != nullptr)
                {
                    const auto sText = std::to_wstring(GetValueFromAny(*pIntProperty));
                    SetDlgItemTextW(m_hWnd, pIter.second, sText.c_str());
                }
            }
        }

        for (auto& pIter : m_mEnabledBindings)
        {
            auto* pProperty = dynamic_cast<const BoolModelProperty*>(ra::data::ModelPropertyBase::GetPropertyForKey(pIter.first));
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

        for (auto& pIter : m_mVisibilityBindings)
        {
            auto* pProperty = dynamic_cast<const BoolModelProperty*>(ra::data::ModelPropertyBase::GetPropertyForKey(pIter.first));
            if (pProperty != nullptr)
            {
                for (const auto nDlgItemId : pIter.second)
                {
                    if (m_vMultipleVisibilityBoundControls.find(nDlgItemId) != m_vMultipleVisibilityBoundControls.end())
                        continue;

                    auto hControl = GetDlgItem(m_hWnd, nDlgItemId);
                    if (hControl)
                        ShowWindow(hControl, GetValueFromAny(*pProperty) ? SW_SHOW : SW_HIDE);
                }
            }
        }

        for (const auto nDlgItemId : m_vMultipleVisibilityBoundControls)
        {
            auto hControl = GetDlgItem(m_hWnd, nDlgItemId);
            if (hControl)
            {
                const bool bVisible = CheckMultiBoundVisibility(nDlgItemId);
                ShowWindow(hControl, bVisible ? SW_SHOW : SW_HIDE);
            }
        }

        RestoreSizeAndPosition();
    }
}

void WindowBinding::UpdateAppTitle()
{
    const auto& pEmulatorViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().Emulator;
    if (pEmulatorViewModel.IsAppTitleManaged())
    {
        InvokeOnUIThread([this]() {
            SetWindowTextW(m_hWnd, GetValue(WindowViewModelBase::WindowTitleProperty).c_str());
        });
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
        const ra::ui::Size oDialogSize{ rcDialog.right - rcDialog.left, rcDialog.bottom - rcDialog.top };

        ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>().SetWindowSize(m_sSizeAndPositionKey, oDialogSize);
    }

    ControlBinding::ForceRepaint(m_hWnd);
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
        const ra::ui::Position oRelativePosition{ rcDialog.left - rcMainWindow.left, rcDialog.top - rcMainWindow.top };

        ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>().SetWindowPosition(m_sSizeAndPositionKey, oRelativePosition);
    }
}

void WindowBinding::OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) noexcept
{
    if (args.Property == WindowViewModelBase::WindowTitleProperty)
    {
        UpdateAppTitle();
        return;
    }

    const auto pIter = m_mLabelBindings.find(args.Property.GetKey());
    if (pIter != m_mLabelBindings.end())
    {
        InvokeOnUIThread([this, nDlgItemId = pIter->second, sValue = args.tNewValue]() noexcept {
            SetDlgItemTextW(m_hWnd, nDlgItemId, sValue.c_str());
        });
    }
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

void WindowBinding::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) noexcept
{
    const auto pIter = m_mLabelBindings.find(args.Property.GetKey());
    if (pIter != m_mLabelBindings.end())
    {
        InvokeOnUIThread([this, nDlgItemId = pIter->second, sText = std::to_wstring(args.tNewValue)]() noexcept {
            SetDlgItemTextW(m_hWnd, nDlgItemId, sText.c_str());
        });
    }

    if (m_pDialog && args.Property == WindowViewModelBase::DialogResultProperty)
        m_pDialog->SetDialogResult(ra::itoe<DialogResult>(args.tNewValue));
}

void WindowBinding::BindLabel(int nDlgItemId, const IntModelProperty& pSourceProperty)
{
    m_mLabelBindings.insert_or_assign(pSourceProperty.GetKey(), nDlgItemId);

    if (m_hWnd)
    {
        // immediately push values from the viewmodel to the UI
        const auto sText = std::to_wstring(GetValueFromAny(pSourceProperty));
        SetDlgItemTextW(m_hWnd, nDlgItemId, sText.c_str());
    }
}

void WindowBinding::OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) noexcept
{
    bool bRepaint = false;

    const auto pEnabledIter = m_mEnabledBindings.find(args.Property.GetKey());
    if (pEnabledIter != m_mEnabledBindings.end())
    {
        for (const auto nDlgItemId : pEnabledIter->second)
        {
            auto hControl = GetDlgItem(m_hWnd, nDlgItemId);
            if (hControl)
            {
                InvokeOnUIThread([hControl, bValue = args.tNewValue ? TRUE : FALSE]() noexcept {
                    EnableWindow(hControl, bValue);
                });
                bRepaint = true;
            }
        }
    }

    const auto pVisibilityIter = m_mVisibilityBindings.find(args.Property.GetKey());
    if (pVisibilityIter != m_mVisibilityBindings.end())
    {
        for (const auto nDlgItemId : pVisibilityIter->second)
        {
            auto hControl = GetDlgItem(m_hWnd, nDlgItemId);
            if (hControl)
            {
                bool bVisible = args.tNewValue;
                if (bVisible && m_vMultipleVisibilityBoundControls.find(nDlgItemId) != m_vMultipleVisibilityBoundControls.end())
                    bVisible = CheckMultiBoundVisibility(nDlgItemId);

                InvokeOnUIThread([hControl, bVisible]() noexcept {
                    ShowWindow(hControl, bVisible ? SW_SHOW : SW_HIDE);
                });
                bRepaint = true;
            }
        }
    }

    if (bRepaint)
        ControlBinding::ForceRepaint(m_hWnd);
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

void WindowBinding::BindVisible(int nDlgItemId, const BoolModelProperty& pSourceProperty)
{
    bool bMultipleBindings = false;
    for (const auto& pPair : m_mVisibilityBindings)
    {
        if (pPair.second.find(nDlgItemId) != pPair.second.end())
        {
            m_vMultipleVisibilityBoundControls.insert(nDlgItemId);
            bMultipleBindings = true;
            break;
        }
    }

    m_mVisibilityBindings[pSourceProperty.GetKey()].insert(nDlgItemId);

    if (m_hWnd)
    {
        // immediately push values from the viewmodel to the UI
        auto hControl = GetDlgItem(m_hWnd, nDlgItemId);
        if (hControl)
        {
            const bool bVisible = bMultipleBindings ?
                CheckMultiBoundVisibility(nDlgItemId) : GetValueFromAny(pSourceProperty);
            ShowWindow(hControl, bVisible ? SW_SHOW : SW_HIDE);
        }
    }
}

bool WindowBinding::CheckMultiBoundVisibility(int nDlgItemId) const
{
    bool bVisible = true;

    for (const auto& pPair : m_mVisibilityBindings)
    {
        if (pPair.second.find(nDlgItemId) != pPair.second.end())
        {
            const auto* pProperty = dynamic_cast<const BoolModelProperty*>(ra::data::ModelPropertyBase::GetPropertyForKey(pPair.first));
            if (pProperty != nullptr)
            {
                bVisible &= GetValueFromAny(*pProperty);
                if (!bVisible)
                    break;
            }
        }
    }

    return bVisible;
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

int WindowBinding::GetValueFromAny(const IntModelProperty& pProperty) const
{
    int nValue = GetValue(pProperty);
    if (nValue == pProperty.GetDefaultValue())
    {
        for (const auto& pChild : m_vChildViewModels)
        {
            nValue = pChild->GetValue(pProperty);
            if (nValue != pProperty.GetDefaultValue())
                break;
        }
    }

    return nValue;
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

void WindowBinding::SetUIThread(DWORD hThreadId) noexcept
{
    if (s_hUIThreadId == hThreadId)
        return;

    s_hUIThreadId = hThreadId;

    s_pDispatchingWindow.reset();
}

void WindowBinding::EnableInvokeOnUIThread()
{
    if (s_pDispatchingWindow == nullptr && IsOnUIThread())
    {
        // create a hidden dummy window for dispatching UI messages

        s_pDispatchingWindow.reset(new DispatchingWindow(s_vmDispatchingWindow));
        if (!s_pDispatchingWindow->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_RICHPRESENCE),
                                                      &s_pDispatchingWindow->m_pPresenter))
        {
            RA_LOG_ERR("Could not create Code Notes dialog!");
            s_pDispatchingWindow.reset();
        }
    }
}

void WindowBinding::InvokeOnUIThread(std::function<void()> fAction)
{
    if (s_pDispatchingWindow == nullptr)
    {
        // no window to post the action to - just run it
        fAction();
    }
    else if (IsOnUIThread())
    {
        // already on the UI thread
        fAction();
    }
    else
    {
        // queue the function to be called from the UI thread
        s_pDispatchingWindow->QueueFunction(fAction);
    }
}

void WindowBinding::InvokeOnUIThreadAndWait(std::function<void()> fAction)
{
    if (s_pDispatchingWindow == nullptr || IsOnUIThread())
    {
        fAction();
    }
    else
    {
        std::mutex pMutex;
        std::condition_variable pCondition;
        bool bExecuted = false;

        s_pDispatchingWindow->QueueFunction([fAction, &pCondition, &bExecuted]() {
            fAction();

            bExecuted = true;
            pCondition.notify_all();
        });

        std::unique_lock<std::mutex> lock(pMutex);
        pCondition.wait(lock, [&bExecuted]() { return bExecuted; });
    }
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
