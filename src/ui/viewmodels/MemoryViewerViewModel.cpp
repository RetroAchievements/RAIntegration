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
        const auto& pBookmarksViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

        auto nFirstAddress = GetFirstAddress();
        m_pColor[args.tOldValue - nFirstAddress] = 0x80 | gsl::narrow_cast<unsigned char>(ra::etoi(GetColor(args.tOldValue, pBookmarksViewModel, pGameContext)));

        const auto nVisibleLines = GetNumVisibleLines();
        if (args.tNewValue >= ra::to_signed(nFirstAddress + nVisibleLines * 16))
        {
            nFirstAddress = (args.tNewValue & ~0x0F) - ((nVisibleLines / 2) * 16);
            SetFirstAddress(nFirstAddress);

            nFirstAddress = GetFirstAddress();
        }

        m_pColor[args.tNewValue - nFirstAddress] = HIGHLIGHTED_COLOR;
        m_bNeedsRedraw = true;

        if (m_pSurface != nullptr)
        {
            RenderAddresses();
            RenderHeader();
        }
    }
    else if (args.Property == FirstAddressProperty)
    {
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
        const auto nVisibleLines = GetNumVisibleLines();
        const auto nMaxFirstAddress = ra::to_signed((pEmulatorContext.TotalMemorySize() + 15) & ~0x0F) - (nVisibleLines * 16);
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

void MemoryViewerViewModel::OnClick(int nX, int nY)
{
    const int nRow = nY / m_szChar.Height;
    if (nRow == 0)
        return;

    const int nColumn = (nX + m_szChar.Width / 2) / m_szChar.Width;
    if (nColumn < 10)
        return;

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();

    const auto nNewAddress = GetFirstAddress() + (nRow - 1) * 16 + (nColumn - 10) / 3;
    if (nNewAddress < pEmulatorContext.TotalMemorySize())
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
        m_pFontSurface->GetWidth(), m_szChar.Height, ra::ui::Color(0xFF000000));

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

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    const auto nFirstAddress = GetFirstAddress();
    if (nFirstAddress + nVisibleLines * 16 > pEmulatorContext.TotalMemorySize())
        nVisibleLines = (pEmulatorContext.TotalMemorySize() - nFirstAddress) / 16;

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
    auto nAddress = GetFirstAddress();
    int nY = m_szChar.Height;

    m_pSurface->FillRectangle(0, 0, m_szChar.Width * 8, nVisibleLines * m_szChar.Height, ra::ui::Color(0xFFFFFFFF));

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    if (nAddress + nVisibleLines * 16 > pEmulatorContext.TotalMemorySize())
        nVisibleLines = (pEmulatorContext.TotalMemorySize() - nAddress) / 16;

    for (int i = 0; i < nVisibleLines; ++i)
    {
        const auto nColor = (nCursorAddress == nAddress) ? ra::ui::Color(0xFFFF8080) : ra::ui::Color(0xFF808080);
        const auto sAddress = ra::StringPrintf(L"0x%06x", nAddress);
        nAddress += 16;

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
        m_pSurface->WriteText(nX, 0, m_nFont, nColor, sValue);
        nX += m_szChar.Width * 3;
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
