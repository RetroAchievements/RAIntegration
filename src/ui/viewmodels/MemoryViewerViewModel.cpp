#include "MemoryViewerViewModel.hh"

#include "RA_Defs.h"

#include "data\context\EmulatorContext.hh"
#include "data\context\GameContext.hh"

#include "ui\EditorTheme.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty MemoryViewerViewModel::AddressProperty("MemoryViewerViewModel", "Address", 0);
const IntModelProperty MemoryViewerViewModel::FirstAddressProperty("MemoryViewerViewModel", "FirstAddress", 0);
const IntModelProperty MemoryViewerViewModel::NumVisibleLinesProperty("MemoryViewerViewModel", "NumVisibleLines", 8);
const IntModelProperty MemoryViewerViewModel::SizeProperty("MemoryViewerViewModel", "Size", ra::etoi(MemSize::EightBit));
const IntModelProperty MemoryViewerViewModel::PendingAddressProperty("MemoryViewerViewModel", "PendingAddress", 0);

constexpr uint8_t STALE_COLOR = 0x80;
constexpr uint8_t HIGHLIGHTED_COLOR = STALE_COLOR | gsl::narrow_cast<uint8_t>(ra::etoi(MemoryViewerViewModel::TextColor::Selected));
constexpr int ADDRESS_COLUMN_WIDTH = 10;
constexpr int BASE_MEMORY_VIEWER_WIDTH_IN_CHARACTERS =
    (ADDRESS_COLUMN_WIDTH + 16 * 3 - 1); // address column + 16 bytes "XX " - last space

std::unique_ptr<ra::ui::drawing::ISurface> MemoryViewerViewModel::s_pFontSurface;
std::unique_ptr<ra::ui::drawing::ISurface> MemoryViewerViewModel::s_pFontASCIISurface;
ra::ui::Size MemoryViewerViewModel::s_szChar;
int MemoryViewerViewModel::s_nFont = 0;

constexpr char FIRST_ASCII_CHAR = 32;
constexpr char LAST_ASCII_CHAR = 127;
constexpr size_t NUM_ASCII_CHARS = (LAST_ASCII_CHAR - FIRST_ASCII_CHAR + 1);

class MemoryViewerViewModel::MemoryBookmarkMonitor : protected ViewModelCollectionBase::NotifyTarget
{
public:
    MemoryBookmarkMonitor(MemoryViewerViewModel& pOwner)
        : m_pOwner(pOwner),
          m_vBookmarks(ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks.Bookmarks())
    {
        m_vBookmarks.AddNotifyTarget(*this);
    }

    ~MemoryBookmarkMonitor()
    {
        GSL_SUPPRESS_F6 m_vBookmarks.RemoveNotifyTarget(*this);
    }

    MemoryBookmarkMonitor(const MemoryBookmarkMonitor&) noexcept = delete;
    MemoryBookmarkMonitor& operator=(const MemoryBookmarkMonitor&) noexcept = delete;
    MemoryBookmarkMonitor(MemoryBookmarkMonitor&&) noexcept = delete;
    MemoryBookmarkMonitor& operator=(MemoryBookmarkMonitor&&) noexcept = delete;

protected:
    void OnEndViewModelCollectionUpdate() override
    {
        m_pOwner.UpdateColors();
    }

    void OnViewModelAdded(gsl::index nIndex) override
    {
        if (!m_vBookmarks.IsUpdating())
        {
            const auto* pBookmark = m_vBookmarks.GetItemAt(nIndex);
            if (pBookmark != nullptr)
                m_pOwner.UpdateColor(pBookmark->GetAddress());
        }
    }

    void OnViewModelRemoved(gsl::index) override
    {
        if (!m_vBookmarks.IsUpdating())
            m_pOwner.UpdateColors();
    }

    void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override
    {
        if (args.Property == ra::ui::viewmodels::MemoryBookmarksViewModel::MemoryBookmarkViewModel::BehaviorProperty)
        {
            // different behaviors appear as different colors
            const auto* pBookmark = m_vBookmarks.GetItemAt(nIndex);
            if (pBookmark != nullptr)
                m_pOwner.UpdateColor(pBookmark->GetAddress());
        }
        else if (args.Property == ra::ui::viewmodels::MemoryBookmarksViewModel::MemoryBookmarkViewModel::AddressProperty)
        {
            m_pOwner.UpdateColor(static_cast<ra::ByteAddress>(args.tOldValue));
            m_pOwner.UpdateColor(static_cast<ra::ByteAddress>(args.tNewValue));
        }
    }

private:
    ViewModelCollection<ra::ui::viewmodels::MemoryBookmarksViewModel::MemoryBookmarkViewModel>& m_vBookmarks;
    MemoryViewerViewModel& m_pOwner;
};

MemoryViewerViewModel::MemoryViewerViewModel()
{
    m_pBuffer.reset(new uint8_t[MaxLines * 16 * 3]);

    m_pMemory = m_pBuffer.get();
    memset(m_pMemory, 0, MaxLines * 16);

    m_pColor = m_pMemory + MaxLines * 16;
    memset(m_pColor, STALE_COLOR, MaxLines * 16);

    m_pColor[0] = HIGHLIGHTED_COLOR;

    m_pInvalid = m_pMemory + MaxLines * 16 * 2;
    memset(m_pInvalid, 0, MaxLines * 16);
}

void MemoryViewerViewModel::InitializeNotifyTargets()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.AddNotifyTarget(*this);

    m_nTotalMemorySize = gsl::narrow<ra::ByteAddress>(pEmulatorContext.TotalMemorySize());

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.AddNotifyTarget(*this);

    m_bReadOnly = (pGameContext.GameId() == 0);

    m_pBookmarkMonitor.reset(new MemoryBookmarkMonitor(*this));
}

void MemoryViewerViewModel::InitializeFixedViewer(ra::ByteAddress nAddress)
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    m_nTotalMemorySize = gsl::narrow<ra::ByteAddress>(pEmulatorContext.TotalMemorySize());

    m_bReadOnly = true;
    m_bAddressFixed = true;

    if (nAddress >= m_nTotalMemorySize)
        nAddress = m_nTotalMemorySize - 1;

    SetFirstAddress(nAddress & ~0x0F);
    SetValue(AddressProperty, nAddress);
}

void MemoryViewerViewModel::DetachNotifyTargets() noexcept
{
    m_pBookmarkMonitor.reset();
}

MemoryViewerViewModel::~MemoryViewerViewModel()
{
    // empty function definition required to generate destroy for forward-declared MemoryBookmarkMonitor
}

static constexpr int NibblesForSize(MemSize nSize)
{
    return ra::data::MemSizeBytes(nSize) * 2;
}

int MemoryViewerViewModel::NibblesPerWord() const
{
    return NibblesForSize(GetSize());
}

int MemoryViewerViewModel::GetSelectedNibbleOffset() const
{
    const auto nSize = GetSize();
    switch (nSize)
    {
        case MemSize::SixteenBit:
        case MemSize::ThirtyTwoBit:
            return (NibblesForSize(nSize) - m_nSelectedNibble - 1);

        default:
            return m_nSelectedNibble ^ 1;
    }
}

static MemoryViewerViewModel::TextColor GetColor(ra::ByteAddress nAddress,
    const ra::ui::viewmodels::MemoryBookmarksViewModel& pBookmarksViewModel,
    const ra::data::context::GameContext& pGameContext, bool bCheckNotes = true)
{
    if (pBookmarksViewModel.HasBookmark(nAddress))
    {
        if (pBookmarksViewModel.HasFrozenBookmark(nAddress))
            return MemoryViewerViewModel::TextColor::Frozen;

        return MemoryViewerViewModel::TextColor::HasBookmark;
    }

    if (bCheckNotes)
    {
        const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
        if (pCodeNotes != nullptr)
        {
            const auto nNoteStart = pCodeNotes->FindCodeNoteStart(nAddress);
            if (nNoteStart != 0xFFFFFFFF)
            {
                if (nNoteStart != nAddress)
                    return MemoryViewerViewModel::TextColor::HasSurrogateNote;

                const auto* pNote = pCodeNotes->FindCodeNote(nAddress);
                if (pNote != nullptr && !pNote->empty())
                    return MemoryViewerViewModel::TextColor::HasNote;
            }
        }
    }

    return MemoryViewerViewModel::TextColor::Default;
}

void MemoryViewerViewModel::UpdateColors()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

    const auto nVisibleLines = GetNumVisibleLines();
    const auto nFirstAddress = GetFirstAddress();

    // identify bookmarks
    const auto& pBookmarksViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    for (int i = 0; i < nVisibleLines * 16; ++i)
        m_pColor[i] = STALE_COLOR | gsl::narrow_cast<uint8_t>(ra::etoi(GetColor(nFirstAddress + i, pBookmarksViewModel, pGameContext, false)));

    // apply code notes
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (pCodeNotes != nullptr)
    {
        const auto nStopAddress = nFirstAddress + nVisibleLines * 16;
        pCodeNotes->EnumerateCodeNotes([nFirstAddress, nStopAddress, this](ra::ByteAddress nAddress, unsigned nBytes, const std::wstring& sNote) {
            if (nAddress + nBytes <= nFirstAddress) // not to viewing window yet
                return true;
            if (nAddress >= nStopAddress) // past viewing window
                return false;
            if (sNote.empty()) // ignore deleted notes
                return true;

            uint8_t* pOffset = m_pColor;
            Expects(pOffset != nullptr);
            if (nAddress < nFirstAddress)
            {
                nBytes -= (nFirstAddress - nAddress);
                nAddress = nFirstAddress;

                // so first processed surrogate will be nFirstAddress
                --pOffset;
                ++nBytes;
            }
            else
            {
                pOffset += (nAddress - nFirstAddress);
                if ((*pOffset & 0x0F) == ra::etoi(TextColor::Default))
                    *pOffset |= ra::etoi(TextColor::HasNote);
            }

            if (nBytes > 1)
            {
                nBytes = std::min(nBytes, nStopAddress - nAddress + 1);
                for (unsigned i = 1; i < nBytes; ++i)
                {
                    ++pOffset;

                    if ((*pOffset & 0x0F) == ra::etoi(TextColor::Default))
                        *pOffset |= ra::etoi(TextColor::HasSurrogateNote);
                }
            }

            return true;
        }, true);
    }

    // flag invalid regions
    UpdateInvalidRegions();

    // update cursor
    UpdateHighlight(GetAddress(), NibblesPerWord() / 2, 0);

    m_nNeedsRedraw |= REDRAW_MEMORY;
}

void MemoryViewerViewModel::UpdateInvalidRegions()
{
    const auto nVisibleLines = ra::to_unsigned(GetNumVisibleLines());
    const auto nFirstAddress = GetFirstAddress();
    const auto nVisibleBytes = std::min(m_nTotalMemorySize - nFirstAddress, nVisibleLines * 16);

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (pEmulatorContext.HasInvalidRegions())
    {
        for (unsigned i = 0; i < nVisibleBytes; ++i)
            m_pInvalid[i] = pEmulatorContext.IsValidAddress(nFirstAddress + i) ? 0 : 1;
    }
    else
    {
        memset(m_pInvalid, 0, nVisibleBytes);
    }

    memset(&m_pInvalid[nVisibleBytes], 1, MaxLines * 16 - nVisibleBytes);
}

void MemoryViewerViewModel::UpdateHighlight(ra::ByteAddress nAddress, int nNewLength, int nOldLength)
{
    if (nNewLength == nOldLength)
        return;

    const auto nFirstAddress = GetFirstAddress();
    if (nAddress < nFirstAddress && nAddress + nNewLength <= nFirstAddress && nAddress + nOldLength <= nFirstAddress)
        return;

    const auto nVisibleLines = GetNumVisibleLines();
    const auto nMaxAddress = nFirstAddress + nVisibleLines * 16;
    if (nAddress >= nMaxAddress)
        return;

    m_nNeedsRedraw |= REDRAW_MEMORY;

    for (int i = 0; i < nNewLength; ++i)
    {
        if (nAddress >= nFirstAddress)
            m_pColor[nAddress - nFirstAddress] = HIGHLIGHTED_COLOR;

        if (++nAddress == nMaxAddress)
            return;
    }

    if (nOldLength > nNewLength)
    {
        const auto& pBookmarksViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

        while (nOldLength > nNewLength)
        {
            if (nAddress >= nFirstAddress)
                m_pColor[nAddress - nFirstAddress] = STALE_COLOR | gsl::narrow_cast<uint8_t>(ra::etoi(GetColor(nAddress, pBookmarksViewModel, pGameContext)));

            if (++nAddress == nMaxAddress)
                return;

            --nOldLength;
        }
    }
}

void MemoryViewerViewModel::UpdateSelectedNibble(int nNewNibble)
{
    if (nNewNibble != m_nSelectedNibble)
    {
        const auto nAddress = GetAddress();
        const auto nFirstAddress = GetFirstAddress();
        m_pColor[nAddress - nFirstAddress + GetSelectedNibbleOffset() / 2] |= STALE_COLOR;
        m_nSelectedNibble = nNewNibble;

        m_pColor[nAddress - nFirstAddress + GetSelectedNibbleOffset() / 2] |= STALE_COLOR;
        m_nNeedsRedraw |= REDRAW_MEMORY;
    }
}

void MemoryViewerViewModel::SetAddress(ra::ByteAddress nValue)
{
    if (m_bAddressFixed)
        return;

    int nSignedValue = ra::to_signed(nValue);
    if (m_nTotalMemorySize > 0)
    {
        if (nSignedValue < 0)
            nSignedValue = 0;
        else if (nSignedValue >= gsl::narrow_cast<int>(m_nTotalMemorySize))
            nSignedValue = gsl::narrow_cast<int>(m_nTotalMemorySize) - 1;

        SetValue(AddressProperty, nSignedValue);
    }
    else
    {
        SetValue(PendingAddressProperty, gsl::narrow_cast<int>(nValue));
    }
}

void MemoryViewerViewModel::SetFirstAddress(ra::ByteAddress value)
{
    int nSignedValue = ra::to_signed(value);
    if (nSignedValue < 0)
    {
        nSignedValue = 0;
    }
    else
    {
        const auto nVisibleLines = GetNumVisibleLines();
        const auto nMaxFirstAddress = ra::to_signed((m_nTotalMemorySize + 15) & ~0x0F) - (nVisibleLines * 16);

        if (nMaxFirstAddress < 0)
            nSignedValue = 0;
        else if (nSignedValue > nMaxFirstAddress)
            nSignedValue = nMaxFirstAddress;
        else
            nSignedValue &= ~0x0F;
    }

    SetValue(FirstAddressProperty, nSignedValue);
}

void MemoryViewerViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == AddressProperty)
    {
        auto nFirstAddress = GetFirstAddress();
        const auto nVisibleLines = GetNumVisibleLines();
        const auto nMaxAddress = nFirstAddress + nVisibleLines * 16;

        const auto nBytes = NibblesPerWord() / 2;
        UpdateHighlight(ra::to_unsigned(args.tOldValue), 0, nBytes);

        const auto nNewValue = ra::to_unsigned(args.tNewValue);
        if (nNewValue < nFirstAddress || nNewValue >= nMaxAddress)
        {
            nFirstAddress = (nNewValue & ~0x0F) - ((nVisibleLines / 2) * 16);
            SetFirstAddress(nFirstAddress);

            nFirstAddress = GetFirstAddress();
        }

        UpdateHighlight(ra::to_unsigned(args.tNewValue), nBytes, 0);
        m_nSelectedNibble = 0;

        m_nNeedsRedraw |= REDRAW_ADDRESSES | REDRAW_HEADERS;
    }
    else if (args.Property == FirstAddressProperty)
    {
        m_nNeedsRedraw |= REDRAW_ADDRESSES;

        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
        pEmulatorContext.ReadMemory(args.tNewValue, m_pMemory, gsl::narrow_cast<size_t>(GetNumVisibleLines()) * 16);

        UpdateColors();
    }
    else if (args.Property == SizeProperty)
    {
        const int nOldBytes = NibblesForSize(ra::itoe<MemSize>(args.tOldValue)) / 2;
        const int nNewBytes = NibblesForSize(ra::itoe<MemSize>(args.tNewValue)) / 2;
        UpdateHighlight(GetAddress(), nNewBytes, nOldBytes);
        ResetSurface();

        DetermineIfASCIIShouldBeVisible();
    }
    else if (args.Property == NumVisibleLinesProperty)
    {
        if (args.tNewValue > args.tOldValue)
        {
            const auto nFirstAddress = GetFirstAddress();

            const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
            pEmulatorContext.ReadMemory(nFirstAddress, m_pMemory, gsl::narrow_cast<size_t>(args.tNewValue) * 16);

            UpdateInvalidRegions();
            UpdateColors();
        }

        ResetSurface();
    }

    ViewModelBase::OnValueChanged(args);
}

void MemoryViewerViewModel::AdvanceCursor()
{
    if (m_bAddressFixed)
        return;

    const auto nNibblesPerWord = NibblesPerWord();
    if (m_nSelectedNibble < nNibblesPerWord - 1)
    {
        const auto nAddress = GetAddress();
        if (nNibblesPerWord > 2)
        {
            const auto nAddressMask = (nNibblesPerWord / 2) - 1;
            if (nAddress & nAddressMask)
            {
                // address is in the middle of a word, move to start of next word
                AdvanceCursorWord();
                return;
            }
        }

        UpdateSelectedNibble(m_nSelectedNibble + 1);
    }
    else
    {
        AdvanceCursorWord();
    }
}

void MemoryViewerViewModel::RetreatCursor()
{
    if (m_bAddressFixed)
        return;

    const auto nAddress = GetAddress();
    const auto nFirstAddress = GetFirstAddress();

    if (m_nSelectedNibble > 0)
    {
        UpdateSelectedNibble(m_nSelectedNibble - 1);
    }
    else if (nAddress > 0)
    {
        const auto nNibblesPerWord = NibblesPerWord();
        const auto nBytesPerWord = nNibblesPerWord / 2;
        if (nBytesPerWord > 1 && (nAddress & (nBytesPerWord - 1)))
        {
            // address is in the middle of a word, move to start of the word
            SetAddress(nAddress & ~(nBytesPerWord - 1));
            UpdateSelectedNibble(0);
        }
        else
        {
            // move to end of previous word
            const auto nNewAddress = nAddress - nBytesPerWord;
            if (nNewAddress < nFirstAddress)
                SetFirstAddress(nFirstAddress - 16);

            SetAddress(nNewAddress);
            UpdateSelectedNibble(nNibblesPerWord - 1);
        }
    }
}

void MemoryViewerViewModel::AdvanceCursorWord()
{
    if (m_bAddressFixed)
        return;

    const auto nAddress = GetAddress();
    const auto nBytesPerWord = NibblesPerWord() / 2;
    const auto nNewAddress = (nAddress + nBytesPerWord) & ~(nBytesPerWord - 1);

    if (nNewAddress < m_nTotalMemorySize)
    {
        if ((nNewAddress & 0x0F) == 0)
        {
            const auto nFirstAddress = GetFirstAddress();
            if (nNewAddress == nFirstAddress + GetNumVisibleLines() * 16)
                SetFirstAddress(nFirstAddress + 16);
        }

        SetAddress(nNewAddress);
        m_nSelectedNibble = 0;
    }
}

void MemoryViewerViewModel::RetreatCursorWord()
{
    if (m_bAddressFixed)
        return;

    const auto nAddress = GetAddress();
    if (m_nSelectedNibble > 0)
    {
        UpdateSelectedNibble(0);
    }
    else if (nAddress > 0)
    {
        const auto nBytesPerWord = NibblesPerWord() / 2;
        if (nBytesPerWord > 1 && (nAddress & (nBytesPerWord - 1)))
        {
            // address is in the middle of a word, adjust to start of the word
            SetAddress(nAddress & ~(nBytesPerWord - 1));
        }
        else
        {
            // move to beginning of previous word; adjust first address if necessary
            const auto nNewAddress = nAddress - nBytesPerWord;
            if ((nAddress & 0x0F) == 0)
            {
                const auto nFirstAddress = GetFirstAddress();
                if (nNewAddress < nFirstAddress)
                    SetFirstAddress(nFirstAddress - 16);
            }

            SetAddress(nNewAddress);
        }
        m_nSelectedNibble = 0;
    }
}

void MemoryViewerViewModel::AdvanceCursorLine()
{
    if (m_bAddressFixed)
        return;

    const auto nAddress = GetAddress();
    if (nAddress < m_nTotalMemorySize - 16)
    {
        const auto nSelectedNibble = m_nSelectedNibble;
        const auto nNewAddress = nAddress + 16;
        const auto nFirstAddress = GetFirstAddress();
        if (nNewAddress >= nFirstAddress + GetNumVisibleLines() * 16)
            SetFirstAddress(nFirstAddress + 16);

        SetAddress(nNewAddress);
        UpdateSelectedNibble(nSelectedNibble);
    }
}

void MemoryViewerViewModel::RetreatCursorLine()
{
    if (m_bAddressFixed)
        return;

    const auto nAddress = GetAddress();
    if (nAddress > 15)
    {
        const auto nSelectedNibble = m_nSelectedNibble;
        const auto nNewAddress = nAddress - 16;
        const auto nFirstAddress = GetFirstAddress();
        if (nNewAddress < nFirstAddress)
            SetFirstAddress(nFirstAddress - 16);

        SetAddress(nNewAddress);
        UpdateSelectedNibble(nSelectedNibble);
    }
}

void MemoryViewerViewModel::AdvanceCursorPage()
{
    if (m_bAddressFixed)
        return;

    const auto nVisibleLines = GetNumVisibleLines();
    const auto nAddress = GetAddress();
    if (nAddress < m_nTotalMemorySize - 16)
    {
        const auto nSelectedNibble = m_nSelectedNibble;

        const auto nFirstAddress = GetFirstAddress();
        const auto nMaxFirstAddress = m_nTotalMemorySize - nVisibleLines * 16;
        const auto nNewFirstAddress = std::min(nFirstAddress + (nVisibleLines - 1) * 16, nMaxFirstAddress);
        SetFirstAddress(nNewFirstAddress);

        auto nNewAddress = nAddress + (nVisibleLines - 1) * 16;
        if (nNewAddress >= m_nTotalMemorySize)
            nNewAddress = ((m_nTotalMemorySize - 15) & ~0x0F) | (nAddress & 0x0F);
        SetAddress(nNewAddress);
        UpdateSelectedNibble(nSelectedNibble);
    }
}

void MemoryViewerViewModel::RetreatCursorPage()
{
    if (m_bAddressFixed)
        return;

    const auto nAddress = GetAddress();
    if (nAddress > 15)
    {
        const auto nSelectedNibble = m_nSelectedNibble;

        const auto nFirstAddress = GetFirstAddress();
        const auto nVisibleLines = GetNumVisibleLines();
        const auto nNewFirstAddress = std::max(ra::to_signed(nFirstAddress) - (nVisibleLines - 1) * 16, 0);
        SetFirstAddress(nNewFirstAddress);

        auto nNewAddress = nAddress - (nVisibleLines - 1) * 16;
        if (nNewAddress < 0)
            nNewAddress = (nAddress & 0x0F);
        SetAddress(nNewAddress);
        UpdateSelectedNibble(nSelectedNibble);
    }
}

bool MemoryViewerViewModel::IncreaseCurrentValue(uint32_t nModifier)
{
    if (m_bReadOnly)
        return false;

    const auto nAddress = GetAddress();
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    auto nMem = pEmulatorContext.ReadMemory(GetAddress(), GetSize());
    auto const nMaxValue = ra::data::MemSizeMax(GetSize());

    if (nMem >= nMaxValue)
        return false;
    if ((nMaxValue - nMem) < nModifier)
        nModifier = (nMaxValue - nMem);
    nMem += nModifier;

    pEmulatorContext.WriteMemory(nAddress, GetSize(), nMem);
    return true;
}

bool MemoryViewerViewModel::DecreaseCurrentValue(uint32_t nModifier)
{
    if (m_bReadOnly)
        return false;

    auto const nAddress = GetAddress();
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    auto nMem = pEmulatorContext.ReadMemory(GetAddress(), GetSize());

    if (nMem == 0)
        return false;
    if (nMem < nModifier)
        nModifier = nMem;
    nMem -= nModifier;

    pEmulatorContext.WriteMemory(nAddress, GetSize(), nMem);
    return true;
}

void MemoryViewerViewModel::OnActiveGameChanged()
{
    m_nNeedsRedraw |= REDRAW_ADDRESSES;

    UpdateColors();

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    m_bReadOnly = (pGameContext.GameId() == 0);
}

void MemoryViewerViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNote)
{
    const auto nFirstAddress = GetFirstAddress();
    if (nAddress < nFirstAddress)
        return;

    auto nOffset = nAddress - nFirstAddress;
    const auto nVisibleLines = GetNumVisibleLines();
    if (nOffset >= ra::to_unsigned(nVisibleLines * 16))
        return;

    const auto& pBookmarksViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    Expects(pCodeNotes != nullptr);

    const auto nSelectedAddress = GetAddress();

    const auto nMax = nFirstAddress + nVisibleLines * 16 - nAddress;
    const auto nSize = std::min(pCodeNotes->GetCodeNoteBytes(nAddress), nMax);
    for (unsigned i = 0; i < nSize; ++i)
    {
        auto nNewColor = (nAddress == nSelectedAddress) ?
            ra::itoe<TextColor>(HIGHLIGHTED_COLOR & 0x0F):
            GetColor(nAddress, pBookmarksViewModel, pGameContext, false);

        if (nNewColor == TextColor::Default)
        {
            if (i != 0)
                nNewColor = TextColor::HasSurrogateNote;
            else if (!sNote.empty())
                nNewColor = TextColor::HasNote;
        }

        if ((m_pColor[nOffset] & 0x0F) != ra::etoi(nNewColor))
        {
            m_pColor[nOffset] = STALE_COLOR | gsl::narrow_cast<uint8_t>(ra::etoi(nNewColor));
            m_nNeedsRedraw |= REDRAW_MEMORY;
        }

        ++nAddress;
        ++nOffset;
    }

    // if the note size shrunk, clear out he surrogate indicators
    const auto nNextAddress = pCodeNotes->GetNextNoteAddress(nAddress);
    const auto nMaxOffset = ra::to_unsigned(nVisibleLines) * 16;
    while (nAddress < nNextAddress && nOffset < nMaxOffset)
    {
        if (ra::itoe<TextColor>(m_pColor[nOffset] & 0x0F) != TextColor::HasSurrogateNote)
            break;

        const auto nNewColor = GetColor(nAddress, pBookmarksViewModel, pGameContext, false);
        m_pColor[nOffset] = STALE_COLOR | gsl::narrow_cast<uint8_t>(nNewColor);
        m_nNeedsRedraw |= REDRAW_MEMORY;

        ++nAddress;
        ++nOffset;
    }
}

void MemoryViewerViewModel::OnTotalMemorySizeChanged()
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    const bool bTotalMemorySizeWasZero = (m_nTotalMemorySize == 0);
    m_nTotalMemorySize = gsl::narrow_cast<ra::ByteAddress>(pEmulatorContext.TotalMemorySize());

    if (m_nTotalMemorySize == 0)
    {
        // size changing to 0, clear out addresses
        SetValue(AddressProperty, AddressProperty.GetDefaultValue());
        SetValue(PendingAddressProperty, PendingAddressProperty.GetDefaultValue());
    }
    else
    {
        if (bTotalMemorySizeWasZero)
        {
            // size changing from 0, see if a memory address was requested
            const auto nPendingAddress = gsl::narrow_cast<ra::ByteAddress>(GetValue(PendingAddressProperty));
            if (nPendingAddress > 0)
            {
                SetValue(PendingAddressProperty, PendingAddressProperty.GetDefaultValue());
                SetAddress(nPendingAddress);
            }
        }

        // make sure the current memory address is valid
        if (GetAddress() >= m_nTotalMemorySize)
            SetValue(AddressProperty, gsl::narrow_cast<int>(m_nTotalMemorySize - 1));
    }

    const auto nVisibleLines = GetNumVisibleLines();
    if (m_nTotalMemorySize < ra::to_unsigned(nVisibleLines * 16))
    {
        SetFirstAddress(0);
    }
    else
    {
        // we don't expect to hit this code - memory should only shrink when calling ClearMemoryBanks()
        // but there are unit tests for it just in case
        const auto nMaxFirstAddress = m_nTotalMemorySize - nVisibleLines * 16;
        if (GetFirstAddress() > nMaxFirstAddress)
            SetFirstAddress(nMaxFirstAddress);
    }

    UpdateInvalidRegions();

    ResetSurface();
}

void MemoryViewerViewModel::OnByteWritten(ra::ByteAddress nAddress, uint8_t nValue)
{
    auto nFirstAddress = GetFirstAddress();
    if (nAddress < nFirstAddress)
        return;

    const auto nVisibleLines = GetNumVisibleLines();
    const auto nMaxAddress = nFirstAddress + nVisibleLines * 16;
    if (nAddress < nMaxAddress)
    {
        m_pMemory[nAddress - nFirstAddress] = nValue;
        m_pColor[nAddress - nFirstAddress] |= STALE_COLOR;
        m_nNeedsRedraw |= REDRAW_MEMORY;
    }
}

#pragma warning(push)
#pragma warning(disable : 5045)

void MemoryViewerViewModel::UpdateColor(ra::ByteAddress nAddress)
{
    auto nFirstAddress = GetFirstAddress();
    if (nAddress < nFirstAddress)
        return;

    const auto nVisibleLines = GetNumVisibleLines();
    const auto nMaxAddress = nFirstAddress + nVisibleLines * 16;
    if (nAddress < nMaxAddress)
    {
        const auto nColor = ra::itoe<TextColor>(m_pColor[nAddress - nFirstAddress] & 0x0F);

        // byte is selected, ignore
        if (nColor == TextColor::Selected)
            return;

        // byte is not selected, update
        const auto& pBookmarksViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

        const auto nNewColor = GetColor(nAddress, pBookmarksViewModel, pGameContext);
        if (nNewColor != nColor)
        {
            m_pColor[nAddress - nFirstAddress] = STALE_COLOR | gsl::narrow_cast<uint8_t>(ra::etoi(nNewColor));
            m_nNeedsRedraw |= REDRAW_MEMORY;
        }
    }
}

#pragma warning(pop)

void MemoryViewerViewModel::OnClick(int nX, int nY)
{
    if (m_bAddressFixed)
        return;

    const int nRow = nY / s_szChar.Height;
    if (nRow == 0)
        return;

    const auto nNibblesPerWord = NibblesPerWord();
    const auto nWordSpacing = nNibblesPerWord + 1;
    const auto nBytesPerWord = nNibblesPerWord / 2;
    const auto nFirstAddress = GetFirstAddress();
    ra::ByteAddress nNewAddress = nFirstAddress + (nRow - 1) * 16;
    int nNewNibble = 0;

    int nColumn = nX / s_szChar.Width;
    if (nColumn > BASE_MEMORY_VIEWER_WIDTH_IN_CHARACTERS + 1)
    {
        nNewAddress += (nColumn - BASE_MEMORY_VIEWER_WIDTH_IN_CHARACTERS - 2);
    }
    else
    {
        if (nColumn < ADDRESS_COLUMN_WIDTH)
        {
            if (nColumn < ADDRESS_COLUMN_WIDTH - 1 || (nX % s_szChar.Width) < (s_szChar.Width / 2))
                return;

            ++nColumn;
        }

        nNewAddress += ((nColumn - ADDRESS_COLUMN_WIDTH) / nWordSpacing) * nBytesPerWord;
        nNewNibble = (nColumn - ADDRESS_COLUMN_WIDTH) % nWordSpacing;
        if (nNewNibble == nNibblesPerWord)
        {
            // when clicking between data, adjust to the nearest data
            const int nMargin = nX % s_szChar.Width;
            if (nMargin < s_szChar.Width / 2)
            {
                nNewNibble--;
            }
            else if ((nNewAddress & 0x0F) < ra::to_unsigned(16 - nBytesPerWord))
            {
                nNewAddress += nBytesPerWord;
                nNewNibble = 0;
            }
        }
    }

    if (nNewAddress < m_nTotalMemorySize)
    {
        if (nNewNibble != nNibblesPerWord)
        {
            SetAddress(nNewAddress);

            m_nSelectedNibble = nNewNibble;
            for (int i = 0; i < nBytesPerWord; ++i)
                m_pColor[nNewAddress - nFirstAddress + i] |= STALE_COLOR;

            m_nNeedsRedraw |= REDRAW_MEMORY;
        }
    }
}

void MemoryViewerViewModel::OnResized(int nWidth, int nHeight)
{
    if (s_pFontSurface == nullptr)
        BuildFontSurface();

    SetNumVisibleLines((nHeight / s_szChar.Height) - 1);

    const int nWordSpacing = NibblesPerWord() + 1;
    const int nBytesPerWord = nWordSpacing / 2;
    const int nWordsPerLine = 16 / nBytesPerWord;
    const int nNeededWidthForASCII = (ADDRESS_COLUMN_WIDTH +         // address column
                                      nWordsPerLine * nWordSpacing + // memory values
                                      16 + 1) * s_szChar.Width;      // ASCII region
    m_bWideEnoughForASCII = nWidth > nNeededWidthForASCII;
    DetermineIfASCIIShouldBeVisible();
}

void MemoryViewerViewModel::DetermineIfASCIIShouldBeVisible()
{
    const bool bShowASCII = m_bWideEnoughForASCII && GetSize() == MemSize::EightBit;
    if (bShowASCII != m_bShowASCII)
    {
        m_bShowASCII = bShowASCII;
        ResetSurface();
    }
}

bool MemoryViewerViewModel::OnChar(char c)
{
    if (m_nTotalMemorySize == 0 || m_bReadOnly)
        return false;

    uint8_t value = 0;
    switch (c)
    {
        case '0': value = 0; break;
        case '1': value = 1; break;
        case '2': value = 2; break;
        case '3': value = 3; break;
        case '4': value = 4; break;
        case '5': value = 5; break;
        case '6': value = 6; break;
        case '7': value = 7; break;
        case '8': value = 8; break;
        case '9': value = 9; break;
        case 'a': case 'A': value = 10; break;
        case 'b': case 'B': value = 11; break;
        case 'c': case 'C': value = 12; break;
        case 'd': case 'D': value = 13; break;
        case 'e': case 'E': value = 14; break;
        case 'f': case 'F': value = 15; break;
        default:
            return false;
    }

    auto nAddress = GetAddress();
    auto nFirstAddress = GetFirstAddress();
    const auto nVisibleLines = GetNumVisibleLines();
    const auto nMaxAddress = nFirstAddress + nVisibleLines * 16;

    // make sure the cursor is on screen (same logic as in OnValueChanged(AddressProperty))
    if (nAddress < nFirstAddress || nAddress >= nMaxAddress)
    {
        nFirstAddress = (nAddress & ~0x0F) - ((nVisibleLines / 2) * 16);
        SetFirstAddress(nFirstAddress);

        nFirstAddress = GetFirstAddress();
    }

    // adjust for 16-bit and 32-bit views
    auto nSelectedNibble = GetSelectedNibbleOffset();
    while (nSelectedNibble > 1)
    {
        ++nAddress;
        nSelectedNibble -= 2;
    }

    auto nIndex = nAddress - nFirstAddress;
    if (m_pInvalid[nIndex])
    {
        // can't update "--"
        AdvanceCursor();
        return true;
    }

    auto nByte = m_pMemory[nIndex];
    if (nSelectedNibble != 0)
    {
        nByte &= 0x0F;
        nByte |= (value << 4);
    }
    else
    {
        nByte &= 0xF0;
        nByte |= value;
    }

    m_pMemory[nIndex] = nByte;
    m_pColor[nIndex] |= STALE_COLOR;
    m_nNeedsRedraw |= REDRAW_MEMORY;

    // push the updated value to the emulator
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.WriteMemoryByte(nAddress, nByte);

    // advance the cursor to the next nibble
    AdvanceCursor();

    return true;
}

void MemoryViewerViewModel::OnGotFocus()
{
    m_bHasFocus = true;
    UpdateHighlight(GetAddress(), NibblesPerWord() / 2, 0);
}

void MemoryViewerViewModel::OnLostFocus()
{
    m_bHasFocus = false;
    UpdateHighlight(GetAddress(), NibblesPerWord() / 2, 0);
}

#pragma warning(push)
#pragma warning(disable : 26446) // pMemory[] is unchecked   (nVisibleLines assertion enforces range)
#pragma warning(disable : 26494) // pMemory is uninitialized (initialized by ReadMemory)

void MemoryViewerViewModel::DoFrame()
{
    uint8_t pMemory[MaxLines * 16];
    const auto nAddress = GetFirstAddress();
    const auto nVisibleLines = GetNumVisibleLines();
    Expects(nVisibleLines < MaxLines);

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    pEmulatorContext.ReadMemory(nAddress, pMemory, gsl::narrow_cast<size_t>(nVisibleLines) * 16);

    constexpr int nStride = 8;
    for (int nIndex = 0; nIndex < nVisibleLines * 16; nIndex += nStride)
    {
        if (memcmp(&m_pMemory[nIndex], &pMemory[nIndex], nStride) == 0)
            continue;

        // STALE_COLOR causes the cell to be redrawn even if the color didn't actually change
        // use branchless logic for additional performance. the compiler should unroll the loop.
        for (int i = 0; i < nStride; i++)
            m_pColor[nIndex + i] |= STALE_COLOR * (m_pMemory[nIndex + i] != pMemory[nIndex + i]);

        memcpy(&m_pMemory[nIndex], &pMemory[nIndex], nStride);
        m_nNeedsRedraw |= REDRAW_MEMORY;
    }

    Redraw();
}

void MemoryViewerViewModel::Redraw()
{
    if (m_nNeedsRedraw)
    {
        if (m_pRepaintNotifyTarget != nullptr)
            m_pRepaintNotifyTarget->OnRepaintMemoryViewer();
    }
}

#pragma warning(pop)

// ====== Render ==============================================================

static constexpr std::array<wchar_t, 17> g_sHexChars = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', '-'
};

void MemoryViewerViewModel::BuildFontSurface()
{
    const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
    auto pSurface = pSurfaceFactory.CreateSurface(1, 1);

    const auto& pEditorTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();

    s_nFont = pSurface->LoadFont(pEditorTheme.FontMemoryViewer(), pEditorTheme.FontSizeMemoryViewer(), ra::ui::FontStyles::Normal);
    s_szChar = pSurface->MeasureText(s_nFont, L"0");
    s_szChar.Height--; // don't need space for dangly bits

    s_pFontSurface = pSurfaceFactory.CreateSurface(s_szChar.Width * gsl::narrow_cast<int>(g_sHexChars.size()), s_szChar.Height * ra::etoi(TextColor::NumColors));
    s_pFontSurface->FillRectangle(0, 0, s_pFontSurface->GetWidth(), s_pFontSurface->GetHeight(), pEditorTheme.ColorBackground());

    s_pFontSurface->FillRectangle(0, s_szChar.Height * ra::etoi(TextColor::Cursor),
        s_pFontSurface->GetWidth(), s_szChar.Height, pEditorTheme.ColorCursor());

    std::wstring sHexChar = L"0";
    ra::ui::Color nColor(0);
    for (int i = 0; i < ra::etoi(TextColor::NumColors); ++i)
    {
        switch (ra::itoe<TextColor>(i))
        {
            case TextColor::Default:          nColor = pEditorTheme.ColorNormal(); break;
            case TextColor::HasNote:          nColor = pEditorTheme.ColorHasNote(); break;
            case TextColor::HasSurrogateNote: nColor = pEditorTheme.ColorHasSurrogateNote(); break;
            case TextColor::HasBookmark:      nColor = pEditorTheme.ColorHasBookmark(); break;
            case TextColor::Cursor:
            case TextColor::Selected:         nColor = pEditorTheme.ColorSelected(); break;
            case TextColor::Frozen:           nColor = pEditorTheme.ColorFrozen(); break;
        }

        for (int j = 0; j < gsl::narrow_cast<int>(g_sHexChars.size()); ++j)
        {
            sHexChar.at(0) = g_sHexChars.at(j);
            s_pFontSurface->WriteText(j * s_szChar.Width, i * s_szChar.Height - 1, s_nFont, nColor, sHexChar);
        }
    }

    s_pFontASCIISurface = pSurfaceFactory.CreateSurface(s_szChar.Width * (NUM_ASCII_CHARS + 1), s_szChar.Height * 2);
    s_pFontASCIISurface->FillRectangle(0, 0,
        s_pFontASCIISurface->GetWidth(), s_pFontASCIISurface->GetHeight(), pEditorTheme.ColorBackground());
    int nX = 0;
    for (int i = FIRST_ASCII_CHAR; i <= LAST_ASCII_CHAR; i++)
    {
        sHexChar.at(0) = gsl::narrow_cast<wchar_t>(i);
        s_pFontASCIISurface->WriteText(nX, -1, s_nFont, pEditorTheme.ColorNormal(), sHexChar);
        s_pFontASCIISurface->WriteText(nX, s_szChar.Height - 1, s_nFont, pEditorTheme.ColorSelected(), sHexChar);
        nX += s_szChar.Width;
    }

    const auto nFadedNormalColor =
        ra::ui::Color::Blend(pEditorTheme.ColorNormal(), pEditorTheme.ColorBackground(), 0.4f);
    const auto nFadedSelectedColor =
        ra::ui::Color::Blend(pEditorTheme.ColorSelected(), pEditorTheme.ColorBackground(), 0.4f);
    sHexChar.at(0) = '.';
    s_pFontASCIISurface->WriteText(nX, -1, s_nFont, nFadedNormalColor, sHexChar);
    s_pFontASCIISurface->WriteText(nX, s_szChar.Height - 1, s_nFont, nFadedSelectedColor, sHexChar);
}

void MemoryViewerViewModel::ResetSurface() noexcept
{
    m_pSurface.reset();
    m_nNeedsRedraw = REDRAW_ALL;
}

#pragma warning(push)
#pragma warning(disable : 5045)

void MemoryViewerViewModel::UpdateRenderImage()
{
    if (m_nNeedsRedraw == 0)
        return;

    int nNeedsRedraw = m_nNeedsRedraw;
    m_nNeedsRedraw = 0;

    if (m_pSurface == nullptr)
    {
        if (s_pFontSurface == nullptr)
            BuildFontSurface();

        const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
        const int nHeight = (GetNumVisibleLines() + 1) * s_szChar.Height;
        int nWidth = BASE_MEMORY_VIEWER_WIDTH_IN_CHARACTERS * s_szChar.Width;
        if (m_bShowASCII)
            nWidth += (2 + 16) * s_szChar.Width;
        m_pSurface = pSurfaceFactory.CreateSurface(nWidth, nHeight);

        // background
        const auto& pEditorTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();
        m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth(), m_pSurface->GetHeight(), pEditorTheme.ColorBackground());

        // separators
        m_pSurface->FillRectangle(9 * s_szChar.Width, s_szChar.Height, 1, nHeight, pEditorTheme.ColorSeparator());
        if (m_bShowASCII)
        {
            m_pSurface->FillRectangle((BASE_MEMORY_VIEWER_WIDTH_IN_CHARACTERS + 1) * s_szChar.Width, s_szChar.Height,
                                      1, nHeight, pEditorTheme.ColorSeparator());
        }

        nNeedsRedraw = REDRAW_ALL;

        const auto nVisibleLines = GetNumVisibleLines();
        for (int i = 0; i < nVisibleLines * 16; ++i)
            m_pColor[i] |= STALE_COLOR;
    }

    if (nNeedsRedraw & REDRAW_ADDRESSES)
        RenderAddresses();
    if (nNeedsRedraw & REDRAW_HEADERS)
        RenderHeader();
    if (nNeedsRedraw & REDRAW_MEMORY)
        RenderMemory();
}

void MemoryViewerViewModel::RenderMemory()
{
    const auto nFirstAddress = GetFirstAddress();
    auto nVisibleLines = GetNumVisibleLines();
    if (nFirstAddress + nVisibleLines * 16 > m_nTotalMemorySize)
        nVisibleLines = (m_nTotalMemorySize - nFirstAddress + 15) / 16;

    const bool bBigEndian = (GetSize() == MemSize::ThirtyTwoBitBigEndian);
    const int nWordSpacing = NibblesPerWord() + 1;
    const int nBytesPerWord = nWordSpacing / 2;
    const int nWordsPerLine = 16 / nBytesPerWord;
    const int nFirstASCIIX = (ADDRESS_COLUMN_WIDTH + (nWordsPerLine * nWordSpacing) + 1) * s_szChar.Width;
    for (int i = 0; i < nVisibleLines * 16; ++i)
    {
        if (!(m_pColor[i] & STALE_COLOR))
            continue;

        const uint8_t nColor = m_pColor[i] & 0x0F;
        m_pColor[i] = nColor;

        TextColor nColorUpper = ra::itoe<TextColor>(nColor);
        TextColor nColorLower = nColorUpper;
        const bool bHasCursor = nColorUpper == TextColor::Selected && m_bHasFocus && !m_bReadOnly;

        const int nY = ((i / 16) + 1) * s_szChar.Height;

        int nX = (((i & 0x0F) / nBytesPerWord) * nWordSpacing + ADDRESS_COLUMN_WIDTH) * s_szChar.Width;
        if (nBytesPerWord > 1)
        {
            const int nByteOffset = (bBigEndian) ?
                (i % nBytesPerWord) : ((nBytesPerWord - 1) - (i % nBytesPerWord));
            nX += (nByteOffset * 2) * s_szChar.Width;

            if (bHasCursor)
            {
                if (m_nSelectedNibble / 2 == nByteOffset)
                {
                    if ((m_nSelectedNibble & 1) == 0)
                        nColorUpper = TextColor::Cursor;
                    else
                        nColorLower = TextColor::Cursor;
                }
            }
        }
        else
        {
            if (bHasCursor)
            {
                if (m_nSelectedNibble == 0)
                    nColorUpper = TextColor::Cursor;
                else
                    nColorLower = TextColor::Cursor;
            }
        }

        if (m_pInvalid[i])
        {
            constexpr int INVALID_CHAR = 16;
            WriteChar(nX, nY, nColorUpper, INVALID_CHAR);
            WriteChar(nX + s_szChar.Width, nY, nColorLower, INVALID_CHAR);
        }
        else
        {
            const auto nValue = m_pMemory[i];
            WriteChar(nX, nY, nColorUpper, nValue >> 4);
            WriteChar(nX + s_szChar.Width, nY, nColorLower, nValue & 0x0F);
        }

        if (m_bShowASCII)
        {
            auto nValue = m_pMemory[i];
            if (nValue < FIRST_ASCII_CHAR || nValue > LAST_ASCII_CHAR)
                nValue = NUM_ASCII_CHARS; // "invalid" character at LAST_ASCII_CHAR + 1
            else
                nValue -= FIRST_ASCII_CHAR;

            nX = nFirstASCIIX + (i & 0x0F) * s_szChar.Width;
            m_pSurface->DrawSurface(nX, nY, *s_pFontASCIISurface, nValue * s_szChar.Width,
                                    (ra::itoe<TextColor>(nColor) == TextColor::Selected) ? s_szChar.Height : 0,
                                    s_szChar.Width, s_szChar.Height);
        }
    }
}

#pragma warning(pop)

void MemoryViewerViewModel::WriteChar(int nX, int nY, TextColor nColor, int hexChar)
{
    m_pSurface->DrawSurface(nX, nY, *s_pFontSurface, hexChar * s_szChar.Width, ra::etoi(nColor) * s_szChar.Height, s_szChar.Width, s_szChar.Height);
}

void MemoryViewerViewModel::RenderAddresses()
{
    auto nVisibleLines = GetNumVisibleLines();
    const auto nCursorAddress = GetAddress() & ~0x0F;
    auto nFirstAddress = GetFirstAddress();
    int nY = s_szChar.Height - 1;

    const auto& pEditorTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();
    m_pSurface->FillRectangle(0, s_szChar.Height, s_szChar.Width * 8, nVisibleLines * s_szChar.Height, pEditorTheme.ColorBackground());

    if (nFirstAddress + nVisibleLines * 16 > m_nTotalMemorySize)
        nVisibleLines = (m_nTotalMemorySize - nFirstAddress + 15) / 16;

    const wchar_t* sFormat = (m_nTotalMemorySize > 0x01000000) ? L"%08x" : L"0x%06x";

    for (int i = 0; i < nVisibleLines; ++i)
    {
        const auto nColor = (nCursorAddress == nFirstAddress) ? pEditorTheme.ColorHeaderSelected() : pEditorTheme.ColorHeader();
        const auto sAddress = ra::StringPrintf(sFormat, nFirstAddress);
        nFirstAddress += 16;

        m_pSurface->WriteText(0, nY, s_nFont, nColor, sAddress);
        nY += s_szChar.Height;
    }
}

void MemoryViewerViewModel::RenderHeader()
{
    const auto nCursorAddress = ra::to_signed(GetAddress() & 0x0F);

    const auto& pEditorTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();
    auto nX = ADDRESS_COLUMN_WIDTH * s_szChar.Width;
    m_pSurface->FillRectangle(nX, 0, 16 * 3 * s_szChar.Width, s_szChar.Height, pEditorTheme.ColorBackground());

    const auto nNibblesPerWord = NibblesPerWord();
    std::wstring sValue(nNibblesPerWord, ' ');
    for (int i = 0; i < 16; i += nNibblesPerWord / 2)
    {
        const auto nColor = (nCursorAddress == i) ? pEditorTheme.ColorHeaderSelected() : pEditorTheme.ColorHeader();
        sValue.at(gsl::narrow_cast<size_t>(nNibblesPerWord) - 1) = g_sHexChars.at(i);
        m_pSurface->WriteText(nX, -2, s_nFont, nColor, sValue);
        nX += s_szChar.Width * (nNibblesPerWord + 1);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
