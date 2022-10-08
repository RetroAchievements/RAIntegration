#ifndef RA_UI_MEMORYINSPECTORVIEWMODEL_H
#define RA_UI_MEMORYINSPECTORVIEWMODEL_H
#pragma once

#include "data\Types.hh"
#include "data\context\GameContext.hh"

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"
#include "ui\viewmodels\MemorySearchViewModel.hh"
#include "ui\viewmodels\MemoryViewerViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MemoryInspectorViewModel : public WindowViewModelBase,
    protected ViewModelBase::NotifyTarget,
    protected ra::data::context::GameContext::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 MemoryInspectorViewModel();
    ~MemoryInspectorViewModel() noexcept = default;

    MemoryInspectorViewModel(const MemoryInspectorViewModel&) noexcept = delete;
    MemoryInspectorViewModel& operator=(const MemoryInspectorViewModel&) noexcept = delete;
    MemoryInspectorViewModel(MemoryInspectorViewModel&&) noexcept = delete;
    MemoryInspectorViewModel& operator=(MemoryInspectorViewModel&&) noexcept = delete;

    void InitializeNotifyTargets();

    void DoFrame();

    MemorySearchViewModel& Search() noexcept { return m_pSearch; }
    const MemorySearchViewModel& Search() const noexcept { return m_pSearch; }

    MemoryViewerViewModel& Viewer() noexcept { return m_pViewer; }
    const MemoryViewerViewModel& Viewer() const noexcept { return m_pViewer; }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the current address.
    /// </summary>
    static const IntModelProperty CurrentAddressProperty;

    /// <summary>
    /// Gets the current address.
    /// </summary>
    ra::ByteAddress GetCurrentAddress() const { return GetValue(CurrentAddressProperty); }

    /// <summary>
    /// Sets the current address.
    /// </summary>
    void SetCurrentAddress(const ra::ByteAddress nValue) { SetValue(CurrentAddressProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the current address as a string.
    /// </summary>
    static const StringModelProperty CurrentAddressTextProperty;

    /// <summary>
    /// Gets the current address as a string.
    /// </summary>
    const std::wstring& GetCurrentAddressText() const { return GetValue(CurrentAddressTextProperty); }

    void BookmarkCurrentAddress() const;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the current address's note.
    /// </summary>
    static const StringModelProperty CurrentAddressNoteProperty;

    /// <summary>
    /// Gets the current address's note.
    /// </summary>
    const std::wstring& GetCurrentAddressNote() const { return GetValue(CurrentAddressNoteProperty); }

    /// <summary>
    /// Sets the current address's note.
    /// </summary>
    void SetCurrentAddressNote(const std::wstring& sValue);

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the current address note is editable.
    /// </summary>
    static const BoolModelProperty CanEditCurrentAddressNoteProperty;

    /// <summary>
    /// Gets whether or not the current address note is editable.
    /// </summary>
    const bool CanEditCurrentAddressNote() const { return GetValue(CanEditCurrentAddressNoteProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the current address note is modified.
    /// </summary>
    static const BoolModelProperty CanRevertCurrentAddressNoteProperty;

    /// <summary>
    /// Gets whether or not the current address note is modified.
    /// </summary>
    const bool CanRevertCurrentAddressNote() const { return GetValue(CanRevertCurrentAddressNoteProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not notes can be modified.
    /// </summary>
    static const BoolModelProperty CanModifyNotesProperty;

    /// <summary>
    /// Gets whether or not notes can be modified.
    /// </summary>
    const bool CanModifyNotes() const { return GetValue(CanModifyNotesProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the current address note can be published.
    /// </summary>
    static const BoolModelProperty CanPublishCurrentAddressNoteProperty;

    /// <summary>
    /// Gets whether or not the current address note can be published.
    /// </summary>
    const bool CanPublishCurrentAddressNote() const { return GetValue(CanPublishCurrentAddressNoteProperty); }

    void PublishCurrentAddressNote();
    void RevertCurrentAddressNote();

    void OpenNotesList();

    /// <summary>
    /// Sets the current address to the next address after the current address associated to a note.
    /// <summary>
    /// <returns><c>true</c> if the current address was changed, <c>false</c> if not.</returns>
    bool NextNote();

    /// <summary>
    /// Sets the current address to the next address before the current address associated to a note.
    /// <summary>
    /// <returns><c>true</c> if the current address was changed, <c>false</c> if not.</returns>
    bool PreviousNote();

    /// <summary>
    /// The <see cref="ModelProperty" /> for the current address's bit representation.
    /// </summary>
    static const StringModelProperty CurrentAddressBitsProperty;

    /// <summary>
    /// Gets the current address's bit representation.
    /// </summary>
    const std::wstring& GetCurrentAddressBits() const { return GetValue(CurrentAddressBitsProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the CurrentBits should be visible.
    /// </summary>
    static const BoolModelProperty CurrentBitsVisibleProperty;

    void ToggleBit(int nBit);

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;

    // ViewModelBase::NotifyTarget
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;

    // GameContext::NotifyTarget
    void OnActiveGameChanged() override;
    void OnEndGameLoad() override;
    void OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote) override;

private:
    static const IntModelProperty CurrentAddressValueProperty;
    void OnCurrentAddressChanged(ra::ByteAddress nNewAddress);

    void UpdateNoteButtons();
    void SaveNotes();

    MemorySearchViewModel m_pSearch;
    MemoryViewerViewModel m_pViewer;
    unsigned m_nGameId = 0;
    bool m_bSyncingAddress = false;
    bool m_bSyncingCodeNote = false;
    ra::ByteAddress m_nSavedNoteAddress = 0xFFFFFFFF;
    std::wstring m_sSavedNote;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MEMORYINSPECTORVIEWMODEL_H
