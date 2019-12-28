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

constexpr unsigned char HIGHLIGHTED_COLOR = 0x80 | gsl::narrow_cast<unsigned char>(ra::etoi(MemoryViewerViewModel::TextColor::Red));

MemoryViewerViewModel::MemoryViewerViewModel() noexcept
{
    m_pBuffer.reset(new unsigned char[MaxLines * 16 * 2]);

    m_pMemory = m_pBuffer.get();
    memset(m_pMemory, 0, MaxLines * 16);

    m_pColor = m_pMemory + MaxLines * 16;
    memset(m_pColor, 0x80, MaxLines * 16);

    m_pColor[0] = HIGHLIGHTED_COLOR;

    AddNotifyTarget(*this);

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
    pGameContext.AddNotifyTarget(*this);
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
        m_pColor[i] = 0x80 | gsl::narrow_cast<unsigned char>(ra::etoi(GetColor(nFirstAddress + i, pBookmarksViewModel, pGameContext)));

    const auto nAddress = GetAddress();
    if (nAddress >= nFirstAddress && nAddress < nFirstAddress + nVisibleLines * 16)
        m_pColor[nAddress - nFirstAddress] = HIGHLIGHTED_COLOR;

    m_bNeedsRedraw = true;
}

void MemoryViewerViewModel::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == AddressProperty)
    {
        auto nFirstAddress = GetFirstAddress();
        const auto nVisibleLines = GetNumVisibleLines();
        const auto nMaxAddress = nFirstAddress + nVisibleLines * 16;

        const auto nOldValue = ra::to_unsigned(args.tOldValue);
        if (nOldValue >= nFirstAddress && nOldValue < nMaxAddress)
        {
            const auto& pBookmarksViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

            m_pColor[nOldValue - nFirstAddress] = 0x80 | gsl::narrow_cast<unsigned char>(ra::etoi(GetColor(nOldValue, pBookmarksViewModel, pGameContext)));
        }

        const auto nNewValue = ra::to_unsigned(args.tNewValue);
        if (nNewValue < nFirstAddress || nNewValue >= nMaxAddress)
        {
            nFirstAddress = (nNewValue & ~0x0F) - ((nVisibleLines / 2) * 16);
            SetFirstAddress(nFirstAddress);

            nFirstAddress = GetFirstAddress();
        }

        m_pColor[nNewValue - nFirstAddress] = HIGHLIGHTED_COLOR;
        m_bNeedsRedraw = true;

        if (m_pSurface != nullptr)
        {
            RenderAddresses();
            RenderHeader();
        }
    }
    else if (args.Property == FirstAddressProperty)
    {
        const auto nVisibleLines = GetNumVisibleLines();
        const auto nMaxFirstAddress = ra::to_signed((m_nTotalMemorySize + 15) & ~0x0F) - (nVisibleLines * 16);
        if (args.tNewValue > nMaxFirstAddress)
        {
            SetFirstAddress(std::max(0, nMaxFirstAddress));
        }
        else if (args.tNewValue < 0)
        {
            SetFirstAddress(0);
        }
        else
        {
            if (m_pSurface != nullptr)
                RenderAddresses();

            UpdateColors();
        }
    }
    else if (args.Property == NumVisibleLinesProperty)
    {
        m_pSurface.reset();
        m_bNeedsRedraw = true;
    }
}

void MemoryViewerViewModel::AdvanceCursor()
{
    if (m_bUpperNibbleSelected)
    {
        m_bUpperNibbleSelected = false;

        const auto nAddress = GetAddress();
        const auto nFirstAddress = GetFirstAddress();

        m_pColor[nAddress - nFirstAddress] |= 0x80;
        m_bNeedsRedraw = true;
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

    if (!m_bUpperNibbleSelected)
    {
        m_bUpperNibbleSelected = true;

        m_pColor[nAddress - nFirstAddress] |= 0x80;
        m_bNeedsRedraw = true;
    }
    else if (nAddress > 0)
    {
        m_bUpperNibbleSelected = false;

        const auto nNewAddress = nAddress - 1;
        if (nNewAddress < nFirstAddress)
            SetFirstAddress(nFirstAddress - 16);

        SetAddress(nNewAddress);
    }
}

void MemoryViewerViewModel::AdvanceCursorWord()
{
    const auto nAddress = GetAddress();
    if (nAddress < m_nTotalMemorySize)
    {
        m_bUpperNibbleSelected = true;

        const auto nNewAddress = nAddress + 1;
        if ((nNewAddress & 0x0F) == 0)
        {
            const auto nFirstAddress = GetFirstAddress();
            if (nNewAddress == nFirstAddress + GetNumVisibleLines() * 16)
                SetFirstAddress(nFirstAddress + 16);
        }

        SetAddress(nNewAddress);
    }
}

void MemoryViewerViewModel::RetreatCursorWord()
{
    const auto nAddress = GetAddress();
    if (nAddress > 0)
    {
        m_bUpperNibbleSelected = true;

        const auto nNewAddress = nAddress - 1;
        if ((nNewAddress & 0x0F) == 0x0F)
        {
            const auto nFirstAddress = GetFirstAddress();
            if (nNewAddress < nFirstAddress)
                SetFirstAddress(nFirstAddress - 16);
        }

        SetAddress(nNewAddress);
    }
}

void MemoryViewerViewModel::AdvanceCursorLine()
{
    const auto nAddress = GetAddress();
    if (nAddress < m_nTotalMemorySize - 16)
    {
        m_bUpperNibbleSelected = true;

        const auto nNewAddress = nAddress + 16;
        const auto nFirstAddress = GetFirstAddress();
        if (nNewAddress >= nFirstAddress + GetNumVisibleLines() * 16)
            SetFirstAddress(nFirstAddress + 16);

        SetAddress(nNewAddress);
    }
}

void MemoryViewerViewModel::RetreatCursorLine()
{
    const auto nAddress = GetAddress();
    if (nAddress > 15)
    {
        m_bUpperNibbleSelected = true;

        const auto nNewAddress = nAddress - 16;
        const auto nFirstAddress = GetFirstAddress();
        if (nNewAddress < nFirstAddress)
            SetFirstAddress(nFirstAddress - 16);

        SetAddress(nNewAddress);
    }
}

void MemoryViewerViewModel::AdvanceCursorPage()
{
    const auto nVisibleLines = GetNumVisibleLines();
    const auto nAddress = GetAddress();
    if (nAddress < m_nTotalMemorySize - 16)
    {
        m_bUpperNibbleSelected = true;

        const auto nFirstAddress = GetFirstAddress();
        const auto nNewFirstAddress = nFirstAddress + (nVisibleLines - 1) * 16;
        const auto nMaxFirstAddress = m_nTotalMemorySize - nVisibleLines * 16;
        if (nNewFirstAddress < nMaxFirstAddress)
        {
            SetFirstAddress(nNewFirstAddress);
            SetAddress(nNewFirstAddress + (nAddress - nFirstAddress));
        }
        else
        {
            SetFirstAddress(nMaxFirstAddress);
            SetAddress(((m_nTotalMemorySize - 15) & ~0x0F) | (nAddress & 0x0F));
        }
    }
}

void MemoryViewerViewModel::RetreatCursorPage()
{
    const auto nAddress = GetAddress();
    if (nAddress > 15)
    {
        m_bUpperNibbleSelected = true;

        const auto nFirstAddress = GetFirstAddress();
        const auto nNewFirstAddress = ra::to_signed(nFirstAddress) - (GetNumVisibleLines() - 1) * 16;
        if (nNewFirstAddress >= 0)
        {
            SetFirstAddress(nNewFirstAddress);
            SetAddress(nNewFirstAddress + (nAddress - nFirstAddress));
        }
        else
        {
            SetFirstAddress(0);
            SetAddress(nAddress & 0x0F);
        }
    }
}

void MemoryViewerViewModel::OnActiveGameChanged()
{
    if (m_pSurface != nullptr)
        RenderAddresses();

    UpdateColors();
}

void MemoryViewerViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring&)
{
    const auto nFirstAddress = GetFirstAddress();
    if (nAddress < nFirstAddress)
        return;

    const auto nOffset = nAddress - nFirstAddress;
    const auto nVisibleLines = GetNumVisibleLines();
    if (nOffset >= nVisibleLines * 16)
        return;

    const auto& pBookmarksViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto nNewColor = GetColor(nAddress, pBookmarksViewModel, pGameContext);

    if ((m_pColor[nOffset] & 0x0F) != ra::etoi(nNewColor))
    {
        m_pColor[nOffset] = 0x80 | gsl::narrow_cast<unsigned char>(ra::etoi(nNewColor));
        m_bNeedsRedraw = true;
    }
}

void MemoryViewerViewModel::OnClick(int nX, int nY)
{
    const int nRow = nY / m_szChar.Height;
    if (nRow == 0)
        return;

    const int nColumn = (nX + m_szChar.Width / 2) / m_szChar.Width;
    if (nColumn < 10)
        return;

    const auto nNewAddress = GetFirstAddress() + (nRow - 1) * 16 + (nColumn - 10) / 3;
    if (nNewAddress < m_nTotalMemorySize)
        SetAddress(nNewAddress);
}

void MemoryViewerViewModel::OnResized(_UNUSED int nWidth, int nHeight)
{
    if (m_pFontSurface == nullptr)
        BuildFontSurface();

    SetNumVisibleLines((nHeight / m_szChar.Height) - 1);
}

void MemoryViewerViewModel::DoFrame()
{
    unsigned char pMemory[MaxLines * 16];
    const auto nAddress = GetFirstAddress();
    const auto nVisibleLines = GetNumVisibleLines();

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    if (m_nTotalMemorySize == 0)
    {
        m_nTotalMemorySize = pEmulatorContext.TotalMemorySize();

        if (m_nTotalMemorySize != 0 && m_pSurface != nullptr)
            RenderAddresses();
    }

    pEmulatorContext.ReadMemory(nAddress, pMemory, nVisibleLines * 16);

    for (auto nIndex = 0; nIndex < nVisibleLines * 16; ++nIndex)
    {
        if (m_pMemory[nIndex] != pMemory[nIndex])
        {
            m_pMemory[nIndex] = pMemory[nIndex];
            m_pColor[nIndex] |= 0x80;
            m_bNeedsRedraw = true;
        }
    }
}

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

    constexpr wchar_t m_sHexChars[] = L"0123456789abcdef";
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
            sHexChar[0] = m_sHexChars[j];
            m_pFontSurface->WriteText(j * m_szChar.Width, i * m_szChar.Height - 1, m_nFont, nColor, sHexChar);
        }
    }
}

#pragma warning(push)
#pragma warning(disable : 5045)

void MemoryViewerViewModel::UpdateRenderImage()
{
    if (!m_bNeedsRedraw)
        return;

    auto nVisibleLines = GetNumVisibleLines();

    if (m_pSurface == nullptr)
    {
        if (m_pFontSurface == nullptr)
            BuildFontSurface();

        const int nWidth = (16 * 3 + 1 + 8) * m_szChar.Width;
        const int nHeight = (nVisibleLines + 1) * m_szChar.Height;

        m_pSurface = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>().CreateSurface(nWidth, nHeight);
        m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth(), m_pSurface->GetHeight(), ra::ui::Color(0xFFFFFFFF));

        RenderAddresses();

        m_pSurface->FillRectangle(9 * m_szChar.Width, m_szChar.Height, 1, nHeight, ra::ui::Color(0xFFC0C0C0));

        RenderHeader();
    }

    const auto nFirstAddress = GetFirstAddress();
    if (nFirstAddress + nVisibleLines * 16 > m_nTotalMemorySize)
        nVisibleLines = (m_nTotalMemorySize - nFirstAddress) / 16;

    unsigned char* pMemory = m_pMemory;
    unsigned char* pColor = m_pColor;

    for (int i = 0; i < nVisibleLines * 16; ++i)
    {
        if (pColor[i] & 0x80)
        {
            const unsigned char nColor = pColor[i] & 0x0F;
            pColor[i] = nColor;

            TextColor nColorUpper = ra::itoe<TextColor>(nColor);
            TextColor nColorLower = nColorUpper;

            if (nColorUpper == TextColor::Red)
            {
                if (m_bUpperNibbleSelected)
                    nColorUpper = TextColor::RedOnBlack;
                else
                    nColorLower = TextColor::RedOnBlack;
            }

            const int nX = ((i & 0x0F) * 3 + 10) * m_szChar.Width;
            const int nY = ((i >> 4) + 1) * m_szChar.Height;

            const auto nValue = pMemory[i];
            WriteChar(nX, nY, nColorUpper, nValue >> 4);
            WriteChar(nX + m_szChar.Width, nY, nColorLower, nValue & 0x0F);
        }
    }
}

#pragma warning(pop)

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

void MemoryViewerViewModel::WriteChar(int nX, int nY, TextColor nColor, int hexChar)
{
    m_pSurface->DrawSurface(nX, nY, *m_pFontSurface, hexChar * m_szChar.Width, (int)nColor * m_szChar.Height, m_szChar.Width, m_szChar.Height);
}

void MemoryViewerViewModel::RenderHeader()
{
    const auto nCursorAddress = ra::to_signed(GetAddress() & 0x0F);

    auto nX = 10 * m_szChar.Width;
    m_pSurface->FillRectangle(nX, 0, 16 * 3 * m_szChar.Width, m_szChar.Height, ra::ui::Color(0xFFFFFFFF));

    for (int i = 0; i < 16; ++i)
    {
        const auto nColor = (nCursorAddress == i) ? ra::ui::Color(0xFFFF8080) : ra::ui::Color(0xFF808080);
        const auto sValue = ra::StringPrintf(L"%02x", i);
        m_pSurface->WriteText(nX, -2, m_nFont, nColor, sValue);
        nX += m_szChar.Width * 3;
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
