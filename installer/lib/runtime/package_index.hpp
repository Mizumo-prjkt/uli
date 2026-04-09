#ifndef ULI_RUNTIME_PACKAGE_INDEX_HPP
#define ULI_RUNTIME_PACKAGE_INDEX_HPP

#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <array>
#include <memory>
#include "warn.hpp"
#include "design_ui.hpp"

namespace uli {
namespace runtime {

class PackageIndex {
private:
    struct CFileDeleter {
        void operator()(FILE* f) const { if (f) pclose(f); }
    };

    static std::string exec_command(const char* cmd) {
        std::array<char, 256> buffer;
        std::string result;
        std::unique_ptr<FILE, CFileDeleter> pipe(popen(cmd, "r"));
        if (!pipe) return "";
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

public:
    // Update local system caches depending on the running distro
    static bool refresh_cache(const std::string& active_distro) {
        std::cout << "\n" << DesignUI::DIM << "Refreshing local package trees..." << DesignUI::RESET << "\n";
        
        int status = -1;
        if (active_distro == "Debian") {
            status = std::system("apt-get update -qq > /dev/null 2>&1");
        } else if (active_distro == "Arch Linux") {
            status = std::system("pacman -Sy --noconfirm > /dev/null 2>&1");
        } else {
            Warn::print_warning("No automated cache update strategy defined for " + active_distro);
            return true; 
        }

        if (status == 0) {
            Warn::print_info("Package index refreshed.");
            return true;
        } else {
            Warn::print_error("Failed to sync package lists. Internet connection or permission issues may block resolving names.");
            return false;
        }
    }

    // Attempt to search for packages visually within the index (Very basic proxy pattern)
    static std::string visual_search_package(const std::string& active_distro, const std::string& query) {
        std::string cmd = "";
        
        if (active_distro == "Debian") {
             cmd = "apt-cache search " + query + " | head -n 5";
        } else if (active_distro == "Arch Linux") {
             cmd = "pacman -Ss " + query + " | grep -v '    ' | head -n 5";
        } 
        
        if (cmd.empty()) {
            return "Search unsupported for " + active_distro;
        }

        return exec_command(cmd.c_str());
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_PACKAGE_INDEX_HPP
