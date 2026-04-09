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

        // --- Pre-Format Prep: Break locks ---
        // 1. Force unmount just in case an automounter grabbed it
        std::string unmount_cmd = "umount -l \"" + device_path + "\" > /dev/null 2>&1";
        uli::runtime::BlackBox::log("PRE-FORMAT: " + unmount_cmd);
        std::system(unmount_cmd.c_str());

        // 2. Wipe existing signatures to prevent udev from re-locking based on old headers
        std::string wipe_cmd = "wipefs -a -f \"" + device_path + "\" > /dev/null 2>&1";
        uli::runtime::BlackBox::log("PRE-FORMAT: " + wipe_cmd);
        std::system(wipe_cmd.c_str());

        // 3. Final synchronization
        std::system("sync");
        std::system("udevadm settle");

        // Swap is handled separately
        if (fs_type == "swap") {
            std::string cmd = "mkswap ";
            if (!label.empty()) cmd += "-L \"" + label + "\" ";
            cmd += "\"" + device_path + "\" > /dev/null 2>&1";
            uli::runtime::BlackBox::log("EXEC: " + cmd);
            return (std::system(cmd.c_str()) == 0);
        }

        std::string cmd_base;
        if (fs_type == "fat" || fs_type == "vfat" || fs_type == "fat32") {
            // Use mkfs.fat -F 32 -I for maximum compatibility and reliability on UEFI systems
            // -I ignores safety checks (needed for some partition alignments)
            cmd_base = "mkfs.fat -F 32 -I ";
            if (!label.empty()) cmd_base += "-n \"" + label + "\" ";
        } else if (fs_type == "ext4") {
            cmd_base = "mkfs.ext4 -F ";
            if (!label.empty()) cmd_base += "-L \"" + label + "\" ";
        } else if (fs_type == "xfs") {
            cmd_base = "mkfs.xfs -f ";
            if (!label.empty()) cmd_base += "-L \"" + label + "\" ";
        } else if (fs_type == "btrfs") {
            cmd_base = "mkfs.btrfs -f ";
            if (!label.empty()) cmd_base += "-L \"" + label + "\" ";
        } else {
            cmd_base = "mkfs." + fs_type + " ";
            if (!label.empty()) cmd_base += "-L \"" + label + "\" ";
        }
        std::string final_cmd = cmd_base + "\"" + device_path + "\"";
        
        // --- Execution Loop with Retry for "Busy" resources ---
        for (int attempt = 1; attempt <= 2; ++attempt) {
            std::string full_exec = final_cmd + " 2>&1";
            uli::runtime::BlackBox::log("EXEC (Attempt " + std::to_string(attempt) + "): " + full_exec);
            
            std::string error_output;
            FILE* pipe = popen(full_exec.c_str(), "r");
            if (pipe) {
                char buffer[128];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    error_output += buffer;
                }
                int status = pclose(pipe);
                if (status == 0) {
                    uli::runtime::BlackBox::log("FORMAT SUCCESS");
                    return true;
                }
            }

            uli::runtime::BlackBox::log("FORMAT FAILED (Attempt " + std::to_string(attempt) + "). OUTPUT: " + error_output);
            
            // If it's a "busy" error, wait and try to clear it again
            if (error_output.find("busy") != std::string::npos || error_output.find("opened exclusively") != std::string::npos) {
                if (attempt == 1) {
                    uli::runtime::BlackBox::log("Device busy. Retrying after specialized settle...");
                    std::system("sync");
                    std::system("udevadm settle");
                    std::system("sleep 1");
                    continue; 
                }
            }
            
            // If we got here on attempt 2 or it wasn't a busy error, fail out
            std::cerr << "[mkfs] ERROR: Formatting failed for " << device_path << std::endl;
            if (!error_output.empty()) {
                uli::runtime::BlackBox::log("MKFS FINAL ERROR OUTPUT: " + error_output);
            }
            break;
        }

        return false;
    }
};

} // namespace format
} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_FORMAT_MKFS_WRAPPER_HPP
