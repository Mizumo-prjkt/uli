#ifndef ULI_RUNTIME_CHECK_HPP
#define ULI_RUNTIME_CHECK_HPP

#include <iostream>
#include <fstream>
#include <string>

namespace uli {
namespace env {

class RuntimeEnv {
public:
    static bool is_live_environment() {
        std::ifstream file("/proc/cmdline");
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        if (std::getline(file, line)) {
            // Check for standard indicators of a Live CD or USB environment
            // archiso: Arch based distros
            // boot=live: Standard debian live config
            // vtoy / VTOY: Indicator of Ventoy multiboot payload execution
            if (line.find("archiso") != std::string::npos || 
                line.find("boot=live") != std::string::npos || 
                line.find("vtoy") != std::string::npos || 
                line.find("VTOY") != std::string::npos) {
                return true;
            }
        }
        return false;
    }
};

} // namespace env
} // namespace uli

#endif // ULI_RUNTIME_CHECK_HPP
