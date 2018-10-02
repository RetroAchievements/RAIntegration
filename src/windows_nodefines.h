#ifndef WINDOWS_NODEFINES_H
#define WINDOWS_NODEFINES_H
#pragma once

// Windows stuff we DO need, they are commented out to show we need them, if for
// some reason you get a compiler error put the offending NO* define here
/*
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

#endif // !WINDOWS_NODEFINES_H
