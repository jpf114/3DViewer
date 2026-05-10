$ErrorActionPreference = "Stop"

function Write-CheckResult {
    param(
        [string]$Label,
        [string]$Status,
        [string]$Detail
    )

    "{0,-18} {1,-6} {2}" -f $Label, $Status, $Detail
}

function Test-PathOrEmpty {
    param([string]$PathValue)
    if ([string]::IsNullOrWhiteSpace($PathValue)) {
        return $false
    }
    return (Test-Path -LiteralPath $PathValue)
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$vcpkgRoot = $env:VCPKG_ROOT
$osgEarthDir = $env:OSGEARTH_DIR
$runtimeRoot = $env:THREEDVIEWER_RUNTIME_ROOT
$vcpkgOsgEarthConfigDir = $null

Write-Host "3DViewer Windows environment check"
Write-Host "Project root: $projectRoot"
Write-Host ""

$failures = 0

if (Test-PathOrEmpty $vcpkgRoot) {
    Write-CheckResult "VCPKG_ROOT" "OK" $vcpkgRoot
    $candidate = Join-Path $vcpkgRoot "installed\x64-windows\share\osgearth"
    if (Test-Path -LiteralPath (Join-Path $candidate "osgearth-config.cmake")) {
        $vcpkgOsgEarthConfigDir = $candidate
    }
} else {
    Write-CheckResult "VCPKG_ROOT" "MISS" "Set VCPKG_ROOT to your vcpkg root."
    $failures++
}

if (Test-PathOrEmpty $osgEarthDir) {
    $camelConfigPath = Join-Path $osgEarthDir "osgEarthConfig.cmake"
    $lowerConfigPath = Join-Path $osgEarthDir "osgearth-config.cmake"
    if ((Test-Path -LiteralPath $camelConfigPath) -or (Test-Path -LiteralPath $lowerConfigPath)) {
        Write-CheckResult "OSGEARTH_DIR" "OK" $osgEarthDir
    } else {
        Write-CheckResult "OSGEARTH_DIR" "MISS" "Directory exists but no osgEarth/osgearth config file was found."
        $failures++
    }
} elseif ($vcpkgOsgEarthConfigDir) {
    Write-CheckResult "OSGEARTH_DIR" "OK" "using vcpkg package at $vcpkgOsgEarthConfigDir"
} else {
    Write-CheckResult "OSGEARTH_DIR" "MISS" "Set OSGEARTH_DIR or install osgearth:x64-windows into vcpkg."
    $failures++
}

if (Test-PathOrEmpty $runtimeRoot) {
    Write-CheckResult "RUNTIME_ROOT" "OK" $runtimeRoot
} else {
    Write-CheckResult "RUNTIME_ROOT" "WARN" "Optional. Set THREEDVIEWER_RUNTIME_ROOT for post-build runtime copying."
}

if (Test-PathOrEmpty $runtimeRoot) {
    $gdalShare = Join-Path $runtimeRoot "share/gdal"
    $projShare = Join-Path $runtimeRoot "share/proj"
    $pluginsRoot = Join-Path $runtimeRoot "plugins"

    if (Test-Path -LiteralPath $gdalShare) {
        Write-CheckResult "share/gdal" "OK" $gdalShare
    } else {
        Write-CheckResult "share/gdal" "WARN" "Missing under THREEDVIEWER_RUNTIME_ROOT."
    }

    if (Test-Path -LiteralPath $projShare) {
        Write-CheckResult "share/proj" "OK" $projShare
    } else {
        Write-CheckResult "share/proj" "WARN" "Missing under THREEDVIEWER_RUNTIME_ROOT."
    }

    if (Test-Path -LiteralPath $pluginsRoot) {
        $pluginDir = Get-ChildItem -LiteralPath $pluginsRoot -Directory -Filter "osgPlugins-*" -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($pluginDir) {
            Write-CheckResult "osgPlugins" "OK" $pluginDir.FullName
        } else {
            Write-CheckResult "osgPlugins" "WARN" "No osgPlugins-* directory found under $pluginsRoot"
        }
    } else {
        Write-CheckResult "osgPlugins" "WARN" "Plugin root not found: $pluginsRoot"
    }
}

$vsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path -LiteralPath $vsWhere) {
    $vsInstall = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($vsInstall)) {
        Write-CheckResult "Visual Studio" "OK" $vsInstall
    } else {
        Write-CheckResult "Visual Studio" "MISS" "Compatible VC toolchain was not detected by vswhere."
        $failures++
    }
} else {
    Write-CheckResult "Visual Studio" "WARN" "vswhere.exe not found; skipped toolchain detection."
}

Write-Host ""
if ($failures -gt 0) {
    Write-Host "Result: environment is incomplete." -ForegroundColor Yellow
    Write-Host "See docs/setup/windows-development-environment.md"
    exit 1
}

Write-Host "Result: core build prerequisites look present." -ForegroundColor Green
