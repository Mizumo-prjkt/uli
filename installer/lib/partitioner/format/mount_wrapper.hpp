#ifndef ULI_PARTITIONER_FORMAT_MOUNT_WRAPPER_HPP
#define ULI_PARTITIONER_FORMAT_MOUNT_WRAPPER_HPP

#include <string>
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <algorithm>

namespace uli {
namespace partitioner {
namespace format {

class MountWrapper {
public:
    // Mounts a device to a target path
    static bool mount(const std::string& device, const std::string& target) {
        std::cout << "[mount] Attaching " << device << " to " << target << "..." << std::endl;
        mkdir_p(target);
        std::string cmd = "mount \"" + device + "\" \"" + target + "\" > /dev/null 2>&1";
        uli::runtime::BlackBox::log("MOUNT: " + device + " -> " + target);
        bool res = (std::system(cmd.c_str()) == 0);
        if (!res) uli::runtime::BlackBox::log("ERROR: Mount failed for " + device);
        return res;
    }

    // Recursively unmounts everything under a target path
    static bool umount_recursive(const std::string& target) {
        if (!std::filesystem::exists(target)) return true;
        std::cout << "[mount] recursively unmounting " << target << "..." << std::endl;
        std::string cmd = "umount -R \"" + target + "\" > /dev/null 2>&1";
        uli::runtime::BlackBox::log("UMOUNT: " + target);
        return (std::system(cmd.c_str()) == 0);
    }

    // Formats and enables swap
    static bool swapon(const std::string& device) {
        std::cout << "[swap] Initializing swap on " << device << "..." << std::endl;
        std::string mk_cmd = "mkswap \"" + device + "\" > /dev/null 2>&1";
        uli::runtime::BlackBox::log("SWAP_INIT: " + device);
        if (std::system(mk_cmd.c_str()) != 0) {
            uli::runtime::BlackBox::log("ERROR: mkswap failed for " + device);
            return false;
        }
        
        std::cout << "[swap] Activating " << device << "..." << std::endl;
        std::string on_cmd = "swapon \"" + device + "\" > /dev/null 2>&1";
        uli::runtime::BlackBox::log("SWAP_ON: " + device);
        bool res = (std::system(on_cmd.c_str()) == 0);
        if (!res) uli::runtime::BlackBox::log("ERROR: swapon failed for " + device);
        return res;
    }

    // Disables all swap (safe for cleanup)
    static void swapoff_all() {
        std::cout << "[swap] Disabling all active swap partitions..." << std::endl;
        uli::runtime::BlackBox::log("SWAP_OFF ALL");
        std::system("swapoff -a > /dev/null 2>&1");
    }

    // Helper to ensure nested directories exist
    static void mkdir_p(const std::string& path) {
        try {
            std::filesystem::create_directories(path);
        } catch (...) {}
    }

    // Prepares a chroot environment by bind-mounting essential virtual filesystems
    static bool prepare_chroot(const std::string& target) {
        std::vector<std::pair<std::string, std::string>> mounts = {
            {"/dev", "/dev"},
            {"/proc", "/proc"},
            {"/sys", "/sys"},
            {"/run", "/run"}
        };

        for (const auto& m : mounts) {
            std::string dest = target + m.second;
            mkdir_p(dest);
            std::string cmd = "mount --bind " + m.first + " " + dest + " > /dev/null 2>&1";
            if (std::system(cmd.c_str()) != 0) {
                 uli::runtime::BlackBox::log("ERROR: Failed to bind mount " + m.first);
                 return false;
            }
        }
        return true;
    }

    // Cleans up chroot bind mounts
    static void cleanup_chroot(const std::string& target) {
        std::vector<std::string> mounts = {"/dev", "/proc", "/sys", "/run"};
        // Unmount in reverse order to handle nesting if any
        std::reverse(mounts.begin(), mounts.end());
        for (const auto& m : mounts) {
            std::string cmd = "umount -l " + target + m + " > /dev/null 2>&1";
            std::system(cmd.c_str());
        }
    }
};

} // namespace format
} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_FORMAT_MOUNT_WRAPPER_HPP
