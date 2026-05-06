#pragma once

#include <optional>
#include <string>

#include "data/DataSourceDescriptor.h"

class GdalDatasetInspector {
public:
    std::optional<DataSourceDescriptor> inspect(const std::string &path) const;
};
