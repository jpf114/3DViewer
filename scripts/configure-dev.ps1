param(
    [ValidateSet("debug", "release")]
    [string]$Preset = "debug",
    [switch]$RunConfigure
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$envCheckScript = Join-Path $PSScriptRoot "check-windows-env.ps1"
$resolvedOsgEarthDir = $env:OSGEARTH_DIR

if (-not (Test-Path -LiteralPath $envCheckScript)) {
    throw "Missing environment check script: $envCheckScript"
}

Write-Host "Running environment check..." -ForegroundColor Cyan
& powershell -ExecutionPolicy Bypass -File $envCheckScript
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "Aborting configure because the environment check failed." -ForegroundColor Yellow
    exit $LASTEXITCODE
}

if ([string]::IsNullOrWhiteSpace($resolvedOsgEarthDir) -and -not [string]::IsNullOrWhiteSpace($env:VCPKG_ROOT)) {
    $vcpkgCandidate = Join-Path $env:VCPKG_ROOT "installed\x64-windows\share\osgearth"
    if (Test-Path -LiteralPath (Join-Path $vcpkgCandidate "osgearth-config.cmake")) {
        $resolvedOsgEarthDir = $vcpkgCandidate
    }
}

$cmakeArgs = @("--preset", $Preset)

if (-not [string]::IsNullOrWhiteSpace($resolvedOsgEarthDir)) {
    $cmakeArgs += "-DTHREEDVIEWER_OSGEARTH_DIR=$resolvedOsgEarthDir"
}

if (-not [string]::IsNullOrWhiteSpace($env:THREEDVIEWER_RUNTIME_ROOT)) {
    $cmakeArgs += "-DTHREEDVIEWER_RUNTIME_ROOT=$env:THREEDVIEWER_RUNTIME_ROOT"
}

Write-Host ""
Write-Host "Suggested configure command:" -ForegroundColor Cyan
Write-Host ("cmake " + ($cmakeArgs -join " "))

if (-not $RunConfigure) {
    Write-Host ""
    Write-Host "Dry run only. Re-run with -RunConfigure to execute CMake." -ForegroundColor Yellow
    exit 0
}

Write-Host ""
Write-Host "Running CMake configure..." -ForegroundColor Cyan
Push-Location $projectRoot
try {
    & cmake @cmakeArgs
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
