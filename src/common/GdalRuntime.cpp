#include "common/GdalRuntime.h"

#include <cstdlib>
#include <mutex>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#include <cpl_conv.h>
#include <gdal_priv.h>

namespace gdalruntime {
namespace {

void appendModuleShareRoot(std::vector<std::filesystem::path> &shareRoots, const wchar_t *moduleName) {
#if defined(_WIN32)
    if (HMODULE module = GetModuleHandleW(moduleName)) {
        wchar_t modulePath[MAX_PATH];
        const DWORD length = GetModuleFileNameW(module, modulePath, MAX_PATH);
        if (length > 0 && length < MAX_PATH) {
            shareRoots.push_back(std::filesystem::path(modulePath).parent_path().parent_path() / "share");
        }
    }
#else
    (void)shareRoots;
    (void)moduleName;
#endif
}

} // namespace

void configureDataPathsFromRoots(const std::vector<std::filesystem::path> &shareRoots) {
#if defined(_WIN32)
    if (CPLGetConfigOption("GDAL_DATA", nullptr) != nullptr &&
        (CPLGetConfigOption("PROJ_DATA", nullptr) != nullptr ||
         CPLGetConfigOption("PROJ_LIB", nullptr) != nullptr)) {
        return;
    }

    for (const auto &shareDir : shareRoots) {
        const std::filesystem::path gdalDir = shareDir / "gdal";
        const std::filesystem::path projDir = shareDir / "proj";

        if (CPLGetConfigOption("GDAL_DATA", nullptr) == nullptr && std::filesystem::exists(gdalDir)) {
            const std::string path = gdalDir.string();
            CPLSetConfigOption("GDAL_DATA", path.c_str());
        }

        if (std::filesystem::exists(projDir)) {
            const std::string path = projDir.string();
            if (CPLGetConfigOption("PROJ_DATA", nullptr) == nullptr) {
                CPLSetConfigOption("PROJ_DATA", path.c_str());
            }
            if (CPLGetConfigOption("PROJ_LIB", nullptr) == nullptr) {
                CPLSetConfigOption("PROJ_LIB", path.c_str());
            }
        }
    }
#else
    (void)shareRoots;
#endif
}

void configureDefaultDataPaths() {
#if defined(_WIN32)
    std::vector<std::filesystem::path> shareRoots;

    wchar_t modulePath[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        shareRoots.push_back(std::filesystem::path(modulePath).parent_path() / "share");
    }

    appendModuleShareRoot(shareRoots, L"gdal.dll");
    appendModuleShareRoot(shareRoots, L"gdald.dll");
    appendModuleShareRoot(shareRoots, L"proj_9.dll");
    appendModuleShareRoot(shareRoots, L"proj_9_d.dll");

    if (const char *vcpkgRoot = std::getenv("VCPKG_ROOT")) {
        shareRoots.emplace_back(std::filesystem::path(vcpkgRoot) / "installed" / "x64-windows" / "share");
    }

    configureDataPathsFromRoots(shareRoots);
#endif
}

void ensureGdalRegistered() {
    static std::once_flag once;
    std::call_once(once, []() {
        configureDefaultDataPaths();
        GDALAllRegister();
    });
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");
    CPLSetConfigOption("SHAPE_ENCODING", "UTF-8");
}

} // namespace gdalruntime
