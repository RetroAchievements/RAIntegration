rem This assumes you have nuget.exe installed and are running from the same directory
rem It used to be possible to install these via packages.config but it doesn't seem to work in VS2017 after moving to ProjectReferences
md packages
cd packages
nuget install rapidjson.v110
nuget install tinyformat
cd ..

rem This directory is deleted because it's cached elsewhere (%NUGET_PACKAGES%), .csproj solves this issue but the C++ one.
del /f /s /q packages 1>nul
rd /s /q packages
