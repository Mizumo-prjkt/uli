#ifndef ULI_CHROOT_HOOK_HPP
#define ULI_CHROOT_HOOK_HPP

#include <string>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include "../runtime/blackbox.hpp"

namespace uli {
namespace hook {

class UniversalChroot {
public:
    static bool is_root() {
        return getuid() == 0;
    }

    // RAII class to manage bind mounts during chroot
    class ScopedChroot {
    private:
        std::string target_root;
        bool mounted;
        bool use_arch_chroot;

    public:
        ScopedChroot(const std::string& root) : target_root(root), mounted(false), use_arch_chroot(false) {
            if (target_root.empty()) return;

            // Detect arch-chroot as it's the premium way to handle this on Arch-based live systems
            use_arch_chroot = (std::system("which arch-chroot > /dev/null 2>&1") == 0);
            uli::runtime::BlackBox::log("CHROOT_HOOK: Entering ScopedChroot for " + target_root + " (arch-chroot=" + (use_arch_chroot ? "YES" : "NO") + ")");

            if (!use_arch_chroot) {
                if (prepare_mounts()) {
                    mounted = true;
                }
            }
        }

        ~ScopedChroot() {
            if (mounted && !use_arch_chroot) {
                cleanup_mounts();
            }
            uli::runtime::BlackBox::log("CHROOT_HOOK: Exiting ScopedChroot");
        }

        bool is_ready() const {
            return !target_root.empty() && (use_arch_chroot || mounted);
        }

        int execute(const std::string& command) {
            if (!is_ready()) return -1;

            std::string full_cmd;
            if (use_arch_chroot) {
                full_cmd = "arch-chroot " + target_root + " " + command + " > /dev/null 2>&1";
            } else {
                full_cmd = "chroot " + target_root + " /bin/bash -c \"" + command + "\" > /dev/null 2>&1";
            }

            uli::runtime::BlackBox::log("CHROOT_EXEC: " + command);
            int status = std::system(full_cmd.c_str());
            
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            return -1;
        }

    private:
        bool prepare_mounts() {
            std::vector<std::string> virtual_fs = {"/dev", "/proc", "/sys", "/run"};
            
            // Read active mounts to avoid redundant binds
            std::ifstream mounts("/proc/mounts");
            std::string line;
            std::vector<std::string> existing;
            while (std::getline(mounts, line)) {
                std::istringstream iss(line);
                std::string dev, mnt;
                if (iss >> dev >> mnt) existing.push_back(mnt);
            }

            for (const auto& fs : virtual_fs) {
                std::string dest = target_root + fs;
                
                // Skip if already mounted
                if (std::find(existing.begin(), existing.end(), dest) != existing.end()) {
                    continue;
                }

                std::filesystem::create_directories(dest);
                std::string cmd = "mount --bind " + fs + " " + dest + " > /dev/null 2>&1";
                if (std::system(cmd.c_str()) != 0) {
                    uli::runtime::BlackBox::log("ERROR: CHROOT_HOOK failed to bind " + fs);
                    return false;
                }
            }
            return true;
        }

        void cleanup_mounts() {
            std::vector<std::string> virtual_fs = {"/dev", "/proc", "/sys", "/run"};
            std::reverse(virtual_fs.begin(), virtual_fs.end());
            for (const auto& fs : virtual_fs) {
                std::string cmd = "umount -l " + target_root + fs + " > /dev/null 2>&1";
                std::system(cmd.c_str());
            }
        }
    };
};

} // namespace hook
} // namespace uli

#endif // ULI_CHROOT_HOOK_HPP
