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

// NB: A huge majority of the warnings were from sign/unsigned mismatches/misuse,
//     we will use a function template to expedite the process.

// NB: Sign/unsigned stuff as well as fallthrough will be done separately because
//     of how many warnings they triggered (around a thousand)

// Warnings that show up in RA files we'll resolve later (too much for one PR)

#if RA_EXPORTS
// enum case labels not handled (resolve by putting them all in before default case with a [[fallthrough]]
#pragma warning(disable : 4061) // Warning count: 70
#pragma warning(disable : 4062) // Same as C4061; Warning count: 355

// unused function parameters (resolve with [[maybe_unused]])
#pragma warning(disable : 4100) // Warning count: 29

// Using "-" on an unsigned type without casting (resolve by casting)
#pragma warning(disable : 4146) // Warning count: 1

// Unused initialized local variables (resolve with [[maybe_unused]])
#pragma warning(disable : 4189)  // Warning count: 31

// Unsafe cast from DialogProc to TIMERPROC (resolve with a special TimerProc
// callback either as std::function, lambda, or non-member function)
#pragma warning(disable : 4191)  // Warning count: 1

// Nonstandard extension used with rapidjson::FileStream.
// Basically using rvalues as lvalues without an overload
// Undefined if there is no overload for rvalue references, worked with luck
// (didn't see any move constructors/assignments in the version used but the latest one does have move semantics.)

// Examples where using an rvalue as lvalue works; any standard container because
// move semantics are defined. Doesn't work with the version of rapidjson used
// because moving is deleted. (deleting copying without defining moving deletes
// moving as well)

// Resolve by making the FileStream rvalues into lvalues
// Example: FileStream{FILE*}          -> FileStream&& (rvalue)
//          FileStream myStream{FILE*} -> FileStream& (lvalue)
#pragma warning(disable : 4239) // Warning count: 1
#pragma warning(disable : 4242) // Implicit narrowing conversion. Warning count: 1
#pragma warning(disable : 4244) // Implicit narrowing conversion. Warning count: 1
#pragma warning(disable : 4245) // Signed/Unsigned mismatch. Warning count: 15
#pragma warning(disable : 4365) // Signed/Unsigned mismatch. Warning count: 318
#pragma warning(disable : 4389) // Signed/Unsigned mismatch. Warning count: 8

// Variable is declared with another variable already declared
// Resolve by either removing the declaration by just making it an assignment or give it a different name.
#pragma warning(disable : 4456) // Warning count: 7

// Local declaration hides function parameter. Resolve by giving it a new name, assuming it's an "in" param.
#pragma warning(disable : 4457) // Warning count: 1

// Unreferenced inline function has been removed (might seem confusing)
// This could mean two things, either the function isn't used or it can't be inline.

// It's safe to assume if something cannot result in a constant expression (std::string) it can't be inline either
// The majority of cases are the latter. Results that could be constant
// expressions but are retrieved by runtime expressions can't be inline either
// (std::vector::begin is NOT constexpr but std::begin is).
#pragma warning(disable : 4514) // Warning count: 2498

// Assignment operator has been implicitly defined as deleted (problematic). From
// observation, it seems that too many classes misuse const for it's members and
// try to use move semantics on them. If you want them to be movable but not
// copyable, remove the const from data members, delete copying and declare
// moving as default (if it can be done as noexcept). Moving by definition uses
// an rvalue reference (temporary) and needs to be drained (0'd out) when done.
// If they are const, it can't be drained, resulting in a dangling pointer
// (memory leak)
#pragma warning(disable : 4626) // Warning count: 60
// uninitialized variable used, resolve by initializing it (primitives don't have default constructors)
#pragma warning(disable : 4700) // Warning count: 1
#pragma warning(disable : 4701) // Same as 4700. Warning count: 4

// format string expected in argument 3 is not a string literal (sprintf)
// resolve by using "%s" if there is only one string.
#pragma warning(disable : 4774) // Warning count: 7

// sprintf: Wrong format specifier used (sprintf sucks)
#pragma warning(disable : 4777) // Warning count: 18

// Struct misalignment
// Resolve by using pragma pack(push,pop) by packing the correct alignment
// Example: Compiler might pad 3 bytes onto a bool, to resolve it put "#pragma pack(push, 1)"
// Do not start popping until all packing as been done.
#pragma warning(disable : 4820) // Warning count: 171

// Deprecation issued by compiler.
// codecvt: resolve by using MutliByteToWideChar or WideCharToMutliByte (windows versions of mbtowcs/wctombs)
// As far as we could tell the Windows versions work better than the Standard C versions in this case.
#pragma warning(disable : 4996) // Warning count: 20

// Move Assignment operator implicitly deleted (extremely troubling)
#pragma warning(disable : 5027) // Warning count: 52

// Initializing data members in the wrong order.
// If a default constructor, remove the definition, initialize all members
// in-class and declare the default constructor as default if any other
// constructor is defined (including destructor)
#pragma warning(disable : 5038) // Warning count: 43

// C functions that throw exceptions; CallWindowProc, SetWindowLongPtr, SetTimer,
// DialogBox, CreateDialog, _RA_InstallSharedFunctionsExt
// Two different was to solve this, the proper and the lazy way.

// Proper: Make class wrappers that test the success of the functions in a
//         constructor and throw on failure
// Lazy: Change the exception model to include C functions (they will still throw
//       and cause undefined behavior but you won't get the warning)
#pragma warning(disable : 5039) // Warning count: 24

// Spectre vulnerabilities (you like getting hacked son?)
// Resolve by not using raw pointers and indexes, in other words use OOP instead
// of the outdated DOP (data-oriented programming)
#pragma warning(disable : 5045) // Warning count: 8  
#endif // RA_EXPORTS


// Disables warnings for libs when compiling with /Wall (0 don't work)
// Warning suppressions popped off so we can see them in our code
#pragma warning(push)
#pragma warning(disable : 4061 4191 4365 4571 4623 4625 4626 4701 4768 4774 4996 5026 5027 5031 5032 5039 5045)


// Uses the macros above
#include <atlbase.h> // Windows.h
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

#include <array> // algorithm
#include <codecvt> // we need to get rid of this
#include <fstream>
#include <iostream>
#include <map>
#include <mutex> // chrono, thread, functional, utility, type_traits
#include <queue>
#include <sstream> // string
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
#pragma warning(pop) // weird, it seems one of the Windows headers misses a pop



#endif // !STDAFX_H

