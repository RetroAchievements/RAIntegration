#ifndef RA_UI_MEMORYVIEWERVIEWMODEL_H
#define RA_UI_MEMORYVIEWERVIEWMODEL_H
#pragma once

#include "data\GameContext.hh"
#include "data\Types.hh"

#include "ui\WindowViewModelBase.hh"

#include "ui\drawing\ISurface.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MemoryViewerViewModel : public ViewModelBase, 
    protected ViewModelBase::NotifyTarget,
    protected ra::data::GameContext::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 MemoryViewerViewModel() noexcept;
    ~MemoryViewerViewModel() = default;

    MemoryViewerViewModel(const MemoryViewerViewModel&) noexcept = delete;
    MemoryViewerViewModel& operator=(const MemoryViewerViewModel&) noexcept = delete;
    MemoryViewerViewModel(MemoryViewerViewModel&&) noexcept = delete;
    MemoryViewerViewModel& operator=(MemoryViewerViewModel&&) noexcept = delete;
    
    void DoFrame();

    bool NeedsRedraw() const { return m_bNeedsRedraw; }

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
        Black = 0, // default
        Red,       // selected
        RedOnBlack,// selected and highlighted
        Blue,      // has notes
        Green,     // has bookmark
        Yellow,    // has frozen bookmark

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
    void SetAddress(ByteAddress value) { SetValue(AddressProperty, value); }

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
    void SetFirstAddress(ByteAddress value) { SetValue(FirstAddressProperty, value); }

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
    void SetNumVisibleLines(int value) { SetValue(NumVisibleLinesProperty, value); }


    void OnClick(int nX, int nY);
    void OnResized(_UNUSED int nWidth, int nHeight);

    void AdvanceCursor();
    void RetreatCursor();
    void AdvanceCursorWord();
    void RetreatCursorWord();
    void AdvanceCursorLine();
    void RetreatCursorLine();
    void AdvanceCursorPage();
    void RetreatCursorPage();

protected:
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;

    // GameContext::NotifyTarget
    void OnActiveGameChanged() override;
    void OnCodeNoteChanged(ra::ByteAddress, const std::wstring&) override;

    unsigned char* m_pMemory;
    unsigned char* m_pColor;

    static constexpr int MaxLines = 128;

private:
    void BuildFontSurface();
    void RenderAddresses();
    void RenderHeader();
    void WriteChar(int nX, int nY, TextColor nColor, int hexChar);
    void UpdateColors();

    std::unique_ptr<ra::ui::drawing::ISurface> m_pSurface;
    std::unique_ptr<ra::ui::drawing::ISurface> m_pFontSurface;
    std::unique_ptr<unsigned char[]> m_pBuffer;

    ra::ui::Size m_szChar;

    bool m_bNeedsRedraw = true;
    bool m_bUpperNibbleSelected = true;
    size_t m_nTotalMemorySize = 0;
    int m_nFont = 0;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MEMORYVIEWERVIEWMODEL_H
