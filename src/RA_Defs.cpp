#include "RA_Defs.h"

#include <iomanip>


#ifdef RA_EXPORTS

GetParseErrorFunc GetJSONParseErrorStr = GetParseError_En;

#endif

namespace ra {

std::string Narrow(const std::wstring& wstr)
{
    return Narrow(wstr.c_str());
}

std::string Narrow(std::wstring&& wstr) noexcept
{
    auto wwstr{ std::move_if_noexcept(wstr) };
    return Narrow(wwstr);
}


std::string Narrow(const wchar_t* wstr)
{
    auto state{ std::mbstate_t{} };
    auto len{ 1 + std::wcsrtombs(nullptr, &wstr, 0_z, &state) };

    // doesn't work with unique_ptr, though it works with sprintf and the functions in RAWeb
    std::string str;
    str.reserve(len);

    ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr, to_signed(len), str.data(),
        to_signed(str.length()), nullptr, nullptr);
    return str.data(); // the .data() part is required
}

std::wstring Widen(const std::string& str)
{
    return Widen(str.c_str());
}

std::wstring Widen(std::string&& str) noexcept
{
    auto sstr{ std::move_if_noexcept(str) };
    return Widen(sstr);
}

std::wstring Widen(const char* str)
{
    auto len{ 1_z + std::mbstowcs(nullptr, str, 0_z) };
    // doesn't work with unique_ptr
    std::wstring wstr;
    wstr.reserve(len);

    ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, to_signed(len), wstr.data(),
        to_signed(len));
    return wstr.data();
}

std::string ByteAddressToString(ra::ByteAddress nAddr)
{
    std::ostringstream oss;
    oss << "0x" << std::setfill('0') << std::setw(6) << std::hex << nAddr;
    return oss.str();
}

std::wstring Widen(const wchar_t* wstr) { return std::remove_reference_t<std::wstring&&>(std::wstring{ wstr }); }
std::wstring Widen(const std::wstring& wstr) { return wstr; }
std::string Narrow(const char* str) { return std::remove_reference_t<std::string&&>(std::string{ str });}
std::string Narrow(const std::string& str) { return str; }


} // namespace ra




void RADebugLogNoFormat(const char* data)
{
    OutputDebugString(NativeStr(data).c_str());

    //SetCurrentDirectory( g_sHomeDir.c_str() );//?
    FILE* pf = nullptr;
    if (fopen_s(&pf, RA_LOG_FILENAME, "a") == 0)
    {
        fwrite(data, sizeof(char), strlen(data), pf);
        fclose(pf);
    }
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

    OutputDebugString(NativeStr(buf).c_str());

    //SetCurrentDirectory( g_sHomeDir.c_str() );//?
    FILE* pf = nullptr;
    if (fopen_s(&pf, RA_LOG_FILENAME, "a") == 0)
    {
        fwrite(buf, sizeof(char), strlen(buf), pf);
        fclose(pf);
    }
}

BOOL DirectoryExists(const char* sPath)
{
    DWORD dwAttrib = GetFileAttributes(NativeStr(sPath).c_str());
    return(dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
