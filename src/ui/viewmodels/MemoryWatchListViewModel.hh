#ifndef RA_UI_MEMORYWATCHLISTVIEWMODEL_H
#define RA_UI_MEMORYWATCHLISTVIEWMODEL_H
#pragma once

#include "data\Types.hh"
#include "data\context\EmulatorContext.hh"
#include "data\context\GameContext.hh"

#include "services\TextReader.hh"
#include "services\TextWriter.hh"

#include "ui\WindowViewModelBase.hh"
#include "ui\viewmodels\MemoryWatchViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MemoryWatchListViewModel : public ViewModelBase,
    protected ra::data::context::GameContext::NotifyTarget,
    protected ra::data::context::EmulatorContext::NotifyTarget,   
    protected ra::data::context::EmulatorContext::DispatchesReadMemory,
    protected ra::ui::ViewModelCollectionBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 MemoryWatchListViewModel() noexcept;
    ~MemoryWatchListViewModel() = default;

    MemoryWatchListViewModel(const MemoryWatchListViewModel&) noexcept = delete;
    MemoryWatchListViewModel& operator=(const MemoryWatchListViewModel&) noexcept = delete;
    MemoryWatchListViewModel(MemoryWatchListViewModel&&) noexcept = delete;
    MemoryWatchListViewModel& operator=(MemoryWatchListViewModel&&) noexcept = delete;
    
    void InitializeNotifyTargets(bool syncNotes = true);

    void DoFrame();

    /// <summary>
    /// Gets the list of watched memory.
    /// </summary>
    ViewModelCollection<MemoryWatchViewModel>& Items() noexcept
    {
        return m_vItems;
    }

    /// <summary>
    /// Gets the list of watched memory.
    /// </summary>
    const ViewModelCollection<MemoryWatchViewModel>& Items() const noexcept
    {
        return m_vItems;
    }

    /// <summary>
    /// Gets the list of selectable sizes for each watched memory.
    /// </summary>
    const LookupItemViewModelCollection& Sizes() const noexcept
    {
        return m_vSizes;
    }

    /// <summary>
    /// Gets the list of selectable formats for each watched memory.
    /// </summary>
    const LookupItemViewModelCollection& Formats() const noexcept
    {
        return m_vFormats;
    }

    static const BoolModelProperty HasSelectionProperty;
    const bool HasSelection() const { return GetValue(HasSelectionProperty); }

    static const BoolModelProperty HasSingleSelectionProperty;
    const bool HasSingleSelection() const { return GetValue(HasSingleSelectionProperty); }

    static const IntModelProperty SingleSelectionIndexProperty;
    int GetSingleSelectionIndex() const { return GetValue(SingleSelectionIndexProperty); }
    void SetSingleSelectionIndex(int nNewIndex) { SetValue(SingleSelectionIndexProperty, nNewIndex); }

    /// <summary>
    /// Resets the Changes counter on all watched memory.
    /// </summary>
    void ClearAllChanges();

protected:
    // ra::data::context::GameContext::NotifyTarget
    void OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote) override;

    // ra::data::context::EmulatorContext::NotifyTarget
    void OnByteWritten(ra::ByteAddress, uint8_t) override;

    // ra::ui::ViewModelCollectionBase::NotifyTarget
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    void OnEndViewModelCollectionUpdate() override;
    void OnViewModelRemoved(gsl::index nIndex) override;

private:
    void UpdateHasSelection();

    ViewModelCollection<MemoryWatchViewModel> m_vItems;
    LookupItemViewModelCollection m_vSizes;
    LookupItemViewModelCollection m_vFormats;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MEMORYWATCHLISTVIEWMODEL_H
