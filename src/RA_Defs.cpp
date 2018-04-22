#include "RA_Defs.h"

#include <stdio.h>
#include <Windows.h>
#include <locale>
#include <codecvt>

GetParseErrorFunc GetJSONParseErrorStr = GetParseError_En;
namespace ra {

inline namespace conversions{
std::string DataStreamAsString(const DataStream& stream) {
    std::ostringstream oss;

    // Ok the string has to come from a json for this to work
    for (auto& i : stream)
        oss << to_signed(i);

    // pesky null character
    auto str{ oss.str() };

    // Not sure if this is needed for the old rapidjson but is needed for the new one since there's an
    // extra nul character, but can't remove that part unless it's changed in RA_Interface as well
    if (!str.empty()) {
        str.pop_back();
    }

    return str; // ok it's definitly showing how it should be
}


std::string Narrow(const std::wstring& wstr)
{
    std::ostringstream out;

    for (auto& i : wstr)
        out << static_cast<char>(i);

    return out.str();
}

// N.B.: Not totally sure if you want the strings erased at conversion or not, if so we should rvalues
// instead.
std::string Narrow(const wchar_t* wstr)
{
    std::wstring swstr{ wstr };
    return Narrow(swstr); // the current function uses an lvalue so we need an actual objet
}

std::wstring Widen(const std::string& str)
{
    std::wostringstream out;

    for (auto& i : str)
        out << static_cast<wchar_t>(i);

    return out.str();
}

std::wstring Widen(const char* str)
{
    std::string std_str{ str };
    return Widen(std_str);
}

std::wstring Widen(const wchar_t* wstr){return std::wstring{ wstr }; }
std::wstring Widen(const std::wstring& wstr) { return wstr; }
std::string Narrow(const char* str) { return std::string{ str }; }
std::string Narrow(const std::string& str) { return str; }
} // inline namespace conversions
} // namespace ra




void RADebugLogNoFormat(const char* data)
{
	OutputDebugString(NativeStr(data).c_str());

	//SetCurrentDirectory( g_sHomeDir.c_str() );//?
	FILE* pf = NULL;
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
	FILE* pf = NULL;
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
