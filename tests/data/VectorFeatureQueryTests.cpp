#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "data/VectorFeatureQuery.h"

namespace {

std::filesystem::path writeSampleGeoJson() {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "vector-feature-query-test.geojson";
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << R"({
  "type": "FeatureCollection",
  "name": "sample-layer",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "name": "Beijing",
        "code": "110000"
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
    const std::filesystem::path datasetPath = writeSampleGeoJson();
    const auto u8Path = datasetPath.u8string();
    const std::string path(u8Path.begin(), u8Path.end());

    const auto hit = queryVectorFeatureAt(path, 116.4, 39.9);
    if (!hit.has_value()) {
        std::cerr << "Expected vector query to hit sample feature.\n";
        return EXIT_FAILURE;
    }

    if (hit->displayText != "Beijing") {
        std::cerr << "Unexpected display text: " << hit->displayText << "\n";
        return EXIT_FAILURE;
    }

    if (hit->attributes.size() < 2) {
        std::cerr << "Expected vector query to return feature attributes.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
