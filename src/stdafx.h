#ifndef STDAFX_H
#define STDAFX_H
#pragma once

// Only library files, or project files that almost never change should be here

// Look at "transitive_includes.txt" in the "Notes" section if you want to know
// why some aren't here.

// Strangely this doesn't disable all warnings
#pragma warning(push, 0)

// It's now our job to fix standard/3rd party libraries
#pragma warning(disable : 4365 4571 4623 4625 4626 4774 5026 5027 5039)


// There's also a few errors with our own code
// W4: 2 warnings
// Wall: 2 warnings, 14 errors

// TODO: Set warning level back to /Wall after those issues have been addressed

#define WIN32_LEAN_AND_MEAN // doesn't include all of Windows.h
#pragma region Windows
#include <wincodec.h> // must be at the top

#ifdef WIN32_LEAN_AND_MEAN
#include <MMSystem.h>
#include <ShellAPI.h>
#include <CommDlg.h>
#endif // WIN32_LEAN_AND_MEAN

#include <winhttp.h>
#include <ddraw.h>
#include <WindowsX.h> // Message Crackers, casts stuff for us
#include <ShlObj.h>
#include <tchar.h>
#include <io.h>
#include <direct.h>
#include <strsafe.h> // pretty sure it's included somewhere, putting it here just incase
#include <atlbase.h>
#pragma endregion




#pragma region Standard C++
#include <sstream>
#include <queue>
#include <stack>
#include <mutex>
#include <fstream>
#include <map> // unordered_map will be included regardless because of mutex

// For these two I noticed some of classes that could benefit from these to enforce uniqueness.
// I think it was in leaderboardpopups
#include <unordered_set> // if you don't care about order
#include <set>
#pragma endregion








#pragma region Standard C
#include <cassert> // might not need it later
#include <cstdio> // might not need it
#pragma endregion


#pragma region 3rd Party Libraries
#ifdef RA_EXPORTS

// TODO: Remove "_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING" from the project defines
// after RapidJSON has been updated


// TODO: Replace filestream.h with IStreamWrapper.h and OStreamWrapper.h later
//	RA-Only
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/filestream.h>
#include <rapidjson/error/en.h>


#include "md5.h"
#endif // RA_EXPORTS

#pragma region Project Files
#include "RA_Resource.h"
#pragma endregion




#pragma endregion




#pragma warning(pop)


#endif // !STDAFX_H

