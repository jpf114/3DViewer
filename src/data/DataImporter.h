#pragma once

#include <memory>
#include <string>
#include <vector>

#include "data/DataSourceDescriptor.h"
#include "data/GdalDatasetInspector.h"

class Layer;

class DataImporter {
public:
    static std::vector<std::string> supportedModelExtensions();
    static std::vector<std::string> availableRuntimeModelExtensions();

    std::shared_ptr<Layer> import(const std::string &path) const;
    std::shared_ptr<Layer> import(const DataSourceDescriptor &descriptor) const;

private:
    GdalDatasetInspector inspector_;
};
