#ifndef ULI_RUNTIME_DEPENDENCY_CHECK_HPP
#define ULI_RUNTIME_DEPENDENCY_CHECK_HPP

#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include "warn.hpp"
#include "blackbox.hpp"

namespace uli {
namespace runtime {

class DependencyChecker {
public:
    static bool has_binary(const std::string& name) {
        std::string cmd = "command -v " + name + " > /dev/null 2>&1";
        return (std::system(cmd.c_str()) == 0);
    }

    struct CheckResult {
        bool success;
        std::string missing_tools;
    };

    static CheckResult check_essentials(const std::string& distro) {
        std::vector<std::string> critical;
        
        // Partitioning cluster (Need at least one)
        bool has_part_tool = has_binary("sgdisk") || has_binary("fdisk") || has_binary("parted");
        
        // Filesystem cluster
        critical.push_back("mkfs.ext4");
        critical.push_back("mkfs.vfat");
        critical.push_back("partprobe");

        // Distro specific
        if (distro == "Arch Linux") {
            critical.push_back("genfstab");
            critical.push_back("pacstrap");
        } else if (distro == "Debian") {
            critical.push_back("debootstrap");
        }

        std::string missing = "";
        if (!has_part_tool) {
            missing += "[sgdisk OR fdisk OR parted] ";
        }

        for (const auto& tool : critical) {
            if (!has_binary(tool)) {
                missing += tool + " ";
            }
        }

        if (missing.empty()) {
            return {true, ""};
        } else {
            BlackBox::log("DEPENDENCY FAILURE: Missing tools: " + missing);
            return {false, missing};
        }
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_DEPENDENCY_CHECK_HPP
