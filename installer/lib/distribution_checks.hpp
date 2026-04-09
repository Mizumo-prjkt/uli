#ifndef ULI_DISTRIBUTION_CHECKS_HPP
#define ULI_DISTRIBUTION_CHECKS_HPP

#include <string>
#include <fstream>
#include <iostream>

namespace uli {
namespace checks {

enum class DistroType {
    DEBIAN,
    ARCH,
    ALPINE, // For future use
    UNKNOWN
};

inline DistroType detect_distribution() {
    std::ifstream file("/etc/os-release");
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open /etc/os-release" << std::endl;
        return DistroType::UNKNOWN;
    }

    std::string line;
    std::string id = "";
    std::string id_like = "";

    while (std::getline(file, line)) {
        if (line.find("ID=") == 0) {
            id = line.substr(3);
            // Remove optional quotes
            if (!id.empty() && id.front() == '"') id.erase(0, 1);
            if (!id.empty() && id.back() == '"') id.pop_back();
        } else if (line.find("ID_LIKE=") == 0) {
            id_like = line.substr(8);
            if (!id_like.empty() && id_like.front() == '"') id_like.erase(0, 1);
            if (!id_like.empty() && id_like.back() == '"') id_like.pop_back();
        }
    }

    // Check ID and ID_LIKE against known patterns
    if (id == "debian" || id == "ubuntu" || id == "kali" || 
        id_like.find("debian") != std::string::npos || id_like.find("ubuntu") != std::string::npos) {
        return DistroType::DEBIAN;
    } else if (id == "arch" || id == "manjaro" || id == "endeavouros" || id == "artix" ||
               id_like.find("arch") != std::string::npos) {
        return DistroType::ARCH;
    } else if (id == "alpine" || id_like.find("alpine") != std::string::npos) {
        return DistroType::ALPINE;
    }

    return DistroType::UNKNOWN;
}

} // namespace checks
} // namespace uli

#endif // ULI_DISTRIBUTION_CHECKS_HPP
