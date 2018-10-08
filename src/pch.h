#ifndef PCH_H
#define PCH_H
#pragma once

/*
    Only put files that never change or are rarely changed in this file.
    An include graph will be provided for some headers that were not explicit
    added. VC++ is a requirement.

    We don't care about warnings for library files.
*/
#pragma warning(push)
#pragma warning(disable : 4191 4365 4464 4571 4619 4623 4625 4626 4768 4774 5026 5027 5039 5045)
/* Windows Stuff */
#include "windows_nodefines.h"
#include <atlbase.h> // atldef.h (Windows.h), atlcore.h (tchar.h), Shlwapi.h
#include <direct.h>
#include <io.h>
#include <ShlObj.h> // CommCtrl
#include <wincodec.h>
#include <WindowsX.h>
#include <winhttp.h>

#ifdef WIN32_LEAN_AND_MEAN
#include <CommDlg.h>
#include <MMSystem.h>
#include <ShellAPI.h>
#endif // WIN32_LEAN_AND_MEAN

/*
    Nuke WinAPI #defines - anything including this file should be using the
    ra versions.
*/
#if RA_EXPORTS
#undef CreateDirectory
#undef GetMessage
#endif /* RA_EXPORTS */

/* C Stuff */
#include <cassert> 
#include <cctype>

/* STL Stuff */
#include <array> // algorithm, iterator, tuple
#include <atomic>
#include <fstream>
#include <iomanip>
#include <map>
#include <memory>
#include <mutex> // chrono (time.h), functional, thread, utility
#include <queue> // deque, vector, algorithm
#include <set>
#include <sstream> // string
#include <stack>

#pragma warning(push)
#pragma warning(disable : 26451 26495)
/* RapidJSON Stuff */
#if RA_EXPORTS
#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_NOMEMBERITERATORCLASS 1

#include <rapidjson\document.h> // has reader.h
#include <rapidjson\writer.h> // has stringbuffer.h
#include <rapidjson\istreamwrapper.h>
#include <rapidjson\ostreamwrapper.h>
#include <rapidjson\error\en.h>  
#endif /* RA_EXPORTS */
#pragma warning(pop)

/* Other Libs */
#include <md5.h>
#pragma warning(pop)

#endif /* !PCH_H */
