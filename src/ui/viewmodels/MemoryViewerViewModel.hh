#ifndef RA_UI_MEMORYVIEWERVIEWMODEL_H
#define RA_UI_MEMORYVIEWERVIEWMODEL_H
#pragma once

#include "data\EmulatorContext.hh"
#include "data\GameContext.hh"
#include "data\Types.hh"

#include "ui\WindowViewModelBase.hh"

#include "ui\drawing\ISurface.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MemoryViewerViewModel : public ViewModelBase,
                              protected ra::data::GameContext::NotifyTarget,
                              protected ra::data::EmulatorContext::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 MemoryViewerViewModel();
    ~MemoryViewerViewModel();

    MemoryViewerViewModel(const MemoryViewerViewModel&) noexcept = delete;
    MemoryViewerViewModel& operator=(const MemoryViewerViewModel&) noexcept = delete;
    MemoryViewerViewModel(MemoryViewerViewModel&&) noexcept = delete;
    MemoryViewerViewModel& operator=(MemoryViewerViewModel&&) noexcept = delete;

    void InitializeNotifyTargets();
    void DetachNotifyTargets() noexcept;

    void DoFrame();

    bool NeedsRedraw() const noexcept { return (m_nNeedsRedraw != 0); }

    class RepaintNotifyTarget
    {
    public:
        RepaintNotifyTarget() noexcept = default;
        virtual ~RepaintNotifyTarget() noexcept = default;
        RepaintNotifyTarget(const RepaintNotifyTarget&) noexcept = delete;
        RepaintNotifyTarget& operator=(const RepaintNotifyTarget&) noexcept = delete;
        RepaintNotifyTarget(RepaintNotifyTarget&&) noexcept = default;
        RepaintNotifyTarget& operator=(RepaintNotifyTarget&&) noexcept = default;

        virtual void OnRepaintMemoryViewer() noexcept(false) {}
    };

    void AddRepaintNotifyTarget(RepaintNotifyTarget& pTarget) noexcept { m_pRepaintNotifyTarget = &pTarget; }
    void RemoveRepaintNotifyTarget(const RepaintNotifyTarget& pTarget) noexcept { if (m_pRepaintNotifyTarget == &pTarget) m_pRepaintNotifyTarget = nullptr; }

private:
    RepaintNotifyTarget* m_pRepaintNotifyTarget = nullptr;

public:
    void UpdateRenderImage();

    /// <summary>
    /// Gets the image to render.
    /// </summary>
    const ra::ui::drawing::ISurface& GetRenderImage() const
    {
        Expects(m_pSurface != nullptr);
        return *m_pSurface;
    }

    enum class TextColor
    {
        Default = 0,     // default
        Selected,        // selected
        Cursor,          // selected and highlighted
        HasNote,         // has notes
        HasBookmark,     // has bookmark
        Frozen,          // has frozen bookmark

        NumColors
    };

    /// <summary>
    /// The <see cref="ModelProperty" /> for address of the selected byte.
    /// </summary>
    static const IntModelProperty AddressProperty;

    /// <summary>
    /// Gets the address of the selected byte.
    /// </summary>
    ByteAddress GetAddress() const { return GetValue(AddressProperty); }

    /// <summary>
    /// Sets the address of the selected byte.
    /// </summary>
    void SetAddress(ByteAddress value)
    {
        int nSignedValue = ra::to_signed(value);
        if (nSignedValue < 0 || m_nTotalMemorySize == 0)
            nSignedValue = 0;
        else if (nSignedValue >= gsl::narrow_cast<int>(m_nTotalMemorySize))
            nSignedValue = gsl::narrow_cast<int>(m_nTotalMemorySize) - 1;

        SetValue(AddressProperty, nSignedValue);
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for address of the first visible byte.
    /// </summary>
    static const IntModelProperty FirstAddressProperty;

    /// <summary>
    /// Gets the address of the first visible byte.
    /// </summary>
    ByteAddress GetFirstAddress() const { return GetValue(FirstAddressProperty); }

    /// <summary>
    /// Sets the address of the first visible byte.
    /// </summary>
    void SetFirstAddress(ByteAddress value);

    /// <summary>
    /// The <see cref="ModelProperty" /> for the number of visible lines.
    /// </summary>
    static const IntModelProperty NumVisibleLinesProperty;

    /// <summary>
    /// Gets the number of visible lines.
    /// </summary>
    int GetNumVisibleLines() const { return GetValue(NumVisibleLinesProperty); }

    /// <summary>
    /// Sets the number of visible lines.
    /// </summary>
    void SetNumVisibleLines(int value)
    {
        if (value > MaxLines)
            value = MaxLines;
        else if (value < 1)
            value = 1;

        SetValue(NumVisibleLinesProperty, value);
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the memory word size.
    /// </summary>
    static const IntModelProperty SizeProperty;

    /// <summary>
    /// Gets the memory word size.
    /// </summary>
    MemSize GetSize() const { return ra::itoe<MemSize>(GetValue(SizeProperty)); }

    /// <summary>
    /// Sets the memory word size.
    /// </summary>
    void SetSize(MemSize value) { SetValue(SizeProperty, ra::etoi(value)); }

    void OnClick(int nX, int nY);
    void OnResized(_UNUSED int nWidth, int nHeight);
    bool OnChar(char c);
    void OnGotFocus();
    void OnLostFocus();

    void AdvanceCursor();
    void RetreatCursor();
    void AdvanceCursorWord();
    void RetreatCursorWord();
    void AdvanceCursorLine();
    void RetreatCursorLine();
    void AdvanceCursorPage();
    void RetreatCursorPage();

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

    // GameContext::NotifyTarget
    void OnActiveGameChanged() override;
    void OnCodeNoteChanged(ra::ByteAddress, const std::wstring&) override;

    // EmulatorContext::NotifyTarget
    void OnTotalMemorySizeChanged() override;
    void OnByteWritten(ra::ByteAddress nAddress, uint8_t nValue) override;

    uint8_t* m_pMemory;
    uint8_t* m_pColor;

    ra::ui::Size m_szChar;
    int m_nSelectedNibble = 0;
    ra::ByteAddress m_nTotalMemorySize = 0;
    bool m_bReadOnly = true;
    bool m_bHasFocus = false;

    static constexpr int REDRAW_MEMORY = 1;
    static constexpr int REDRAW_ADDRESSES = 2;
    static constexpr int REDRAW_HEADERS = 4;
    static constexpr int REDRAW_ALL = REDRAW_MEMORY | REDRAW_ADDRESSES | REDRAW_HEADERS;
    int m_nNeedsRedraw = REDRAW_ALL;

    static constexpr int MaxLines = 128;

private:
    void BuildFontSurface();
    void RenderAddresses();
    void RenderHeader();
    void RenderMemory();
    void WriteChar(int nX, int nY, TextColor nColor, int hexChar);

    void UpdateColor(ra::ByteAddress nAddress);
    void UpdateColors();
    void UpdateHighlight(ra::ByteAddress nAddress, int nNewLength, int nOldLength);

    int NibblesPerWord() const;
    void UpdateSelectedNibble(int nNewNibble);

    std::unique_ptr<ra::ui::drawing::ISurface> m_pSurface;
    std::unique_ptr<ra::ui::drawing::ISurface> m_pFontSurface;
    std::unique_ptr<uint8_t[]> m_pBuffer;

    int m_nFont = 0;

    class MemoryBookmarkMonitor;
    friend class MemoryBookmarkMonitor;
    std::unique_ptr<MemoryBookmarkMonitor> m_pBookmarkMonitor;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MEMORYVIEWERVIEWMODEL_H
