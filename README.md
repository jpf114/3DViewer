# 3DViewer

Windows desktop 3D GIS viewer built with Qt, OpenSceneGraph, osgEarth, and
GDAL.

## Development setup

For a fresh Windows machine, start here:

- [Windows development environment](docs/setup/windows-development-environment.md)
- [Runtime deployment notes](docs/notes/runtime-deployment.md)

## Quick checks

Before running CMake, confirm:

1. `VCPKG_ROOT` is set
2. `OSGEARTH_DIR` points to the directory containing `osgEarthConfig.cmake`
3. the machine has a compatible Visual Studio 2022 C++ toolchain

You can also run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/check-windows-env.ps1
```

To generate the recommended configure command automatically:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/configure-dev.ps1
```

To run CMake directly after the checks pass:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/configure-dev.ps1 -Preset debug -RunConfigure
```

To configure and build in one step:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-dev.ps1 -Preset debug
```

To configure, build, and run tests:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-dev.ps1 -Preset debug -RunTests
```

To initialize a suggested set of environment variables for the current session:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/bootstrap-dev-env.ps1
```

If `osgEarth` is installed outside the default layout, pass it explicitly:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/bootstrap-dev-env.ps1 -OsgEarthDir "D:\SDK\osgEarth\lib\cmake\osgEarth"
```
