param(
    [ValidateSet("debug", "release")]
    [string]$Preset = "debug",
    [switch]$RunTests
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$configureScript = Join-Path $PSScriptRoot "configure-dev.ps1"

if (-not (Test-Path -LiteralPath $configureScript)) {
    throw "Missing configure script: $configureScript"
}

Write-Host "Running configure helper..." -ForegroundColor Cyan
& powershell -ExecutionPolicy Bypass -File $configureScript -Preset $Preset -RunConfigure
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "Aborting build because configure failed." -ForegroundColor Yellow
    exit $LASTEXITCODE
}

Push-Location $projectRoot
try {
    Write-Host ""
    Write-Host "Running build preset '$Preset'..." -ForegroundColor Cyan
    & cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    if ($RunTests) {
        Write-Host ""
        Write-Host "Running test preset '$Preset'..." -ForegroundColor Cyan
        & ctest --preset $Preset --output-on-failure
        exit $LASTEXITCODE
    }

    exit 0
} finally {
    Pop-Location
}
