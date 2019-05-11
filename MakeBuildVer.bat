@echo off
setlocal

if not exist MakeBuildVer.bat cd ..

rem === Get the most recent tag matching our prefix ===
git describe --tags --match "RAIntegration.*" > Temp.txt
set /p ACTIVE_TAG=<Temp.txt
for /f "tokens=1,2 delims=-" %%a in ("%ACTIVE_TAG:~14%") do set VERSION_TAG=%%a&set VERSION_REVISION=%%b
if "%VERSION_REVISION%" == "" set VERSION_REVISION=0

rem === Extract the major/minor/patch version from the tag (append 0s if necessary) ===
for /f "tokens=1,2,3 delims=." %%a in ("%VERSION_TAG%.0.0") do set VERSION_MAJOR=%%a&set VERSION_MINOR=%%b&set VERSION_PATCH=%%c
set VERSION_PRODUCT=%VERSION_MAJOR%.%VERSION_MINOR%

rem === If there are any local modifications, increment revision ===
git diff HEAD > Temp.txt
for /F "usebackq" %%A in ('"Temp.txt"') do set DIFF_FILE_SIZE=%%~zA
if %DIFF_FILE_SIZE% GTR 0 (
    set ACTIVE_TAG=Unstaged changes
    set /A VERSION_REVISION=VERSION_REVISION+1
)

rem === If on a branch, append the branch name to the product version ===
git rev-parse --abbrev-ref HEAD > Temp.txt
set /p ACTIVE_BRANCH=<Temp.txt
if not "%ACTIVE_BRANCH%"=="master" (
    set VERSION_PRODUCT=%VERSION_PRODUCT%-%ACTIVE_BRANCH%
)

rem === Generate a new version file ===
@echo Tag: %ACTIVE_TAG% (%VERSION_TAG%)

echo #define RA_INTEGRATION_VERSION "%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%VERSION_REVISION%" > RA_BuildVer.h
echo #define RA_INTEGRATION_VERSION_MAJOR %VERSION_MAJOR% >> RA_BuildVer.h
echo #define RA_INTEGRATION_VERSION_MINOR %VERSION_MINOR% >> RA_BuildVer.h
echo #define RA_INTEGRATION_VERSION_PATCH %VERSION_PATCH% >> RA_BuildVer.h
echo #define RA_INTEGRATION_VERSION_REVISION %VERSION_REVISION% >> RA_BuildVer.h
echo #define RA_INTEGRATION_VERSION_PRODUCT "%VERSION_PRODUCT%" >> RA_BuildVer.h

rem === Update the existing file only if the new file differs ===
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

rem === Clean up after ourselves ===
del Temp.txt
