@echo off

git describe --tags --match "RAIntegration.*" > LiveTag.txt
@set /p ACTIVE_TAG=<LiveTag.txt
@set VERSION_NUM=%ACTIVE_TAG:~14,3%

git rev-parse --abbrev-ref HEAD > Temp.txt
@set /p ACTIVE_BRANCH=<Temp.txt
@if NOT "%ACTIVE_BRANCH%"=="master" GOTO NonMasterBranch

setlocal
git diff HEAD > Diffs.txt
@for /F "usebackq" %%A in ('"Diffs.txt"') do set DIFF_FILE_SIZE=%%~zA
@if %DIFF_FILE_SIZE% GTR 0 set ACTIVE_TAG=Unstaged Changes

@echo RAIntegration Tag: %ACTIVE_TAG% (%VERSION_NUM%)
@echo #define RA_INTEGRATION_VERSION "0.%VERSION_NUM%" > ./RA_BuildVer.h

@GOTO End

:NonMasterBranch
@echo RAIntegration Tag 0.999 - Not on Master Branch!
@echo #define RA_INTEGRATION_VERSION "0.999" > ./RA_BuildVer.h

:End