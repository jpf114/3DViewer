#include "data/GdalDatasetInspector.h"

#include <filesystem>
#include <mutex>

#include <gdal_priv.h>

namespace {

void ensureGdalRegistered() {
    static std::once_flag once;
    std::call_once(once, []() {
        GDALAllRegister();
    });
}

std::string stemOrFilename(const std::string &path) {
    const std::filesystem::path filePath(path);
    const auto stem = filePath.stem().string();
    if (!stem.empty()) {
        return stem;
    }

    return filePath.filename().string();
}

DataSourceKind classifyDataset(GDALDataset &dataset) {
    if (dataset.GetLayerCount() > 0 && dataset.GetRasterCount() == 0) {
        return DataSourceKind::Vector;
    }

    if (dataset.GetRasterCount() == 1) {
        if (GDALRasterBand *band = dataset.GetRasterBand(1)) {
            const auto interpretation = band->GetColorInterpretation();
            if (interpretation == GCI_GrayIndex || interpretation == GCI_Undefined) {
                return DataSourceKind::RasterElevation;
            }
        }
    }

    return DataSourceKind::RasterImagery;
}

} // namespace

std::optional<DataSourceDescriptor> GdalDatasetInspector::inspect(const std::string &path) const {
    ensureGdalRegistered();

    GDALDataset *dataset =
        static_cast<GDALDataset *>(GDALOpenEx(path.c_str(), GDAL_OF_READONLY | GDAL_OF_VECTOR | GDAL_OF_RASTER, nullptr, nullptr, nullptr));
    if (dataset == nullptr) {
        return std::nullopt;
    }

    DataSourceDescriptor descriptor{
        path,
        stemOrFilename(path),
        path,
        classifyDataset(*dataset),
    };

    GDALClose(dataset);
    return descriptor;
}
