# 3DViewer 运行时部署说明

## 推荐：单 `build` 目录下的 Debug / Release（Visual Studio 多配置）

在仓库根目录执行（按本机修改工具链与 `CMAKE_PREFIX_PATH` / `CMAKE_TOOLCHAIN_FILE`）：

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
cmake --build build --config Release
```

生成物路径为：

- `build/Debug/3DViewer.exe`
- `build/Release/3DViewer.exe`

构建后若找到 `windeployqt`，会对上述目录各执行一次部署（与 `THREEDVIEWER_RUNTIME_POSTBUILD` 一致）。

## 安装到仓库根下的 `install/`（最小可运行集）

**建议** Debug / Release 装到不同子目录，避免互相覆盖：

```bat
cmake --install build --config Debug   --prefix install\Debug
cmake --install build --config Release --prefix install\Release
```

安装阶段会：

1. 安装 `3DViewer.exe` 到前缀根目录；  
2. 若找到 `windeployqt`，在**该前缀目录**下再运行一次，把 **Qt** 运行库、`platforms` 等拷到与 exe 同级（与 POST_BUILD 行为一致，便于「只拷 install 目录」分发）；  
3. 若配置了 `THREEDVIEWER_RUNTIME_ROOT` 且存在对应目录，再安装 `share/gdal`、`share/proj` 与 `osgPlugins-*`；  
4. 尝试安装 `platforms\qwindows.dll`（`OPTIONAL`，作补充）。

**仍可能不在安装树里的依赖**：osgEarth、GDAL、PROJ、GEOS、spdlog 等 **非 Qt** DLL 通常仍在 vcpkg / 本机 `bin`；「最小」分发需把所需 DLL 一并拷入前缀目录，或保证目标机 PATH 指向这些库。设置 `THREEDVIEWER_RUNTIME_ROOT` 可至少带上 **GDAL/PROJ 数据** 与 **OSG 插件**。

## 构建输出布局

- 可执行文件：`build/<Config>/3DViewer.exe`（或生成器对应目录）
- 使用 `THREEDVIEWER_RUNTIME_ROOT` 时，POST_BUILD 可能生成：
  - `share/gdal`：GDAL 数据文件（`GDAL_DATA` 可指向此目录）
  - `share/proj`：PROJ 数据（`PROJ_DATA` 或 `PROJ_LIB` 可指向此目录）
  - `osgPlugins-*`：与 vcpkg/OSG 布局一致的 OSG 插件目录，与 exe 同级的目录名需被 `OSG_LIBRARY_PATH` 或 OSG 默认搜索路径找到

## CMake 选项

| 变量 | 含义 |
|------|------|
| `THREEDVIEWER_RUNTIME_POSTBUILD` | 是否启用构建后复制与 `windeployqt`（默认 ON） |
| `THREEDVIEWER_RUNTIME_ROOT` | 类似 vcpkg `installed/<triplet>` 的前缀，含 `bin`、`share/gdal`、`share/proj`、osg 插件 |
| `THREEDVIEWER_OSG_PLUGINS_DIR` | 显式指定含 `osgPlugins-*` 的目录；留空时从 `THREEDVIEWER_RUNTIME_ROOT` 下自动猜测 |

## windeployqt

若能在 `Qt6_DIR` 推得的 `bin` 目录中找到 `windeployqt` / `windeployqt6`，POST_BUILD 会运行其将 Qt 运行库与 `platforms` 等依赖拷到输出目录。若未找到，则回退为仅复制 `qwindows.dll` 到 `platforms/`（与旧行为一致）。

## 安装树

`cmake --install` 在设置 `THREEDVIEWER_RUNTIME_ROOT` 且相应目录存在时，会安装 `share/gdal`、`share/proj` 与 osg 插件目录。Release/Debug 的 `qwindows.dll` 通过 `install(FILES ... OPTIONAL)` 在文件存在时安装。

## 干净机验证

1. 在仅含 `install/` 或构建输出目录的环境中设置（若需要）：

   - `PATH`：含 exe 与所有第三方 DLL（`windeployqt` 可处理 Qt；osg/GDAL/PROJ 等常需从 vcpkg `bin` 复制或随包携带）
   - `GDAL_DATA`：指向 `share/gdal` 的绝对路径
   - `PROJ_DATA` 或 `PROJ_LIB`：指向 `share/proj`（含 `proj.db`）
   - OSG：确保 `osgPlugins-*` 可被加载（与 exe 相邻或通过 `OSG_LIBRARY_PATH`）

2. 双击 `3DViewer.exe`，确认无缺少 DLL 弹窗且地球与导入流程可用。
