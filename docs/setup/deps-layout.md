# 3DViewer Dependency Layout

This document defines the recommended Windows dependency layout for local
development and team onboarding.

## Goal

Make every developer machine look similar enough that:

- CMake finds the same packages
- runtime deployment copies from predictable locations
- troubleshooting starts from one shared assumption set

## Recommended layout

### Shared package manager root

```text
D:\
  Code\
    GitHubCode\
      vcpkg\
        installed\
          x64-windows\
            bin\
            debug\
            include\
            lib\
            plugins\
            share\
              gdal\
              proj\
```

Recommended environment variable:

```powershell
$env:VCPKG_ROOT = "D:\Code\GitHubCode\vcpkg"
```

### Separate osgEarth SDK

```text
D:\
  SDK\
    osgEarth\
      bin\
      include\
      lib\
        cmake\
          osgEarth\
            osgEarthConfig.cmake
```

Recommended environment variable:

```powershell
$env:OSGEARTH_DIR = "D:\SDK\osgEarth\lib\cmake\osgEarth"
```

### Runtime-copy root

Use the `vcpkg` installed prefix directly unless your team has a custom merged
runtime tree:

```powershell
$env:THREEDVIEWER_RUNTIME_ROOT = "D:\Code\GitHubCode\vcpkg\installed\x64-windows"
```

## Why this split is recommended

`vcpkg` already gives the project most of what it needs:

- Qt
- OpenSceneGraph
- GDAL
- PROJ
- GEOS
- spdlog

`osgEarth` is the one dependency that commonly lives outside that default
prefix, so the cleanest mental model is:

- `VCPKG_ROOT` for the broad dependency set
- `OSGEARTH_DIR` for the explicit osgEarth config path

## What must match across dependencies

Keep these aligned:

1. compiler family: Visual Studio 2022 / MSVC
2. architecture: x64
3. debug vs release runtime expectations
4. OSG and osgEarth ABI compatibility

If OSG comes from one toolchain and osgEarth from another, configure may pass
but link or runtime behavior can still be unstable.

## Team convention

If you want fewer support questions, freeze these conventions:

1. one supported `vcpkg` root path
2. one supported osgEarth SDK path
3. one supported Visual Studio major version
4. one active architecture

## Related files

- [Windows development environment](./windows-development-environment.md)
- [Runtime deployment notes](../notes/runtime-deployment.md)
