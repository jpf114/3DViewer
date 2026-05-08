#pragma once

#include <string>

namespace basemap {

inline void replaceAll(std::string &text, const std::string &needle, const std::string &replacement) {
    if (needle.empty()) {
        return;
    }

    std::size_t pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
        text.replace(pos, needle.size(), replacement);
        pos += replacement.size();
    }
}

inline std::string applyUrlTemplate(std::string url,
                                    const std::string &subdomains,
                                    const std::string &token) {
    const std::string subdomain = subdomains.empty() ? std::string() : std::string(1, subdomains.front());
    replaceAll(url, "{s}", subdomain);
    replaceAll(url, "{token}", token);
    return url;
}

} // namespace basemap
