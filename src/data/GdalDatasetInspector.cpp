#include "data/GdalDatasetInspector.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <limits>
#include <mutex>

#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>

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

std::optional<GeographicBounds> transformExtentToWgs84(const OGRSpatialReference &rawSrc, double minx, double miny, double maxx, double maxy) {
    OGRSpatialReference src(rawSrc);
    src.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    OGRSpatialReference dst;
    if (dst.importFromEPSG(4326) != OGRERR_NONE) {
        return std::nullopt;
    }
    dst.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    auto *ct = OGRCreateCoordinateTransformation(&src, &dst);
    if (ct == nullptr) {
        return std::nullopt;
    }

    const double xs[4] = {minx, maxx, maxx, minx};
    const double ys[4] = {miny, miny, maxy, maxy};
    double minLon = std::numeric_limits<double>::infinity();
    double minLat = std::numeric_limits<double>::infinity();
    double maxLon = -std::numeric_limits<double>::infinity();
    double maxLat = -std::numeric_limits<double>::infinity();

    for (int i = 0; i < 4; ++i) {
        double x = xs[i];
        double y = ys[i];
        int success = 0;
        if (!ct->Transform(1, &x, &y, nullptr, &success) || !success) {
            OGRCoordinateTransformation::DestroyCT(ct);
            return std::nullopt;
        }
        minLon = (std::min)(minLon, x);
        maxLon = (std::max)(maxLon, x);
        minLat = (std::min)(minLat, y);
        maxLat = (std::max)(maxLat, y);
    }

    OGRCoordinateTransformation::DestroyCT(ct);

    GeographicBounds bounds{minLon, minLat, maxLon, maxLat};
    if (!bounds.isValid()) {
        return std::nullopt;
    }
    return bounds;
}

std::optional<GeographicBounds> boundsForRaster(GDALDataset &ds) {
    if (ds.GetRasterCount() < 1) {
        return std::nullopt;
    }

    double gt[6];
    if (ds.GetGeoTransform(gt) != CE_None) {
        return std::nullopt;
    }

    const int w = ds.GetRasterXSize();
    const int h = ds.GetRasterYSize();
    if (w <= 0 || h <= 0) {
        return std::nullopt;
    }

    if (gt[1] == 1.0 && gt[5] == -1.0 && gt[2] == 0.0 && gt[4] == 0.0 && gt[0] == 0.0 && gt[3] == 0.0) {
        return std::nullopt;
    }

    double minx = std::numeric_limits<double>::infinity();
    double miny = std::numeric_limits<double>::infinity();
    double maxx = -std::numeric_limits<double>::infinity();
    double maxy = -std::numeric_limits<double>::infinity();
    const double px[4] = {0.0, static_cast<double>(w), static_cast<double>(w), 0.0};
    const double py[4] = {0.0, 0.0, static_cast<double>(h), static_cast<double>(h)};
    for (int i = 0; i < 4; ++i) {
        double gx = 0.0;
        double gy = 0.0;
        GDALApplyGeoTransform(gt, px[i], py[i], &gx, &gy);
        minx = (std::min)(minx, gx);
        maxx = (std::max)(maxx, gx);
        miny = (std::min)(miny, gy);
        maxy = (std::max)(maxy, gy);
    }

    const OGRSpatialReference *srs = ds.GetSpatialRef();
    if (srs == nullptr) {
        return std::nullopt;
    }

    OGRSpatialReference src(*srs);
    return transformExtentToWgs84(src, minx, miny, maxx, maxy);
}

std::optional<GeographicBounds> boundsForVector(GDALDataset &ds) {
    OGRLayer *layer = ds.GetLayer(0);
    if (layer == nullptr) {
        return std::nullopt;
    }

    OGREnvelope env;
    if (layer->GetExtent(&env) != OGRERR_NONE) {
        return std::nullopt;
    }

    const OGRSpatialReference *srs = layer->GetSpatialRef();
    if (srs == nullptr) {
        return std::nullopt;
    }

    OGRSpatialReference src(*srs);
    return transformExtentToWgs84(src, env.MinX, env.MinY, env.MaxX, env.MaxY);
}

std::optional<GeographicBounds> computeGeographicBounds(GDALDataset &ds, DataSourceKind kind) {
    if (kind == DataSourceKind::Vector) {
        return boundsForVector(ds);
    }
    return boundsForRaster(ds);
}

} // namespace

std::optional<DataSourceDescriptor> GdalDatasetInspector::inspect(const std::string &path) const {
    ensureGdalRegistered();

    GDALDataset *dataset =
        static_cast<GDALDataset *>(GDALOpenEx(path.c_str(), GDAL_OF_READONLY | GDAL_OF_VECTOR | GDAL_OF_RASTER, nullptr, nullptr, nullptr));
    if (dataset == nullptr) {
        return std::nullopt;
    }

    const std::filesystem::path filePath(path);
    const std::string stem = stemOrFilename(path);
    const DataSourceKind kind = classifyDataset(*dataset);

    const auto pathHash = std::to_string(std::hash<std::string>{}(path));
    const std::string layerId = stem + "-" + pathHash.substr(0, 8);

    DataSourceDescriptor descriptor{
        layerId,
        filePath.filename().string(),
        path,
        kind,
        std::nullopt,
    };

    if (auto bounds = computeGeographicBounds(*dataset, kind)) {
        descriptor.geographicBounds = *bounds;
    }

    GDALClose(dataset);
    return descriptor;
}
