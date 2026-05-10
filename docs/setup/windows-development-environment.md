# 3DViewer Windows Development Environment

This guide describes the recommended Windows setup for building and running
`3DViewer` on a fresh machine.

## 1. What this project depends on

The project expects these pieces to exist on the machine:

- Visual Studio 2022 with C++ desktop tools
- CMake 3.25+
- Qt 6
- OpenSceneGraph
- GDAL
- PROJ
- GEOS
- spdlog
- osgEarth

The current CMake setup can discover most third-party libraries from a shared
`vcpkg` installation, but `osgEarth` is the fragile part. On this machine,
`cmake --preset debug` succeeded far enough to find OSG, GDAL, PROJ, GEOS, and
spdlog from `VCPKG_ROOT`, then failed because `osgEarthConfig.cmake` was not
available in the active prefixes.

## 2. Recommended directory layout

Use one stable dependency root for the team.

### Option A: shared vcpkg root plus separate osgEarth SDK

- `VCPKG_ROOT=D:\Code\GitHubCode\vcpkg`
- `VCPKG_ROOT\installed\x64-windows\...` for Qt, OSG, GDAL, PROJ, GEOS, spdlog
- `D:\SDK\osgEarth\lib\cmake\osgEarth\osgEarthConfig.cmake` for osgEarth

In that setup:

- keep `VCPKG_ROOT` set in the user environment
- set `OSGEARTH_DIR=D:\SDK\osgEarth\lib\cmake\osgEarth`

### Option B: one custom prefix that also contains osgEarth

If your team already maintains a unified SDK prefix, point both:

- `VCPKG_ROOT` at the toolchain root
- `OSGEARTH_DIR` at the folder containing `osgEarthConfig.cmake`

Do not assume `osgEarth` is present just because OSG is present.

## 3. Required environment variables

The minimum recommended variables are:

```powershell
$env:VCPKG_ROOT = "D:\Code\GitHubCode\vcpkg"
$env:OSGEARTH_DIR = "D:\SDK\osgEarth\lib\cmake\osgEarth"
```

For runtime validation on a clean machine, these are often also useful:

```powershell
$env:THREEDVIEWER_RUNTIME_ROOT = "D:\Code\GitHubCode\vcpkg\installed\x64-windows"
```

If you want a quick session-only bootstrap, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/bootstrap-dev-env.ps1
```

If your osgEarth SDK lives in a custom location, pass it explicitly:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/bootstrap-dev-env.ps1 `
  -OsgEarthDir "D:\SDK\osgEarth\lib\cmake\osgEarth"
```

`THREEDVIEWER_RUNTIME_ROOT` helps the post-build and install steps copy:

- `share/gdal`
- `share/proj`
- `osgPlugins-*`

## 4. Configure and build

From the repository root:

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

Or use the helper script:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/configure-dev.ps1
```

It first runs the environment check, then prints the exact `cmake` command it
recommends for the current machine. Add `-RunConfigure` if you want the script
to execute that configure step directly.

For the normal daily workflow, use:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-dev.ps1 -Preset debug
```

That script:

1. runs the environment check
2. executes configure
3. executes build
4. optionally runs `ctest` when `-RunTests` is provided

For release:

```powershell
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
```

## 5. What to verify when configure fails

### If CMake cannot find `osgEarth`

Check:

1. `OSGEARTH_DIR` points to the directory that directly contains
   `osgEarthConfig.cmake`
2. osgEarth was built for the same compiler family and architecture
3. osgEarth links against the same OSG family as the one found by CMake

Typical symptom:

- OSG packages are found from `vcpkg`
- configure stops at `find_package(osgEarth ...)`

That is the exact failure observed in this workspace on May 10, 2026.

### If runtime starts but data access fails

Check:

1. `share/gdal` is present near the executable or reachable through
   `GDAL_DATA`
2. `share/proj` is present near the executable or reachable through
   `PROJ_DATA` or `PROJ_LIB`
3. `osgPlugins-*` is deployed next to the executable or discoverable through
   the OSG plugin search path

## 6. Recommended team workflow

For a small team, I recommend:

1. Freeze one supported toolchain tuple:
   `VS2022 + x64 + one Qt version + one OSG/osgEarth set`
2. Publish one dependency bootstrap note with exact install paths
3. Validate on one clean machine after any dependency upgrade
4. Treat dependency upgrades as explicit chores, not incidental drift

The concrete folder convention is documented in:

- [Dependency layout](./deps-layout.md)

## 7. Current project risks

From today's inspection, these are the main environment risks:

- The project depends on a machine-global SDK layout
- `osgEarth` discovery is not implicit in the current `vcpkg` prefix
- runtime data path setup is duplicated between GDAL-related modules
- the repository currently lacks one obvious "new machine bootstrap" document

## 8. Suggested next hardening steps

1. Add a top-level `README.md` with a short bootstrap section that links here
2. Add a CI or local validation script that checks:
   - `VCPKG_ROOT`
   - `OSGEARTH_DIR`
   - presence of `osgEarthConfig.cmake`
   - presence of `share/gdal` and `share/proj`
3. Consider centralizing GDAL/PROJ runtime path initialization into one shared
   utility instead of duplicating the logic across modules
