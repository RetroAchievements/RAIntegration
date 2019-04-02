#ifndef RA_UI_OVERLAY_LISTPAGE_VIEWMODEL_H
#define RA_UI_OVERLAY_LISTPAGE_VIEWMODEL_H

#include "OverlayViewModel.hh"

#include "ui\ViewModelCollection.hh"
#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlayListPageViewModel : public OverlayViewModel::PageViewModel
{
public:
    /// <summary>
    /// The <see cref="ModelProperty" /> for the list title.
    /// </summary>
    static const StringModelProperty ListTitleProperty;

    /// <summary>
    /// Gets the list title.
    /// </summary>
    const std::wstring& GetListTitle() const { return GetValue(ListTitleProperty); }

    /// <summary>
    /// Sets the list title.
    /// </summary>
    void SetListTitle(const std::wstring& sValue) { SetValue(ListTitleProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the summary information.
    /// </summary>
    static const StringModelProperty SummaryProperty;

    /// <summary>
    /// Gets the summary information.
    /// </summary>
    const std::wstring& GetSummary() const { return GetValue(SummaryProperty); }

    /// <summary>
    /// Sets the summary information.
    /// </summary>
    void SetSummary(const std::wstring& sValue) { SetValue(SummaryProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the index of the selected item.
    /// </summary>
    static const IntModelProperty SelectedItemIndexProperty;

    /// <summary>
    /// Gets the index of the selected achievement.
    /// </summary>
    int GetSelectedItemIndex() const { return GetValue(SelectedItemIndexProperty); }

    /// <summary>
    /// Sets the index of the selected achievement.
    /// </summary>
    void SetSelectedItemIndex(int nValue) { SetValue(SelectedItemIndexProperty, nValue); }

    void Refresh() override;
    bool Update(double fElapsed) override;
    void Render(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const override;
    bool ProcessInput(const ControllerInput& pInput) override;

    class ItemViewModel : public LookupItemViewModel
    {
    public:
        /// <summary>
        /// The <see cref="ModelProperty" /> for the item detail.
        /// </summary>
        static const StringModelProperty DetailProperty;

        /// <summary>
        /// Gets the item detail.
        /// </summary>
        const std::wstring& GetDetail() const { return GetValue(DetailProperty); }

        /// <summary>
        /// Sets the item detail.
        /// </summary>
        void SetDetail(const std::wstring& sValue) { SetValue(DetailProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for whether or not the item is disabled.
        /// </summary>
        static const BoolModelProperty IsDisabledProperty;

        /// <summary>
        /// Gets whether or not the item is disabled.
        /// </summary>
        bool IsDisabled() const { return GetValue(IsDisabledProperty); }

        /// <summary>
        /// Sets whether the item is disabled.
        /// </summary>
        void SetDisabled(bool bValue) { SetValue(IsDisabledProperty, bValue); }

        ImageReference Image{};
    };

protected:
    bool SetDetail(bool bDetail);
    void ForceRedraw() noexcept { m_bRedraw = true; }

    gsl::index m_nScrollOffset = 0;
    mutable unsigned int m_nVisibleItems = 0;
    double m_fElapsed = 0.0;
    std::wstring m_sTitle;
    std::wstring m_sDetailTitle;

    ra::ui::ViewModelCollection<ItemViewModel> m_vItems;

private:
    void RenderList(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const;
    virtual void RenderDetail(_UNUSED ra::ui::drawing::ISurface& pSurface, 
        _UNUSED int nX, _UNUSED int nY, _UNUSED int nWidth, _UNUSED int nHeight) const noexcept(false) {};
    virtual void FetchItemDetail(_UNUSED ItemViewModel& vmItem) noexcept(false) {};

    unsigned int m_nImagesPending = 0;
    bool m_bDetail = false;
    bool m_bRedraw = false;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_LISTPAGE_VIEWMODEL_H
