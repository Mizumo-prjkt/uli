#ifndef ULI_PARTITIONER_FORMAT_MKFS_WRAPPER_HPP
#define ULI_PARTITIONER_FORMAT_MKFS_WRAPPER_HPP

#include <string>
#include <iostream>
#include <cstdlib>

namespace uli {
namespace partitioner {
namespace format {

class MkfsWrapper {
public:
    static bool format_partition(const std::string& device_path, const std::string& fs_type, const std::string& label = "") {
        std::cout << "[mkfs] Formatting \"" << device_path << "\" as " << fs_type << "..." << std::endl;
        
        // Swap is handled separately
        if (fs_type == "swap") {
            std::string cmd = "mkswap ";
            if (!label.empty()) cmd += "-L \"" + label + "\" ";
            cmd += "\"" + device_path + "\" > /dev/null 2>&1";
            return (std::system(cmd.c_str()) == 0);
        }

        std::string cmd;
        if (fs_type == "fat" || fs_type == "vfat" || fs_type == "fat32") {
            // Use mkfs.fat -F 32 for maximum compatibility and reliability on UEFI systems
            cmd = "mkfs.fat -F 32 ";
            if (!label.empty()) cmd += "-n \"" + label + "\" ";
        } else if (fs_type == "ext4") {
            cmd = "mkfs.ext4 -F ";
            if (!label.empty()) cmd += "-L \"" + label + "\" ";
        } else if (fs_type == "xfs") {
            cmd = "mkfs.xfs -f ";
            if (!label.empty()) cmd += "-L \"" + label + "\" ";
        } else if (fs_type == "btrfs") {
            cmd = "mkfs.btrfs -f ";
            if (!label.empty()) cmd += "-L \"" + label + "\" ";
        } else {
            cmd = "mkfs." + fs_type + " ";
            if (!label.empty()) cmd += "-L \"" + label + "\" ";
        }
        cmd += "\"" + device_path + "\" > /dev/null 2>&1";
        
        bool success = (std::system(cmd.c_str()) == 0);
        if (!success) {
            std::cerr << "[mkfs] ERROR: Formatting failed for " << device_path << std::endl;
        }
        return success;
    }
};

} // namespace format
} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_FORMAT_MKFS_WRAPPER_HPP
