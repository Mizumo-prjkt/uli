#ifndef ULI_PARTITIONER_FORMAT_MKFS_WRAPPER_HPP
#define ULI_PARTITIONER_FORMAT_MKFS_WRAPPER_HPP

#include <string>
#include <iostream>
#include <cstdlib>
#include "../../runtime/blackbox.hpp"

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
            uli::runtime::BlackBox::log("EXEC: " + cmd);
            return (std::system(cmd.c_str()) == 0);
        }

        std::string cmd;
        if (fs_type == "fat" || fs_type == "vfat" || fs_type == "fat32") {
            // Use mkfs.fat -F 32 -I for maximum compatibility and reliability on UEFI systems
            // -I ignores safety checks (needed for some partition alignments)
            cmd = "mkfs.fat -F 32 -I ";
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
        cmd += "\"" + device_path + "\"";
        
        // Append stderr redirection for the system call to capture output if we need to debug
        std::string full_cmd = cmd + " 2>&1";
        
        uli::runtime::BlackBox::log("EXEC: " + full_cmd);
        
        // Use a pipe to capture the error move if the return code is non-zero
        std::string error_output;
        FILE* pipe = popen(full_cmd.c_str(), "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                error_output += buffer;
            }
            int status = pclose(pipe);
            if (status == 0) return true;
        }

        std::cerr << "[mkfs] ERROR: Formatting failed for " << device_path << std::endl;
        if (!error_output.empty()) {
            uli::runtime::BlackBox::log("MKFS ERROR OUTPUT: " + error_output);
        }
        return false;
    }
};

} // namespace format
} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_FORMAT_MKFS_WRAPPER_HPP
