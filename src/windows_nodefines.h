#ifndef WINDOWS_NODEFINES_H
#define WINDOWS_NODEFINES_H
#pragma once

/*
    Windows stuff we DO need, they are commented out to show we need them, if
    for some reason you get a compiler error put the offending NO* define here

    #define NOCOLOR
    #define NOCLIPBOARD - gave an error when put in the pch
    #define NOCTLMGR
    #define NODRAWTEXT
    #define NOGDI
    #define NOMB
    #define NOMENUS
    #define NOMSG
    #define NONLS
    #define NOOPENFILE
    #define NORASTEROPS
    #define NOSHOWWINDOW
    #define NOTEXTMETRIC
    #define NOUSER
    #define NOVIRTUALKEYCODES
    #define NOWINMESSAGES
    #define NOWINOFFSETS
    #define NOWINSTYLES
*/

/* Windows stuff we don't need */
#define WIN32_LEAN_AND_MEAN 1
#define NOGDICAPMASKS       1
#define NOSYSMETRICS        1
#define NOICONS             1
#define NOKEYSTATES         1
#define NOSYSCOMMANDS       1
#define OEMRESOURCE         1
#define NOATOM              1
#define NOKERNEL            1
#define NOMEMMGR            1
#define NOMETAFILE          1
#define NOMINMAX            1
#define NOSCROLL            1
#define NOSERVICE           1
#define NOSOUND             1
#define NOWH                1
#define NOCOMM              1
#define NOKANJI             1                                                             
#define NOHELP              1
#define NOPROFILER          1
#define NODEFERWINDOWPOS    1
#define NOMCX               1

/*
    Problem struct, this file should get included before any windows header
    file anyway
*/          
struct IUnknown;

#endif /* !WINDOWS_NODEFINES_H */
