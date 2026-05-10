#pragma once

#include <filesystem>
#include <vector>

namespace gdalruntime {

void configureDataPathsFromRoots(const std::vector<std::filesystem::path> &shareRoots);
void configureDefaultDataPaths();
void ensureGdalRegistered();

} // namespace gdalruntime
