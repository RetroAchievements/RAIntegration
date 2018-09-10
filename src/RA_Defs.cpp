#include "RA_Defs.h"

#include <iomanip>

namespace ra {

_Use_decl_annotations_
std::string Narrow(const std::wstring& wstr)
{
    return Narrow(wstr.c_str());
}

_Use_decl_annotations_
std::string Narrow(std::wstring&& wstr) noexcept
{
    const auto wwstr{ std::move_if_noexcept(wstr) };
    return Narrow(wwstr);
}

_Use_decl_annotations_
std::string Narrow(const wchar_t* wstr)
{
    const auto len{ ra::to_signed(std::wcslen(wstr)) };

    const auto needed{
        ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr, len + 1, nullptr, 0, nullptr, nullptr)
    };

    std::string str(ra::to_unsigned(needed), '\000'); // allocate required space (including terminator)
    ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr, len + 1, str.data(), ra::to_signed(str.capacity()),
                          nullptr, nullptr);
    str.resize(ra::to_unsigned(needed - 1)); // terminator is not actually part of the string
    return str;
}

_Use_decl_annotations_
std::wstring Widen(const std::string& str)
{
    return Widen(str.c_str());
}

_Use_decl_annotations_
std::wstring Widen(std::string&& str) noexcept
{
    const auto sstr{ std::move_if_noexcept(str) };
    return Widen(sstr);
}

_Use_decl_annotations_
std::wstring Widen(const char* str)
{
    const auto len{ ra::to_signed(std::strlen(str)) };
    const auto needed{ ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, len + 1, nullptr, 0) };
    // doesn't seem wchar_t is treated like a character type by default
    std::wstring wstr(ra::to_unsigned(needed), L'\x0');
    ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, len + 1, wstr.data(),
                          ra::to_signed(wstr.capacity()));
    wstr.resize(ra::to_unsigned(needed - 1));

    return wstr;
}

std::string ByteAddressToString(ra::ByteAddress nAddr)
{
    std::ostringstream oss;
    oss << "0x" << std::setfill('0') << std::setw(6) << std::hex << nAddr;
    return oss.str();
}

_Use_decl_annotations_
std::wstring Widen(const wchar_t* wstr)
{
    const std::wstring _wstr{ wstr };
    return _wstr;
}

_Use_decl_annotations_ std::wstring Widen(const std::wstring& wstr) { return wstr; }

_Use_decl_annotations_
std::string Narrow(const char* str)
{
    const std::string _str{ str };
    return _str;
}

_Use_decl_annotations_
std::string Narrow(const std::string& str)
{
    return str;
}

} /* namespace ra */

#ifndef RA_UTEST
extern std::string g_sHomeDir;
#endif

void RADebugLogNoFormat(const char* data)
{
    OutputDebugString(NativeStr(data).c_str());

#ifndef RA_UTEST
    std::string sLogFile = g_sHomeDir + RA_LOG_FILENAME;
    FILE* pf = nullptr;
    if (fopen_s(&pf, sLogFile.c_str(), "a") == 0)
    {
        fwrite(data, sizeof(char), strlen(data), pf);
        fclose(pf);
    }
#endif
}

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

BOOL DirectoryExists(const char* sPath)
{
    DWORD dwAttrib = GetFileAttributes(NativeStr(sPath).c_str());
    return(dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
