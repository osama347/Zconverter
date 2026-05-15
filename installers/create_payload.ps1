<#
create_payload.ps1
Creates a prestaged payload zip for zconverter on a Windows build machine.

Usage (from project root):
PowerShell -ExecutionPolicy Bypass -File installers\create_payload.ps1 \
  -StagedDir "out\\install\\x64-release" -OutZip "out\\install\\zconverter-payload.zip" -LibreOfficeRoot "C:\\tools\\LibreOfficePortable"

Notes:
- Requires `windeployqt` available in PATH or discoverable via Qt install.
- Must be run on Windows where binaries were built (MSVC) so windeployqt and exe are compatible.
#>
[CmdletBinding()]
param(
    [string]$StagedDir = "out\\install\\x64-release",
    [string]$OutZip = "out\\install\\zconverter-payload.zip",
    [string]$LibreOfficeRoot = ""
)

Write-Host "Staging dir: $StagedDir"
Write-Host "Output zip:  $OutZip"

$binDir = Join-Path $StagedDir "bin"
if (-not (Test-Path $binDir)) {
    Write-Error "Staged bin directory not found: $binDir"
    exit 2
}

$exe = Join-Path $binDir "zconverter.exe"
if (-not (Test-Path $exe)) {
    Write-Error "zconverter.exe not found in staged bin: $exe"
    exit 3
}

# Find windeployqt
$windeployqt = Get-Command windeployqt -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty Source
if (-not $windeployqt) {
    Write-Error "windeployqt not found in PATH. Ensure Qt (MSVC) is installed and windeployqt is on PATH."
    exit 4
}
Write-Host "Using windeployqt: $windeployqt"

# Run windeployqt to stage Qt runtime into the bin folder
Write-Host "Running windeployqt to collect Qt runtime into $binDir"
$deployArgs = @("--compiler-runtime","--no-translations","--dir",$binDir,$exe)
$proc = Start-Process -FilePath $windeployqt -ArgumentList $deployArgs -Wait -PassThru
if ($proc.ExitCode -ne 0) {
    Write-Error "windeployqt failed with exit code $($proc.ExitCode)"
    exit $proc.ExitCode
}

# Optionally copy portable LibreOffice
if ($LibreOfficeRoot -ne "" -and (Test-Path (Join-Path $LibreOfficeRoot "program\\soffice.exe"))) {
    Write-Host "Bundling LibreOffice from: $LibreOfficeRoot"
    $dest = Join-Path $binDir "libreoffice"
    if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
    Copy-Item -Path (Join-Path $LibreOfficeRoot "*") -Destination $dest -Recurse -Force
} elseif ($LibreOfficeRoot -ne "") {
    Write-Warning "LibreOffice root specified but soffice.exe not found under it. Skipping LibreOffice bundle."
}

# Ensure output directory exists
$zipDir = Split-Path $OutZip -Parent
if (-not (Test-Path $zipDir)) { New-Item -ItemType Directory -Path $zipDir | Out-Null }

# Create the zip
if (Test-Path $OutZip) { Remove-Item $OutZip -Force }
Write-Host "Creating payload zip: $OutZip"
Compress-Archive -Path (Join-Path $StagedDir "*") -DestinationPath $OutZip -Force

Write-Host "Payload created: $OutZip"
exit 0
