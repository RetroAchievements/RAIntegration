#include "RA_Defs.h"

#include <stdio.h>
#include <Windows.h>
#include <locale>
#include <codecvt>
#include <fstream>


#ifdef RA_EXPORTS

GetParseErrorFunc GetJSONParseErrorStr = GetParseError_En;

#endif

static_assert(sizeof(BYTE*) == sizeof(char*), "dangerous cast ahead");
char* DataStreamAsString(DataStream& stream)
{
    return reinterpret_cast<char*>(stream.data());
}

std::string Narrow(const wchar_t* wstr)
{
    static std::wstring_convert< std::codecvt_utf8_utf16< wchar_t >, wchar_t > converter;
    return converter.to_bytes(wstr);
}

std::string Narrow(const std::wstring& wstr)
{
    static std::wstring_convert< std::codecvt_utf8_utf16< wchar_t >, wchar_t > converter;
    return converter.to_bytes(wstr);
}

std::wstring Widen(const char* str)
{
    static std::wstring_convert< std::codecvt_utf8_utf16< wchar_t >, wchar_t > converter;
    return converter.from_bytes(str);
}

std::wstring Widen(const std::string& str)
{
    static std::wstring_convert< std::codecvt_utf8_utf16< wchar_t >, wchar_t > converter;
    return converter.from_bytes(str);
}

std::wstring Widen(const wchar_t* wstr)
{
    return std::wstring(wstr);
}

std::wstring Widen(const std::wstring& wstr)
{
    return wstr;
}

std::string Narrow(const char* str)
{
    return std::string(str);
}

std::string Narrow(const std::string& str)
{
    return str;
}

void RADebugLogNoFormat(const std::string& data)
{
#if _DEBUG
    OutputDebugString(NativeStr(data).c_str());
#endif

    std::ofstream ofile{ RA_LOG_FILENAME, std::ios::app | std::ios::ate };
    ofile << data;
}



BOOL DirectoryExists(const char* sPath)
{
    DWORD dwAttrib = GetFileAttributes(NativeStr(sPath).c_str());
    return(dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
