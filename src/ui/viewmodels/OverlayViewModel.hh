#ifndef RA_UI_OVERLAY_VIEWMODEL_H
#define RA_UI_OVERLAY_VIEWMODEL_H

#include "PopupViewModelBase.hh"

#include "RA_Interface.h"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlayViewModel : public PopupViewModelBase
{
public:
    enum class State
    {
        Hidden,
        FadeIn,
        Visible,
        FadeOutRequested,
        FadeOut,
    };

    State CurrentState() const noexcept { return m_nState; }

    /// <summary>
    /// Begins the animation cycle.
    /// </summary>
    void BeginAnimation() override;

    /// <summary>
    /// Updates the image to render.
    /// </summary>
    bool UpdateRenderImage(double fElapsed) override;

    /// <summary>
    /// Causes the cached render image to be rebuilt.
    /// </summary>
    void RebuildRenderImage() noexcept { m_bSurfaceStale = true; }

    /// <summary>
    /// Releases the memory used to cache the render image.
    /// </summary>
    void DestroyRenderImage() noexcept
    {
        m_bSurfaceStale = false;
        m_pSurface.reset();
    }

    /// <summary>
    /// Determines whether the animation cycle has started.
    /// </summary>
    bool IsAnimationStarted() const noexcept override { return (m_fAnimationProgress >= 0.0); }

    /// <summary>
    /// Determines whether the animation cycle has completed.
    /// </summary>
    bool IsAnimationComplete() const noexcept override { return (m_nState == State::Hidden); }

    void ProcessInput(const ControllerInput& pInput);

    void Resize(int nWidth, int nHeight);

    void Activate();
    void Deactivate();
    void DeactivateImmediately();

    class PageViewModel : public ViewModelBase
    {
    public:
        /// <summary>
        /// The <see cref="ModelProperty" /> for the page title.
        /// </summary>
        static const StringModelProperty TitleProperty;

        /// <summary>
        /// Gets the page title to display.
        /// </summary>
        const std::wstring& GetTitle() const { return GetValue(TitleProperty); }

        /// <summary>
        /// Sets the page title to display.
        /// </summary>
        void SetTitle(const std::wstring& sValue) { SetValue(TitleProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the page subtitle.
        /// </summary>
        static const StringModelProperty SubTitleProperty;

        /// <summary>
        /// Gets the page subtitle to display.
        /// </summary>
        const std::wstring& GetSubTitle() const { return GetValue(SubTitleProperty); }

        /// <summary>
        /// Sets the page subtitle to display.
        /// </summary>
        void SetSubTitle(const std::wstring& sValue) { SetValue(SubTitleProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the page title detail.
        /// </summary>
        static const StringModelProperty TitleDetailProperty;

        /// <summary>
        /// Gets the page title detail to display.
        /// </summary>
        const std::wstring& GetTitleDetail() const { return GetValue(TitleDetailProperty); }

        /// <summary>
        /// Sets the page title detail to display.
        /// </summary>
        void SetTitleDetail(const std::wstring& sValue) { SetValue(TitleDetailProperty, sValue); }

        virtual const wchar_t* GetAcceptButtonText() const noexcept(false) { return L"Select"; }
        virtual const wchar_t* GetCancelButtonText() const noexcept(false) { return L"Close"; }
        virtual const wchar_t* GetPrevButtonText() const noexcept(false) { return L"Prev"; }
        virtual const wchar_t* GetNextButtonText() const noexcept(false) { return L"Next"; }

        virtual void Refresh() = 0;

        virtual bool Update(_UNUSED double fElapsed) noexcept(false) { return false; }

        virtual bool ProcessInput(const ControllerInput& pInput) = 0;

        virtual void Render(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const = 0;
    };

    PageViewModel& CurrentPage() const { return *m_vPages.at(m_nSelectedPage); }

private:
    void PopulatePages();
    void CreateRenderImage();

    void RefreshOverlay();

    State m_nState = State::Hidden;
    bool m_bSurfaceStale = false;

    enum class NavButton
    {
        None,
        Up,
        Down,
        Left,
        Right,
        Confirm,
        Cancel,
        Quit,
    };

    NavButton m_nCurrentButton = NavButton::None;
    std::chrono::steady_clock::time_point m_tCurrentButtonPressed;

    std::vector<std::unique_ptr<PageViewModel>> m_vPages;
    gsl::index m_nSelectedPage = 0;

    double m_fAnimationProgress = -1.0;
    static _CONSTANT_VAR INOUT_TIME = 0.4;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_VIEWMODEL_H
