param(
    [string]$VcpkgRoot = "D:\Code\GitHubCode\vcpkg",
    [string]$OsgEarthDir = "",
    [string]$RuntimeRoot = ""
)

$ErrorActionPreference = "Stop"

function Write-EnvLine {
    param(
        [string]$Name,
        [string]$Value
    )

    "{0,-24} {1}" -f $Name, $Value
}

function Show-PathStatus {
    param(
        [string]$Label,
        [string]$PathValue
    )

    if ([string]::IsNullOrWhiteSpace($PathValue)) {
        Write-Host ("{0,-24} {1}" -f $Label, "<unset>") -ForegroundColor Yellow
        return
    }

    if (Test-Path -LiteralPath $PathValue) {
        Write-Host ("{0,-24} {1}" -f $Label, $PathValue) -ForegroundColor Green
    } else {
        Write-Host ("{0,-24} {1}" -f $Label, $PathValue) -ForegroundColor Yellow
    }
}

$env:VCPKG_ROOT = $VcpkgRoot

if ([string]::IsNullOrWhiteSpace($RuntimeRoot)) {
    $RuntimeRoot = Join-Path $VcpkgRoot "installed\x64-windows"
}
$env:THREEDVIEWER_RUNTIME_ROOT = $RuntimeRoot

if (-not [string]::IsNullOrWhiteSpace($OsgEarthDir)) {
    $env:OSGEARTH_DIR = $OsgEarthDir
} elseif ([string]::IsNullOrWhiteSpace($env:OSGEARTH_DIR)) {
    Remove-Item Env:OSGEARTH_DIR -ErrorAction SilentlyContinue
}

Write-Host "3DViewer session environment initialized" -ForegroundColor Green
Write-Host ""
Show-PathStatus "VCPKG_ROOT" $env:VCPKG_ROOT
Show-PathStatus "OSGEARTH_DIR" $env:OSGEARTH_DIR
Show-PathStatus "THREEDVIEWER_RUNTIME_ROOT" $env:THREEDVIEWER_RUNTIME_ROOT
Write-Host ""
if ([string]::IsNullOrWhiteSpace($env:OSGEARTH_DIR)) {
    Write-Host "OSGEARTH_DIR is not set. This machine still needs a real osgEarth SDK path." -ForegroundColor Yellow
    Write-Host "Example:" -ForegroundColor Yellow
    Write-Host "  powershell -ExecutionPolicy Bypass -File scripts/bootstrap-dev-env.ps1 -OsgEarthDir 'D:\SDK\osgEarth\lib\cmake\osgEarth'"
    Write-Host ""
}
Write-Host "These values only apply to the current PowerShell session." -ForegroundColor Yellow
Write-Host "Next step:"
Write-Host "  powershell -ExecutionPolicy Bypass -File scripts/build-dev.ps1 -Preset debug"
