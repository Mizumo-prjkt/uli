#ifndef ULI_RUNTIME_PROFILE_MINIMAL_HPP
#define ULI_RUNTIME_PROFILE_MINIMAL_HPP

#include "../../../../package_mgr/indexer.hpp"
#include "../../../menu.hpp"
#include <string>
#include <vector>

namespace uli {
namespace runtime {
namespace contents {
namespace profiles {

class ProfileMinimal {
public:
    static void apply_dependencies(MenuState& state, const std::string& os_distro) {
        uli::package_mgr::PackageIndexer::TranslationMap sway_map = {
            {"Arch Linux", {"sway", "swaybg", "swayidle", "swaylock", "waybar", "foot"}},
            {"Debian", {"sway", "swaybg", "swayidle", "swaylock", "waybar", "foot"}},
            {"Ubuntu", {"sway", "swaybg", "swayidle", "swaylock", "waybar", "foot"}},
            {"Alpine", {"sway", "swaybg", "swayidle", "swaylock", "waybar", "foot"}}
        };

        // As requested by the user, minimal may map to Sway or Openbox
        // In the context of generic profile application, we might provide a choice later,
        // but for now, we'll map to 'sway' as the modern Wayland default.
        auto pkgs = uli::package_mgr::PackageIndexer::get_packages_from_map(sway_map, os_distro, "sway");
        for (const auto& pkg : pkgs) {
            state.additional_packages.push_back(pkg);
        }
    }
};

} // namespace profiles
} // namespace contents
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_PROFILE_MINIMAL_HPP
