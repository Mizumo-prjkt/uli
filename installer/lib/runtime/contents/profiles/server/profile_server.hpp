#ifndef ULI_RUNTIME_PROFILE_SERVER_HPP
#define ULI_RUNTIME_PROFILE_SERVER_HPP

#include "../../../../package_mgr/indexer.hpp"
#include "../../../menu.hpp"
#include <string>
#include <vector>

namespace uli {
namespace runtime {
namespace contents {
namespace profiles {

class ProfileServer {
public:
    static void apply_dependencies(MenuState& state, const std::string& os_distro) {
        uli::package_mgr::PackageIndexer::TranslationMap ssh_map = {
            {"Arch Linux", {"openssh"}},
            {"Debian", {"openssh-server"}},
            {"Ubuntu", {"openssh-server"}},
            {"Alpine", {"openssh-server"}}
        };

        // Server maps to openssh natively
        auto pkgs = uli::package_mgr::PackageIndexer::get_packages_from_map(ssh_map, os_distro, "openssh");
        for (const auto& pkg : pkgs) {
            state.additional_packages.push_back(pkg);
        }
    }
};

} // namespace profiles
} // namespace contents
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_PROFILE_SERVER_HPP
