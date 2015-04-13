@echo off

REM git describe --tags --long > LiveTag.txt
git describe --tags --match "RAIntegration.*" > LiveTag.txt
@set /p ACTIVE_TAG=<LiveTag.txt
@set VERSION_NUM=%ACTIVE_TAG:~14,3%

git diff HEAD > Diffs.txt
@set /p RAW_DIFFS_FOUND=<Diffs.txt

setlocal
@for /F "usebackq" %%A in ('"Diffs.txt"') do set DIFF_FILE_SIZE=%%~zA
@if %DIFF_FILE_SIZE% GTR 0 set ACTIVE_TAG=Unstaged Changes

@echo RAIntegration Tag: %ACTIVE_TAG% (%VERSION_NUM%)
@echo #define RA_INTEGRATION_VERSION "0.%VERSION_NUM%" > ./RA_BuildVer.h