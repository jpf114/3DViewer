#include "data/GdalDatasetInspector.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <limits>
#include <mutex>
#include <spdlog/spdlog.h>

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
                const char *unitType = band->GetUnitType();
                if (unitType && (strcmp(unitType, "m") == 0 || strcmp(unitType, "metre") == 0 || strcmp(unitType, "meter") == 0)) {
                    return DataSourceKind::RasterElevation;
                }
                const auto nodata = band->GetNoDataValue();
                int hasNodata = 0;
                band->GetNoDataValue(&hasNodata);
                if (hasNodata && nodata < -9999.0) {
                    return DataSourceKind::RasterElevation;
                }
                double adfMinMax[2] = {0.0, 0.0};
                band->ComputeRasterMinMax(true, adfMinMax);
                if (adfMinMax[0] < -500.0 || adfMinMax[1] > 9000.0) {
                    return DataSourceKind::RasterElevation;
                }
                return DataSourceKind::RasterImagery;
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

const char *gdalDataTypeName(GDALDataType dt) {
    switch (dt) {
    case GDT_Byte: return "Byte";
    case GDT_UInt16: return "UInt16";
    case GDT_Int16: return "Int16";
    case GDT_UInt32: return "UInt32";
    case GDT_Int32: return "Int32";
    case GDT_Float32: return "Float32";
    case GDT_Float64: return "Float64";
    default: return "Unknown";
    }
}

const char *colorInterpName(GDALColorInterp ci) {
    switch (ci) {
    case GCI_Undefined: return "Undefined";
    case GCI_GrayIndex: return "Gray";
    case GCI_PaletteIndex: return "Palette";
    case GCI_RedBand: return "Red";
    case GCI_GreenBand: return "Green";
    case GCI_BlueBand: return "Blue";
    case GCI_AlphaBand: return "Alpha";
    case GCI_HueBand: return "Hue";
    case GCI_SaturationBand: return "Saturation";
    case GCI_LightnessBand: return "Lightness";
    case GCI_CyanBand: return "Cyan";
    case GCI_MagentaBand: return "Magenta";
    case GCI_YellowBand: return "Yellow";
    case GCI_BlackBand: return "Black";
    case GCI_YCbCr_YBand: return "Y";
    case GCI_YCbCr_CbBand: return "Cb";
    case GCI_YCbCr_CrBand: return "Cr";
    default: return "Unknown";
    }
}

RasterMetadata collectRasterMetadata(GDALDataset &ds) {
    RasterMetadata meta;
    meta.rasterXSize = ds.GetRasterXSize();
    meta.rasterYSize = ds.GetRasterYSize();
    meta.bandCount = ds.GetRasterCount();
    meta.driverName = ds.GetDriverName();

    const OGRSpatialReference *srs = ds.GetSpatialRef();
    if (srs) {
        char *wkt = nullptr;
        srs->exportToWkt(&wkt);
        if (wkt) {
            meta.crsWkt = wkt;
            CPLFree(wkt);
        }
        const char *authName = srs->GetAuthorityName(nullptr);
        const char *authCode = srs->GetAuthorityCode(nullptr);
        if (authName && authCode && strcmp(authName, "EPSG") == 0) {
            meta.epsgCode = std::string("EPSG:") + authCode;
        }
    }

    double gt[6];
    if (ds.GetGeoTransform(gt) == CE_None) {
        meta.pixelSizeX = std::abs(gt[1]);
        meta.pixelSizeY = std::abs(gt[5]);
    }

    for (int i = 1; i <= ds.GetRasterCount(); ++i) {
        GDALRasterBand *band = ds.GetRasterBand(i);
        BandInfo bi;
        bi.index = i;
        bi.dataType = gdalDataTypeName(band->GetRasterDataType());
        bi.colorInterpretation = colorInterpName(band->GetColorInterpretation());
        const char *desc = band->GetDescription();
        if (desc && desc[0] != '\0') {
            bi.description = desc;
        }
        int hasNodata = 0;
        bi.noDataValue = band->GetNoDataValue(&hasNodata);
        bi.hasNoDataValue = hasNodata != 0;
        double adfMinMax[2] = {0.0, 0.0};
        if (band->ComputeRasterMinMax(true, adfMinMax) == CE_None) {
            bi.min = adfMinMax[0];
            bi.max = adfMinMax[1];
            bi.hasMinMax = true;
        }
        meta.bands.push_back(bi);
    }

    return meta;
}

const char *ogrGeomTypeName(OGRwkbGeometryType gt) {
    switch (gt) {
    case wkbPoint: return "Point";
    case wkbMultiPoint: return "MultiPoint";
    case wkbLineString: return "LineString";
    case wkbMultiLineString: return "MultiLineString";
    case wkbPolygon: return "Polygon";
    case wkbMultiPolygon: return "MultiPolygon";
    case wkbGeometryCollection: return "GeometryCollection";
    case wkbPoint25D: return "PointZ";
    case wkbMultiPoint25D: return "MultiPointZ";
    case wkbLineString25D: return "LineStringZ";
    case wkbMultiLineString25D: return "MultiLineStringZ";
    case wkbPolygon25D: return "PolygonZ";
    case wkbMultiPolygon25D: return "MultiPolygonZ";
    default: return "Unknown";
    }
}

VectorLayerInfo collectVectorMetadata(GDALDataset &ds) {
    VectorLayerInfo info;
    OGRLayer *layer = ds.GetLayer(0);
    if (!layer) {
        return info;
    }

    info.name = layer->GetName();
    info.geometryType = ogrGeomTypeName(layer->GetGeomType());
    info.featureCount = static_cast<int>(layer->GetFeatureCount());

    const OGRSpatialReference *srs = layer->GetSpatialRef();
    if (srs) {
        const char *authName = srs->GetAuthorityName(nullptr);
        const char *authCode = srs->GetAuthorityCode(nullptr);
        if (authName && authCode && strcmp(authName, "EPSG") == 0) {
            info.epsgCode = std::string("EPSG:") + authCode;
        }
    }

    const OGRFeatureDefn *defn = layer->GetLayerDefn();
    for (int i = 0; i < defn->GetFieldCount(); ++i) {
        const OGRFieldDefn *field = defn->GetFieldDefn(i);
        VectorFieldInfo fi;
        fi.name = field->GetNameRef();
        fi.typeName = field->GetFieldTypeName(field->GetType());
        info.fields.push_back(fi);
    }

    return info;
}

std::optional<DataSourceDescriptor> GdalDatasetInspector::inspect(const std::string &path) const {
    ensureGdalRegistered();

    struct DatasetCloser {
        void operator()(GDALDataset *ds) const {
            if (ds) GDALClose(ds);
        }
    };
    using DatasetPtr = std::unique_ptr<GDALDataset, DatasetCloser>;

    DatasetPtr dataset(
        static_cast<GDALDataset *>(GDALOpenEx(path.c_str(), GDAL_OF_READONLY | GDAL_OF_VECTOR | GDAL_OF_RASTER, nullptr, nullptr, nullptr)));
    if (!dataset) {
        spdlog::debug("GdalDatasetInspector: failed to open '{}'", path);
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

    if (kind == DataSourceKind::Vector) {
        descriptor.vectorMetadata = collectVectorMetadata(*dataset);
    } else {
        descriptor.rasterMetadata = collectRasterMetadata(*dataset);
    }

    spdlog::info("GdalDatasetInspector: inspected '{}' as kind={} id={}", path, static_cast<int>(kind), layerId);
    return descriptor;
}
