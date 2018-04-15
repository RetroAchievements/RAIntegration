@echo off

if not exist MakeBuildVer.bat cd ..

git describe --tags --match "RAIntegration.*" > Temp.txt
set /p ACTIVE_TAG=<Temp.txt
set VERSION_MAJOR=0
set VERSION_MINOR=%ACTIVE_TAG:~14,3%
set VERSION_REVISION=%ACTIVE_TAG:~18,-9%
if "%VERSION_REVISION%"=="" set VERSION_REVISION=0

set VERSION_PRODUCT=%VERSION_MAJOR%.%VERSION_MINOR%

setlocal
git diff HEAD > Temp.txt
for /F "usebackq" %%A in ('"Temp.txt"') do set DIFF_FILE_SIZE=%%~zA
if %DIFF_FILE_SIZE% GTR 0 (
    set ACTIVE_TAG=Unstaged changes
    set VERSION_MODIFIED=1
) else (
    set VERSION_MODIFIED=0
)

git rev-parse --abbrev-ref HEAD > Temp.txt
set /p ACTIVE_BRANCH=<Temp.txt
if "%ACTIVE_BRANCH%"=="master" (
    echo RAIntegration Tag: %ACTIVE_TAG% [%VERSION_MINOR%]
    echo #define RA_INTEGRATION_VERSION "0.%VERSION_MINOR%" > RA_BuildVer.h
) else (
    echo RAIntegration Tag 0.999 - Not on Master Branch!
    echo #define RA_INTEGRATION_VERSION "0.999" > RA_BuildVer.h
    set VERSION_PRODUCT=%VERSION_PRODUCT%-%ACTIVE_BRANCH%
)

echo #define RA_INTEGRATION_VERSION_MAJOR %VERSION_MAJOR% >> RA_BuildVer.h
echo #define RA_INTEGRATION_VERSION_MINOR %VERSION_MINOR% >> RA_BuildVer.h
echo #define RA_INTEGRATION_VERSION_REVISION %VERSION_REVISION% >> RA_BuildVer.h
echo #define RA_INTEGRATION_VERSION_MODIFIED %VERSION_MODIFIED% >> RA_BuildVer.h
echo #define RA_INTEGRATION_VERSION_PRODUCT "%VERSION_PRODUCT%" >> RA_BuildVer.h

if not exist src\RA_BuildVer.h goto nonexistant
fc src\RA_BuildVer.h RA_BuildVer.h > nul
if errorlevel 1 goto different
del RA_BuildVer.h
goto done
:different
del src\RA_BuildVer.h
:nonexistant
move RA_BuildVer.h src\RA_BuildVer.h > nul
:done


del Temp.txt
