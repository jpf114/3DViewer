#include <cstdlib>
#include <iostream>

#include "data/GdalDatasetInspector.h"

int main() {
    GdalDatasetInspector inspector;
    const auto descriptor = inspector.inspect("definitely-missing-dataset.xyz");

    if (descriptor.has_value()) {
        std::cerr << "Expected missing file inspection to fail.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
