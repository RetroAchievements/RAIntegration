#ifndef STDAFX_H
#define STDAFX_H
#pragma once

// Windows stuff we don't need
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define OEMRESOURCE
#define NOATOM
#define NOKERNEL
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

// Disables most warnings for libs when compiling with /Wall (0 don't work)
#pragma warning(push)
#pragma warning(disable : 4191 4365 4571 4623 4625 4626 4701 4768 4774 4996 5026 5027 5031 5032 5039 5045)


// Uses the macros above
#include <atlbase.h>
#include <ddraw.h>
#include <direct.h>
#include <io.h>		//	_access()
#include <ShlObj.h>
#include <wincodec.h>
#include <WindowsX.h>
#include <winhttp.h>

#ifdef WIN32_LEAN_AND_MEAN
#include <CommDlg.h>
#include <MMSystem.h>
#include <ShellAPI.h>
#endif // WIN32_LEAN_AND_MEAN

#include <array>
#include <codecvt> // we need to get rid of this
#include <fstream>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <stack>

#if RA_EXPORTS
//NB. These must NOT be accessible from the emulator!
// This is not needed the most recent version

#define RAPIDJSON_HAS_STDSTRING 1
//	RA-Only
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/filestream.h>
#include <rapidjson/error/en.h>

#endif	//RA_EXPORTS


// Other RA Stuff that almost never changes
#include <md5.h>
#include "RA_Resource.h"
#pragma warning(pop)
#pragma warning(pop) // weird

#endif // !STDAFX_H

