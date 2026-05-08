#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "data/GdalDatasetInspector.h"

namespace {

std::filesystem::path writeSampleGeoJson() {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "中文样例.geojson";
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << R"({
  "type": "FeatureCollection",
  "name": "sample-layer",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "name": "测试面",
        "code": "sample-1"
      },
      "geometry": {
        "type": "Polygon",
        "coordinates": [[
          [116.0, 39.6],
          [116.8, 39.6],
          [116.8, 40.2],
          [116.0, 40.2],
          [116.0, 39.6]
        ]]
      }
    }
  ]
})";
    return path;
}

} // namespace

int main() {
    GdalDatasetInspector inspector;

    const auto missingDescriptor = inspector.inspect("definitely-missing-dataset.xyz");
    if (missingDescriptor.has_value()) {
        std::cerr << "Expected missing file inspection to fail.\n";
        return EXIT_FAILURE;
    }

    const std::filesystem::path datasetPath = writeSampleGeoJson();
    const auto u8Path = datasetPath.u8string();
    const std::string path(u8Path.begin(), u8Path.end());
    const auto descriptor = inspector.inspect(path);

    if (!descriptor.has_value()) {
        std::cerr << "Expected sample GeoJSON inspection to succeed.\n";
        return EXIT_FAILURE;
    }

    if (descriptor->kind != DataSourceKind::Vector) {
        std::cerr << "Expected sample GeoJSON to be classified as vector.\n";
        return EXIT_FAILURE;
    }

    if (descriptor->name.empty()) {
        std::cerr << "Expected non-empty descriptor name.\n";
        return EXIT_FAILURE;
    }

    if (!descriptor->vectorMetadata.has_value()) {
        std::cerr << "Expected vector metadata.\n";
        return EXIT_FAILURE;
    }

    if (descriptor->vectorMetadata->featureCount != 1) {
        std::cerr << "Expected one feature in vector metadata.\n";
        return EXIT_FAILURE;
    }

    if (descriptor->vectorMetadata->fields.size() < 2) {
        std::cerr << "Expected vector fields metadata.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
