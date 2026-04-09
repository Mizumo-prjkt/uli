#include "desktop/profile_desktop.hpp"
#include "minimal/profile_minimal.hpp"
#include "server/profile_server.hpp"
#include "../../menu.hpp"
#include "../../../package_mgr/indexer.hpp"
#include <string>
#include <algorithm>

namespace uli {
namespace runtime {
namespace contents {
namespace profiles {

class ProfileManager {
public:
    static void apply_profile(MenuState& state, const std::string& os_distro) {
        if (state.profile == "Desktop" || state.profile == "desktop") {
            ProfileDesktop::apply_dependencies(state, os_distro);
        } else if (state.profile == "Minimal" || state.profile == "minimal") {
            ProfileMinimal::apply_dependencies(state, os_distro);
        } else if (state.profile == "Server" || state.profile == "server") {
            ProfileServer::apply_dependencies(state, os_distro);
        }
        
        uli::package_mgr::PackageIndexer::TranslationMap base_utils_map = {
            {"Arch Linux", {"base", "base-devel", "linux-firmware", "nano", "vim", "networkmanager"}},
            {"Debian", {"build-essential", "firmware-linux", "nano", "vim", "network-manager"}},
            {"Ubuntu", {"build-essential", "linux-firmware", "nano", "vim", "network-manager"}},
            {"Alpine", {"alpine-base", "nano", "vim", "networkmanager"}}
        };

        // Base Utils are generally needed everywhere 
        auto base_pkgs = uli::package_mgr::PackageIndexer::get_packages_from_map(base_utils_map, os_distro, "base-utils");
        for (const auto& pkg : base_pkgs) {
            state.additional_packages.push_back(pkg);
        }
        
        // Apply microcodes if Arch Linux (or generic fallback mapping)
        // Usually AMD and Intel are both added on Arch install media, or we could conditionally add both.
        if (os_distro == "Arch Linux") {
            uli::package_mgr::PackageIndexer::TranslationMap amd_map = {
                {"Arch Linux", {"amd-ucode"}},
                {"Debian", {"amd64-microcode"}},
                {"Ubuntu", {"amd64-microcode"}},
                {"Alpine", {"linux-firmware-amd"}}
            };
            uli::package_mgr::PackageIndexer::TranslationMap intel_map = {
                {"Arch Linux", {"intel-ucode"}},
                {"Debian", {"intel-microcode"}},
                {"Ubuntu", {"intel-microcode"}},
                {"Alpine", {"intel-ucode"}}
            };

            auto amd = uli::package_mgr::PackageIndexer::get_packages_from_map(amd_map, os_distro, "amd-ucode");
            auto intel = uli::package_mgr::PackageIndexer::get_packages_from_map(intel_map, os_distro, "intel-ucode");
            for (const auto& pkg : amd) state.additional_packages.push_back(pkg);
            for (const auto& pkg : intel) state.additional_packages.push_back(pkg);
        }
        
        // Ensure kernel headers match current kernel selection
        if (!state.kernel.empty()) {
            std::string header_pkg = state.kernel + "-headers";
            // Check for duplicates
            if (std::find(state.additional_packages.begin(), state.additional_packages.end(), header_pkg) == state.additional_packages.end()) {
                state.additional_packages.push_back(header_pkg);
            }
        }
    }
};

} // namespace profiles
} // namespace contents
} // namespace runtime
} // namespace uli
