#include <cstdlib>
#include <iostream>

#include "globe/SceneController.h"

int main() {
    SceneController controller;
    controller.initializeDefaultScene(1280, 720);

    if (!controller.isInitialized()) {
        std::cerr << "Expected scene controller to initialize.\n";
        return EXIT_FAILURE;
    }

    if (!controller.hasViewer()) {
        std::cerr << "Expected scene controller to create an osgViewer instance.\n";
        return EXIT_FAILURE;
    }

    if (!controller.hasMapNode()) {
        std::cerr << "Expected scene controller to create an osgEarth map node.\n";
        return EXIT_FAILURE;
    }

    if (!controller.hasBaseLayer()) {
        std::cerr << "Expected scene controller to add a base imagery layer.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
