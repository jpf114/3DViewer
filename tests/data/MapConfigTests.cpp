#include <cstdlib>
#include <fstream>

#include <QDir>
#include <QTemporaryDir>

#include "data/BasemapUrlTemplate.h"
#include "data/MapConfig.h"

namespace {

bool writeTextFile(const QString &path, const char *content) {
    std::ofstream out(path.toStdString(), std::ios::binary);
    out << content;
    return static_cast<bool>(out);
}

}

int main() {
    if (basemap::applyUrlTemplate("https://t{s}.example.com?tk={token}", "012", "abc") !=
        "https://t0.example.com?tk=abc") {
        return EXIT_FAILURE;
    }

    QTemporaryDir dir;
    if (!dir.isValid()) {
        return EXIT_FAILURE;
    }

    const QString jsonPath = QDir(dir.path()).filePath("basemap.json");
    if (!writeTextFile(jsonPath,
        "{\n"
        "  \"basemaps\": [\n"
        "    {\n"
        "      \"id\": \"tdt_img\",\n"
        "      \"name\": \"TDT\",\n"
        "      \"type\": \"tms\",\n"
        "      \"enabled\": true,\n"
        "      \"url\": \"https://t{s}.example.com?tk={token}\",\n"
        "      \"subdomains\": \"01234567\",\n"
        "      \"token\": \"demo-token\",\n"
        "      \"profile\": \"spherical-mercator\"\n"
        "    }\n"
        "  ]\n"
        "}\n")) {
        return EXIT_FAILURE;
    }

    const auto config = loadBasemapConfig(dir.path().toStdString());
    if (!config.has_value() || config->basemaps.size() != 1) {
        return EXIT_FAILURE;
    }

    const auto &entry = config->basemaps.front();
    if (entry.id != "tdt_img" || entry.type != BasemapType::TMS || !entry.enabled) {
        return EXIT_FAILURE;
    }

    if (entry.subdomains != "01234567" || entry.token != "demo-token") {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
