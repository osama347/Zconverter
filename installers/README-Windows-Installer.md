zconverter — Windows installer build instructions

Prerequisites (on Windows build machine):
- Visual Studio (MSVC) matching your Qt build (e.g. MSVC 2022)
- Qt 6 (MSVC) installed and available in PATH or via Qt installer
- CMake 3.28+
- NSIS (makensis) for producing the .exe installer
- Optional: LibreOffice portable folder if you want to bundle LibreOffice

Quick steps (recommended, from project root):

1) Configure for Windows (uses the existing preset):

```powershell
cmake --preset x64-release
```

2) Build and run the install/packaging stage (this runs the CMake install which triggers windeployqt):

```powershell
cmake --build --preset x64-release --config Release --target package
```

This should produce an NSIS installer via CPack (if `NSIS` is installed and CPack finds it). The generated installer typically appears in the build folder (e.g. `out/build/x64-release`).

Manual alternative: Create a staged payload and run makensis

If you prefer to build the installer yourself with a custom staging folder:

1. Build and install to a staging directory:

```powershell
cmake --preset x64-release
cmake --build --preset x64-release --config Release --target install
```

By default the `installDir` in `CMakePresets.json` is set and you can inspect `out/install/x64-release` (or the path from your preset). Ensure the staged directory contains:
- `bin\zconverter.exe`
- `bin\zconverter_shell.dll`
- all required Qt runtime DLLs and plugins (use `windeployqt` to stage them if needed)
- optional `libreoffice\` folder (portable LibreOffice root) if bundling

2. Run NSIS with the provided script (from repo `installers` folder). Example (PowerShell):

```powershell
# from repo root
makensis -DSTAGEDIR="out\install\x64-release" installers\zconverter_installer.nsi
```

The resulting `zconverter-setup.exe` will be produced in the current folder.

Notes & important details
- The installer must run elevated to register the shell extension (COM DLL). The NSIS script sets `RequestExecutionLevel admin` and calls `regsvr32` during installation.
- The `windeployqt` step should be performed on the Windows build machine before packaging so the installer bundles all required Qt DLLs/plugins.
- If you want to bundle LibreOffice, set `ZCONVERTER_LIBREOFFICE_ROOT` in CMake configure step to point to the portable LibreOffice root; the installer will include it into the install tree.

Example to set the LibreOffice bundle before configure:

```powershell
cmake --preset x64-release -DZCONVERTER_LIBREOFFICE_ROOT="C:/tools/LibreOfficePortable"
cmake --build --preset x64-release --config Release --target package
```

Testing on Windows
- Run the produced `zconverter-setup.exe` on a test Windows VM.
- Confirm `zconverter` appears in Start Menu and `zconverter_shell.dll` is registered.
- Right-click supported files in Explorer and verify the "Convert to PDF" context menu appears and works.

If you want, I can also:
- Produce a pre-staged `payload` zip (but that requires packaging runtime Qt and LibreOffice which must be done on Windows due to toolchain differences).
- Generate a CPack NSIS template for more advanced installer UI and versioning.
Creating a pre-staged payload zip (recommended)

I added `installers/create_payload.ps1` and `installers/create_payload.bat` to create a pre-staged payload zip containing:
- `bin\zconverter.exe` and any runtime Qt DLLs/plugins (collected with `windeployqt`)
- `bin\zconverter_shell.dll`
- optional `bin\libreoffice` (if you pass a portable LibreOffice root)

Run on a Windows build machine after you produced the staged install (`cmake --install`):

```powershell
# from repo root (adjust paths if you used different presets)
# Stage install first
cmake --preset x64-release
cmake --build --preset x64-release --config Release --target install

# Create the payload zip (PowerShell)
PowerShell -ExecutionPolicy Bypass -File installers\create_payload.ps1 \
	-StagedDir "out\\install\\x64-release" \
	-OutZip "out\\install\\zconverter-payload.zip" \
	-LibreOfficeRoot "C:\\tools\\LibreOfficePortable"

# Or use the batch helper
installers\create_payload.bat "out\\install\\x64-release" "out\\install\\zconverter-payload.zip" "C:\\tools\\LibreOfficePortable"
```

The created payload ZIP is suitable for distribution or further packaging steps (e.g., building an NSIS installer from the already-populated staging folder). If you'd like, I can also add an automated CI job (Windows runner) that builds the project and produces the payload zip for you.
