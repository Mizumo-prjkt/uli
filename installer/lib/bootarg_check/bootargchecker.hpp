#ifndef ULI_BOOTARGCHECKER_HPP
#define ULI_BOOTARGCHECKER_HPP

#include <string>
#include <fstream>
#include <sstream>

namespace uli {
namespace bootarg {

struct BootArgs {
    bool autorun = false;
    bool unattended = false;
    std::string config_file = "";
    bool is_uli_in_bootdir = false;
    std::string translation_yaml = "";
    std::string distro_override = "";
    bool force_small_disk = false;
};

inline BootArgs parse_boot_args() {
    BootArgs args;
    std::ifstream file("/proc/cmdline");
    if (!file.is_open()) return args;

    std::string line;
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        while (iss >> token) {
            if (token == "uli.autorun=1") args.autorun = true;
            else if (token == "uli.unattended=1") args.unattended = true;
            else if (token == "uli.is_uli_in_bootdir=1") args.is_uli_in_bootdir = true;
            else if (token.find("uli.config_file=") == 0) args.config_file = token.substr(16);
            else if (token.find("uli.translation_yaml=") == 0) args.translation_yaml = token.substr(21);
            else if (token.find("uli.distro_override=") == 0) args.distro_override = token.substr(20);
            else if (token == "uli.force_small_disk=1") args.force_small_disk = true;
        }
    }

    if (args.is_uli_in_bootdir) {
        if (!args.config_file.empty() && args.config_file.find("/boot") != 0) {
            if (args.config_file.front() == '/') {
                args.config_file = "/boot" + args.config_file;
            } else {
                args.config_file = "/boot/" + args.config_file;
            }
        }

        if (!args.translation_yaml.empty() && args.translation_yaml.find("/boot") != 0) {
            if (args.translation_yaml.front() == '/') {
                args.translation_yaml = "/boot" + args.translation_yaml;
            } else {
                args.translation_yaml = "/boot/" + args.translation_yaml;
            }
        }
    }

    return args;
}

} // namespace bootarg
} // namespace uli

#endif // ULI_BOOTARGCHECKER_HPP
