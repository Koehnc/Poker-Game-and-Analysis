# Build Requirements

## Toolchain

| Component | Version | Notes |
|---|---|---|
| Qt | 6.8.3, `mingw_64` kit | Installed at `C:\Qt\6.8.3\mingw_64`. Provides Qt Widgets and the bundled compiler below. |
| Compiler | MinGW-w64 GCC 13.1.0 | Qt's bundled `mingw1310_64` kit at `C:\Qt\Tools\mingw1310_64\bin\g++.exe` — **not** any other MinGW/GCC install on the machine. See [Why the exact compiler matters](#why-the-exact-compiler-matters) below. |
| CMake | 3.16+ | Tested with 4.3.2 (`C:\Program Files\CMake\bin\cmake.exe`). Qt's own Tools directory doesn't ship a `cmake.exe`, so this is usually a separate install. |
| Generator | MinGW Makefiles | `mingw32-make.exe` ships alongside the compiler in `mingw1310_64\bin`. |

C++17, set via `CMAKE_CXX_STANDARD` in `CMakeLists.txt` — nothing to configure manually.

## Configuring a build directory

```powershell
mkdir build
cd build
& "C:\Program Files\CMake\bin\cmake.exe" .. -G "MinGW Makefiles" `
    -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe" `
    -DCMAKE_MAKE_PROGRAM="C:/Qt/Tools/mingw1310_64/bin/mingw32-make.exe" `
    -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"
```

`CMAKE_PREFIX_PATH` is what lets `find_package(Qt6 ...)` in `CMakeLists.txt` locate Qt at all — without it, `poker-ui` silently gets skipped ("Qt6 not found — poker-ui target skipped") and only `poker-train`/`test-genome` get configured.

## Building

```powershell
& "C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe" poker-train poker-ui test-genome -j8
```

`poker-ui.exe` needs its Qt runtime DLLs (`Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, the `platforms/qwindows.dll` plugin, etc.) sitting next to it to actually launch. If `build/` already has them from a previous `windeployqt` run or a copy from another build directory, a fresh `poker-ui.exe` will pick them up automatically — Windows searches the executable's own directory first.

## Why the exact compiler matters

If CMake picks up a *different* GCC/MinGW install — for example, a system-wide one on `PATH` from a package manager — the build **succeeds without any error or warning**. The resulting `poker-ui.exe` looks fine, but fails to launch at all: it exits almost instantly with no window, no console output, and exit code `-1073741511` (`STATUS_DLL_NOT_FOUND`, `0xC0000135`).

The cause: different MinGW-w64 builds target different C runtimes. Qt's bundled `mingw1310_64` links against the legacy `msvcrt.dll` (present on every Windows install), while some other toolchains link against the split Universal CRT (`api-ms-win-crt-*.dll` forwarder stubs). The Qt-deployed runtime DLLs sitting next to `poker-ui.exe` (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`) match the `msvcrt` model — an exe built against the UCRT model won't resolve against them, and the OS loader fails before any of your code (even `main()`) runs.

To check which compiler a given `build/` directory is actually configured to use:

```powershell
Select-String "CMAKE_CXX_COMPILER:" build\CMakeCache.txt
```

It should point at `C:/Qt/Tools/mingw1310_64/bin/g++.exe`. If it points anywhere else, reconfigure that build directory with the exact `cmake` invocation above rather than assuming the build is fine just because it compiled — a successful compile does not mean the binary will run.

You can also compare an executable's actual import table to check for this:

```powershell
& "C:\path\to\objdump.exe" -p build\poker-ui.exe | Select-String "DLL Name"
```

A working build shows `msvcrt.dll` in the list. A mis-toolchained one shows a run of `api-ms-win-crt-*.dll` entries instead.
