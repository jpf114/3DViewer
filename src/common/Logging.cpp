#include "common/Logging.h"

#include <spdlog/spdlog.h>

namespace logging {

void initialize() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::info("logging initialized");
}

}  // namespace logging
