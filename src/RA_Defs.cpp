#include "RA_Defs.h"

#include <iomanip>


GetParseErrorFunc GetJSONParseErrorStr = GetParseError_En;
namespace ra {



std::string DataStreamAsString(const DataStream& stream) {

	std::ostringstream oss;

	// This is guarenteed to work
	for (auto& i : stream) {
		oss << static_cast<char>(i);
	}

	auto str{ oss.str() };

    // pesky null character
    if (!str.empty()) {
        str.pop_back();
    }

    return str; // ok it's definitly showing how it should be
}


std::string Narrow(const std::wstring& wstr) {
    return Narrow(wstr.c_str());
}

// N.B.: Not totally sure if you want the strings erased at conversion or not, if so we should rvalues
// instead.
std::string Narrow(const wchar_t* wstr)
{
	auto state = std::mbstate_t();
	auto len = 1 + std::wcsrtombs(nullptr, &wstr, 0_z, &state);
	auto str{ ""s };

	str.reserve(len);
	// Last two params have to be nullptr, the resulting doesn't have to be
	// multi-byte string but if it is nothing bad will happen.
	WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr, to_signed(len), str.data(),
		to_signed(str.capacity()), nullptr, nullptr);

	return str.data();
}

std::wstring Widen(const std::string& str)
{
    return Widen(str.c_str());
}

std::wstring Widen(const char* str)
{
    auto state = std::mbstate_t();
    auto len = 1 + std::mbsrtowcs(nullptr, &str, 0_z, &state);
    auto wstr{ L""s };
    wstr.reserve(len);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, len, wstr.data(),
        to_signed(wstr.capacity()));

    return wstr.data();
}

std::string ByteAddressToString(ByteAddress nAddr)
{
    // with tinyformat it's easier but I can do it without it
    // if we used tinyformat it would be like this
    //return tfm::format("0x%06x", nAddr);
    // Yup, it's that simple



	std::ostringstream oss;
	oss << "0x" << std::setfill('0') << std::setw(6) << std::hex << nAddr;
	return oss.str();
}


std::wstring Widen(const wchar_t* wstr){return std::wstring{ wstr }; }
std::wstring Widen(const std::wstring& wstr) { return wstr; }
std::string Narrow(const char* str) { return std::string{ str }; }
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
