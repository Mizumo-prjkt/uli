#ifndef ULI_RUNTIME_PROFILE_DESKTOP_HPP
#define ULI_RUNTIME_PROFILE_DESKTOP_HPP

#include "../../../../package_mgr/indexer.hpp"
#include "../../../menu.hpp"
#include <string>
#include <vector>

namespace uli {
namespace runtime {
namespace contents {
namespace profiles {

class ProfileDesktop {
public:
    static void apply_dependencies(MenuState& state, const std::string& os_distro) {
        uli::package_mgr::PackageIndexer::TranslationMap kde_map = {
            {"Arch Linux", {"plasma-meta", "kde-applications-meta", "sddm"}},
            {"Debian", {"kde-plasma-desktop", "sddm"}},
            {"Ubuntu", {"kubuntu-desktop"}},
            {"Alpine", {"plasma-desktop", "sddm"}}
        };

        // Desktop maps to KDE Plasma natively
        auto pkgs = uli::package_mgr::PackageIndexer::get_packages_from_map(kde_map, os_distro, "kde-plasma");
        for (const auto& pkg : pkgs) {
            state.additional_packages.push_back(pkg);
        }
    }
};

} // namespace profiles
} // namespace contents
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_PROFILE_DESKTOP_HPP
