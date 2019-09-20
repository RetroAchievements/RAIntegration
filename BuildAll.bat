@echo off
setlocal

rem === Globals ===

set BUILDLOG=obj\buildall.log

set BUILDALL=1
set BUILDCLEAN=0
set W32_DEBUG=0
set W32_RELEASE=0
set W32_TESTS=0
set W32_ANALYSIS=0

for %%A in (%*) do (
    if /I "%%A" == "Clean" (
        set BUILDCLEAN=1
    ) else if /I "%%A" == "Debug" (
        set W32_DEBUG=1
        set BUILDALL=0
    ) else if /I "%%A" == "Release" (
        set W32_RELEASE=1
        set BUILDALL=0
    ) else if /I "%%A" == "Tests" (
        set W32_TESTS=1
        set BUILDALL=0
    ) else if /I "%%A" == "Analysis" (
        set W32_ANALYSIS=1
        set BUILDALL=0   
    )
)

if %BUILDALL% equ 1 (
    set W32_DEBUG=1
    set W32_RELEASE=1
    set W32_TESTS=1
    set W32_ANALYSIS=1
)

rem === Get hash for current code state ===

git log --oneline -1 > Temp.txt
set /p GIT_HASH=<Temp.txt
set GIT_HASH=%GIT_HASH:~0,7%

git diff HEAD > Temp.txt
CertUtil -hashfile Temp.txt MD5 > Temp2.txt
for /f "skip=1 tokens=1" %%a in (Temp2.txt) do set DIFF_HASH=%%a & goto havehash
:havehash
set BUILD_HASH=%GIT_HASH%-%DIFF_HASH%
del Temp.txt
del Temp2.txt

rem === If the buildlog doesn't exist or the hash has changed, start a new buildlog ===

if not exist %BUILDLOG% echo %BUILD_HASH% > %BUILDLOG%
find /c "%BUILD_HASH%" %BUILDLOG% >nul 2>&1
if %ERRORLEVEL% equ 1 echo %BUILD_HASH% > %BUILDLOG%

rem === Initialize Visual Studio environment ===

echo Initializing Visual Studio environment
if "%VSINSTALLDIR%"=="" set VSINSTALLDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\
if not exist "%VSINSTALLDIR%" set VSINSTALLDIR="%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\"  
if not exist "%VSINSTALLDIR%" (
    echo Could not determine VSINSTALLDIR
    exit /B 1
)
echo using VSINSTALLDIR=%VSINSTALLDIR%

call "%VSINSTALLDIR%VC\Auxiliary\Build\vcvars32.bat"

rem === Build each project ===

if %W32_DEBUG% equ 1 (
    if %BUILDCLEAN% equ 1 (
        call :build Clean Debug
    )
    call :build RA_Integration Debug
    if not ERRORLEVEL 0 goto eof
)

if %BUILDCLEAN% equ 1 (
    if %W32_RELEASE% equ 1 (
        call :build Clean Release
    ) else if %W32_TESTS% equ 1 (
        call :build Clean Release
    )
)

if %W32_RELEASE% equ 1 (
    call :build RA_Integration Release
    if not ERRORLEVEL 0 goto eof
)

if %W32_TESTS% equ 1 (
    call :build RA_Integration.Tests Release
    if not ERRORLEVEL 0 goto eof
    call :build RA_Interface.Tests Release 
    if not ERRORLEVEL 0 goto eof
)

if %W32_ANALYSIS% equ 1 (
    if %BUILDCLEAN% equ 1 (
        call :build Clean Analysis
    )
    call :build RA_Integration Analysis
    if not ERRORLEVEL 0 goto eof
)

exit /B 0

rem === Build subroutine ===

:build

rem === If the project was already successfully built for this hash, ignore it ===

set PROJECTKEY=%~1-%~2
find /c "%PROJECTKEY%" %BUILDLOG% >nul 2>&1 
if %ERRORLEVEL% equ 0 (
    if not "%~1" equ "Clean" (
        echo.
        echo %~1 %~2 up-to-date!
    )
    exit /B 0
)

rem === Do the build ===

echo.
echo Building %~1 %~2

rem -- vsbuild doesn't like periods in the "-t" parameter, so replace them with underscores
rem -- https://stackoverflow.com/questions/56253635/what-could-i-be-doing-wrong-with-my-msbuild-argument
rem -- https://stackoverflow.com/questions/15441422/replace-character-of-string-in-batch-script/25812295
set ESCAPEDKEY=%~1
:replace_periods
for /f "tokens=1* delims=." %%i in ("%ESCAPEDKEY%") do (
   set ESCAPEDKEY=%%j
   if defined ESCAPEDKEY (
      set ESCAPEDKEY=%%i_%%j
      goto replace_periods
   ) else (
      set ESCAPEDKEY=%%i
   )
)

msbuild.exe RA_Integration.sln -t:%ESCAPEDKEY% -p:Configuration=%~2 -p:Platform=Win32 /warnaserror /nowarn:MSB8051,C5045

rem === If build failed, bail ===

if not ERRORLEVEL 0 (
    echo %~1 %~2 failed.
    goto eof
)

rem === If test project, run tests ===

if "%ESCAPEDKEY:~-6%" neq "_Tests" goto not_tests

set DLL_PATH=bin\%~2\tests\%~1.dll
if not exist %DLL_PATH% set DLL_PATH=bin\%~2\tests\%ESCAPEDKEY:~0,-6%\%~1.dll

if exist %DLL_PATH% (
    VsTest.Console.exe %DLL_PATH%
    
    if not ERRORLEVEL 0 goto eof
) else (
    echo Could not locate %~1.dll
    goto eof
)
:not_tests

rem === Update build log and return ===

echo %PROJECTKEY% >> %BUILDLOG%
exit /B 0

rem === For termination from within function ===

:eof
