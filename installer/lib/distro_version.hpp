#ifndef ULI_DISTRO_VERSION_HPP
#define ULI_DISTRO_VERSION_HPP

#include <string>
#include <fstream>
#include <iostream>

namespace uli {
namespace checks {

class DistroVersion {
public:
    // Parses /etc/os-release and returns the integer major version (e.g. 12, 13, 24).
    // Returns 0 if detection fails.
    static int detect_major_version() {
        std::ifstream file("/etc/os-release");
        if (!file.is_open()) return 0;

        std::string line;
        while (std::getline(file, line)) {
            if (line.find("VERSION_ID=") == 0) {
                std::string ver = line.substr(11);
                // Strip quotes
                if (!ver.empty() && ver.front() == '"') ver.erase(0, 1);
                if (!ver.empty() && ver.back() == '"') ver.pop_back();
                // Extract major version (before any '.' or '-')
                std::string major_str;
                for (char c : ver) {
                    if (c == '.' || c == '-') break;
                    if (c >= '0' && c <= '9') major_str += c;
                }
                if (!major_str.empty()) {
                    try { return std::stoi(major_str); } catch (...) { return 0; }
                }
            }
        }
        return 0;
    }

    // Parses a --dist value like "debian:13" into base distro and version.
    // Returns: pair<base_distro, version> where version=0 means auto-detect.
    static std::pair<std::string, int> parse_dist_flag(const std::string& dist_value) {
        size_t colon = dist_value.find(':');
        if (colon == std::string::npos) {
            return {dist_value, 0}; // No version specified → auto-detect
        }
        std::string base = dist_value.substr(0, colon);
        std::string ver_str = dist_value.substr(colon + 1);
        int ver = 0;
        try { ver = std::stoi(ver_str); } catch (...) { ver = 0; }
        return {base, ver};
    }
};

} // namespace checks
} // namespace uli

#endif // ULI_DISTRO_VERSION_HPP
