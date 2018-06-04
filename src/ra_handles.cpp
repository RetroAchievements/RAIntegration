#include "ra_handles.h"


// We are using __stdcall because of how similar it is to __thiscall. We expect
// the callers to cleanup not the callee, that's what RAII is mostly about. Most
// these functions call the Windows API anyway.

// Single param callbacks can't be forwarded as single param constructors are
// explicit to prevent implicit conversion to the param

namespace ra {

#pragma region Kernel Resource Callbacks
_Use_decl_annotations_
FileH CALLBACK make_file(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    HANDLE hTemplateFile) noexcept
{
    return { lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, dwFlagsAndAttributes,
        lpSecurityAttributes, hTemplateFile };
}

_Use_decl_annotations_
FileSearchH CALLBACK make_find_file(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData) noexcept
{
    return { lpFileName, lpFindFileData };
}

_Use_decl_annotations_
_NODISCARD MutexH CALLBACK make_mutex(BOOL bInitialOwner, LPSECURITY_ATTRIBUTES lpMutexAttributes, LPCTSTR lpName) noexcept
{
    return { bInitialOwner, lpMutexAttributes, lpName };
}

_Use_decl_annotations_
ThreadH CALLBACK make_thread(SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, DWORD dwCreationFlags,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,LPVOID lpParameter,
    LPDWORD lpThreadId) noexcept
{
    return { dwStackSize, lpStartAddress, dwCreationFlags, lpThreadAttributes, lpParameter, lpThreadId };
}
#pragma endregion




} // namespace ra


