@echo off
setlocal

rem === Globals ===

set BUILDLOG=obj\buildall.log

set BUILDALL=1
set BUILDCLEAN=0
set DEBUG=0
set RELEASE=0
set TESTS=0
set ANALYSIS=0
set W32=0
set W64=0
set RESULT=0

for %%A in (%*) do (
    if /I "%%A" == "Clean" (
        set BUILDCLEAN=1
        del %BUILDLOG%
    ) else if /I "%%A" == "Debug" (
        set DEBUG=1
        set BUILDALL=0
    ) else if /I "%%A" == "Release" (
        set RELEASE=1
        set BUILDALL=0
    ) else if /I "%%A" == "Tests" (
        set TESTS=1
        set BUILDALL=0
    ) else if /I "%%A" == "Analysis" (
        set ANALYSIS=1
        set BUILDALL=0   
    ) else if /I "%%A" == "x86" (
        set W32=1
    ) else if /I "%%A" == "x64" (
        set W64=1
    )
)

if %BUILDALL% equ 1 (
    set DEBUG=1
    set RELEASE=1
    set TESTS=1
    set ANALYSIS=1
)

if %W32% equ 0 if %W64% equ 0 (
    set W32=1
    set W64=1
)

rem === If building tests, make sure the DLL exists for the Integration test ===
if %TESTS% equ 1 if %RELEASE% equ 0 (
    if %W32% equ 1 if not exist bin\Win32\Release\RA_Integration.dll set RELEASE=1
    if %W64% equ 1 if not exist bin\x64\Release\RA_Integration.dll set RELEASE=1
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
if ERRORLEVEL 1 echo %BUILD_HASH% > %BUILDLOG%

rem === Initialize Visual Studio environment ===

echo Initializing Visual Studio environment
if "%VSINSTALLDIR%"=="" set VSINSTALLDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\
if not exist "%VSINSTALLDIR%" set VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\
if not exist "%VSINSTALLDIR%" set VSINSTALLDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\
if not exist "%VSINSTALLDIR%" set VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\
if not exist "%VSINSTALLDIR%" (
    echo Could not determine VSINSTALLDIR
    exit /B 1
)
echo using VSINSTALLDIR=%VSINSTALLDIR%

if "%VSDEVCMD%"=="" set VSDEVCMD=%VSINSTALLDIR%Common7\Tools\VsDevCmd.bat
if not exist "%VSDEVCMD%" set VSDEVCMD=%VSINSTALLDIR%VC\Auxiliary\Build\vcvars32.bat
set VSCMD_SKIP_SENDTELEMETRY=1
echo calling "%VSDEVCMD%"
call "%VSDEVCMD%"

rem === Build each project ===

if %BUILDCLEAN% equ 1 (
    if %DEBUG% equ 1 (
        call :build Clean Debug || goto eof
    )

    if %RELEASE% equ 1 (
        call :build Clean Release || goto eof
    ) else if %TESTS% equ 1 (
        call :build Clean Release || goto eof
    )

    if %ANALYSIS% equ 1 (
        call :build Clean Analysis || goto eof
    )
) else (
    if %DEBUG% equ 1 (
        call :build RA_Integration Debug || goto eof
    )

    if %RELEASE% equ 1 (
        call :build RA_Integration Release || goto eof
    )

    if %TESTS% equ 1 (
        call :build RA_Integration.Tests Release || goto eof
        call :build RA_Interface.Tests Release || goto eof
    )

    if %ANALYSIS% equ 1 (
        del /s *.lastcodeanalysissucceeded
        call :build RA_Integration Analysis || goto eof
        call :build RA_Integration.Tests Analysis || goto eof
    )
)

exit /B

rem === Build subroutine ===

:build

set PROJECT=%~1
set CONFIG=%~2

if %W32% equ 1 call :build2 %PROJECT% %CONFIG% Win32
if %W64% equ 1 call :build2 %PROJECT% %CONFIG% x64

exit /B

:build2

rem === If the project was already successfully built for this hash, ignore it ===

set PROJECTKEY=%~1-%~2-%~3
find /c "%PROJECTKEY%" %BUILDLOG% >nul 2>&1 
if not ERRORLEVEL 1 (
    if not "%~1" equ "Clean" (
        echo.
        echo %~1 %~2 %~3 up-to-date!
        exit /B 0
    )
)

rem === Do the build ===

echo.
echo Building %~1 %~2 %~3

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

set RESULT=0
msbuild.exe RA_Integration.sln -t:%ESCAPEDKEY% -p:Configuration=%~2 -p:Platform=%~3 /warnaserror /nowarn:MSB8051,C5045 || set RESULT=1234

rem === If build failed, bail ===

if not %RESULT% equ 0 (
    echo %~1 %~2 %~3 failed: %RESULT%
    exit /B %RESULT%
)

rem === If test project, run tests ===

if "%ESCAPEDKEY:~-6%" neq "_Tests" goto not_tests
if "%~2%" equ "Analysis" goto not_tests

set DLL_PATH=bin\%~3\%~2\tests\%~1.dll
if not exist %DLL_PATH% set DLL_PATH=bin\%~3\%~2\tests\%ESCAPEDKEY:~0,-6%\%~1.dll

if not exist %DLL_PATH% (
    echo "Could not locate %~1.dll (%~2 %~3)"
    exit /B 1
)

rem -- the default VsTest.Console.exe (in CommonExtensions) does not return ERRORLEVEL, use one in Extensions instead
rem -- see https://github.com/Microsoft/vstest/issues/1113
rem -- also, cannot use exists on path with spaces, so use dir and check result
set VSTEST_PATH=%VSINSTALLDIR%Common7\IDE\Extensions\TestPlatform\VsTest.Console.exe
dir "%VSTEST_PATH%" > nul || set VSTEST_PATH=VsTest.Console.exe

echo.
echo Calling %VSTEST_PATH% %DLL_PATH%

set RESULT=0
"%VSTEST_PATH%" /Blame %DLL_PATH% || set RESULT=1235

rem -- report any errors captured by /Blame --
rem if exist TestResults (
rem     cd TestResults
rem     for /R %%f in (*.xml) do (
rem         type "%%f"
rem     )
rem     cd ..
rem )

if not %RESULT% equ 0 exit /B %RESULT%

:not_tests

rem === Update build log and return ===

echo %PROJECTKEY% >> %BUILDLOG%

rem === For termination from within function ===

:eof
echo BuildAll returning exit code %RESULT%
exit /B %RESULT%
