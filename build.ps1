param($Config = "Release")

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path

# Kill any running instance so the linker can overwrite
Stop-Process -Name "RoN-ESP" -Force -ErrorAction SilentlyContinue

# Configure (only needed once, or when CMakeLists.txt changes)
if (-not (Test-Path "$Root\build\CMakeCache.txt")) {
    cmake -B "$Root\build" -S $Root
}

# Build
cmake --build "$Root\build" --config $Config

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[+] Built: $Root\build\$Config\RoN-ESP.exe" -ForegroundColor Green
}
