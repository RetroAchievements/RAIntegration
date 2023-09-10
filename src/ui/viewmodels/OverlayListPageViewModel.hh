#ifndef RA_UI_OVERLAY_LISTPAGE_VIEWMODEL_H
#define RA_UI_OVERLAY_LISTPAGE_VIEWMODEL_H

#include "OverlayViewModel.hh"

#include "data\models\AssetModelBase.hh"

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
    /// The <see cref="ModelProperty" /> for whether or not headers can be collapsed.
    /// </summary>
    static const BoolModelProperty CanCollapseHeadersProperty;

    /// <summary>
    /// Gets whether or not headers can be collapsed.
    /// </summary>
    bool GetCanCollapseHeaders() const { return GetValue(CanCollapseHeadersProperty); }

    /// <summary>
    /// Sets whether or not headers can be collapsed.
    /// </summary>
    void SetCanCollapseHeaders(bool bValue) { SetValue(CanCollapseHeadersProperty, bValue); }

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
    const wchar_t* GetAcceptButtonText() const;

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

        /// <summary>
        /// The <see cref="ModelProperty" /> for whether or not the item is collapsed.
        /// </summary>
        static const BoolModelProperty IsCollapsedProperty;

        /// <summary>
        /// Gets whether or not the item is collapsed.
        /// </summary>
        bool IsCollapsed() const { return GetValue(IsCollapsedProperty); }

        /// <summary>
        /// Sets whether the item is collapsed.
        /// </summary>
        void SetCollapsed(bool bValue) { SetValue(IsCollapsedProperty, bValue); }

        ImageReference Image{};

        /// <summary>
        /// The <see cref="ModelProperty" /> for the current progress bar label.
        /// </summary>
        static const StringModelProperty ProgressValueProperty;

        /// <summary>
        /// Gets the current progress bar value.
        /// </summary>
        const std::wstring& GetProgressString() const { return GetValue(ProgressValueProperty); }

        /// <summary>
        /// Sets the current progress bar value.
        /// </summary>
        void SetProgressString(const std::wstring& sProgress) { SetValue(ProgressValueProperty, sProgress); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the current progress bar value.
        /// </summary>
        static const IntModelProperty ProgressPercentageProperty;

        /// <summary>
        /// Sets the percentage of the progress bar that should be filled.
        /// </summary>
        float GetProgressPercentage() const { return GetValue(ProgressPercentageProperty) / 100.0f; }

        /// <summary>
        /// Sets the percentage of the progress bar that should be filled.
        /// </summary>
        void SetProgressPercentage(float fValue) { SetValue(ProgressPercentageProperty, static_cast<int>(fValue * 100.0f)); }

        /// <summary>
        /// Gets whether the current item is a header item.
        /// </summary>
        bool IsHeader() const { return GetId() == 0; }
    };

protected:
    void EnsureSelectedItemIndexValid();
    bool SetDetail(bool bDetail);
    void ForceRedraw();
    virtual void FetchItemDetail(_UNUSED ItemViewModel& vmItem) noexcept(false) {}
    virtual bool OnHeaderClicked(_UNUSED ItemViewModel& vmItem) noexcept(false) { return false; }

    gsl::index m_nScrollOffset = 0;
    mutable unsigned int m_nVisibleItems = 0;
    double m_fElapsed = 0.0;
    bool m_bDetail = false;
    std::wstring m_sTitle;
    std::wstring m_sDetailTitle;

    ItemViewModel& GetNextItem(size_t* nIndex);
    static void SetHeader(OverlayListPageViewModel::ItemViewModel& vmItem, const std::wstring& sHeader);
    static bool AssetAppearsInFilter(const ra::data::models::AssetModelBase& pAsset);

    ra::ui::ViewModelCollection<ItemViewModel> m_vItems;

private:
    void RenderList(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const;
    virtual void RenderDetail(_UNUSED ra::ui::drawing::ISurface& pSurface, 
        _UNUSED int nX, _UNUSED int nY, _UNUSED int nWidth, _UNUSED int nHeight) const noexcept(false) {};

    unsigned int m_nImagesPending = 0;
    bool m_bRedraw = false;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_LISTPAGE_VIEWMODEL_H
