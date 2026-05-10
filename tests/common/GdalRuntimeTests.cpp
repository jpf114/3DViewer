#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

#include <cpl_conv.h>

#include "common/GdalRuntime.h"

namespace {

struct ConfigGuard {
    std::string gdalData;
    std::string projData;
    std::string projLib;
    bool hadGdalData = false;
    bool hadProjData = false;
    bool hadProjLib = false;

    ConfigGuard() {
        if (const char *value = CPLGetConfigOption("GDAL_DATA", nullptr)) {
            gdalData = value;
            hadGdalData = true;
        }
        if (const char *value = CPLGetConfigOption("PROJ_DATA", nullptr)) {
            projData = value;
            hadProjData = true;
        }
        if (const char *value = CPLGetConfigOption("PROJ_LIB", nullptr)) {
            projLib = value;
            hadProjLib = true;
        }
    }

    ~ConfigGuard() {
        CPLSetConfigOption("GDAL_DATA", hadGdalData ? gdalData.c_str() : nullptr);
        CPLSetConfigOption("PROJ_DATA", hadProjData ? projData.c_str() : nullptr);
        CPLSetConfigOption("PROJ_LIB", hadProjLib ? projLib.c_str() : nullptr);
    }
};

bool require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << message << "\n";
        return false;
    }
    return true;
}

} // namespace

int main() {
    ConfigGuard guard;

    const std::filesystem::path tempRoot =
        std::filesystem::temp_directory_path() / "threeviewer-gdal-runtime-test";
    const std::filesystem::path shareRoot = tempRoot / "share";
    const std::filesystem::path gdalDir = shareRoot / "gdal";
    const std::filesystem::path projDir = shareRoot / "proj";

    std::filesystem::create_directories(gdalDir);
    std::filesystem::create_directories(projDir);

    CPLSetConfigOption("GDAL_DATA", nullptr);
    CPLSetConfigOption("PROJ_DATA", nullptr);
    CPLSetConfigOption("PROJ_LIB", nullptr);

    gdalruntime::configureDataPathsFromRoots({shareRoot});

    const char *gdalData = CPLGetConfigOption("GDAL_DATA", nullptr);
    const char *projData = CPLGetConfigOption("PROJ_DATA", nullptr);
    const char *projLib = CPLGetConfigOption("PROJ_LIB", nullptr);

    const std::string expectedGdal = gdalDir.string();
    const std::string expectedProj = projDir.string();

    if (!require(gdalData != nullptr && expectedGdal == gdalData,
                 "Expected GDAL_DATA to point at share/gdal.")) {
        return EXIT_FAILURE;
    }

    if (!require(projData != nullptr && expectedProj == projData,
                 "Expected PROJ_DATA to point at share/proj.")) {
        return EXIT_FAILURE;
    }

    if (!require(projLib != nullptr && expectedProj == projLib,
                 "Expected PROJ_LIB to point at share/proj.")) {
        return EXIT_FAILURE;
    }

    std::filesystem::remove_all(tempRoot);
    return EXIT_SUCCESS;
}
