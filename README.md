# RAIntegration

The DLL used for interfacing with RetroAchievements.org

### Prerequisites for Building

- Visual Studio 2022 Community Edition with the following components:
  - MSVC v142 - VS 2019 C++ x64/x86 build tools (v14.29)
  - MSVC v143 - VS 2022 C++ x64/x86 build tools (latest)
  - C++ ATL for latest v143 build tools (x86 & x64)
  - C++ MFC for latest v143 build tools (x86 & x64)
- git for windows: https://git-scm.com/download/win
  Note: the `git` binary must be in the PATH environment variable (select "Use Git from the Windows Command Prompt" on installation).

### Running Unit Tests

Unit tests are written using the Visual Studio testing framework (VSTest) and are automatically built when building the solution. To run them, simply open the Test Explorer window (Tests > Windows > Test Explorer) and click Run All.
