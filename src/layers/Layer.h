#pragma once

#include <optional>
#include <string>

#include "data/DataSourceDescriptor.h"
#include "layers/GeographicBounds.h"
#include "layers/MeasurementLayerData.h"
#include "layers/LayerTypes.h"

class Layer {
public:
    Layer(std::string id, std::string name, std::string sourceUri, LayerKind kind);
    virtual ~Layer() = default;

    const std::string &id() const;
    const std::string &name() const;
    const std::string &sourceUri() const;
    LayerKind kind() const;
    bool visible() const;
    double opacity() const;

    void setId(std::string id);
    void setName(std::string name);
    void setVisible(bool visible);
    void setOpacity(double opacity);

    void setGeographicBounds(const GeographicBounds &bounds);
    std::optional<GeographicBounds> geographicBounds() const;

    void setRasterMetadata(const RasterMetadata &meta);
    std::optional<RasterMetadata> rasterMetadata() const;

    void setVectorMetadata(const VectorLayerInfo &meta);
    std::optional<VectorLayerInfo> vectorMetadata() const;

    void setModelPlacement(const ModelPlacement &placement);
    std::optional<ModelPlacement> modelPlacement() const;

    void setMeasurementData(const MeasurementLayerData &data);
    std::optional<MeasurementLayerData> measurementData() const;

    void setBandMapping(int redBand, int greenBand, int blueBand);
    int redBand() const;
    int greenBand() const;
    int blueBand() const;

private:
    std::string id_;
    std::string name_;
    std::string sourceUri_;
    LayerKind kind_;
    bool visible_ = true;
    double opacity_ = 1.0;
    std::optional<GeographicBounds> geographicBounds_;
    std::optional<RasterMetadata> rasterMetadata_;
    std::optional<VectorLayerInfo> vectorMetadata_;
    std::optional<ModelPlacement> modelPlacement_;
    std::optional<MeasurementLayerData> measurementData_;
    int redBand_ = 1;
    int greenBand_ = 2;
    int blueBand_ = 3;
};
