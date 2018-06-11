#include "RA_Defs.h"

#include <codecvt>


#ifdef RA_EXPORTS

GetParseErrorFunc GetJSONParseErrorStr = GetParseError_En;

#endif

static_assert(sizeof(BYTE*) == sizeof(char*), "dangerous cast ahead");

_Use_decl_annotations_
char* DataStreamAsString(DataStream& stream)
{
    return reinterpret_cast<char*>(stream.data());
}

_Use_decl_annotations_
std::string Narrow(const wchar_t* wstr)
{
    std::wstring_convert< std::codecvt_utf8_utf16< wchar_t >, wchar_t > converter;
    return converter.to_bytes(wstr);
}

_Use_decl_annotations_
std::string Narrow(const std::wstring& wstr)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    return converter.to_bytes(wstr);
}

_Use_decl_annotations_
std::wstring Widen(const char* str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    return converter.from_bytes(str);
}

_Use_decl_annotations_
std::wstring Widen(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    return converter.from_bytes(str);
}

_Use_decl_annotations_
std::wstring Widen(const wchar_t* wstr)
{
    return std::wstring{ wstr };
}

_Use_decl_annotations_
std::wstring Widen(const std::wstring& wstr)
{
    return wstr;
}

_Use_decl_annotations_
std::string Narrow(const char* str)
{
    return std::string{ str };
}

_Use_decl_annotations_
std::string Narrow(const std::string& str)
{
    return str;
}

_Use_decl_annotations_
NativeString NativeStr(NonNativeCString str) noexcept
{
#if _MBCS
    return Narrow(str);
#elif _UNICODE 
    return Widen(str);
#else
#error Invalid character set, only MBCS and Unicode are supported.
#endif // _MBCS
}

_Use_decl_annotations_
NativeString NativeStr(const NonNativeString& str) noexcept
{
#if _MBCS
    return Narrow(str);
#elif _UNICODE 
    return Widen(str);
#else
#error Invalid character set, only MBCS and Unicode are supported.
#endif // _MBCS
}

_Use_decl_annotations_
NativeString NativeStr(NonNativeString&& str) noexcept
{
#if _MBCS
    return Narrow(std::move(str));
#elif _UNICODE 
    return Widen(std::move(str));
#else
#error Invalid character set, only MBCS and Unicode are supported.
#endif // _MBCS
}

_Use_decl_annotations_
NativeString NativeStr(NativeString&& str) noexcept
{
#if _MBCS
    return Narrow(std::move(str));
#elif _UNICODE 
    return Widen(std::move(str));
#else
#error Invalid character set, only MBCS and Unicode are supported.
#endif // _MBCS
}

_Use_decl_annotations_
NativeString NativeStr(const NativeString& str) noexcept
{
#if _MBCS
    return Narrow(str);
#elif _UNICODE 
    return Widen(str);
#else
#error Invalid character set, only MBCS and Unicode are supported.
#endif // _MBCS
}

_Use_decl_annotations_
NativeString NativeStr(NativeCString str) noexcept
{
#if _MBCS
    return Narrow(str);
#elif _UNICODE 
    return Widen(str);
#else
#error Invalid character set, only MBCS and Unicode are supported.
#endif // _MBCS
}

_Use_decl_annotations_
void RADebugLogNoFormat(const char* data)
{
    OutputDebugString(NativeStr(data).c_str());

    //SetCurrentDirectory( g_sHomeDir.c_str() );//?
    // SAL didn't like this
    std::unique_ptr<FILE, decltype(&std::fclose)> fp{ std::fopen(RA_LOG_FILENAME, "a"), std::fclose };
    fwrite(data, sizeof(char), strlen(data), fp.get());
}


_Use_decl_annotations_
void RADebugLog(const char* format, ...)
{
    char buf[4096];
    char* p = buf;

    va_list args;
    va_start(args, format);
    int n = _vsnprintf_s(p, 4096, sizeof buf - 3, format, args); // buf-3 is room for CR/LF/NUL
    va_end(args);

    p += (n < 0) ? sizeof buf - 3 : n;

    while ((p > buf) && (isspace(p[-1])))
        *--p = '\0';

    *p++ = '\r';
    *p++ = '\n';
    *p = '\0';

    RADebugLogNoFormat(buf);
}

_Use_decl_annotations_
BOOL DirectoryExists(const char* sPath)
{
    DWORD dwAttrib = GetFileAttributes(NativeStr(sPath).c_str());
    return(dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

_Use_decl_annotations_
ResizeContent::ResizeContent(HWND parentHwnd, HWND contentHwnd, AlignType newAlignType, bool isResize) noexcept
{
    hwnd.reset(contentHwnd);
    nAlignType = newAlignType;
    bResize = isResize;


    auto lpRect{ std::make_unique<RECT>() };
    GetWindowRect(hwnd.get(), lpRect.get());

    pLT.x = lpRect->left;  pLT.y = lpRect->top;
    pRB.x = lpRect->right; pRB.y = lpRect->bottom;

    ScreenToClient(parentHwnd, &pLT);
    ScreenToClient(parentHwnd, &pRB);

    GetWindowRect(parentHwnd, lpRect.get());

    // Though RARect can be built in compile-time it's already runtime
    const RARect rect{ *lpRect.release() };
    nDistanceX = rect.Width() - pLT.x;
    nDistanceY = rect.Height() - pLT.y;

    if (bResize)
    {
        nDistanceX -= (pRB.x - pLT.x);
        nDistanceY -= (pRB.y - pLT.y);
    }
}

_Use_decl_annotations_
void ResizeContent::Resize(int width, int height) noexcept
{
    int xPos = 0, yPos = 0;

    switch (nAlignType)
    {
        case AlignType::NO_ALIGN:
            _FALLTHROUGH;
        case AlignType::ALIGN_RIGHT:
            xPos = width - nDistanceX - (bResize ? pLT.x : 0);
            yPos = bResize ? (pRB.y - pLT.x) : pLT.y;
            break;
        case AlignType::ALIGN_BOTTOM:
            xPos = bResize ? (pRB.x - pLT.x) : pLT.x;
            yPos = height - nDistanceY - (bResize ? pLT.y : 0);
            break;
        case AlignType::ALIGN_BOTTOM_RIGHT:
            xPos = width - nDistanceX - (bResize ? pLT.x : 0);
            yPos = height - nDistanceY - (bResize ? pLT.y : 0);
            break;
        default:
            xPos = bResize ? (pRB.x - pLT.x) : pLT.x;
            yPos = bResize ? (pRB.y - pLT.x) : pLT.y;
    }

    // TODO: Put ra::to_unsigned instead of UINT{} when a PR that contains it
    //       is accepted (at least one, but exists in multiple)
    if (!bResize)
        SetWindowPos(hwnd.get(), nullptr, xPos, yPos, 0, 0, UINT{ SWP_NOSIZE | SWP_NOZORDER });
    else
        SetWindowPos(hwnd.get(), nullptr, 0, 0, xPos, yPos, UINT{ SWP_NOMOVE | SWP_NOZORDER });
}
