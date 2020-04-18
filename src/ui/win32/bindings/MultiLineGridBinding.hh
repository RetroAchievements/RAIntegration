#ifndef RA_UI_WIN32_MULTILINEGRIDBINDING_H
#define RA_UI_WIN32_MULTILINEGRIDBINDING_H
#pragma once

#include "GridBinding.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class MultiLineGridBinding : public GridBinding
{
public:
    explicit MultiLineGridBinding(ViewModelBase& vmViewModel) noexcept : GridBinding(vmViewModel) {}
    GSL_SUPPRESS_F6 ~MultiLineGridBinding() noexcept = default;

    MultiLineGridBinding(const MultiLineGridBinding&) noexcept = delete;
    MultiLineGridBinding& operator=(const MultiLineGridBinding&) noexcept = delete;
    MultiLineGridBinding(MultiLineGridBinding&&) noexcept = delete;
    MultiLineGridBinding& operator=(MultiLineGridBinding&&) noexcept = delete;

    GSL_SUPPRESS_CON3 LRESULT OnLvnItemChanging(const LPNMLISTVIEW pnmListView) override;
    GSL_SUPPRESS_CON3 void OnLvnItemChanged(const LPNMLISTVIEW pnmListView) override;
    LRESULT OnCustomDraw(NMLVCUSTOMDRAW* pCustomDraw) override;
    void OnNmClick(const NMITEMACTIVATE* pnmItemActivate) override;
    void OnNmDblClick(const NMITEMACTIVATE* pnmItemActivate) override;

    void EnsureVisible(gsl::index nIndex);

protected:
    void UpdateAllItems() override;
    void UpdateItems(gsl::index nColumn) override;

    void OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) override;
    void OnViewModelAdded(gsl::index nIndex) override;
    void OnViewModelRemoved(gsl::index nIndex) override;
    void OnEndViewModelCollectionUpdate() override;

private:
    gsl::index GetIndexForLine(gsl::index nLine) const;
    void UpdateLineBreaks(gsl::index nIndex, gsl::index nColumn, const ra::ui::win32::bindings::GridColumnBinding* pColumn, int nChars);
    static void GetLineBreaks(const std::wstring& sText, int nChars, std::vector<unsigned int>& vLineBreaks);
    void UpdateLineOffsets();
    int GetMaxCharsForColumn(gsl::index nColumn) const;

    struct ItemMetrics
    {
        unsigned int nFirstLine = 0;
        unsigned int nNumLines = 1;
        std::map<int, std::vector<unsigned int>> mColumnLineOffsets;
    };

    std::vector<ItemMetrics> m_vItemMetrics;
    gsl::index m_nLastClickedItem = 0;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_MULTILINEGRIDBINDING_H
