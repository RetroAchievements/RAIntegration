#include "MemoryViewerViewModel.hh"

#include "RA_Defs.h"

#include "data\EmulatorContext.hh"
#include "data\GameContext.hh"

#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty MemoryViewerViewModel::AddressProperty("MemoryViewerViewModel", "Address", 0);
const IntModelProperty MemoryViewerViewModel::FirstAddressProperty("MemoryViewerViewModel", "FirstAddress", 0);
const IntModelProperty MemoryViewerViewModel::NumVisibleLinesProperty("MemoryViewerViewModel", "NumVisibleLines", 8);
const IntModelProperty MemoryViewerViewModel::SizeProperty("MemoryViewerViewModel", "Size", ra::etoi(MemSize::EightBit));

constexpr uint8_t STALE_COLOR = 0x80;
constexpr uint8_t HIGHLIGHTED_COLOR = STALE_COLOR | gsl::narrow_cast<uint8_t>(ra::etoi(MemoryViewerViewModel::TextColor::Red));
constexpr int ADDRESS_COLUMN_WIDTH = 10;

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
        m_vBookmarks.RemoveNotifyTarget(*this);
    }

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

MemoryViewerViewModel::MemoryViewerViewModel() noexcept
{
    m_pBuffer.reset(new uint8_t[MaxLines * 16 * 2]);

    m_pMemory = m_pBuffer.get();
    memset(m_pMemory, 0, MaxLines * 16);

    m_pColor = m_pMemory + MaxLines * 16;
    memset(m_pColor, STALE_COLOR, MaxLines * 16);

    m_pColor[0] = HIGHLIGHTED_COLOR;

    AddNotifyTarget(*this);

#ifndef RA_UTEST
    InitializeNotifyTargets();
#endif
}

void MemoryViewerViewModel::InitializeNotifyTargets()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>();
    pEmulatorContext.AddNotifyTarget(*this);

    m_nTotalMemorySize = gsl::narrow<ra::ByteAddress>(pEmulatorContext.TotalMemorySize());

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
    pGameContext.AddNotifyTarget(*this);

    m_bReadOnly = (pGameContext.GameId() == 0);

    m_pBookmarkMonitor.reset(new MemoryBookmarkMonitor(*this));
}

MemoryViewerViewModel::~MemoryViewerViewModel()
{
    // empty function definition required to generate destroy for forward-declared MemoryBookmarkMonitor
}

static constexpr int NibblesForSize(MemSize nSize)
{
    switch (nSize)
    {
        default:
        case MemSize::EightBit: return 2;
        case MemSize::SixteenBit: return 4;
        case MemSize::ThirtyTwoBit: return 8;
    }
}

int MemoryViewerViewModel::NibblesPerWord() const
{
    return NibblesForSize(GetSize());
}

static MemoryViewerViewModel::TextColor GetColor(ra::ByteAddress nAddress,
    const ra::ui::viewmodels::MemoryBookmarksViewModel& pBookmarksViewModel,
    const ra::data::GameContext& pGameContext)
{
    if (pBookmarksViewModel.HasBookmark(nAddress))
    {
        if (pBookmarksViewModel.HasFrozenBookmark(nAddress))
            return MemoryViewerViewModel::TextColor::Yellow;

        return MemoryViewerViewModel::TextColor::Green;
    }

    if (pGameContext.FindCodeNote(nAddress) != nullptr)
        return MemoryViewerViewModel::TextColor::Blue;

    return MemoryViewerViewModel::TextColor::Black;
}

void MemoryViewerViewModel::UpdateColors()
{
    const auto& pBookmarksViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    const auto nVisibleLines = GetNumVisibleLines();
    const auto nFirstAddress = GetFirstAddress();

    for (int i = 0; i < nVisibleLines * 16; ++i)
        m_pColor[i] = STALE_COLOR | gsl::narrow_cast<uint8_t>(ra::etoi(GetColor(nFirstAddress + i, pBookmarksViewModel, pGameContext)));

    UpdateHighlight(GetAddress(), NibblesPerWord() / 2, 0);

    m_nNeedsRedraw |= REDRAW_MEMORY;
}

void MemoryViewerViewModel::UpdateHighlight(ra::ByteAddress nAddress, int nNewLength, int nOldLength)
{
    if (nNewLength == nOldLength)
        return;

    const auto nFirstAddress = GetFirstAddress();
    if (nAddress < nFirstAddress && nAddress + nNewLength < nFirstAddress && nAddress + nOldLength < nFirstAddress)
        return;

    const auto nVisibleLines = GetNumVisibleLines();
    const auto nMaxAddress = nFirstAddress + nVisibleLines * 16;
    if (nAddress >= nMaxAddress)
        return;

    m_nNeedsRedraw |= REDRAW_MEMORY;

    for (int i = 0; i < nNewLength; ++i)
    {
        m_pColor[nAddress - nFirstAddress] = HIGHLIGHTED_COLOR;
        if (++nAddress == nMaxAddress)
            return;
    }

    if (nOldLength > nNewLength)
    {
        const auto& pBookmarksViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

        while (nOldLength > nNewLength)
        {
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
        const auto nNibblesPerWord = NibblesPerWord();
        m_pColor[nAddress - nFirstAddress + (nNibblesPerWord - m_nSelectedNibble - 1) / 2] |= STALE_COLOR;
        m_nSelectedNibble = nNewNibble;

        m_pColor[nAddress - nFirstAddress + (nNibblesPerWord - m_nSelectedNibble - 1) / 2] |= STALE_COLOR;
        m_nNeedsRedraw |= REDRAW_MEMORY;
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

void MemoryViewerViewModel::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
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

        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
        pEmulatorContext.ReadMemory(args.tNewValue, m_pMemory, GetNumVisibleLines() * 16);

        UpdateColors();
    }
    else if (args.Property == SizeProperty)
    {
        const int nOldBytes = NibblesForSize(ra::itoe<MemSize>(args.tOldValue)) / 2;
        const int nNewBytes = NibblesForSize(ra::itoe<MemSize>(args.tNewValue)) / 2;
        UpdateHighlight(GetAddress(), nNewBytes, nOldBytes);

        const auto nVisibleLines = GetNumVisibleLines();
        for (int i = 0; i < nVisibleLines * 16; ++i)
            m_pColor[i] |= STALE_COLOR;

        m_pSurface.reset();
        m_nNeedsRedraw = REDRAW_ALL;
    }
    else if (args.Property == NumVisibleLinesProperty)
    {
        if (args.tNewValue > args.tOldValue)
        {
            const auto nFirstAddress = GetFirstAddress();

            const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
            pEmulatorContext.ReadMemory(nFirstAddress, m_pMemory, args.tNewValue * 16);

            UpdateColors();
        }

        m_pSurface.reset();
        m_nNeedsRedraw = REDRAW_ALL;
    }
}

void MemoryViewerViewModel::AdvanceCursor()
{
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

void MemoryViewerViewModel::OnActiveGameChanged()
{
    m_nNeedsRedraw |= REDRAW_ADDRESSES;

    UpdateColors();

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    m_bReadOnly = (pGameContext.GameId() == 0);
}

void MemoryViewerViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring&)
{
    const auto nFirstAddress = GetFirstAddress();
    if (nAddress < nFirstAddress)
        return;

    const auto nOffset = nAddress - nFirstAddress;
    const auto nVisibleLines = GetNumVisibleLines();
    if (nOffset >= ra::to_unsigned(nVisibleLines * 16))
        return;

    const auto& pBookmarksViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto nNewColor = GetColor(nAddress, pBookmarksViewModel, pGameContext);

    if ((m_pColor[nOffset] & 0x0F) != ra::etoi(nNewColor))
    {
        m_pColor[nOffset] = STALE_COLOR | gsl::narrow_cast<uint8_t>(ra::etoi(nNewColor));
        m_nNeedsRedraw |= REDRAW_MEMORY;
    }
}

void MemoryViewerViewModel::OnTotalMemorySizeChanged()
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    m_nTotalMemorySize = gsl::narrow<ra::ByteAddress>(pEmulatorContext.TotalMemorySize());

    if (GetAddress() >= m_nTotalMemorySize)
        SetAddress(m_nTotalMemorySize - 1);

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

    m_nNeedsRedraw |= REDRAW_ADDRESSES | REDRAW_MEMORY;
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
        if (nColor == TextColor::Red)
            return;

        // byte is not selected, update
        const auto& pBookmarksViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

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
    const int nRow = nY / m_szChar.Height;
    if (nRow == 0)
        return;

    const int nColumn = nX / m_szChar.Width;
    if (nColumn < ADDRESS_COLUMN_WIDTH)
        return;

    const auto nFirstAddress = GetFirstAddress();
    const auto nNibblesPerWord = NibblesPerWord();
    const auto nWordSpacing = nNibblesPerWord + 1;
    const auto nBytesPerWord = nNibblesPerWord / 2;

    const auto nNewAddress = nFirstAddress + (nRow - 1) * 16 + ((nColumn - ADDRESS_COLUMN_WIDTH) / nWordSpacing) * nBytesPerWord;
    if (nNewAddress < m_nTotalMemorySize)
    {
        const auto nNewNibble = (nColumn - ADDRESS_COLUMN_WIDTH) % nWordSpacing;
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

void MemoryViewerViewModel::OnResized(_UNUSED int nWidth, int nHeight)
{
    if (m_pFontSurface == nullptr)
        BuildFontSurface();

    SetNumVisibleLines((nHeight / m_szChar.Height) - 1);
}

bool MemoryViewerViewModel::OnChar(char c)
{
    if (m_nTotalMemorySize == 0 || m_bReadOnly)
        return false;

    uint8_t value;
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

    auto nIndex = GetAddress() - GetFirstAddress();

    const auto nNibblesPerWord = NibblesPerWord();
    auto nSelectedNibble = (nNibblesPerWord - m_nSelectedNibble - 1);
    while (nSelectedNibble > 1)
    {
        ++nIndex;
        nSelectedNibble -= 2;
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

    const auto nAddress = GetFirstAddress() + nIndex;

    // push the updated value to the emulator
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>();
    pEmulatorContext.WriteMemoryByte(nAddress, nByte);

    // if a bookmark exists for the modified address, update the current value
    auto& pBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    pBookmarks.OnEditMemory(nAddress);

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

void MemoryViewerViewModel::DoFrame()
{
    uint8_t pMemory[MaxLines * 16];
    const auto nAddress = GetFirstAddress();
    const auto nVisibleLines = GetNumVisibleLines();

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    pEmulatorContext.ReadMemory(nAddress, pMemory, nVisibleLines * 16);

    for (auto nIndex = 0; nIndex < nVisibleLines * 16; ++nIndex)
    {
        if (m_pMemory[nIndex] != pMemory[nIndex])
        {
            m_pMemory[nIndex] = pMemory[nIndex];
            m_pColor[nIndex] |= STALE_COLOR;
            m_nNeedsRedraw |= REDRAW_MEMORY;
        }
    }
}

// ====== Render ==============================================================

static constexpr std::array<wchar_t, 16> g_sHexChars = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

void MemoryViewerViewModel::BuildFontSurface()
{
    const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
    auto pSurface = pSurfaceFactory.CreateSurface(1, 1);

    m_nFont = pSurface->LoadFont("Consolas", 17, ra::ui::FontStyles::Normal);
    m_szChar = pSurface->MeasureText(m_nFont, L"0");
    m_szChar.Height--; // don't need space for dangly bits

    m_pFontSurface = pSurfaceFactory.CreateSurface(m_szChar.Width * 16, m_szChar.Height * ra::etoi(TextColor::NumColors));
    m_pFontSurface->FillRectangle(0, 0, m_pFontSurface->GetWidth(), m_pFontSurface->GetHeight(), ra::ui::Color(0xFFFFFFFF));

    m_pFontSurface->FillRectangle(0, m_szChar.Height * ra::etoi(TextColor::RedOnBlack),
        m_pFontSurface->GetWidth(), m_szChar.Height, ra::ui::Color(0xFFC0C0C0));

    std::wstring sHexChar = L"0";
    ra::ui::Color nColor(0);
    for (int i = 0; i < ra::etoi(TextColor::NumColors); ++i)
    {
        switch ((TextColor)i)
        {
            case TextColor::Black:  nColor.ARGB = 0xFF000000; break;
            case TextColor::Blue:   nColor.ARGB = 0xFF0000FF; break;
            case TextColor::Green:  nColor.ARGB = 0xFF00A000; break;
            case TextColor::RedOnBlack:
            case TextColor::Red:    nColor.ARGB = 0xFFFF0000; break;
            case TextColor::Yellow: nColor.ARGB = 0xFFFFC800; break;
        }

        for (int j = 0; j < 16; ++j)
        {
            sHexChar[0] = g_sHexChars.at(j);
            m_pFontSurface->WriteText(j * m_szChar.Width, i * m_szChar.Height - 1, m_nFont, nColor, sHexChar);
        }
    }
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
        if (m_pFontSurface == nullptr)
            BuildFontSurface();

        const int nWidth = (16 * 3 + 1 + 8) * m_szChar.Width;
        const int nHeight = (GetNumVisibleLines() + 1) * m_szChar.Height;
        m_pSurface = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>().CreateSurface(nWidth, nHeight);

        // background
        m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth(), m_pSurface->GetHeight(), ra::ui::Color(0xFFFFFFFF));

        // separator
        m_pSurface->FillRectangle(9 * m_szChar.Width, m_szChar.Height, 1, nHeight, ra::ui::Color(0xFFC0C0C0));

        nNeedsRedraw = REDRAW_ALL;
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
        nVisibleLines = (m_nTotalMemorySize - nFirstAddress) / 16;

    const int nWordSpacing = NibblesPerWord() + 1;
    const int nBytesPerWord = nWordSpacing / 2;
    for (int i = 0; i < nVisibleLines * 16; ++i)
    {
        if (!(m_pColor[i] & STALE_COLOR))
            continue;

        const uint8_t nColor = m_pColor[i] & 0x0F;
        m_pColor[i] = nColor;

        TextColor nColorUpper = ra::itoe<TextColor>(nColor);
        TextColor nColorLower = nColorUpper;

        const int nY = ((i / 16) + 1) * m_szChar.Height;

        int nX = (((i & 0x0F) / nBytesPerWord) * nWordSpacing + ADDRESS_COLUMN_WIDTH) * m_szChar.Width;
        if (nBytesPerWord > 1)
        {
            const int nByteOffset = ((nBytesPerWord - 1) - (i % nBytesPerWord));
            nX += (nByteOffset * 2) * m_szChar.Width;

            if (nColorUpper == TextColor::Red && m_bHasFocus && !m_bReadOnly)
            {
                if (m_nSelectedNibble / 2 == nByteOffset)
                {
                    if ((m_nSelectedNibble & 1) == 0)
                        nColorUpper = TextColor::RedOnBlack;
                    else
                        nColorLower = TextColor::RedOnBlack;
                }
            }
        }
        else
        {
            if (nColorUpper == TextColor::Red && m_bHasFocus && !m_bReadOnly)
            {
                if (m_nSelectedNibble == 0)
                    nColorUpper = TextColor::RedOnBlack;
                else
                    nColorLower = TextColor::RedOnBlack;
            }
        }

        const auto nValue = m_pMemory[i];
        WriteChar(nX, nY, nColorUpper, nValue >> 4);
        WriteChar(nX + m_szChar.Width, nY, nColorLower, nValue & 0x0F);
    }
}

#pragma warning(pop)

void MemoryViewerViewModel::WriteChar(int nX, int nY, TextColor nColor, int hexChar)
{
    m_pSurface->DrawSurface(nX, nY, *m_pFontSurface, hexChar * m_szChar.Width, (int)nColor * m_szChar.Height, m_szChar.Width, m_szChar.Height);
}

void MemoryViewerViewModel::RenderAddresses()
{
    auto nVisibleLines = GetNumVisibleLines();
    const auto nCursorAddress = GetAddress() & ~0x0F;
    auto nFirstAddress = GetFirstAddress();
    int nY = m_szChar.Height - 1;

    m_pSurface->FillRectangle(0, m_szChar.Height, m_szChar.Width * 8, nVisibleLines * m_szChar.Height, ra::ui::Color(0xFFFFFFFF));

    if (nFirstAddress + nVisibleLines * 16 > m_nTotalMemorySize)
        nVisibleLines = (m_nTotalMemorySize - nFirstAddress) / 16;

    for (int i = 0; i < nVisibleLines; ++i)
    {
        const auto nColor = (nCursorAddress == nFirstAddress) ? ra::ui::Color(0xFFFF8080) : ra::ui::Color(0xFF808080);
        const auto sAddress = ra::StringPrintf(L"0x%06x", nFirstAddress);
        nFirstAddress += 16;

        m_pSurface->WriteText(0, nY, m_nFont, nColor, sAddress);
        nY += m_szChar.Height;
    }
}

void MemoryViewerViewModel::RenderHeader()
{
    const auto nCursorAddress = ra::to_signed(GetAddress() & 0x0F);

    auto nX = ADDRESS_COLUMN_WIDTH * m_szChar.Width;
    m_pSurface->FillRectangle(nX, 0, 16 * 3 * m_szChar.Width, m_szChar.Height, ra::ui::Color(0xFFFFFFFF));

    const auto nNibblesPerWord = NibblesPerWord();
    std::wstring sValue(nNibblesPerWord, ' ');
    for (int i = 0; i < 16; i += nNibblesPerWord / 2)
    {
        const auto nColor = (nCursorAddress == i) ? ra::ui::Color(0xFFFF8080) : ra::ui::Color(0xFF808080);
        sValue[nNibblesPerWord - 1] = g_sHexChars.at(i);
        m_pSurface->WriteText(nX, -2, m_nFont, nColor, sValue);
        nX += m_szChar.Width * (nNibblesPerWord + 1);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
