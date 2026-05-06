#pragma once

#include <memory>
#include <string>

#include "data/DataSourceDescriptor.h"
#include "data/GdalDatasetInspector.h"

class Layer;

class DataImporter {
public:
    std::shared_ptr<Layer> import(const std::string &path) const;
    std::shared_ptr<Layer> import(const DataSourceDescriptor &descriptor) const;

private:
    GdalDatasetInspector inspector_;
};
