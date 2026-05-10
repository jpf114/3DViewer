#include "data/VectorFeatureQuery.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>

#include <cpl_conv.h>
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>

#include "common/GdalRuntime.h"

namespace {

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
    gdalruntime::ensureGdalRegistered();

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
