@echo off
REM create_payload.bat
REM Usage: create_payload.bat out\install\x64-release out\install\zconverter-payload.zip C:\tools\LibreOfficePortable

SET STAGEDIR=%1
SET OUTZIP=%2
SET LIBROOT=%3

IF "%STAGEDIR%"=="" (
  ECHO Usage: create_payload.bat <stagedir> <outzip> [libreoffice_root]
  EXIT /B 2
)

IF "%OUTZIP%"=="" (
  SET OUTZIP=%STAGEDIR%\zconverter-payload.zip
)

IF NOT EXIST "%STAGEDIR%" (
  ECHO Staged dir not found: %STAGEDIR%
  EXIT /B 3
)

SET BIN=%STAGEDIR%\bin
IF NOT EXIST "%BIN%\zconverter.exe" (
  ECHO zconverter.exe not found in %BIN%
  EXIT /B 4
)

REM locate windeployqt
where windeployqt >nul 2>nul
IF ERRORLEVEL 1 (
  ECHO windeployqt not found in PATH. Ensure Qt (MSVC) is installed and windeployqt is on PATH.
  EXIT /B 5
)

ECHO Running windeployqt to collect Qt runtime into %BIN%
windeployqt --compiler-runtime --no-translations --dir "%BIN%" "%BIN%\zconverter.exe"
IF ERRORLEVEL 1 (
  ECHO windeployqt failed.
  EXIT /B 6
)

IF NOT "%LIBROOT%"=="" (
  IF EXIST "%LIBROOT%\program\soffice.exe" (
    ECHO Bundling LibreOffice from %LIBROOT%
    xcopy "%LIBROOT%" "%BIN%\\libreoffice" /E /I /Y
  ) ELSE (
    ECHO LibreOffice root specified but soffice.exe not found. Skipping.
  )
)

IF EXIST "%OUTZIP%" del /F "%OUTZIP%"
powershell -Command "Compress-Archive -Path '%STAGEDIR%\*' -DestinationPath '%OUTZIP%' -Force"
ECHO Created payload: %OUTZIP%
EXIT /B 0
