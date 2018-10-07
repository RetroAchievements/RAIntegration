#ifndef PCH_H
#define PCH_H
#pragma once

/*
    Only put files that never change or are rarely changed in this file.
    An include graph will be provided for some headers that were not explicit
    added. VC++ is a requirement.
*/

/* Windows Stuff */
#include "windows_config.h"
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

/* STL Stuff */
#include <array> // algorithm, iterator, tuple
#include <atomic>
#include <fstream>
#include <iomanip>
#include <map>
#include <mutex> // chrono (time.h), functional, thread (memory), utility
#include <queue> // deque, vector, algorithm
#include <set>
#include <sstream> // string
#include <stack>

/* Other Libs */
#include <md5.h>

#endif /* !PCH_H */
