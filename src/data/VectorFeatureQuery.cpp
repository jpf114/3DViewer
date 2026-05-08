#include "data/VectorFeatureQuery.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <string>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#include <cpl_conv.h>
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>

namespace {

void configureRuntimeDataPaths() {
#if defined(_WIN32)
    if (CPLGetConfigOption("GDAL_DATA", nullptr) != nullptr &&
        CPLGetConfigOption("PROJ_DATA", nullptr) != nullptr &&
        CPLGetConfigOption("PROJ_LIB", nullptr) != nullptr) {
        return;
    }

    wchar_t modulePath[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return;
    }

    const std::filesystem::path exeDir = std::filesystem::path(modulePath).parent_path();
    const std::filesystem::path shareDir = exeDir / "share";
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
#endif
}

void ensureGdalRegistered() {
    static std::once_flag once;
    std::call_once(once, []() {
        configureRuntimeDataPaths();
        GDALAllRegister();
    });
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");
    CPLSetConfigOption("SHAPE_ENCODING", "UTF-8");
}

double layerTolerance(const OGREnvelope &env) {
    const double width = std::abs(env.MaxX - env.MinX);
    const double height = std::abs(env.MaxY - env.MinY);
    const double maxDim = (std::max)(width, height);
    return (std::max)(1e-6, maxDim / 1000.0);
}

struct DatasetCloser {
    void operator()(GDALDataset *dataset) const {
        if (dataset) {
            GDALClose(dataset);
        }
    }
};

using DatasetPtr = std::unique_ptr<GDALDataset, DatasetCloser>;
using FeaturePtr = std::unique_ptr<OGRFeature, decltype(&OGRFeature::DestroyFeature)>;

std::vector<VectorFeatureAttribute> collectAttributes(const OGRFeature &feature) {
    std::vector<VectorFeatureAttribute> attributes;
    const OGRFeatureDefn *defn = feature.GetDefnRef();
    if (!defn) {
        return attributes;
    }

    attributes.reserve(defn->GetFieldCount());
    for (int i = 0; i < defn->GetFieldCount(); ++i) {
        if (!feature.IsFieldSetAndNotNull(i)) {
            continue;
        }

        const OGRFieldDefn *field = defn->GetFieldDefn(i);
        if (!field) {
            continue;
        }

        VectorFeatureAttribute attr;
        attr.name = field->GetNameRef();
        attr.value = feature.GetFieldAsString(i);
        attributes.push_back(std::move(attr));
    }
    return attributes;
}

std::string featureDisplayText(const OGRFeature &feature, const OGRLayer &layer) {
    static constexpr const char *kNameCandidates[] = {
        "NAME",
        "Name",
        "name",
        "NAME_CHN",
        "NAME_EN",
        "NL_NAME_1"
    };

    for (const char *fieldName : kNameCandidates) {
        const int idx = feature.GetFieldIndex(fieldName);
        if (idx >= 0 && feature.IsFieldSetAndNotNull(idx)) {
            const char *value = feature.GetFieldAsString(idx);
            if (value && value[0] != '\0') {
                return value;
            }
        }
    }

    const char *layerName = layer.GetName();
    return layerName ? layerName : "Vector Feature";
}

std::optional<double> geometryHitDistance(const OGRGeometry &geometry, const OGRPoint &point, double tolerance) {
    if (geometry.Contains(&point) || geometry.Intersects(&point)) {
        return 0.0;
    }

    const double distance = geometry.Distance(&point);
    if (distance <= tolerance) {
        return distance;
    }

    return std::nullopt;
}

} // namespace

std::optional<VectorFeatureHit> queryVectorFeatureAt(const std::string &path,
                                                     double longitude,
                                                     double latitude) {
    ensureGdalRegistered();

    DatasetPtr dataset(
        static_cast<GDALDataset *>(GDALOpenEx(path.c_str(), GDAL_OF_READONLY | GDAL_OF_VECTOR, nullptr, nullptr, nullptr)));
    if (!dataset) {
        return std::nullopt;
    }

    OGRLayer *layer = dataset->GetLayer(0);
    if (!layer) {
        return std::nullopt;
    }

    OGRSpatialReference wgs84;
    if (wgs84.importFromEPSG(4326) != OGRERR_NONE) {
        return std::nullopt;
    }
    wgs84.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    OGRSpatialReference layerSrs;
    const OGRSpatialReference *layerSrsRaw = layer->GetSpatialRef();
    if (layerSrsRaw) {
        layerSrs = *layerSrsRaw;
    } else if (layerSrs.importFromEPSG(4326) != OGRERR_NONE) {
        return std::nullopt;
    }
    layerSrs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    OGRPoint point(longitude, latitude);
    point.assignSpatialReference(&wgs84);
    if (!wgs84.IsSame(&layerSrs)) {
        auto ct = std::unique_ptr<OGRCoordinateTransformation, decltype(&OGRCoordinateTransformation::DestroyCT)>(
            OGRCreateCoordinateTransformation(&wgs84, &layerSrs),
            OGRCoordinateTransformation::DestroyCT);
        if (!ct) {
            return std::nullopt;
        }

        if (!point.transform(ct.get())) {
            return std::nullopt;
        }
    }

    OGREnvelope env;
    if (layer->GetExtent(&env) != OGRERR_NONE) {
        return std::nullopt;
    }
    const double tolerance = layerTolerance(env);

    layer->SetSpatialFilterRect(point.getX() - tolerance, point.getY() - tolerance,
                                point.getX() + tolerance, point.getY() + tolerance);
    layer->ResetReading();

    VectorFeatureHit bestHit;
    bool found = false;
    double bestDistance = std::numeric_limits<double>::infinity();

    while (true) {
        FeaturePtr feature(layer->GetNextFeature(), OGRFeature::DestroyFeature);
        if (!feature) {
            break;
        }

        OGRGeometry *geometry = feature->GetGeometryRef();
        if (!geometry) {
            continue;
        }

        const auto hitDistance = geometryHitDistance(*geometry, point, tolerance);
        if (!hitDistance.has_value()) {
            continue;
        }

        const double distance = *hitDistance;
        if (distance > bestDistance) {
            continue;
        }

        bestDistance = distance;
        bestHit.displayText = featureDisplayText(*feature, *layer);
        bestHit.attributes = collectAttributes(*feature);
        found = true;
    }

    layer->SetSpatialFilter(nullptr);
    return found ? std::optional<VectorFeatureHit>(bestHit) : std::nullopt;
}
