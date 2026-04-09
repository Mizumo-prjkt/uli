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
        std::cout << "[mkfs] Formatting " << device_path << " as " << fs_type << std::endl;
        std::string cmd = "mkfs." + fs_type + " -F "; // Force format directly over standard wrappers
        
        if (!label.empty()) {
            if (fs_type == "fat" || fs_type == "vfat") {
                cmd += "-n " + label + " ";
            } else if (fs_type == "xfs" || fs_type == "btrfs" || fs_type == "ext4") {
                cmd += "-L " + label + " ";
            }
        }
        cmd += device_path + " > /dev/null 2>&1";
        
        if (fs_type == "swap") {
            cmd = "mkswap ";
            if (!label.empty()) cmd += "-L " + label + " ";
            cmd += device_path + " > /dev/null 2>&1";
        }
        
        return (std::system(cmd.c_str()) == 0);
    }
};

} // namespace format
} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_FORMAT_MKFS_WRAPPER_HPP
