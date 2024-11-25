#ifndef RA_UI_POINTERINSPECTORVIEWMODEL_H
#define RA_UI_POINTERINSPECTORVIEWMODEL_H
#pragma once

#include "data\Types.hh"
#include "data\context\GameContext.hh"

#include "ui\viewmodels\MemoryBookmarksViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class PointerInspectorViewModel : public MemoryBookmarksViewModel
{
public:
    GSL_SUPPRESS_F6 PointerInspectorViewModel();
    ~PointerInspectorViewModel() noexcept = default;

    PointerInspectorViewModel(const PointerInspectorViewModel&) noexcept = delete;
    PointerInspectorViewModel& operator=(const PointerInspectorViewModel&) noexcept = delete;
    PointerInspectorViewModel(PointerInspectorViewModel&&) noexcept = delete;
    PointerInspectorViewModel& operator=(PointerInspectorViewModel&&) noexcept = delete;

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

    /// <summary>
    /// Sets the current address as a string.
    /// </summary>
    void SetCurrentAddressText(const std::wstring& sValue) { SetValue(CurrentAddressTextProperty, sValue); }

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
    void SetCurrentAddressNote(const std::wstring& sValue) { SetValue(CurrentAddressNoteProperty, sValue); }

    class StructFieldViewModel : public MemoryBookmarkViewModel
    {
    public:
        /// <summary>
        /// The <see cref="ModelProperty" /> for the field offset.
        /// </summary>
        static const StringModelProperty OffsetProperty;

        /// <summary>
        /// Gets the field offset.
        /// </summary>
        const std::wstring& GetOffset() const { return GetValue(OffsetProperty); }

        /// <summary>
        /// Sets the field offset.
        /// </summary>
        void SetOffset(const std::wstring& sValue) { SetValue(OffsetProperty, sValue); }

        int32_t m_nOffset;
    };

    /// <summary>
    /// Gets the list of fields.
    /// </summary>
    ViewModelCollection<StructFieldViewModel>& Fields() noexcept { return m_vFields; }

    /// <summary>
    /// Gets the list of fields.
    /// </summary>
    const ViewModelCollection<StructFieldViewModel>& Fields() const noexcept { return m_vFields; }

    /// <summary>
    /// Gets the list of available nodes.
    /// </summary>
    LookupItemViewModelCollection& Nodes() noexcept { return m_vNodes; }

    /// <summary>
    /// Gets the list of available nodes.
    /// </summary>
    const LookupItemViewModelCollection& Nodes() const noexcept { return m_vNodes; }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the selected node.
    /// </summary>
    static const IntModelProperty SelectedNodeProperty;

    /// <summary>
    /// Gets the selected node index.
    /// </summary>
    int GetSelectedNode() const { return GetValue(SelectedNodeProperty); }

    /// <summary>
    /// Sets the selected node index.
    /// </summary>
    void SetSelectedNode(int nValue) { SetValue(SelectedNodeProperty, nValue); }

    void DoFrame() override;

    void CopyDefinition() const;

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;

    // GameContext::NotifyTarget
    void OnActiveGameChanged() override;
    void OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote) override;

private:
    void OnCurrentAddressChanged(ra::ByteAddress nNewAddress);
    void OnSelectedNodeChanged(int nNode);
    void LoadNote(const ra::data::models::CodeNoteModel* pNote);
    void LoadNodes(const ra::data::models::CodeNoteModel* pNote);
    const ra::data::models::CodeNoteModel* FindNestedCodeNoteModel(const ra::data::models::CodeNoteModel& pRootNote, int nNewNode);
    void GetPointerChain(gsl::index nIndex, std::stack<const LookupItemViewModel*>& sChain) const;
    void SyncField(StructFieldViewModel& pFieldViewModel, const ra::data::models::CodeNoteModel& pOffsetNote);
    void UpdateValues();
    std::string GetDefinition() const;

    LookupItemViewModelCollection m_vNodes;
    ViewModelCollection<StructFieldViewModel> m_vFields;
    bool m_bSyncingAddress = false;

    const ra::data::models::CodeNoteModel* m_pCurrentNote = nullptr;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_POINTERINSPECTORVIEWMODEL_H
