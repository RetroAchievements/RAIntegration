#ifndef RA_UI_WIN32_WINDOWBINDING_H
#define RA_UI_WIN32_WINDOWBINDING_H
#pragma once

#include "ui/BindingBase.hh"
#include "ui/WindowViewModelBase.hh"

namespace ra {
namespace ui {
namespace win32 {

class DialogBase;

namespace bindings {

class WindowBinding : protected BindingBase
{
public:
    explicit WindowBinding(WindowViewModelBase& vmWindowViewModel) noexcept
        : BindingBase(vmWindowViewModel)
    {
        GSL_SUPPRESS_F6 s_vKnownBindings.push_back(this);
    }

    GSL_SUPPRESS_F6 ~WindowBinding() noexcept
    {
        for (auto iter = s_vKnownBindings.begin(); iter != s_vKnownBindings.end(); ++iter)
        {
            if (*iter == this)
            {
                s_vKnownBindings.erase(iter);
                break;
            }
        }
    }

    WindowBinding(const WindowBinding&) noexcept = delete;
    WindowBinding& operator=(const WindowBinding&) noexcept = delete;
    WindowBinding(WindowBinding&&) noexcept = delete;
    WindowBinding& operator=(WindowBinding&&) noexcept = delete;

    /// <summary>
    /// Gets the binding for the provided <see cref="WindowViewModelBase" />.
    /// </summary>
    /// <param name="vmWindowViewModel">The window view model.</param>
    /// <returns>Associated binding, <c>nullptr</c> if not found.</returns>
    static WindowBinding* GetBindingFor(const WindowViewModelBase& vmWindowViewModel) noexcept
    {
        for (auto* pBinding : s_vKnownBindings)
        {
            if (pBinding && &pBinding->GetViewModel<WindowViewModelBase>() == &vmWindowViewModel)
                return pBinding;
        }

        return nullptr;
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
    /// <param name="pDialog">The dialog containing the binding.</param>
    /// <param name="hWnd">The window handle.</param>
    void SetHWND(ra::ui::win32::DialogBase* pDialog, HWND hWnd);
    
    /// <summary>
    /// Gets the HWND bound to the window.
    /// </summary>
    /// <returns>The window handle.</returns>
    HWND GetHWnd() const noexcept { return m_hWnd; }

    void DestroyWindow() noexcept;

    /// <summary>
    /// Specifies a secondary view model to watch for BindLabel and BindEnabled bindings.
    /// </summary>
    void AddChildViewModel(ViewModelBase& vmChild)
    {
        auto pChild = std::make_unique<ChildBinding>(vmChild, *this);
        m_vChildViewModels.emplace_back(std::move(pChild));
    }

    /// <summary>
    /// Binds the a label to a property of the viewmodel.
    /// </summary>
    /// <param name="nDlgItemId">The unique identifier of the label in the dialog.</param>
    /// <param name="pSourceProperty">The property to bind to.</param>
    void BindLabel(int nDlgItemId, const StringModelProperty& pSourceProperty);

    /// <summary>
    /// Binds the a label to a property of the viewmodel.
    /// </summary>
    /// <param name="nDlgItemId">The unique identifier of the label in the dialog.</param>
    /// <param name="pSourceProperty">The property to bind to.</param>
    void BindLabel(int nDlgItemId, const IntModelProperty& pSourceProperty);

    /// <summary>
    /// Binds the a enabledness of a control to a property of the viewmodel.
    /// </summary>
    /// <param name="nDlgItemId">The unique identifier of the control in the dialog.</param>
    /// <param name="pSourceProperty">The property to bind to.</param>
    void BindEnabled(int nDlgItemId, const BoolModelProperty& pSourceProperty);

    /// <summary>
    /// Binds the a visibility of a control to a property of the viewmodel.
    /// </summary>
    /// <param name="nDlgItemId">The unique identifier of the control in the dialog.</param>
    /// <param name="pSourceProperty">The property to bind to.</param>
    void BindVisible(int nDlgItemId, const BoolModelProperty& pSourceProperty);

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

    /// <summary>
    /// Specifies the UI thread.
    /// </summary>
    static void SetUIThread(DWORD hUIThreadId) noexcept;

    /// <summary>
    /// Ensures the UI thread dispatcher is enabled.
    /// </summary>
    static void EnableInvokeOnUIThread();

    /// <summary>
    /// Returns <c>true</c> if the current thread is the UI thread.
    /// </summary>
    static bool IsOnUIThread() noexcept { return GetCurrentThreadId() == s_hUIThreadId; }

    /// <summary>
    /// Dispatches a function to the UI thread.
    /// </summary>
    static void InvokeOnUIThread(std::function<void()> fAction);

    /// <summary>
    /// Executes a function on the UI thread and waits for it to complete.
    /// </summary>
    static void InvokeOnUIThreadAndWait(std::function<void()> fAction);

protected:
    // ViewModelBase::NotifyTarget
    GSL_SUPPRESS_F6 void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) noexcept override;
    GSL_SUPPRESS_F6 void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) noexcept override;
    GSL_SUPPRESS_F6 void OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) noexcept override;

    void RestoreSizeAndPosition();
    void UpdateAppTitle();

private:
    std::unordered_map<int, int> m_mLabelBindings;
    std::unordered_map<int, std::set<int>> m_mEnabledBindings;
    std::unordered_map<int, std::set<int>> m_mVisibilityBindings;
    std::set<int> m_vMultipleVisibilityBoundControls;

    class ChildBinding : public BindingBase
    {
    public:
        ChildBinding(ViewModelBase& vmViewModel, WindowBinding& pOwner) noexcept
            : BindingBase(vmViewModel), m_pOwner(pOwner)
        {
        }

        /* expost protected functions */
        using BindingBase::GetValue;

    protected:
        void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) noexcept override
        {
            m_pOwner.OnViewModelStringValueChanged(args);
        }

        void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) noexcept override
        {
            m_pOwner.OnViewModelIntValueChanged(args);
        }

        void OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) noexcept override
        {
            m_pOwner.OnViewModelBoolValueChanged(args);
        }

    private:
        WindowBinding& m_pOwner;
    };
    std::vector<std::unique_ptr<ChildBinding>> m_vChildViewModels;

    const std::wstring& GetValueFromAny(const StringModelProperty& pProperty) const;
    int GetValueFromAny(const IntModelProperty& pProperty) const;
    bool GetValueFromAny(const BoolModelProperty& pProperty) const;
    bool CheckMultiBoundVisibility(int nDlgItemId) const;

    std::string m_sSizeAndPositionKey;
    RelativePosition m_nDefaultHorizontalLocation{ RelativePosition::None };
    RelativePosition m_nDefaultVerticalLocation{ RelativePosition::None };
    
    ra::ui::win32::DialogBase* m_pDialog = nullptr;
    HWND m_hWnd{};

    static DWORD s_hUIThreadId;

    static std::vector<WindowBinding*> s_vKnownBindings;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_WINDOWBINDING_H
