#ifndef ULI_PACKAGE_MGR_INDEXER_HPP
#define ULI_PACKAGE_MGR_INDEXER_HPP

#include <string>
#include <vector>
#include <map>

#include "../runtime/menu.hpp"

namespace uli {
namespace package_mgr {

class PackageIndexer {
public:
    using TranslationMap = std::map<std::string, std::vector<std::string>>;

    // Takes a provided configuration map and returns the distribution-specific package name string(s).
    // The os_distro parameter is expected to be strings like "Debian", "Ubuntu", "Arch Linux", "Alpine".
    static std::vector<std::string> get_packages_from_map(
        const TranslationMap& package_map, 
        const std::string& os_distro,
        const std::string& fallback_generic_name = ""
    );

    // New prioritized resolution that checks user overrides first
    static std::string resolve_package_name(
        const std::string& generic_name,
        const std::string& os_distro,
        const std::vector<uli::runtime::ManualMapping>& manual_mappings
    );
};

} // namespace package_mgr
} // namespace uli

#endif // ULI_PACKAGE_MGR_INDEXER_HPP
