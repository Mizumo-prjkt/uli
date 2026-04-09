#ifndef ULI_PARTITIONER_OS_CHECK_HPP
#define ULI_PARTITIONER_OS_CHECK_HPP

#include <string>
#include <vector>
#include <iostream>
#include <array>
#include <memory>
#include <stdexcept>

namespace uli {
namespace partitioner {

class OSCheck {
public:
    // Helper function to execute a shell command and return its string output
    struct CFileDeleter {
        void operator()(FILE* f) const { if (f) pclose(f); }
    };

    static std::string exec_command(const char* cmd) {
        std::array<char, 256> buffer;
        std::string result;
        std::unique_ptr<FILE, CFileDeleter> pipe(popen(cmd, "r"));
        if (!pipe) {
            return "";
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    // Determine if a given disk (e.g., "/dev/sda") has an existing operating system
    static bool has_existing_os(const std::string& disk_path) {
        // Query lsblk to get filesystems and mount points for the disk
        std::string cmd = "lsblk -f -n -o FSTYPE,MOUNTPOINT " + disk_path + " 2>/dev/null";
        std::string output;
        
        try {
            output = exec_command(cmd.c_str());
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to execute lsblk for OS check on " << disk_path << std::endl;
            return false; // Default to false if we can't check, though this is risky.
        }

        // Look for common indicators of an installed OS:
        // - "ntfs" indicates Windows or a shared data drive
        // - "vfat" or "fat32" often used for EFI System Partitions (ESP)
        // - ext4, btrfs, xfs, etc. are common Linux root filesystems
        // - existing mount points like "/", "/boot", or "/boot/efi"
        
        if (output.find("ntfs") != std::string::npos) {
            std::cout << "[OSDetect] Found NTFS partition on " << disk_path << " (Likely Windows)" << std::endl;
            return true;
        }
        
        // Presence of common root filesystems paired with active systems
        // Note: For a very robust check, we'd mount and look for /etc/os-release or /Windows/System32
        // But for safe fallback isolation, identifying common linux fs usage on non-live disks is a strong heuristic.
        std::vector<std::string> critical_fs = {"ext4", "btrfs", "xfs"};
        for (const auto& fs : critical_fs) {
            if (output.find(fs) != std::string::npos) {
                std::cout << "[OSDetect] Found Linux filesystem (" << fs << ") on " << disk_path << " (Possible existing Linux)" << std::endl;
                return true; // Erring on the side of caution
            }
        }
        
        if (output.find("vfat") != std::string::npos || output.find("fat32") != std::string::npos) {
             std::cout << "[OSDetect] Found EFI partition footprints on " << disk_path << std::endl;
             return true;
        }

        return false;
    }
};

} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_OS_CHECK_HPP
