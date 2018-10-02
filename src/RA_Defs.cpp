#include "RA_Defs.h"

#ifndef RA_UTEST
extern std::wstring g_sHomeDir;
#endif

void RADebugLogNoFormat(const char* data)
{
    OutputDebugString(ra::NativeStr(data).c_str());
#ifndef RA_UTEST
    std::wstring sLogFile = g_sHomeDir + RA_LOG_FILENAME;
    FILE* pf = nullptr;
    if (_wfopen_s(&pf, sLogFile.c_str(), L"a") == 0)
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
    DWORD dwAttrib = GetFileAttributes(ra::NativeStr(sPath).c_str());
    return(dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
