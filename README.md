# RAIntegration

The DLL used for interfacing with RetroAchievements.org

### Prerequisites for Building

- Visual Studio 2019 Community Edition with the following components:
  - MSVC v141 - VS 2017 C++ x64/x86 build tools (v14.16)
  - MSVC v142 - VS 2019 C++ x64/x86 build tools (Latest)
  - C++ ATL for v141 build tools (x86 & x64)
  - C++ MFC for v141 build tools (x86 & x64)
  - C++ ATL for latest v142 build tools (x86 & x64)
  - C++ MFC for latest v142 build tools (x86 & x64)
  - C++ Windows XP Support for VS 2017 (v141) tools [Deprecated]
- Windows SDK 7.0 (included in 8.1 distribution)
  - Install via [chocolatey](https://docs.chocolatey.org/en-us/choco/setup). This may take a while; just be patient.
    ```
    choco install windows-sdk-8.1
    ```
- git for windows: https://git-scm.com/download/win
  Note: the `git` binary must be in the PATH environment variable (select "Use Git from the Windows Command Prompt" on installation).

### Running Unit Tests

Unit tests are written using the Visual Studio testing framework (VSTest) and are automatically built when building the solution. To run them, simply open the Test Explorer window (Tests > Windows > Test Explorer) and click Run All.
