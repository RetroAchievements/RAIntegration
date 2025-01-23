#ifndef RA_UI_POINTERINSPECTORVIEWMODEL_H
#define RA_UI_POINTERINSPECTORVIEWMODEL_H
#pragma once

#include "data\Types.hh"
#include "data\context\GameContext.hh"

#include "ui\viewmodels\MemoryBookmarksViewModel.hh"

namespace ra {
namespace data {
namespace models {

class CodeNoteModel;

} // namespace models
} // namespace data
} // namespace ra

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

        /// <summary>
        /// The <see cref="ModelProperty" /> for the field note body.
        /// </summary>
        static const StringModelProperty BodyProperty;

        /// <summary>
        /// Gets the field note body.
        /// </summary>
        const std::wstring& GetBody() const { return GetValue(BodyProperty); }

        /// <summary>
        /// Sets the field note body.
        /// </summary>
        void SetBody(const std::wstring& sValue) { SetValue(BodyProperty, sValue); }

        using MemoryBookmarkViewModel::SetAddressWithoutUpdatingValue;

        int32_t m_nOffset = 0;
    };

    class PointerNodeViewModel : public LookupItemViewModel
    {
    public:
        static const int RootNodeId = -1;

        PointerNodeViewModel(gsl::index nParentIndex, int32_t nOffset, const std::wstring& sLabel) noexcept
            : LookupItemViewModel(nParentIndex < 0 ? RootNodeId : gsl::narrow_cast<int>((nParentIndex << 24) | (nOffset & 0x00FFFFFF)), sLabel),
              m_nParentIndex(nParentIndex),
              m_nOffset(nOffset)
        {
        }

        bool IsRootNode() const noexcept { return m_nParentIndex == -1; }
        gsl::index GetParentIndex() const noexcept { return m_nParentIndex; }
        int32_t GetOffset() const noexcept { return m_nOffset; }

    private:
        gsl::index m_nParentIndex = -1;
        int32_t m_nOffset = 0;
    };

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

    /// <summary>
    /// Gets the pointer chain list.
    /// </summary>
    ViewModelCollection<StructFieldViewModel>& PointerChain() noexcept { return m_vPointerChain; }

    /// <summary>
    /// Gets the pointer chain list.
    /// </summary>
    const ViewModelCollection<StructFieldViewModel>& PointerChain() const noexcept { return m_vPointerChain; }

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
    void GetPointerChain(gsl::index nIndex, std::stack<const PointerNodeViewModel*>& sChain) const;
    void SyncField(StructFieldViewModel& pFieldViewModel, const ra::data::models::CodeNoteModel& pOffsetNote);
    const ra::data::models::CodeNoteModel* UpdatePointerChain(int nNewNode);
    void UpdatePointerChainRowColor(StructFieldViewModel& pPointer);
    void UpdatePointerChainValues();
    void UpdateValues();
    std::string GetDefinition() const;

    LookupItemViewModelCollection m_vNodes;
    ViewModelCollection<StructFieldViewModel> m_vPointerChain;
    bool m_bSyncingAddress = false;

    const ra::data::models::CodeNoteModel* m_pCurrentNote = nullptr;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_POINTERINSPECTORVIEWMODEL_H
