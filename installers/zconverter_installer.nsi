; zconverter NSIS installer
; Usage:
; 1) Stage installed files into a folder (default: out/install/x64-release)
; 2) Run makensis with optional STAGEDIR override:
;    makensis -DSTAGEDIR="out\\install\\x64-release" zconverter_installer.nsi

!include "MUI2.nsh"

!ifndef STAGEDIR
!define STAGEDIR "out\\install\\x64-release"
!endif

Name "zconverter"
OutFile "zconverter-setup.exe"
InstallDir "$PROGRAMFILES\\zconverter"
RequestExecutionLevel admin

!define PRODUCT_NAME "zconverter"
!define PRODUCT_VERSION "1.0.0"

!insertmacro MUI_LANGUAGE "English"

Section "Install"
    SetOutPath "$INSTDIR"

    ; Copy all staged files into the installation folder
    ; The ${STAGEDIR} value is expanded at compile-time by makensis
    File /r "${STAGEDIR}\\*.*"

    ; Register the COM shell extension (if present)
    IfFileExists "$INSTDIR\\bin\\zconverter_shell.dll" 0 +2
        ExecWait '"$SYSDIR\\regsvr32.exe" /s "$INSTDIR\\bin\\zconverter_shell.dll"'

    ; Create Start Menu shortcut
    CreateDirectory "$SMPROGRAMS\\${PRODUCT_NAME}"
    CreateShortCut "$SMPROGRAMS\\${PRODUCT_NAME}\\zconverter.lnk" "$INSTDIR\\bin\\zconverter.exe"

    ; Optionally add to PATH? disabled by default to avoid system modifications
    ; WriteRegStr HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path" "$INSTDIR\\bin;${PATH}"

SectionEnd

Section "Uninstall"
    ; Unregister shell extension if present
    IfFileExists "$INSTDIR\\bin\\zconverter_shell.dll" 0 +2
        ExecWait '"$SYSDIR\\regsvr32.exe" /s /u "$INSTDIR\\bin\\zconverter_shell.dll"'

    ; Remove files
    RMDir /r "$INSTDIR"

    ; Remove Start Menu
    Delete "$SMPROGRAMS\\${PRODUCT_NAME}\\zconverter.lnk"
    RMDir "$SMPROGRAMS\\${PRODUCT_NAME}"

SectionEnd
